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
# 019 FR-006: a program that opens counters prints one column per name after
# the metric fields and one statistic line per name in the aggregate family.
COUNTER_ROW = STDOUT[1] + "  bytes_per_second=1048576.0"
COUNTER_STAT = (
    "bmSum            bytes_per_second mean=1048576.0 median=1048576.0 "
    "sd=1.2 cv=0.0018 min=1048575.0 max=1048577.0 n=2"
)
# 019 FR-011: a labelled row carries the trailing label field, and the
# aggregate carries it only when every repetition shares it.
LABEL_ROW = STDOUT[1] + "  label=avx2"
LABEL_AGGREGATE = AGGREGATE + "  label=avx2"

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

    eaten = STDOUT[1].replace("real time/iter (ns)=  660.5",
                              "real time/iter (ns)=11052.473")
    check("a wider value that eats the padding after an equals sign folds equal",
          mask(STDOUT[1]) == mask(eaten),
          f"{mask(STDOUT[1])!r} against {mask(eaten)!r}")

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

    wider_counter = COUNTER_ROW.replace("bytes_per_second=1048576.0",
                                       "bytes_per_second=    1048576.02")
    check("a counter column whose value eats the field padding folds equal",
          mask(COUNTER_ROW) == mask(wider_counter),
          f"{mask(COUNTER_ROW)!r} against {mask(wider_counter)!r}")

    renamed_counter = COUNTER_ROW.replace("bytes_per_second", "items_per_second")
    check("a renamed counter column folds different",
          mask(COUNTER_ROW) != mask(renamed_counter),
          "both folded to the same shape")

    check("a counter statistic line classifies as a statistic line",
          "omits this statistic line" in " ".join(
              compare(list(STDOUT), [*STDOUT, COUNTER_STAT])),
          compare(list(STDOUT), [*STDOUT, COUNTER_STAT])[:1])

    # FR-011: the label is a field of the row shape, so the fold keeps it and
    # a block that leaves it out cannot describe a run that prints it.
    check("a row that carries a label folds different from one that does not",
          mask(LABEL_ROW) != mask(STDOUT[1]),
          f"{mask(LABEL_ROW)!r} against {mask(STDOUT[1])!r}")

    other_label = LABEL_ROW.replace("label=avx2", "label=sse")
    check("another label text folds different",
          mask(LABEL_ROW) != mask(other_label),
          f"{mask(LABEL_ROW)!r} against {mask(other_label)!r}")

    check("an aggregate that carries the shared label folds different",
          mask(LABEL_AGGREGATE) != mask(AGGREGATE),
          f"{mask(LABEL_AGGREGATE)!r} against {mask(AGGREGATE)!r}")

    check("a labelled row still classifies as a row line",
          "row line differs" in " ".join(compare([LABEL_ROW], [STDOUT[1]])),
          " ".join(compare([LABEL_ROW], [STDOUT[1]]))[:1])


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

        # Each case names the block it plants and the stdout the fake binary
        # prints, so the pair the comparator sees stands in one line.
        counter_run = (STDOUT[0], COUNTER_ROW, STDOUT[2], COUNTER_STAT)
        label_run = (STDOUT[0], LABEL_ROW, LABEL_AGGREGATE, STDOUT[2])
        cases = (
            ("a block that matches the run in shape", True, list(STDOUT), STDOUT),
            ("a planted renamed column header", False,
             [STDOUT[0], STDOUT[1].replace("real time/iter", "wall time/iter"),
              STDOUT[2]], STDOUT),
            ("a planted missing row line", False, list(STDOUT[:2]), STDOUT),
            ("a planted extra aggregate line", False,
             [STDOUT[0], STDOUT[1], AGGREGATE, STDOUT[2]], STDOUT),
            ("a block that carries the counter columns of the run", True,
             [STDOUT[0], COUNTER_ROW, STDOUT[2], COUNTER_STAT], counter_run),
            # FR-006, SC-02: a run that opens counters cannot be described by
            # a block that omits them, so the built-in sample keeps its exact
            # bytes only for a program that opens none.
            ("a run with counters against a block without them", False,
             list(STDOUT), counter_run),
            # FR-011: the trailing label field belongs to the row shape on both
            # sides, so a block that carries it matches the run that prints it
            # and fails a run that prints none.
            ("a block that carries the label the run prints", True,
             [STDOUT[0], LABEL_ROW, LABEL_AGGREGATE, STDOUT[2]], label_run),
            ("a run that prints a label the block omits", False,
             [STDOUT[0], STDOUT[1], AGGREGATE, STDOUT[2]], label_run),
            ("a block that carries a label the run does not print", False,
             [STDOUT[0], LABEL_ROW, LABEL_AGGREGATE, STDOUT[2]],
             list(STDOUT)),
        )
        for case, want_pass, block, stdout in cases:
            root.joinpath("README.md").write_text(page_text(block))
            code, output = run_tool(root, tool, stdout)
            check(case, (code == 0) == want_pass,
                  f"exit {code}: {output.strip().splitlines()[:1]}")

        root.joinpath("README.md").write_text(
            page_text([STDOUT[0], COUNTER_ROW.replace("bytes_per_second",
                                                     "items_per_second"),
                       STDOUT[2], COUNTER_STAT]))
        code, output = run_tool(
            root, tool, (STDOUT[0], COUNTER_ROW, STDOUT[2], COUNTER_STAT))
        check("a block naming another counter name fails",
              code != 0, f"exit {code}: {output.strip().splitlines()[:1]}")

        sanitizer = (STDOUT[0].replace("build=Release", "build=Sanitize"),
                     *STDOUT[1:])
        root.joinpath("README.md").write_text(page_text(list(STDOUT)))
        code, output = run_tool(root, tool, sanitizer)
        check("a run from another build type passes",
              code == 0, f"exit {code}: {output.strip().splitlines()[:1]}")

        wide = (STDOUT[0],
                STDOUT[1].replace(
                    "real time/iter (ns)=  660.5  cpu time/iter (ns)=  659.4",
                    "real time/iter (ns)=11052.473  cpu time/iter (ns)=10843.136"),
                STDOUT[2])
        root.joinpath("README.md").write_text(page_text(list(STDOUT)))
        code, output = run_tool(root, tool, wide)
        check("a run whose wider values eat the field padding passes",
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
