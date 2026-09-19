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
import os
import re
import shutil
import subprocess
import sys
import time
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

# SC-003 developer-machine bound, measured 0.062 s whole tree by plan R-04.
TIMING_BOUND_SECONDS = 10.0


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
           exactly the case commits; each case asserts exit code, the
           named finding rules with scope tokens, and expect.records
  both     run both checks (default)

loader phase (runs on every invocation, whatever --check selects, T012):
  four single-mutation mutants of the loaded rules file, written only
  under {out}/loader-mutants and cleaned after the run, must exit 2 with
  stderr naming the offending field path; the real rules file must load
  clean (exit 0 with the zero summary line).

expected matrix:
  authoritative per-case expectations live in commits/cases.yaml
  per commit case            harness PASS when exit code, expect.findings
                             rule lines (scope tokens present in the named
                             commit field), and expect.records all match
  prose fixtures             T018 verdict asserts against the real gate

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
    env: dict[str, str] | None = None,
) -> subprocess.CompletedProcess[str]:
    argv = [sys.executable, str(GATE), "--rules", str(rules), *extra_args]
    try:
        return subprocess.run(
            argv, cwd=cwd, env=env, capture_output=True, text=True, check=False
        )
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
    # IS the id-set assertion. Discovery is real code now, so this assert
    # runs in an empty staged repository: no sources, zero summary, exit 0.
    empty_repo = out_dir / "work" / "loader-empty"
    shutil.rmtree(empty_repo, ignore_errors=True)
    empty_repo.mkdir(parents=True)
    run_git(empty_repo, "init")
    run_git(empty_repo, "commit", "--allow-empty", "-m", "loader empty base")
    proc = run_gate(["--check", "prose", "--mode", "tree"], empty_repo, rules)
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
) -> tuple[int, list[dict[str, Any]]]:
    """T018 prose asserts: per-family reports, silence, marker, authorship,
    plus the T026 parity and whole-tree timing asserts.

    Scenario surfaces are throwaway staged repositories under --out; the
    fixture corpus itself is never mutated. The corpus's designed oracle:
    violations.md lines 7 to 15 (one finding each, ids and tokens below),
    comments.c line 11, comments.sh line 4, comments.cmake line 3, and
    silent.md silent. The em dash token is written as an escape so this
    source carries no dash code point.
    """
    stage_repo = out_dir / "work" / "prose-stage"
    stage_prose_repo(prose, stage_repo)
    proc = run_gate(["--check", "prose", "--mode", "tree"], stage_repo, rules)
    echo(proc)

    findings = parse_findings(proc.stderr)
    expected = {
        ("violations.md", 7, "XI1.EMDASH", "\u2014"),
        ("violations.md", 8, "XI1.DOUBLE-HYHEN", "--"),
        ("violations.md", 9, "XI2.CONTRASTIVE", ", not "),
        ("violations.md", 10, "XI2.CONTRASTIVE", " rather than "),
        ("violations.md", 11, "XI2.CONTRASTIVE", " instead of "),
        ("violations.md", 12, "XI3.VOUCHER", "Frankly"),
        ("violations.md", 13, "XI4.META-EDITORIALIZING", "In this section we"),
        ("violations.md", 14, "XI5.FILLER", "in order to"),
        ("violations.md", 15, "XI5.MARKETING", "robust"),
        ("comments.c", 11, "XI1.EMDASH", "\u2014"),
        ("comments.sh", 4, "XI1.EMDASH", "\u2014"),
        ("comments.cmake", 3, "XI1.EMDASH", "\u2014"),
    }

    failures = 0
    rows: list[dict[str, Any]] = []

    def verdict(name: str, expected_text: str, observed_text: str, ok: bool) -> None:
        nonlocal failures
        result = "PASS" if ok else "FAIL"
        print_verdict(name, "prose", expected_text, observed_text, result)
        rows.append(
            {
                "interface": name,
                "check": "prose",
                "expected": expected_text,
                "observed": observed_text,
                "result": result,
            }
        )
        if not ok:
            failures += 1

    for path, line, rule_id, token in sorted(expected):
        found = (path, line, rule_id, token) in findings
        verdict(
            f"prose.report.{path}:{line}",
            f"fires {rule_id} '{token}' at {path}:{line}",
            "fires" if found else "missing",
            found,
        )

    set_ok = findings == expected
    verdict(
        "prose.silent.no-extras",
        "finding set equals the 12 designed findings exactly",
        f"{len(findings)} findings, {len(findings ^ expected)} unmatched",
        set_ok,
    )
    silent_md = sorted(item for item in findings if item[0] == "silent.md")
    verdict(
        "prose.silent.silent-md",
        "silent.md contributes zero findings",
        f"{len(silent_md)} findings",
        not silent_md,
    )
    comments = {item for item in findings if item[0].startswith("comments.")}
    verdict(
        "prose.silent.comment-lines",
        "comment files fire only on their designed lines",
        f"{sorted(comments)}",
        comments == {item for item in expected if item[0].startswith("comments.")},
    )
    marker_line = [item for item in findings if item[0] == "silent.md" and item[1] == 26]
    verdict(
        "prose.silent.marked-quotation",
        "valid marker suppresses the marked quotation at silent.md:26",
        f"{marker_line}",
        not marker_line,
    )

    marker_failures, marker_rows = run_marker_cases(rules, out_dir)
    marker_author_failures, marker_author_rows = run_authorship_case(rules, out_dir)
    clean_failures, clean_rows = run_clean_tree_case(
        rules, out_dir, prose["silent.md"]
    )
    parity_failures, parity_rows = run_parity_case(rules, stage_repo)
    timing_failures, timing_rows = run_timing_case()
    failures += (
        marker_failures
        + marker_author_failures
        + clean_failures
        + parity_failures
        + timing_failures
    )
    rows.extend(marker_rows)
    rows.extend(marker_author_rows)
    rows.extend(clean_rows)
    rows.extend(parity_rows)
    rows.extend(timing_rows)
    return failures, rows


FINDING_RE = re.compile(
    r"^(\S+):(\d+): ([A-Z0-9][A-Z0-9._-]*) family=(\S+) '(.*)': "
)


def parse_findings(stderr_text: str) -> set[tuple[str, int, str, str]]:
    hits: set[tuple[str, int, str, str]] = set()
    for line in stderr_text.split("\n"):
        match = FINDING_RE.match(line)
        if match is not None:
            hits.add((match.group(1), int(match.group(2)), match.group(3), match.group(5)))
    return hits


MARKER_FILE_LINES = [
    "bare marker prose-lint: allow at end\n",
    'empty reason prose-lint: allow reason="  " ok\n',
    "guarded `prose-lint: allow` inside code span stays silent\n",
    'valid prose-lint: allow reason="quoted term" suppresses robust marketing\n',
]


def run_marker_cases(
    rules: Path, out_dir: Path
) -> tuple[int, list[dict[str, Any]]]:
    """MARKER.NO-REASON positives and the guarded/valid silent halves."""
    stage_repo = out_dir / "work" / "marker-stage"
    shutil.rmtree(stage_repo, ignore_errors=True)
    stage_repo.mkdir(parents=True)
    (stage_repo / "m.md").write_text("".join(MARKER_FILE_LINES), encoding="utf-8")
    run_git(stage_repo, "init")
    run_git(stage_repo, "add", "-A")
    run_git(stage_repo, "commit", "-m", "Docs: stage marker fixture")
    proc = run_gate(["--check", "prose", "--mode", "tree"], stage_repo, rules)
    echo(proc)
    findings = parse_findings(proc.stderr)
    failures = 0
    rows: list[dict[str, Any]] = []

    def verdict(name: str, expected_text: str, observed_text: str, ok: bool) -> None:
        nonlocal failures
        result = "PASS" if ok else "FAIL"
        print_verdict(name, "prose", expected_text, observed_text, result)
        rows.append(
            {
                "interface": name,
                "check": "prose",
                "expected": expected_text,
                "observed": observed_text,
                "result": result,
            }
        )
        if not ok:
            failures += 1

    bare = ("m.md", 1, "MARKER.NO-REASON", "prose-lint: allow") in findings
    empty = ("m.md", 2, "MARKER.NO-REASON", "prose-lint: allow") in findings
    verdict("prose.marker.bare", "bare marker raises MARKER.NO-REASON", "raised" if bare else "missing", bare)
    verdict("prose.marker.empty-reason", "empty reason raises MARKER.NO-REASON", "raised" if empty else "missing", empty)
    designed = {
        ("m.md", 1, "MARKER.NO-REASON", "prose-lint: allow"),
        ("m.md", 2, "MARKER.NO-REASON", "prose-lint: allow"),
    }
    extra = findings - designed
    verdict(
        "prose.marker.silent-guarded",
        "code-span guard and valid marker stay silent, nothing extra",
        f"{sorted(extra)}",
        not extra,
    )
    return failures, rows


AUTHORSHIP_BASE_LINES = [
    "alpha\n",
    "alpha\n",
    "alpha\n",
    "alpha\n",
    "one dash \u2014 here\n",
    "alpha\n",
    "alpha\n",
    "alpha\n",
    "alpha\n",
    "distant dash \u2014 silent\n",
    "alpha\n",
    "distant dash \u2014 silent\n",
    "alpha\n",
]


def run_authorship_case(
    rules: Path, out_dir: Path
) -> tuple[int, list[dict[str, Any]]]:
    """R-08: modified one below an added line fires; two below stays silent."""
    stage_repo = out_dir / "work" / "authorship-stage"
    shutil.rmtree(stage_repo, ignore_errors=True)
    stage_repo.mkdir(parents=True)
    doc = stage_repo / "doc.md"
    doc.write_text("".join(AUTHORSHIP_BASE_LINES), encoding="utf-8")
    run_git(stage_repo, "init")
    run_git(stage_repo, "add", "-A")
    run_git(stage_repo, "commit", "-m", "Docs: authorship base")
    lines = list(AUTHORSHIP_BASE_LINES)
    lines.insert(4, "added line \u2014 here\n")
    doc.write_text("".join(lines), encoding="utf-8")
    run_git(stage_repo, "add", "-A")
    run_git(stage_repo, "commit", "-m", "Docs: authorship insert")
    base = run_git(stage_repo, "rev-parse", "HEAD~1").stdout.strip()
    proc = run_gate(
        ["--check", "prose", "--base", base, "--head", "HEAD"], stage_repo, rules
    )
    echo(proc)
    findings = parse_findings(proc.stderr)
    expected = {
        ("doc.md", 5, "XI1.EMDASH", "\u2014"),
        ("doc.md", 6, "XI1.EMDASH", "\u2014"),
    }
    ok = findings == expected
    print_verdict(
        "prose.authorship.modified-one-below",
        "prose",
        "added line 5 and modified line 6 fire, lines 11 and 13 grandfathered",
        f"{sorted(findings)}",
        "PASS" if ok else "FAIL",
    )
    row = {
        "interface": "prose.authorship.modified-one-below",
        "check": "prose",
        "expected": "doc.md:5 new and doc.md:6 modified only",
        "observed": sorted(findings),
        "result": "PASS" if ok else "FAIL",
    }
    return (0 if ok else 1), [row]


def run_clean_tree_case(
    rules: Path, out_dir: Path, silent_src: Path
) -> tuple[int, list[dict[str, Any]]]:
    """US1 scenario 7: clean tree exits 0 with a nonzero units count."""
    stage_repo = out_dir / "work" / "clean-stage"
    shutil.rmtree(stage_repo, ignore_errors=True)
    stage_repo.mkdir(parents=True)
    shutil.copy2(silent_src, stage_repo / "silent.md")
    run_git(stage_repo, "init")
    run_git(stage_repo, "add", "-A")
    run_git(stage_repo, "commit", "-m", "Docs: clean tree base")
    proc = run_gate(["--check", "prose", "--mode", "tree"], stage_repo, rules)
    echo(proc)
    summary = re.search(
        r"prose-lint: (\d+) sources, (\d+) units examined, (\d+) findings",
        proc.stdout,
    )
    ok = (
        proc.returncode == 0
        and summary is not None
        and int(summary.group(1)) == 1
        and int(summary.group(2)) > 0
        and int(summary.group(3)) == 0
    )
    print_verdict(
        "prose.clean-tree",
        "prose",
        "exit 0, one source, units > 0, zero findings",
        proc.stdout.strip(),
        "PASS" if ok else "FAIL",
    )
    row = {
        "interface": "prose.clean-tree",
        "check": "prose",
        "expected": "exit:0 units>0 findings:0",
        "observed": proc.stdout.strip(),
        "result": "PASS" if ok else "FAIL",
    }
    return (0 if ok else 1), [row]


def error_annotation_lines(stderr_text: str) -> list[str]:
    return [line for line in stderr_text.splitlines() if line.startswith("::error")]


def run_parity_case(
    rules: Path, stage_repo: Path
) -> tuple[int, list[dict[str, Any]]]:
    """T026 parity assert (contracts/cli.md section 4, FR-017).

    The same tree-mode prose run over the same staged repository, once with
    GITHUB_ACTIONS=true in the child environment and once without it, must
    yield identical finding sets once the ::error workflow annotation lines
    are removed, and those annotations must appear only in the
    GITHUB_ACTIONS run, one per finding.
    """
    plain_env = {k: v for k, v in os.environ.items() if k != "GITHUB_ACTIONS"}
    actions_env = {**os.environ, "GITHUB_ACTIONS": "true"}
    plain = run_gate(
        ["--check", "prose", "--mode", "tree"], stage_repo, rules, env=plain_env
    )
    actions = run_gate(
        ["--check", "prose", "--mode", "tree"], stage_repo, rules, env=actions_env
    )
    plain_findings = parse_findings(plain.stderr)
    actions_findings = parse_findings(actions.stderr)
    plain_marks = error_annotation_lines(plain.stderr)
    actions_marks = error_annotation_lines(actions.stderr)

    failures = 0
    rows: list[dict[str, Any]] = []

    def verdict(name: str, expected_text: str, observed_text: str, ok: bool) -> None:
        nonlocal failures
        result = "PASS" if ok else "FAIL"
        print_verdict(name, "prose", expected_text, observed_text, result)
        rows.append(
            {
                "interface": name,
                "check": "prose",
                "expected": expected_text,
                "observed": observed_text,
                "result": result,
            }
        )
        if not ok:
            failures += 1

    verdict(
        "prose.parity.finding-sets",
        "identical finding sets with and without GITHUB_ACTIONS",
        f"{len(plain_findings)} vs {len(actions_findings)} findings, "
        f"{len(plain_findings ^ actions_findings)} unmatched",
        plain_findings == actions_findings,
    )
    verdict(
        "prose.parity.annotations",
        "::error lines only in the GITHUB_ACTIONS run, one per finding",
        f"plain:{len(plain_marks)} actions:{len(actions_marks)} "
        f"for {len(actions_findings)} findings",
        not plain_marks
        and bool(actions_marks)
        and len(actions_marks) == len(actions_findings),
    )
    return failures, rows


TIMING_SUMMARY_RE = re.compile(r"prose-lint: .*")


def run_timing_case() -> tuple[int, list[dict[str, Any]]]:
    """T026 timing assert (SC-003, plan R-04 measured 0.062 s whole tree).

    The whole-tree prose scan of the real repository (REPO_ROOT, not the
    throwaway fixture repo) against the repo rules file must finish under
    the 10 s bound. It is a developer-machine bound, so the case is
    evaluated only when GITHUB_ACTIONS is unset in this harness process;
    under CI it prints a SKIP verdict instead of holding a developer bound
    against runner load.

    Adapted from the task sketch's `--paths .`: the gate's in_narrowed()
    matches repo-relative paths against `p` or `p/` prefixes, and no such
    path equals `.` or starts with `./`, so that invocation reports 0
    sources and times an empty scan, the vacuous pass the repository
    treats as a defect. The case uses plain tree-mode discovery, the
    canonical SC-003 surface (68 sources on this checkout). The real tree
    carries grandfathered banned tokens, so whole-file mode legitimately
    exits 1 with findings; the assert measures wall clock and accepts
    exit 0 or 1, never 2, because exit 2 means nothing was checked and no
    bound was proven.
    """
    name = "timing.whole-tree"
    expected = f"exit:0|1 wall<{TIMING_BOUND_SECONDS}s"
    if "GITHUB_ACTIONS" in os.environ:
        observed = "skipped: developer-machine bound under GITHUB_ACTIONS"
        print_verdict(name, "timing", expected, observed, "SKIP")
        return 0, [
            {
                "interface": name,
                "check": "timing",
                "expected": expected,
                "observed": observed,
                "result": "SKIP",
            }
        ]
    started = time.monotonic()
    proc = run_gate(["--check", "prose", "--mode", "tree"], REPO_ROOT, DEFAULT_RULES)
    elapsed = time.monotonic() - started
    ok = proc.returncode in (0, 1) and elapsed < TIMING_BOUND_SECONDS
    summary = TIMING_SUMMARY_RE.search(proc.stdout)
    observed = f"exit:{proc.returncode} wall:{elapsed:.3f}s"
    if summary is not None:
        observed += f" {summary.group(0)}"
    result = "PASS" if ok else "FAIL"
    print_verdict(name, "timing", expected, observed, result)
    row = {
        "interface": name,
        "check": "timing",
        "expected": expected,
        "observed": observed,
        "result": result,
        "exit_code": proc.returncode,
        "elapsed_seconds": round(elapsed, 3),
    }
    return (0 if ok else 1), [row]


SUMMARY_SOURCES_RE = re.compile(r"prose-lint: (\d+) sources,")


COMMIT_FINDING_RE = re.compile(
    r"^commit ([0-9a-f]{7}): ([A-Z0-9][A-Z0-9._-]*)"
    r"(?: family=(\S+) '([^']*)')?: (.*)$"
)


def commit_findings(stderr_text: str) -> list[tuple[str, str, str]]:
    """(short hash, rule id, token) triples from the gate's finding lines."""
    findings: list[tuple[str, str, str]] = []
    for line in stderr_text.splitlines():
        match = COMMIT_FINDING_RE.match(line)
        if match:
            findings.append((match.group(1), match.group(2), match.group(4)))
    return findings


def scope_field_text(case: dict[str, Any], scope: str) -> str:
    if scope == "commit-title":
        return str(case.get("title") or "")
    body = case.get("body")
    if isinstance(body, list):
        return "\n".join(str(line) for line in body)
    return str(body or "")


def evaluate_commit_case(
    case: dict[str, Any], proc: subprocess.CompletedProcess[str]
) -> tuple[str, str, str]:
    """PASS only when exit code, findings, scopes, and records all match."""
    expect = case["expect"]
    want_rules = sorted(
        str(entry.get("rule")) for entry in expect.get("findings") or []
    )
    want_records = expect.get("records")
    findings = commit_findings(proc.stderr)
    got_rules = sorted(rule for _, rule, _ in findings)
    expected = f"exit:{expect['exit']} findings:[{','.join(want_rules)}]"
    observed = f"exit:{proc.returncode} findings:[{','.join(got_rules)}]"
    if want_records is not None:
        summary = SUMMARY_SOURCES_RE.search(proc.stdout)
        got_records = int(summary.group(1)) if summary else -1
        expected += f" records:{want_records}"
        observed += f" records:{got_records}"
        if got_records != want_records:
            return expected, observed, "FAIL"
    if proc.returncode != expect["exit"] or got_rules != want_rules:
        return expected, observed, "FAIL"
    for entry in expect.get("findings") or []:
        scope = entry.get("scope")
        if not scope:
            continue
        rule = str(entry.get("rule"))
        tokens = [tok for _, matched, tok in findings if matched == rule]
        text = scope_field_text(case, str(scope))
        if not any(tok and tok in text for tok in tokens):
            return expected, observed + f" scope:{scope}-mismatch", "FAIL"
    return expected, observed, "PASS"


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
        expected, observed, result = evaluate_commit_case(case, proc)
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
    mixed_failures, mixed_rows = run_mixed_range_case(rules, stage_repo.parent)
    failures += mixed_failures
    rows.extend(mixed_rows)
    return failures, rows


def run_mixed_range_case(
    rules: Path, work_dir: Path
) -> tuple[int, list[dict[str, Any]]]:
    """US2 mixed range (T023): one range, two offenders, named by short hash."""
    stage_repo = work_dir / "mixed-range"
    shutil.rmtree(stage_repo, ignore_errors=True)
    stage_repo.mkdir(parents=True)
    run_git(stage_repo, "init")
    run_git(
        stage_repo,
        "commit",
        "--allow-empty",
        "-m",
        assemble_message(
            "test: seed mixed range fixture base",
            None,
            "Approved-by: charlesarcher",
        ),
    )
    base = git_head(stage_repo)
    run_git(
        stage_repo,
        "commit",
        "--allow-empty",
        "-m",
        assemble_message(
            "runner guard teardown of idle workers",
            None,
            "Approved-by: charlesarcher",
        ),
    )
    first = git_head(stage_repo)[:7]
    run_git(
        stage_repo,
        "commit",
        "--allow-empty",
        "-m",
        assemble_message(
            "wip",
            None,
            "Approved-by: charlesarcher",
        ),
    )
    head = git_head(stage_repo)
    second = head[:7]
    proc = run_gate(
        ["--check", "commit", "--base", base, "--head", head],
        stage_repo,
        rules,
    )
    got = {(short, rule) for short, rule, _ in commit_findings(proc.stderr)}
    want = {
        (first, "CM.TITLE-FORMAT"),
        (second, "CM.TITLE-FORMAT"),
        (second, "CM.VAGUE-TITLE"),
    }
    ok = proc.returncode == 1 and got == want
    expected = (
        f"exit:1 findings:[{first}:CM.TITLE-FORMAT,"
        f"{second}:CM.TITLE-FORMAT,{second}:CM.VAGUE-TITLE]"
    )
    observed = (
        f"exit:{proc.returncode} "
        f"findings:[{','.join(f'{s}:{r}' for s, r in sorted(got))}]"
    )
    result = "PASS" if ok else "FAIL"
    print_verdict(
        "us2-mixed-range-two-offenders", "commit", expected, observed, result
    )
    echo(proc)
    row = {
        "interface": "us2-mixed-range-two-offenders",
        "check": "commit",
        "expected": expected,
        "observed": observed,
        "result": result,
        "exit_code": proc.returncode,
        "base": base,
        "head": head,
    }
    return (0 if ok else 1), [row]


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
        prose_failures, prose_rows = run_prose_check(prose, rules, out_dir)
        failures += prose_failures
        rows.extend(prose_rows)
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
    print("prose gate assertions passed", flush=True)
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
