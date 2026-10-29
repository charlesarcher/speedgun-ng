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
# contexts share one cpu or outnumber the cpus the host has online. The
# account flag stands after a slot that named a cpu, so the no-pin line
# holds the wait mode and the placement word.
DISCLOSURE = "  contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN"
PINNED = "  contexts=2  ctx0=SPIN/3/applied  ctx1=SPIN/7/applied"
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
        "  contexts=4  ctx1=SPIN/NO_PIN"
        "  ctx3=SPIN/NO_PIN"
    )
    check("another context count and index fold equal",
          mask(DISCLOSURE) == mask(other_counts),
          f"{mask(DISCLOSURE)!r} against {mask(other_counts)!r}")

    other_cpus = "  contexts=2  ctx0=SPIN/12/applied  ctx1=SPIN/70/applied"
    check("another pinned cpu index folds equal",
          mask(PINNED) == mask(other_cpus),
          f"{mask(PINNED)!r} against {mask(other_cpus)!r}")

    check("a flipped applied flag folds different",
          mask(PINNED) != mask(PINNED.replace("applied", "unapplied")),
          "both folded to the same shape")

    # FR-025: the flag belongs to the shape of the field, so a block that
    # prints one cannot describe a run whose slot named no cpu.
    flagless = "  contexts=2  ctx0=SPIN/NO_PIN"
    check("a present account flag folds different from an absent one",
          mask(flagless + "/applied") != mask(flagless),
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

    # specs/034 FR-017, FR-018: the pre-check block's host-owned words fold
    # beside the run-owned values: the float edge words an unlucky window
    # prints, the governor a host runs, the kernel setting values and the irq
    # affinity list the host states, each beside the source token that keeps
    # its bytes.
    quiet = "gapsPerSecond: 259.19"
    unlucky = "gapsPerSecond: inf"
    check("a float edge word a kept-single-gap window prints folds equal",
          mask(quiet) == mask(unlucky),
          f"{mask(quiet)!r} against {mask(unlucky)!r}")

    check("a nan figure folds equal beside a figure",
          mask("totalPercent: 0.22") == mask("totalPercent: nan"),
          f"{mask('totalPercent: 0.22')!r} against {mask('totalPercent: nan')!r}")

    governor = ("governor: cpu 24 powersave "
                "(source: /sys devices system cpu scaling_governor)")
    other_governor = governor.replace("powersave", "performance")
    check("another governor a host runs folds equal",
          mask(governor) == mask(other_governor),
          f"{mask(governor)!r} against {mask(other_governor)!r}")

    absent_governor = governor.replace("powersave", "not available")
    check("an absent governor folds equal beside a stated one",
          mask(governor) == mask(absent_governor),
          f"{mask(governor)!r} against {mask(absent_governor)!r}")

    renamed_governor_source = governor.replace("scaling_governor", "scaling_driver")
    check("a renamed governor source folds different",
          mask(governor) != mask(renamed_governor_source),
          "both folded to the same shape")

    settings = ("host settings: isolcpus=not available nohz_full=not available "
                "rcu_nocbs=not available")
    stated = "host settings: isolcpus=2-3 nohz_full=1 rcu_nocbs=0-1"
    check("the kernel settings a host states fold equal beside absent ones",
          mask(settings) == mask(stated),
          f"{mask(settings)!r} against {mask(stated)!r}")

    renamed_setting = stated.replace("isolcpus", "isolcpus-set")
    check("a renamed kernel setting folds different",
          mask(stated) != mask(renamed_setting), "both folded to the same shape")

    affinity = ("irq affinity: cpu 24 " + ",".join(str(n) for n in range(1, 40))
                + " (source: /proc/irq)")
    short_affinity = "irq affinity: cpu 24 not available (source: /proc/irq)"
    check("an irq affinity list folds equal beside an absent one",
          mask(affinity) == mask(short_affinity),
          f"{mask(affinity)!r} against {mask(short_affinity)!r}")

    renamed_affinity_source = affinity.replace("/proc/irq", "/proc/irqs")
    check("a renamed affinity source folds different",
          mask(affinity) != mask(renamed_affinity_source),
          "both folded to the same shape")

    renamed_kind = "softirq TIMER delta: 3 (source: /proc/softirqs)"
    other_kind = "softirq TIMERS delta: 3 (source: /proc/softirqs)"
    check("a renamed softirq kind folds different",
          mask(renamed_kind) != mask(other_kind), "both folded to the same shape")

    interrupts = "interrupts delta: 156 (source: /proc/interrupts)"
    absent_interrupts = "interrupts delta: not available (source: /proc/interrupts)"
    check("an attribution delta figure folds equal beside an absent one",
          mask(interrupts) == mask(absent_interrupts),
          f"{mask(interrupts)!r} against {mask(absent_interrupts)!r}")

    renamed_interrupts_source = interrupts.replace("/proc/interrupts",
                                                   "/proc/misread")
    check("a renamed attribution source folds different",
          mask(interrupts) != mask(renamed_interrupts_source),
          "both folded to the same shape")

    softirq = "softirq TIMER delta: 3 (source: /proc/softirqs)"
    absent_softirq = "softirq TIMER delta: not available (source: /proc/softirqs)"
    check("an absent softirq delta folds equal beside a stated one",
          mask(softirq) == mask(absent_softirq),
          f"{mask(softirq)!r} against {mask(absent_softirq)!r}")

    schedstat = "switch delta: 5, schedule delta: 40 (source: /proc/schedstat)"
    absent_schedstat = ("switch delta: not available, schedule delta: 40 "
                        "(source: /proc/schedstat)")
    check("a schedstat delta figure folds equal beside an absent one",
          mask(schedstat) == mask(absent_schedstat),
          f"{mask(schedstat)!r} against {mask(absent_schedstat)!r}")

    renamed_schedstat_source = schedstat.replace("/proc/schedstat",
                                                 "/proc/misread")
    check("a renamed schedstat source folds different",
          mask(schedstat) != mask(renamed_schedstat_source),
          "both folded to the same shape")

    # specs/036 FR-023, R-14: the tracer figure line folds the figure
    # beside the not-available wording, the source words and the cpu
    # index keeping their bytes; the header and the disclosure lines
    # keep their bytes.
    tracer_figure = "tracer cpu 1 nmi: 12"
    absent_tracer = "tracer cpu 1 nmi: not available"
    check("a tracer figure folds equal beside an absent one",
          mask(tracer_figure) == mask(absent_tracer),
          f"{mask(tracer_figure)!r} against {mask(absent_tracer)!r}")

    renamed_tracer_source = tracer_figure.replace(" nmi:", " nmi-total:")
    check("a renamed tracer source folds different",
          mask(tracer_figure) != mask(renamed_tracer_source),
          "both folded to the same shape")

    tracer_header = "tracer: engine osnoise cpus 1,9"
    other_header = "tracer: engine timerlat cpus 1,9"
    check("the tracer header line keeps its bytes",
          mask(tracer_header) != mask(other_header),
          "both folded to the same shape")

    tracer_disclosure = "tracer: not available"
    renamed_disclosure = "tracer: unavailable"
    check("the tracer disclosure line folds equal beside itself",
          mask(tracer_disclosure) == mask(tracer_disclosure),
          f"{mask(tracer_disclosure)!r} against the literal")
    check("a renamed tracer disclosure folds different",
          mask(tracer_disclosure) != mask(renamed_disclosure),
          "both folded to the same shape")

    # specs/035 FR-011, D-5: the placement field's node tail folds by the
    # standing digit rule with no fold of its own, so the bound and the
    # unbound shapes fold different, checked in both directions.
    bound = "  contexts=2  ctx0=SPIN/3@1@0/applied  ctx1=SPIN/7@1@0/applied"
    check("a bound context's node tail folds different from the standing pair",
          mask(PINNED) != mask(bound), "both folded to the same shape")
    check("the node tail fails a block that omits it, in both directions",
          "disclosure line differs"
          in " ".join(compare([bound], [PINNED]))
          and "disclosure line differs"
          in " ".join(compare([PINNED], [bound])),
          " ".join(compare([bound], [PINNED]))[:1])

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
