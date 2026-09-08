#!/usr/bin/env python3
"""Gate-effectiveness harness for the Phase 0 DBC coverage gate (T024).

Orchestrates tools/dbc/dbc_doc_gate.py and tools/dbc/dbc_pair_gate.py against
the committed fixtures in this directory and checks the expected verdicts
(SC-006, FR-027/FR-028/FR-031, US4 acceptance 1-5).

This script does not implement either gate. When the gate scripts are absent
it exits non-zero with the diagnostic:

    gate scripts not present (TDD red)

CLI::

    python3 run_gate_fixtures.py --gate <doc|pair|both> \\
        --registry tools/dbc/macros.yaml --out <dir> <fixture-dir>
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError as exc:  # pragma: no cover - environment guard
    sys.stderr.write(
        "run_gate_fixtures.py requires PyYAML (install python3-yaml): "
        f"{exc}\n"
    )
    sys.exit(3)

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent
DOC_GATE = REPO_ROOT / "tools" / "dbc" / "dbc_doc_gate.py"
PAIR_GATE = REPO_ROOT / "tools" / "dbc" / "dbc_pair_gate.py"
DBC_HPP = REPO_ROOT / "include" / "speedgun-ng" / "dbc.hpp"
DEFAULT_REGISTRY = REPO_ROOT / "tools" / "dbc" / "macros.yaml"

CONTRACT_MACRO_RE = re.compile(
    r"^\s*#\s*define\s+(SG_(?:REQUIRE|ENSURE|INVARIANT|ASSERT)(?:_ALWAYS)?)\b",
    re.MULTILINE,
)

SCRIPTS_MISSING = "gate scripts not present (TDD red)"

EXIT_OK = 0
EXIT_ASSERT = 1
EXIT_SCRIPTS_MISSING = 2
EXIT_USAGE = 3

SECTION_TOKENS = ("precondition", "postcondition", "pre", "post", "section")


@dataclass(frozen=True)
class Expectation:
    """Expected verdict for one (fixture, gate) cell of the matrix."""

    result: str  # "pass" or "fail"
    interface: str | None = None
    missing_section: bool = False
    kind: str | None = None
    drift: str | None = None


# Per-fixture assertion matrix (T024). None = run the gate, do not assert.
EXPECTATIONS: dict[str, dict[str, Expectation | None]] = {
    "fixture_missing_docs": {
        "doc": Expectation(
            result="fail",
            interface="missing_docs",
            missing_section=True,
        ),
        "pair": None,
    },
    "fixture_drift": {
        "doc": None,
        "pair": Expectation(
            result="fail",
            interface="drift",
            kind="precondition",
            drift="documented-not-enforced",
        ),
    },
    "fixture_enforced_not_documented": {
        "doc": None,
        "pair": Expectation(
            result="fail",
            interface="enforced_not_documented",
            kind="precondition",
            drift="enforced-not-documented",
        ),
    },
    "fixture_exempt": {
        "doc": Expectation(result="pass"),
        "pair": Expectation(result="pass"),
    },
    "fixture_clean": {
        "doc": Expectation(result="pass"),
        "pair": Expectation(result="pass"),
    },
}


def die(message: str, code: int = EXIT_USAGE) -> None:
    sys.stdout.flush()
    sys.stderr.write(f"{message}\n")
    raise SystemExit(code)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="run_gate_fixtures.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=(
            "Run the Phase 0 DBC gate halves against the committed "
            "gate-effectiveness fixtures and check the expected verdicts."
        ),
        epilog="""\
gates:
  doc    tools/dbc/dbc_doc_gate.py  (documentation-presence, FR-027)
  pair   tools/dbc/dbc_pair_gate.py (doc-to-enforcement pairing, FR-028)
  both   run both halves (default)

expected matrix:
  fixture_missing_docs              doc  FAILURE  (interface + missing section)
  fixture_drift                     pair FAILURE  (interface + kind + drift)
  fixture_enforced_not_documented   pair FAILURE  (interface + kind + drift)
  fixture_exempt                    both PASS
  fixture_clean                     both PASS

every run writes a DBC matrix artifact under --out (FR-031).
when the gate scripts are absent the harness exits 2 with:
  gate scripts not present (TDD red)

examples:
  python3 run_gate_fixtures.py --gate both \\
      --registry tools/dbc/macros.yaml --out /tmp/dbc-matrix \\
      test/dbc-gate-fixture
""",
    )
    parser.add_argument(
        "--gate",
        choices=("doc", "pair", "both"),
        default="both",
        help="which gate half to run (default: both)",
    )
    parser.add_argument(
        "--registry",
        type=Path,
        default=DEFAULT_REGISTRY,
        help="path to tools/dbc/macros.yaml (default: repo macros.yaml)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="directory for DBC matrix artifacts (created if missing)",
    )
    parser.add_argument(
        "fixture_dir",
        type=Path,
        help="directory containing fixture_*.hpp",
    )
    return parser.parse_args(argv)


def selected_gates(gate: str) -> list[str]:
    if gate == "both":
        return ["doc", "pair"]
    return [gate]


def script_for(gate: str) -> Path:
    return DOC_GATE if gate == "doc" else PAIR_GATE


def check_registry(registry_path: Path) -> list[str]:
    """Every contract SG_* in dbc.hpp has exactly one macros.yaml entry."""
    if not registry_path.is_file():
        die(f"registry not found: {registry_path}")
    if not DBC_HPP.is_file():
        die(f"dbc.hpp not found: {DBC_HPP}")

    try:
        loaded = yaml.safe_load(registry_path.read_text(encoding="utf-8"))
    except yaml.YAMLError as exc:
        die(f"registry is not valid YAML: {exc}")

    if not isinstance(loaded, list):
        die("registry must be a YAML list of macro entries")

    registry_macros: set[str] = set()
    for entry in loaded:
        if not isinstance(entry, dict) or "macro" not in entry:
            die(f"registry entry missing 'macro': {entry!r}")
        name = str(entry["macro"])
        if name in registry_macros:
            die(f"registry has duplicate entry for {name}")
        registry_macros.add(name)

    header_macros = set(CONTRACT_MACRO_RE.findall(DBC_HPP.read_text(encoding="utf-8")))

    missing_in_registry = sorted(header_macros - registry_macros)
    missing_in_header = sorted(registry_macros - header_macros)
    if missing_in_registry or missing_in_header:
        parts = []
        if missing_in_registry:
            parts.append(
                "in dbc.hpp but not macros.yaml: " + ", ".join(missing_in_registry)
            )
        if missing_in_header:
            parts.append(
                "in macros.yaml but not dbc.hpp: " + ", ".join(missing_in_header)
            )
        die("registry completeness failed: " + "; ".join(parts))

    ordered = sorted(registry_macros)
    print(
        f"REGISTRY  ok  {len(ordered)} contract macros match "
        f"{registry_path} and {DBC_HPP}",
        flush=True,
    )
    return ordered


def discover_fixtures(fixture_dir: Path) -> dict[str, Path]:
    if not fixture_dir.is_dir():
        die(f"fixture directory not found: {fixture_dir}")
    found = {path.stem: path for path in sorted(fixture_dir.glob("fixture_*.hpp"))}
    missing = [name for name in EXPECTATIONS if name not in found]
    if missing:
        die("missing fixture files: " + ", ".join(missing))
    return found


def write_matrix(out_dir: Path, payload: dict[str, Any]) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / "dbc_matrix.yaml"
    path.write_text(
        yaml.safe_dump(payload, sort_keys=False, allow_unicode=True),
        encoding="utf-8",
    )
    print(f"MATRIX  {path}", flush=True)
    return path


def format_expected(exp: Expectation | None) -> str:
    if exp is None:
        return "unspecified"
    return "PASS" if exp.result == "pass" else "FAILURE"


def diagnostic_matches(text: str, exp: Expectation) -> bool:
    lowered = text.lower()
    if exp.interface and exp.interface not in text:
        return False
    if exp.missing_section:
        if "missing" not in lowered:
            return False
        if not any(token in lowered for token in SECTION_TOKENS):
            return False
    if exp.kind and exp.kind.lower() not in lowered:
        return False
    if exp.drift and exp.drift.lower() not in lowered:
        return False
    return True


def print_verdict(
    fixture: str,
    gate: str,
    expected: str,
    observed: str,
    result: str,
) -> None:
    print(
        f"VERDICT  fixture={fixture}  gate={gate}  "
        f"expected={expected}  observed={observed}  {result}",
        flush=True,
    )


def emit_scripts_missing_verdicts(gates: list[str]) -> None:
    for fixture in EXPECTATIONS:
        for gate in gates:
            exp = EXPECTATIONS[fixture][gate]
            expected = format_expected(exp)
            if exp is None:
                print_verdict(fixture, gate, expected, SCRIPTS_MISSING, "SKIP")
            else:
                print_verdict(fixture, gate, expected, SCRIPTS_MISSING, "FAIL")


def stage_fixture(src: Path, work_dir: Path) -> Path:
    work_dir.mkdir(parents=True, exist_ok=True)
    dest = work_dir / src.name
    shutil.copy2(src, dest)
    return dest


def generate_doxygen_xml(src_dir: Path, xml_root: Path) -> Path:
    doxygen = shutil.which("doxygen")
    if doxygen is None:
        die("doxygen not found on PATH (needed to feed dbc_doc_gate.py)")
    xml_root.mkdir(parents=True, exist_ok=True)
    doxyfile = xml_root / "Doxyfile"
    xml_out = xml_root / "xml"
    doxyfile.write_text(
        "\n".join(
            [
                "PROJECT_NAME = dbc-gate-fixture",
                f"OUTPUT_DIRECTORY = {xml_root}",
                "GENERATE_HTML = NO",
                "GENERATE_LATEX = NO",
                "GENERATE_XML = YES",
                "XML_OUTPUT = xml",
                f"INPUT = {src_dir}",
                "EXTRACT_ALL = YES",
                "EXTRACT_STATIC = YES",
                "ENABLE_PREPROCESSING = YES",
                "MACRO_EXPANSION = NO",
                "SEARCH_INCLUDES = NO",
                "QUIET = YES",
                "WARNINGS = NO",
                "RECURSIVE = NO",
                "",
            ]
        ),
        encoding="utf-8",
    )
    completed = subprocess.run(
        [doxygen, str(doxyfile)],
        cwd=xml_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode != 0:
        sys.stderr.write(completed.stdout)
        sys.stderr.write(completed.stderr)
        die(f"doxygen failed for {src_dir} (exit {completed.returncode})")
    if not (xml_out / "index.xml").is_file() and not xml_out.is_dir():
        die(f"doxygen produced no XML under {xml_out}")
    return xml_out


def run_doc_gate(
    staged_dir: Path,
    registry: Path,
    matrix_path: Path,
    xml_root: Path,
) -> subprocess.CompletedProcess[str]:
    xml_dir = generate_doxygen_xml(staged_dir, xml_root)
    argv = [
        sys.executable,
        str(DOC_GATE),
        "--xml",
        str(xml_dir),
        "--registry",
        str(registry),
        "--out",
        str(matrix_path),
    ]
    try:
        return subprocess.run(argv, capture_output=True, text=True, check=False)
    except OSError as exc:
        die(f"failed to invoke {DOC_GATE}: {exc}")


def run_gate(
    gate: str,
    staged_dir: Path,
    registry: Path,
    matrix_path: Path,
    xml_root: Path,
    doc_matrix_path: Path | None = None,
) -> subprocess.CompletedProcess[str]:
    script = script_for(gate)
    matrix_path.parent.mkdir(parents=True, exist_ok=True)
    if gate == "doc":
        return run_doc_gate(staged_dir, registry, matrix_path, xml_root)
    if doc_matrix_path is None or not doc_matrix_path.is_file():
        doc_matrix_path = matrix_path.parent / f"{matrix_path.stem}_doc_input.yaml"
        doc_proc = run_doc_gate(
            staged_dir, registry, doc_matrix_path, xml_root
        )
        if doc_proc.returncode not in (0, 1):
            return doc_proc
    argv = [
        sys.executable,
        str(script),
        "--src",
        str(staged_dir),
        "--src2",
        str(staged_dir),
        "--registry",
        str(registry),
        "--doc-matrix",
        str(doc_matrix_path),
        "--out",
        str(matrix_path),
    ]
    try:
        return subprocess.run(argv, capture_output=True, text=True, check=False)
    except OSError as exc:
        die(f"failed to invoke {script}: {exc}")


def load_optional_matrix(path: Path) -> Any:
    if not path.is_file():
        return None
    text = path.read_text(encoding="utf-8")
    if path.suffix == ".json":
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return text
    try:
        return yaml.safe_load(text)
    except yaml.YAMLError:
        return text


def evaluate_run(exp: Expectation | None, proc: subprocess.CompletedProcess[str]) -> tuple[str, str]:
    """Return (observed, result) for one gate invocation."""
    output = (proc.stdout or "") + (proc.stderr or "")
    passed = proc.returncode == 0
    observed = "PASS" if passed else "FAILURE"
    if exp is None:
        return observed, "SKIP"
    if exp.result == "pass":
        if passed:
            return observed, "PASS"
        return observed, "FAIL"
    # expected fail: non-zero exit and diagnostic content
    if passed:
        return observed, "FAIL"
    if diagnostic_matches(output, exp):
        return observed, "PASS"
    return "FAILURE (diagnostic mismatch)", "FAIL"


def run_matrix(
    gates: list[str],
    fixtures: dict[str, Path],
    registry: Path,
    out_dir: Path,
) -> tuple[int, list[dict[str, Any]]]:
    failures = 0
    rows: list[dict[str, Any]] = []
    for fixture, src in fixtures.items():
        if fixture not in EXPECTATIONS:
            continue
        staged_dir = out_dir / "work" / fixture
        stage_fixture(src, staged_dir)
        doc_matrix_path = out_dir / f"{fixture}_doc.yaml"
        for gate in gates:
            exp = EXPECTATIONS[fixture][gate]
            matrix_path = out_dir / f"{fixture}_{gate}.yaml"
            xml_root = out_dir / "work" / f"{fixture}_{gate}_xml"
            proc = run_gate(
                gate,
                staged_dir,
                registry,
                matrix_path,
                xml_root,
                doc_matrix_path=doc_matrix_path,
            )
            observed, result = evaluate_run(exp, proc)
            print_verdict(fixture, gate, format_expected(exp), observed, result)
            if proc.stdout:
                sys.stdout.write(proc.stdout)
                if not proc.stdout.endswith("\n"):
                    sys.stdout.write("\n")
            if proc.stderr:
                sys.stderr.write(proc.stderr)
                if not proc.stderr.endswith("\n"):
                    sys.stderr.write("\n")
            if result == "FAIL":
                failures += 1
            rows.append(
                {
                    "interface": fixture,
                    "gate": gate,
                    "expected": format_expected(exp),
                    "observed": observed,
                    "result": result,
                    "exit_code": proc.returncode,
                    "matrix": load_optional_matrix(matrix_path),
                }
            )
    return failures, rows


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    registry = args.registry.expanduser().resolve()
    fixture_dir = args.fixture_dir.expanduser().resolve()
    out_dir = args.out.expanduser().resolve()
    gates = selected_gates(args.gate)

    print(
        f"DBC gate-effectiveness harness  gate={args.gate}  "
        f"registry={registry}  fixtures={fixture_dir}  out={out_dir}",
        flush=True,
    )

    check_registry(registry)
    fixtures = discover_fixtures(fixture_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    missing = [script_for(gate) for gate in gates if not script_for(gate).is_file()]
    if missing:
        write_matrix(
            out_dir,
            {
                "status": "scripts_missing",
                "diagnostic": SCRIPTS_MISSING,
                "missing_scripts": [str(path) for path in missing],
                "interfaces": [],
            },
        )
        emit_scripts_missing_verdicts(gates)
        sys.stdout.flush()
        sys.stderr.write(f"{SCRIPTS_MISSING}\n")
        for path in missing:
            sys.stderr.write(f"  missing: {path}\n")
        sys.stderr.flush()
        return EXIT_SCRIPTS_MISSING

    failures, rows = run_matrix(gates, fixtures, registry, out_dir)
    write_matrix(
        out_dir,
        {
            "status": "ok" if failures == 0 else "assert_failed",
            "interfaces": rows,
        },
    )
    if failures:
        sys.stderr.write(
            f"gate-effectiveness assertions failed: {failures} FAIL verdict(s)\n"
        )
        return EXIT_ASSERT
    print("gate-effectiveness assertions passed", flush=True)
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
