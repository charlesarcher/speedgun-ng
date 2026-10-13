#!/usr/bin/env python3
"""Fixture harness for tools/docs/report_shape.py (contracts/report-shape.md S-5).

The mask golden pairs are pinned constants checked in both directions. The
end-to-end cases run the tool against a scratch page and a fake binary
script that prints pinned stdout, so the planted failure proves the
comparison can fail, not only the fold. The runner imports the mask and the
comparator: with the tool absent, that import is the red state.
"""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "docs"))

try:
    from report_shape import compare, mask
except ImportError as error:
    print(f"red state: the report shape gate has no tool: {error}")
    raise SystemExit(1)

STDOUT = (
    "speedgun-ng 0.8.0 build=Release",
    "bmSum    iterations=1048576  real time/iter (ns)=  660.5  "
    "cpu time/iter (ns)=  659.4  overhead floor (ns)=680.000  endpoint samples=2",
    "bmTouch  iterations=52428800  real time/iter (ns)=0.12  "
    "cpu time/iter (ns)=0.11  overhead floor (ns)=681.000  endpoint samples=2",
)
AGGREGATE = (
    "bmSum     AGGREGATE  primary time/iter (ns) mean=660.1 median=660.1 "
    "sd=1.2 cv=0.0018 min=659.4 max=660.5 n=2"
)

FAILURES: list[str] = []


def check(case: str, ok: bool, detail: str = "") -> None:
    if ok:
        print(f"PASS {case}")
        return
    FAILURES.append(f"{case}: {detail}")
    print(f"FAIL {case}: {detail}")


def golden_pairs() -> None:
    digits = list(STDOUT[1].split())
    other = list(digits)
    other[1] = "iterations=2097152"
    check("a digit-run change folds equal",
          mask(" ".join(digits)) == mask(" ".join(other)),
          f"{mask(' '.join(digits))!r} against {mask(' '.join(other))!r}")

    spread = STDOUT[1].replace("real  time/iter", "real time/iter")
    check("a whitespace-run change folds equal",
          mask(STDOUT[1]) == mask(spread),
          f"{mask(STDOUT[1])!r} against {mask(spread)!r}")

    renamed = STDOUT[1].replace("real time/iter", "wall time/iter")
    check("a header rename folds different",
          mask(STDOUT[1]) != mask(renamed), "both folded to the same shape")

    wider = STDOUT[1].replace("iterations=1048576", "iterations=10485760")
    check("a width-only change folds equal",
          mask(STDOUT[1]) == mask(wider),
          f"{mask(STDOUT[1])!r} against {mask(wider)!r}")

    other_build = STDOUT[0].replace("build=Release", "build=Sanitize")
    check("a build field naming another build folds equal",
          mask(STDOUT[0]) == mask(other_build),
          f"{mask(STDOUT[0])!r} against {mask(other_build)!r}")

    renamed_field = STDOUT[0].replace("build=Release", "built=Release")
    check("a renamed build field folds different",
          mask(STDOUT[0]) != mask(renamed_field), "both folded to the same shape")

    check("a missing aggregate line is a comparator finding",
          any("omits" in line for line in compare(list(STDOUT), [*STDOUT[:1], STDOUT[1],
                                                        AGGREGATE, STDOUT[2]])),
          "the comparator saw no difference")


def page_text(block: list[str]) -> str:
    body = "\n".join(block)
    return (
        "# speedgun-ng\n\nThe sample of one host's run:\n\n"
        "<!-- sg-sample-output: first_benchmark.expected#report -->\n"
        f"```text\n{body}\n```\n"
    )


def run_tool(root: Path, tool: Path,
             stdout: tuple[str, ...] = STDOUT) -> tuple[int, str]:
    binary = root / "fake.sh"
    binary.write_text("#!/bin/sh\ncat <<'EOF'\n" + "\n".join(stdout) + "\nEOF\n")
    binary.chmod(0o755)
    result = subprocess.run(
        [sys.executable, str(tool), "--binary", str(binary),
         "--readme", str(root / "README.md")],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode, result.stdout + result.stderr


def end_to_end(tool: Path) -> None:
    with tempfile.TemporaryDirectory() as name:
        root = Path(name)

        cases = (
            ("a block that matches the run in shape", True, list(STDOUT)),
            ("a planted renamed column header", False,
             [STDOUT[0], STDOUT[1].replace("real time/iter", "wall time/iter"),
              STDOUT[2]]),
            ("a planted missing row line", False, list(STDOUT[:2])),
            ("a planted extra aggregate line", False,
             [STDOUT[0], STDOUT[1], AGGREGATE, STDOUT[2]]),
        )
        for case, want_pass, block in cases:
            root.joinpath("README.md").write_text(page_text(block))
            code, output = run_tool(root, tool)
            check(case, (code == 0) == want_pass,
                  f"exit {code}: {output.strip().splitlines()[:1]}")

        sanitizer = (STDOUT[0].replace("build=Release", "build=Sanitize"),
                     *STDOUT[1:])
        root.joinpath("README.md").write_text(page_text(list(STDOUT)))
        code, output = run_tool(root, tool, sanitizer)
        check("a run from another build type passes",
              code == 0, f"exit {code}: {output.strip().splitlines()[:1]}")


def main() -> int:
    repo = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    tool = repo / "tools" / "docs" / "report_shape.py"
    if not tool.is_file():
        print(f"red state: the report shape gate has no tool at {tool}")
        return 1
    golden_pairs()
    end_to_end(tool)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(main())
