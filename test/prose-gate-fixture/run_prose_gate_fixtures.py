#!/usr/bin/env python3
"""Gate-effectiveness harness for the prose and commit lint gate (T008).

Orchestrates tools/prose/prose_gate.py against the committed prose fixtures
and the commit cases in this directory and checks the expected verdicts
(specs/002-prose-commit-lint: loader asserts T012, prose verdict asserts
T018, full commit-case asserts T023, parity and timing asserts T026).

This script does not implement the gate. While the gate script is absent it
exits 2 with the diagnostic:

    gate script not present (TDD red)

Commit cases are built and run inside a throwaway git repo under --out; the
harness never runs git in the invoking repository.

CLI::

    python3 run_prose_gate_fixtures.py --check <prose|commit|both> \\
        --rules tools/prose/prose_rules.yaml --out <dir> <fixture-dir>
"""

from __future__ import annotations

import argparse
import copy
import shutil
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError as exc:  # pragma: no cover - environment guard
    sys.stderr.write(
        "run_prose_gate_fixtures.py requires PyYAML (install python3-yaml): "
        f"{exc}\n"
    )
    sys.exit(3)

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent
GATE = REPO_ROOT / "tools" / "prose" / "prose_gate.py"
DEFAULT_RULES = REPO_ROOT / "tools" / "prose" / "prose_rules.yaml"

SCRIPTS_MISSING = "gate script not present (TDD red)"

EXIT_OK = 0
EXIT_ASSERT = 1
EXIT_SCRIPTS_MISSING = 2
EXIT_USAGE = 3

PROSE_FIXTURES = (
    "violations.md",
    "silent.md",
    "comments.c",
    "comments.sh",
    "comments.cmake",
)

CASE_KINDS = ("commit", "merge", "rename", "binary", "empty-range")

GIT_IDENTITY = (
    "-c",
    "user.name=fixture",
    "-c",
    "user.email=fixture@example",
    "-c",
    "commit.gpgsign=false",
)

# Loader-assert phase (T012): mutants of the loaded rules file, never
# written into the repo tree, only under {out}/loader-mutants.
WILDCARD_RULE_ID = "XI2.CONTRASTIVE"
FILLER_RULE_ID = "XI5.FILLER"
ZERO_SUMMARY = "prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped"


def die(message: str, code: int = EXIT_USAGE) -> None:
    sys.stdout.flush()
    sys.stderr.write(f"{message}\n")
    raise SystemExit(code)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="run_prose_gate_fixtures.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=(
            "Run the prose and commit lint gate against the committed prose "
            "fixtures and commit cases and check the expected verdicts."
        ),
        epilog="""\
checks:
  prose    tree-mode prose check over the five staged prose fixtures
           (violations.md, silent.md, comments.c, comments.sh,
           comments.cmake); verdict asserts land in task T018, loader
           asserts in task T012
  commit   range commit check over a throwaway repo under --out holding
           exactly the case commit; the skeleton evaluates each case
           against expect.exit alone, full asserts land in task T023
  both     run both checks (default)

loader phase (runs on every invocation, whatever --check selects, T012):
  four single-mutation mutants of the loaded rules file, written only
  under {out}/loader-mutants and cleaned after the run, must exit 2 with
  stderr naming the offending field path; the real rules file must load
  clean (exit 0 with the zero summary line).

expected matrix:
  authoritative per-case expectations live in commits/cases.yaml
  cases with expect.exit 0   harness PASS when the gate exits 0
  cases with expect.exit 1   harness PASS when the gate exits 1
  prose fixtures             SKIP placeholder until T018 asserts

every run writes a prose matrix artifact under --out.
when the gate script is absent the harness exits 2 with:
  gate script not present (TDD red)

examples:
  python3 run_prose_gate_fixtures.py --check both \\
      --rules test/prose-gate-fixture/fixture_rules.yaml \\
      --out /tmp/prose-matrix test/prose-gate-fixture
""",
    )
    parser.add_argument(
        "--check",
        choices=("prose", "commit", "both"),
        default="both",
        help="which gate check to run (default: both)",
    )
    parser.add_argument(
        "--rules",
        type=Path,
        default=DEFAULT_RULES,
        help="path to tools/prose/prose_rules.yaml (default: repo rules)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="scratch and artifact directory (created if missing)",
    )
    parser.add_argument(
        "fixture_dir",
        type=Path,
        help="the prose-gate-fixture directory itself",
    )
    return parser.parse_args(argv)


def selected_checks(check: str) -> list[str]:
    if check == "both":
        return ["prose", "commit"]
    return [check]


def discover_prose_fixtures(fixture_dir: Path) -> dict[str, Path]:
    prose_dir = fixture_dir / "prose"
    if not prose_dir.is_dir():
        die(f"prose fixture directory not found: {prose_dir}")
    found: dict[str, Path] = {}
    for name in PROSE_FIXTURES:
        path = prose_dir / name
        if not path.is_file():
            die(f"missing prose fixture file: {path}")
        found[name] = path
    return found


def load_cases(cases_path: Path) -> list[dict[str, Any]]:
    """Load commits/cases.yaml and validate the per-case field shape."""
    if not cases_path.is_file():
        die(f"commit cases file not found: {cases_path}")
    try:
        loaded = yaml.safe_load(cases_path.read_text(encoding="utf-8"))
    except yaml.YAMLError as exc:
        die(f"commit cases file is not valid YAML: {exc}")
    if not isinstance(loaded, dict) or not isinstance(loaded.get("cases"), list):
        die("commit cases file must be a mapping with a 'cases' list")

    cases: list[dict[str, Any]] = []
    seen: set[str] = set()
    for raw in loaded["cases"]:
        if not isinstance(raw, dict) or not isinstance(raw.get("name"), str):
            die(f"commit case missing a string 'name': {raw!r}")
        name = raw["name"]
        if name in seen:
            die(f"commit cases have a duplicate case name: {name}")
        seen.add(name)

        kind = raw.get("kind", "commit")
        if kind not in CASE_KINDS:
            die(f"commit case {name} has unknown kind: {kind!r}")

        expect = raw.get("expect")
        if not isinstance(expect, dict) or not isinstance(expect.get("exit"), int):
            die(f"commit case {name} needs an expect mapping with an integer exit")
        findings = expect.get("findings")
        if findings is not None and not isinstance(findings, list):
            die(f"commit case {name} has a non-list expect.findings")
        records = expect.get("records")
        if records is not None and not isinstance(records, int):
            die(f"commit case {name} has a non-integer expect.records")

        if kind != "empty-range" and not isinstance(raw.get("title"), str):
            die(f"commit case {name} needs a string title")
        if kind == "commit":
            changed = raw.get("changed_lines")
            if not isinstance(changed, int) or changed < 0:
                die(f"commit case {name} needs a non-negative integer changed_lines")
        if kind == "merge" and not isinstance(raw.get("side_commit"), dict):
            die(f"commit case {name} of kind merge needs a side_commit mapping")
        if kind == "rename":
            if not isinstance(raw.get("setup_commit"), dict):
                die(f"commit case {name} of kind rename needs a setup_commit mapping")
            move = raw.get("rename")
            if not isinstance(move, dict) or "from" not in move or "to" not in move:
                die(f"commit case {name} of kind rename needs a rename from/to mapping")
        if kind == "binary" and not isinstance(raw.get("binary_file"), str):
            die(f"commit case {name} of kind binary needs a binary_file path")
        cases.append(raw)
    return cases


def write_matrix(out_dir: Path, payload: dict[str, Any]) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / "prose_matrix.yaml"
    path.write_text(
        yaml.safe_dump(payload, sort_keys=False, allow_unicode=True),
        encoding="utf-8",
    )
    print(f"MATRIX  {path}", flush=True)
    return path


def print_verdict(
    name: str,
    check: str,
    expected: str,
    observed: str,
    result: str,
) -> None:
    print(
        f"VERDICT  name={name}  check={check}  "
        f"expected={expected}  observed={observed}  {result}",
        flush=True,
    )


def emit_scripts_missing_verdicts(
    checks: list[str],
    prose: dict[str, Path],
    cases: list[dict[str, Any]],
) -> None:
    for name in prose:
        if "prose" in checks:
            print_verdict(name, "prose", "pending(T018)", SCRIPTS_MISSING, "SKIP")
    for case in cases:
        if "commit" in checks:
            print_verdict(
                case["name"],
                "commit",
                f"exit:{case['expect']['exit']}",
                SCRIPTS_MISSING,
                "FAIL",
            )


def run_git(repo: Path, *args: str) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(
        ["git", *GIT_IDENTITY, *args],
        cwd=repo,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        die(
            f"git {' '.join(args)} failed in {repo} "
            f"(exit {proc.returncode}): {proc.stderr.strip()}"
        )
    return proc


def git_head(repo: Path) -> str:
    return run_git(repo, "rev-parse", "HEAD").stdout.strip()


def assemble_message(title: Any, body: Any, footer: Any) -> str:
    """Join only the present sections: title, blank, body, blank, footer."""
    sections: list[str] = []
    if title:
        sections.append(str(title))
    if body:
        if isinstance(body, list):
            sections.append("\n".join(str(line) for line in body))
        else:
            sections.append(str(body))
    if footer:
        if isinstance(footer, list):
            sections.append("\n".join(str(line) for line in footer))
        else:
            sections.append(str(footer))
    return "\n\n".join(sections)


def write_numbered_lines(path: Path, count: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "".join(f"changed line {i}\n" for i in range(1, count + 1)),
        encoding="utf-8",
    )


def stage_prose_repo(prose: dict[str, Path], stage_repo: Path) -> None:
    """Throwaway repo holding the five prose fixture files, committed."""
    shutil.rmtree(stage_repo, ignore_errors=True)
    stage_repo.mkdir(parents=True)
    run_git(stage_repo, "init")
    for name, src in prose.items():
        shutil.copy2(src, stage_repo / name)
    run_git(stage_repo, "add", "-A")
    run_git(
        stage_repo,
        "commit",
        "-m",
        assemble_message(
            "Docs: stage prose gate fixtures", None, "Approved-by: charlesarcher"
        ),
    )


def init_commit_repo(stage_repo: Path) -> str:
    """Throwaway repo with one conforming empty base commit; returns branch."""
    shutil.rmtree(stage_repo, ignore_errors=True)
    stage_repo.mkdir(parents=True)
    run_git(stage_repo, "init")
    run_git(
        stage_repo,
        "commit",
        "--allow-empty",
        "-m",
        assemble_message(
            "test: seed prose gate fixture base",
            None,
            "Approved-by: charlesarcher",
        ),
    )
    return run_git(stage_repo, "rev-parse", "--abbrev-ref", "HEAD").stdout.strip()


def commit_side_files(repo: Path, spec: dict[str, Any]) -> None:
    for entry in spec.get("files", []):
        write_numbered_lines(repo / str(entry["path"]), int(entry.get("lines", 1)))
    run_git(repo, "add", "-A")


def build_case_commit(
    repo: Path,
    case: dict[str, Any],
    branch: str,
) -> tuple[str, str]:
    """Build the case commit on top of HEAD; return its (base, head) range."""
    kind = case.get("kind", "commit")
    base = git_head(repo)

    if kind == "empty-range":
        return base, base

    if kind == "merge":
        side = case["side_commit"]
        run_git(repo, "checkout", "-B", "side")
        commit_side_files(repo, side)
        run_git(
            repo,
            "commit",
            "-m",
            assemble_message(side.get("title"), side.get("body"), side.get("footer")),
        )
        run_git(repo, "checkout", branch)
        run_git(
            repo,
            "merge",
            "--no-ff",
            "side",
            "-m",
            assemble_message(case.get("title"), case.get("body"), case.get("footer")),
        )
    elif kind == "rename":
        setup = case["setup_commit"]
        commit_side_files(repo, setup)
        run_git(
            repo,
            "commit",
            "-m",
            assemble_message(
                setup.get("title"), setup.get("body"), setup.get("footer")
            ),
        )
        # The setup commit is the range base for the rename case itself.
        setup_sha = git_head(repo)
        move = case["rename"]
        target = repo / str(move["to"])
        target.parent.mkdir(parents=True, exist_ok=True)
        run_git(repo, "mv", str(move["from"]), str(move["to"]))
        run_git(
            repo,
            "commit",
            "-m",
            assemble_message(case.get("title"), case.get("body"), case.get("footer")),
        )
        return setup_sha, git_head(repo)
    elif kind == "binary":
        target = repo / str(case["binary_file"])
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(bytes(range(256)))
        run_git(repo, "add", "-A")
        run_git(
            repo,
            "commit",
            "-m",
            assemble_message(case.get("title"), case.get("body"), case.get("footer")),
        )
    else:
        # Plain commit: a unique new text file with exactly changed_lines
        # added lines, so `git show --numstat -M` sums to exactly that count.
        write_numbered_lines(
            repo / f"fixture/changed-{case['name']}.txt",
            int(case["changed_lines"]),
        )
        run_git(repo, "add", "-A")
        run_git(
            repo,
            "commit",
            "-m",
            assemble_message(case.get("title"), case.get("body"), case.get("footer")),
        )

    return base, git_head(repo)


def run_gate(
    extra_args: list[str],
    cwd: Path,
    rules: Path,
) -> subprocess.CompletedProcess[str]:
    argv = [sys.executable, str(GATE), "--rules", str(rules), *extra_args]
    try:
        return subprocess.run(argv, cwd=cwd, capture_output=True, text=True, check=False)
    except OSError as exc:
        die(f"failed to invoke {GATE}: {exc}")


def echo(proc: subprocess.CompletedProcess[str]) -> None:
    if proc.stdout:
        sys.stdout.write(proc.stdout)
        if not proc.stdout.endswith("\n"):
            sys.stdout.write("\n")
    if proc.stderr:
        sys.stderr.write(proc.stderr)
        if not proc.stderr.endswith("\n"):
            sys.stderr.write("\n")


def rule_entry(data: dict[str, Any], rule_id: str) -> dict[str, Any]:
    for entry in data["rules"]:
        if isinstance(entry, dict) and entry.get("id") == rule_id:
            return entry
    die(f"rules file carries no rule {rule_id}, cannot build the loader mutant")


def mutate_wildcard(data: dict[str, Any]) -> None:
    rule_entry(data, WILDCARD_RULE_ID)["pattern"] = ".*"


def mutate_vocabulary_probe(data: dict[str, Any]) -> None:
    # Trailing punctuation: the whole-word vocabulary probe cannot hit it.
    rule_entry(data, FILLER_RULE_ID)["tokens"][0] = "very,"


def mutate_drop_canonical(data: dict[str, Any]) -> None:
    # XI5.FILLER is dropped; XI5.MARKETING stays (tasks.md T012).
    data["rules"] = [
        entry
        for entry in data["rules"]
        if not (isinstance(entry, dict) and entry.get("id") == FILLER_RULE_ID)
    ]


def mutate_threshold_bounds(data: dict[str, Any]) -> None:
    data["thresholds"]["title_max"] = 500


# One mutation per case, stable stderr substrings only (prose_gate.py's
# load-time validation names the offending field path on exit 2).
LOADER_MUTANT_CASES: tuple[tuple[str, Callable[[dict[str, Any]], None], str], ...] = (
    ("loader.wildcard", mutate_wildcard, "wildcard forbidden"),
    ("loader.vocabulary-probe", mutate_vocabulary_probe, "probe failed"),
    (
        "loader.canonical-coverage",
        mutate_drop_canonical,
        "missing canonical id XI5.FILLER",
    ),
    ("loader.threshold-bounds", mutate_threshold_bounds, "thresholds.title_max"),
)


def loader_verdict(
    name: str,
    expected: str,
    observed: str,
    result: str,
    exit_code: int,
) -> dict[str, Any]:
    print_verdict(name, "loader", expected, observed, result)
    return {
        "interface": name,
        "check": "loader",
        "expected": expected,
        "observed": observed,
        "result": result,
        "exit_code": exit_code,
    }


def run_loader_cases(rules: Path, out_dir: Path) -> tuple[int, list[dict[str, Any]]]:
    """T012 loader asserts: broken rule data exits 2 naming the field path.

    Runs on every invocation regardless of --check: the gate validates the
    Rule Data File on every run, --check prose included, so the prose-only
    invocation keeps the unimplemented phases irrelevant to the assert.
    """
    try:
        base = yaml.safe_load(rules.read_text(encoding="utf-8"))
    except (OSError, yaml.YAMLError) as exc:
        die(f"loader phase cannot load the rules file {rules}: {exc}")
    if not isinstance(base, dict):
        die(f"loader phase: {rules} does not hold a YAML mapping")

    mutants_dir = out_dir / "loader-mutants"
    mutants_dir.mkdir(parents=True, exist_ok=True)
    failures = 0
    rows: list[dict[str, Any]] = []
    for name, mutate, needle in LOADER_MUTANT_CASES:
        mutant = copy.deepcopy(base)
        mutate(mutant)
        mutant_path = mutants_dir / f"{name}.yaml"
        mutant_path.write_text(
            yaml.safe_dump(mutant, sort_keys=False, allow_unicode=True),
            encoding="utf-8",
        )
        proc = run_gate(["--check", "prose"], out_dir, mutant_path)
        expected = "exit:2"
        observed = f"exit:{proc.returncode}"
        result = "PASS" if proc.returncode == 2 and needle in proc.stderr else "FAIL"
        rows.append(loader_verdict(name, expected, observed, result, proc.returncode))
        echo(proc)
        if result == "FAIL":
            failures += 1
        mutant_path.unlink(missing_ok=True)

    # The loader's canonical coverage check enforces the id set equals the
    # seven canonical ids of the rule-data namespace table, so a clean load
    # IS the id-set assertion.
    proc = run_gate(["--check", "prose"], out_dir, rules)
    code = proc.returncode
    expected = "exit:0"
    observed = f"exit:{code}"
    result = "PASS" if code == 0 and ZERO_SUMMARY in proc.stdout else "FAIL"
    rows.append(loader_verdict("loader.valid-load", expected, observed, result, code))
    echo(proc)
    if result == "FAIL":
        failures += 1
    try:
        mutants_dir.rmdir()
    except OSError:
        pass
    return failures, rows


def run_prose_check(
    prose: dict[str, Path],
    rules: Path,
    out_dir: Path,
) -> list[dict[str, Any]]:
    stage_repo = out_dir / "work" / "prose-stage"
    stage_prose_repo(prose, stage_repo)
    proc = run_gate(["--check", "prose", "--mode", "tree"], stage_repo, rules)
    echo(proc)
    # TODO(T018): replace the SKIP placeholder below with the per-file prose
    # verdict asserts: reported file, line, rule id, and token per family,
    # every silent case silent, authorship and marker behavior included.
    rows: list[dict[str, Any]] = []
    for name in prose:
        print_verdict(name, "prose", "pending(T018)", "not asserted", "SKIP")
        rows.append(
            {
                "interface": name,
                "check": "prose",
                "expected": "pending(T018)",
                "result": "SKIP",
            }
        )
    return rows


def run_commit_cases(
    cases: list[dict[str, Any]],
    rules: Path,
    out_dir: Path,
) -> tuple[int, list[dict[str, Any]]]:
    stage_repo = out_dir / "work" / "commit-stage"
    branch = init_commit_repo(stage_repo)
    failures = 0
    rows: list[dict[str, Any]] = []
    for case in cases:
        base, head = build_case_commit(stage_repo, case, branch)
        proc = run_gate(
            ["--check", "commit", "--base", base, "--head", head],
            stage_repo,
            rules,
        )
        expected = f"exit:{case['expect']['exit']}"
        observed = f"exit:{proc.returncode}"
        # TODO(T023): upgrade this exit-code-only evaluation to the full
        # per-case assert suite: expect.findings rule matches against the
        # stderr finding lines, finding scopes, and expect.records.
        result = "PASS" if proc.returncode == case["expect"]["exit"] else "FAIL"
        print_verdict(case["name"], "commit", expected, observed, result)
        echo(proc)
        if result == "FAIL":
            failures += 1
        rows.append(
            {
                "interface": case["name"],
                "check": "commit",
                "expected": expected,
                "observed": observed,
                "result": result,
                "exit_code": proc.returncode,
                "base": base,
                "head": head,
            }
        )
    return failures, rows


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    rules = args.rules.expanduser().resolve()
    fixture_dir = args.fixture_dir.expanduser().resolve()
    out_dir = args.out.expanduser().resolve()
    checks = selected_checks(args.check)

    print(
        f"prose gate-effectiveness harness  check={args.check}  "
        f"rules={rules}  fixtures={fixture_dir}  out={out_dir}",
        flush=True,
    )

    prose = discover_prose_fixtures(fixture_dir)
    cases = load_cases(fixture_dir / "commits" / "cases.yaml")
    out_dir.mkdir(parents=True, exist_ok=True)

    if not GATE.is_file():
        write_matrix(
            out_dir,
            {
                "status": "scripts_missing",
                "diagnostic": SCRIPTS_MISSING,
                "missing_scripts": [str(GATE)],
                "interfaces": [],
            },
        )
        emit_scripts_missing_verdicts(checks, prose, cases)
        sys.stdout.flush()
        sys.stderr.write(f"{SCRIPTS_MISSING}\n")
        sys.stderr.write(f"  missing: {GATE}\n")
        sys.stderr.flush()
        return EXIT_SCRIPTS_MISSING

    failures = 0
    rows: list[dict[str, Any]] = []
    # The loader phase runs on every invocation, whatever --check selects.
    loader_failures, loader_rows = run_loader_cases(rules, out_dir)
    failures += loader_failures
    rows.extend(loader_rows)
    if "prose" in checks:
        rows.extend(run_prose_check(prose, rules, out_dir))
    if "commit" in checks:
        commit_failures, commit_rows = run_commit_cases(cases, rules, out_dir)
        failures += commit_failures
        rows.extend(commit_rows)

    write_matrix(
        out_dir,
        {
            "status": "ok" if failures == 0 else "assert_failed",
            "interfaces": rows,
        },
    )
    if failures:
        sys.stderr.write(
            f"prose gate assertions failed: {failures} FAIL verdict(s)\n"
        )
        return EXIT_ASSERT
    print(
        "prose gate assertions passed (prose verdict asserts land in T018)",
        flush=True,
    )
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
