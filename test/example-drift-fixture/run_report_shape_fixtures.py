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
# 019 FR-011: a labeled row carries the trailing label field, and the
# aggregate carries it only when every repetition shares it.
LABEL_ROW = STDOUT[1] + "  label=avx2"
LABEL_AGGREGATE = AGGREGATE + "  label=avx2"
# 020 FR-025, FR-026: a threaded result prints one disclosure line after its
# rows, and one noise warning line after that result when its spinning
# contexts share one cpu or outnumber the cpus the host has online.
DISCLOSURE = (
    "  contexts=2  ctx0=SPIN/NO_PIN/unapplied  ctx1=SPIN/NO_PIN/unapplied"
)
WARNING = "warning: spinning contexts oversubscribed (2 spin, 8 cpus)"
THREAD_RUN = (STDOUT[0], STDOUT[1], DISCLOSURE, WARNING)

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

    # The fit column's value is the class the run chose, so a build that measures
    # differently may print another class for the same program.
    fitted = STDOUT[1] + "  BIG_O  time/iter N (    0.243)"
    other_class = STDOUT[1] + "  BIG_O  time/iter NlgN (    0.169)"
    check("a fitted complexity class the run chose differently folds equal",
          mask(fitted) == mask(other_class),
          f"{mask(fitted)!r} against {mask(other_class)!r}")

    quadratic = STDOUT[1] + "  BIG_O  time/iter N^2 (  104.5)"
    check("a quadratic curve and a linear one fold equal",
          mask(quadratic) == mask(fitted),
          f"{mask(quadratic)!r} against {mask(fitted)!r}")

    constant = STDOUT[1] + "  BIG_O  time/iter 1 (    7.500)"
    user_curve = STDOUT[1] + "  BIG_O  time/iter f(N) (    7.500)"
    check("the constant curve and a user curve fold equal",
          mask(constant) == mask(user_curve),
          f"{mask(constant)!r} against {mask(user_curve)!r}")

    wide = STDOUT[1] + "  BIG_O  time/iter N (999999.999)"
    check("a coefficient wide enough to eat the paren padding folds equal",
          mask(wide) == mask(fitted),
          f"{mask(wide)!r} against {mask(fitted)!r}")

    plain = STDOUT[1] + "  time/iter N (    0.243)"
    other_word = STDOUT[1] + "  time/iter NlgN (    0.243)"
    check("a class word on a line the fit column does not mark folds different",
          mask(plain) != mask(other_word), "both folded to the same shape")

    name = "bmNRows  BIG_O  time/iter N (0.250)"
    check("a benchmark name holding the class letters is not folded",
          mask(name).startswith("bmNRows BIG_O time/iter #"), mask(name))

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

    check("a labeled row still classifies as a row line",
          "row line differs" in " ".join(compare([LABEL_ROW], [STDOUT[1]])),
          " ".join(compare([LABEL_ROW], [STDOUT[1]]))[:1])

    # 020 FR-025, FR-026: the counts of the disclosure line and of the noise
    # warning belong to the run, the mode words and the fixed wording do not.
    other_counts = (
        "  contexts=4  ctx1=SPIN/NO_PIN/unapplied"
        "  ctx3=SPIN/NO_PIN/unapplied"
    )
    check("another context count and index fold equal",
          mask(DISCLOSURE) == mask(other_counts),
          f"{mask(DISCLOSURE)!r} against {mask(other_counts)!r}")

    other_cpus = "  contexts=2  ctx0=SPIN/3/applied  ctx1=SPIN/7/applied"
    unpinned = DISCLOSURE.replace("NO_PIN", "0").replace("unapplied", "applied")
    check("another pinned cpu index folds equal",
          mask(unpinned) == mask(other_cpus),
          f"{mask(unpinned)!r} against {mask(other_cpus)!r}")

    check("a flipped applied flag folds different",
          mask(DISCLOSURE) != mask(DISCLOSURE.replace("unapplied", "applied")),
          "both folded to the same shape")

    other_mode = DISCLOSURE.replace("SPIN", "BLOCK")
    check("another wait mode folds different",
          mask(DISCLOSURE) != mask(other_mode), "both folded to the same shape")

    renamed_placement = DISCLOSURE.replace("NO_PIN", "NONE")
    check("a renamed placement token folds different",
          mask(DISCLOSURE) != mask(renamed_placement),
          "both folded to the same shape")

    renamed_field = DISCLOSURE.replace("ctx0", "context0")
    check("a renamed context field folds different",
          mask(DISCLOSURE) != mask(renamed_field), "both folded to the same shape")

    other_warning = "warning: spinning contexts oversubscribed (5 spin, 24 cpus)"
    check("another spin and cpu count fold equal",
          mask(WARNING) == mask(other_warning),
          f"{mask(WARNING)!r} against {mask(other_warning)!r}")

    renamed_warning = WARNING.replace("oversubscribed", "over subscribed")
    check("a renamed noise warning token folds different",
          mask(WARNING) != mask(renamed_warning), "both folded to the same shape")

    check("a disclosure line classifies as a disclosure line",
          "disclosure line differs" in " ".join(compare([DISCLOSURE], [WARNING])),
          " ".join(compare([DISCLOSURE], [WARNING]))[:1])

    check("a noise warning classifies as a noise warning line",
          "noise warning line differs"
          in " ".join(compare([WARNING], [DISCLOSURE])),
          " ".join(compare([WARNING], [DISCLOSURE]))[:1])


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
            # 020 FR-025, FR-026, SC-11: the disclosure line and the noise
            # warning ride the same line-for-line comparison, and a built-in
            # token planted wrong beside them still fails by its own name.
            ("a block that carries the disclosure line and the warning", True,
             list(THREAD_RUN), THREAD_RUN),
            ("a planted disclosure wait mode", False,
             [STDOUT[0], STDOUT[1], DISCLOSURE.replace("SPIN", "BLOCK"), WARNING],
             THREAD_RUN),
            ("a planted noise warning token", False,
             [STDOUT[0], STDOUT[1], DISCLOSURE,
              WARNING.replace("spinning contexts", "spinning threads")],
             THREAD_RUN),
            ("a run that prints a disclosure line the block omits", False,
             [STDOUT[0], STDOUT[1], WARNING], THREAD_RUN),
            ("a block that carries a warning the run does not print", False,
             list(THREAD_RUN), (STDOUT[0], STDOUT[1], DISCLOSURE)),
            ("a planted renamed column header beside the disclosure line", False,
             [STDOUT[0], STDOUT[1].replace("real time/iter", "wall time/iter"),
              DISCLOSURE, WARNING], THREAD_RUN),
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
