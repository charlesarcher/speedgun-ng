#!/usr/bin/env python3
"""Compare the sample output block with the report the program really prints.

Contract: specs/018-docs-main-page/contracts/report-shape.md. The mask is
the row-shape fold the harness test suite owns (test/source/harness_cli_test.cpp):
a digit run folds to one '#', a whitespace run to one ' ', any other
character copies. Widths are not compared, because a sanitizer run shifts
field padding, and the check folds whitespace after an '=' away with it: a
value one column wider eats that padding, so the same column reads "= #" on
one host and "=#" on another. The build field of the header is folded as
well: it names the configuration of the build that ran, so the sanitize and
coverage builds print another value for one and the same page (contract S-2).
Counter columns ride the same fold: a counter column and a counter statistic
line belong to the row shape, so a program that opens counters prints them and
the block carries them, while the built-in fields keep the bytes they always
kept (019 report-shape contract, FR-006, SC-02). The trailing label field rides
it too: the field is text the run owns, so a block that carries a label matches
a run that prints it and fails a run that prints another or none, while the
built-in fields of an unlabelled row keep their bytes (019 FR-011, SC-02).
The disclosure line and the noise warning of a threaded result ride the same
fold (020 FR-025, FR-026, SC-11): the context count, the context index, a cpu
index and the two counts inside the warning are values the run owns, so they
fold equal, while the mode words, the `contexts=` field and the fixed warning
wording are letters, so a change folds different. A block that leaves either
line out cannot describe a threaded run, and a single-context block keeps its
exact bytes.
The pre-check block and the probe disclosure print one unit per cpu the host
gives the run, and a host that leaves no candidate cpu discloses that once
beside the host settings instead of printing a candidate unit (034 console
contract, the empty-candidate-set answer). A recorded block is one host's
run, so the comparison reads the host's cpu multiplicity out of both sides
before it compares: a repeated governor, irq affinity or probe line folds to
one, a candidate unit stands beside the line that discloses none, a per-cpu
value list and a cpu index list fold to one entry, and the skipped-group
disclosure stands on either side. What is left is the shape the program
owns: every field name, every line class and their order. The fold is what
lets a run under an affinity mask describe a block recorded on a wider host
(033 FR-042). Exit 0 clean, 1 with any finding, 2 on a usage error.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from example_drift import Finding, scan_blocks  # noqa: E402


BUILD_FIELD = re.compile(r"\bbuild=\S+")
FIT_FIELD = re.compile(r"\bBIG_O\b")
# The curve tokens source/harness/complexity.cpp prints: kCurves in order, plus
# the token of a user-supplied curve. Longest first: an alternation takes the
# first branch that matches, so N^2 must stand before N.
FIT_CLASSES = ("NlgN", "N^2", "N^3", "lgN", "f(N)", "N", "1")
FIT_CLASS = re.compile(
    r"(?<![A-Za-z0-9])(?:" + "|".join(re.escape(c) for c in FIT_CLASSES) + r")(?![A-Za-z0-9])"
)


# The float edge words a window that kept one gap prints in the place a
# figure stands: glibc printf spells infinity `inf` and the undefined mean
# `nan` (specs/034 FR-016). They fold to the decimal shape a figure carries,
# so the block and the run fold equal whatever the host's noise was.
FLOAT_EDGE = re.compile(r"(?<![A-Za-z0-9-])(?:-?inf|nan)(?![A-Za-z0-9])")
SETTING_VALUE = re.compile(
    r"(isolcpus|nohz_full|rcu_nocbs)=.*?(?= nohz_full=| rcu_nocbs=|$)"
)
GOVERNOR_VALUE = re.compile(
    r"^(governor: cpu #) (?:not available|\S+)"
    r" (\(source: /sys devices system cpu scaling_governor\))$"
)
IRQ_AFFINITY_VALUE = re.compile(
    r"^(irq affinity: cpu #) .* (\(source: /proc/irq\))$"
)
ATTRIBUTION_VALUE = re.compile(
    r"^((?:interrupts|softirq \S+) delta:) (?:not available|#)"
    r" (\(source: /proc/(?:interrupts|softirqs)\))$"
)
# specs/036 FR-023, R-14: the tracer figure lines stand the not-available
# wording in the figure position on a host without the rights, so the
# position folds beside a figure while the source word and the prefix keep
# their bytes. The disclosure line and the header line keep their bytes.
TRACER_FIGURE_VALUE = re.compile(
    r"^(tracer cpu #) (nmi|irq|softirq|thread|hardware): (?:not available|#)$"
)
SCHEDSTAT_VALUE = re.compile(
    r"^(switch delta:) (?:not available|#), (schedule delta:)"
    r" (?:not available|#) (\(source: /proc/schedstat\))$"
)

# The lines the host owns the count of, in the shape the mask leaves them in.
# The pre-check block prints one unit per measured cpu, the host settings one
# governor line and one affinity line per measured cpu, and the probe block
# one disclosure line per probe cpu (034 console contract, block order).
CPU_UNIT = re.compile(r"^precheck cpu # \((planned|candidate)\)$")
CANDIDATE_UNIT = re.compile(r"^precheck cpu # \(candidate\)$")
UNIT_END = re.compile(r"^(host settings:|warning:|pinning candidates:)")
PER_CPU_LINE = (
    re.compile(r"^governor: cpu # "),
    re.compile(r"^irq affinity: cpu # "),
    re.compile(r"^jitter: cpu # kept # dropped #$"),
)
# A host with no candidate cpu prints this line beside the host settings
# instead of a candidate unit, and a host with an L3 group that leaves no cpu
# free prints the skipped-group disclosure (034 console contract, the empty
# candidate set answer; 027 FR-035). Whether each of the two stands is the
# host's answer, not the shape the program owns.
NO_CANDIDATES = "precheck candidates: none"
HOST_CONDITION = (re.compile(r"^jitter: skipped L# group\(s\) #"),)
# The probe fold block: one value per probe cpu on every field line, and the
# cpu indexes the block and its disclosure name.
PROBE_HEAD = re.compile(r"^jitter probe: cpus ")
PROBE_ON = re.compile(r"^jitter: probe on cpus ")
PROBE_TAIL = re.compile(r"^jitter: ")
VALUE_TOKEN = re.compile(r"(?:#\.)*#")


def mask(line: str) -> str:
    """Fold the values a run owns: digits, whitespace runs, the build field and,
    on a line carrying the fit column, the curve the fit chose. The pre-check
    block's host-owned words fold beside them (specs/034 FR-017, FR-018): the
    float edge words an unlucky window prints, the governor a host runs, the
    kernel setting values and the irq affinity list the host states, the
    attribution delta figures and their `not available` wording, each
    beside the source token that keeps its bytes, and the tracer figure
    position folds the `not available` wording beside a figure the
    engine states (specs/036 FR-023, R-14)."""
    folded = BUILD_FIELD.sub("build=#", line)
    if FIT_FIELD.search(folded):
        folded = FIT_CLASS.sub("#", folded)
    folded = re.sub(r"\s+", " ", re.sub(r"\d+", "#", folded))
    folded = FLOAT_EDGE.sub("#.#", folded)
    folded = SETTING_VALUE.sub(r"\1=#", folded)
    folded = GOVERNOR_VALUE.sub(r"\1 # \2", folded)
    folded = IRQ_AFFINITY_VALUE.sub(r"\1 # \2", folded)
    folded = ATTRIBUTION_VALUE.sub(r"\1 # \2", folded)
    folded = SCHEDSTAT_VALUE.sub(r"\1 #, \2 # \3", folded)
    folded = TRACER_FIGURE_VALUE.sub(r"\1 \2: #", folded)
    # Two field paddings are dropped rather than collapsed: a value wide enough
    # to consume the padding after an equals sign, or after the open paren of the
    # coefficient field of a fit row, prints without it.
    return re.sub(r"\( +", "(", re.sub(r"= ", "=", folded)).strip()


def classify(shape: str) -> str:
    if shape.startswith("contexts="):
        # The one line a threaded result prints after its rows, and the line
        # the noise warning follows (020 FR-025, FR-026).
        return "disclosure line"
    if shape.startswith("warning:"):
        return "noise warning line"
    if "AGGREGATE" in shape:
        return "aggregate line"
    if "iterations=" in shape:
        return "row line"
    if "mean=" in shape:
        # The aggregate family prints one statistic line per attached metric
        # and one per opened counter, and neither carries the AGGREGATE word
        # (018 contract, 019 FR-006).
        return "statistic line"
    return "header line"


class Folded(NamedTuple):
    """One line of one side of the comparison: the shape the mask leaves, the
    text the page or the run printed, and the line that text stands on."""

    shape: str
    text: str
    line: int


def fold_cpu_lists(shape: str) -> str:
    """Read a per-cpu list as one entry. The probe fold block carries one
    value per probe cpu on every field line, and the block and its disclosure
    name one cpu index per probe cpu, so the count the host's cpus set folds
    away while the field name and the value shape stay."""
    shape = re.sub(r"#(,#)+", "#", shape)
    kept: list[str] = []
    for token in shape.split(" "):
        if kept and token == kept[-1] and VALUE_TOKEN.fullmatch(token):
            continue
        kept.append(token)
    return " ".join(kept)


def fold_host(side: list[Folded], no_candidates: bool) -> list[Folded]:
    """Read the host's cpu multiplicity out of one masked side.

    The pre-check block prints one unit per measured cpu, the host settings
    print one governor line and one affinity line per measured cpu, and the
    probe block prints one value, one cpu index and one disclosure line per
    probe cpu. A recorded block is one host's run, so a count the host's cpus
    set cannot stand against it: a repeated per-cpu line reads as one, a
    per-cpu value or cpu list reads as one entry, a candidate unit stands
    where the other side discloses that its host left one out, and the
    skipped-group disclosure stands on either side. Every other line keeps its
    place, so a field name, a line class or an order that changes still reads
    different.
    """
    folded: list[Folded] = []
    in_fold = False
    index = 0
    while index < len(side):
        entry = side[index]
        if CPU_UNIT.match(entry.shape):
            end = index + 1
            while end < len(side) and not CPU_UNIT.match(side[end].shape) \
                    and not UNIT_END.match(side[end].shape):
                end += 1
            if not (no_candidates and CANDIDATE_UNIT.match(entry.shape)):
                folded.extend(side[index:end])
            index = end
            continue
        if PROBE_HEAD.match(entry.shape):
            in_fold = True
        elif PROBE_TAIL.match(entry.shape):
            in_fold = False
        if in_fold or PROBE_HEAD.match(entry.shape) or PROBE_ON.match(entry.shape):
            entry = entry._replace(shape=fold_cpu_lists(entry.shape))
        if entry.shape == NO_CANDIDATES and no_candidates:
            index += 1
            continue
        if any(rule.match(entry.shape) for rule in HOST_CONDITION):
            index += 1
            continue
        if folded and folded[-1].shape == entry.shape and any(
                rule.match(entry.shape) for rule in PER_CPU_LINE
        ):
            index += 1
            continue
        folded.append(entry)
        index += 1
    return folded


def compare(block: list[str], stdout: list[str]) -> list[str]:
    sides = [
        [Folded(mask(line), line, number) for number, line in enumerate(side, 1)]
        for side in (block, stdout)
    ]
    # A candidate unit and the line that discloses no candidate answer one
    # host question, so they stand as one fact across the two sides: the block
    # of a host that had a candidate describes a run whose host left one out,
    # once that run names that it did.
    no_candidates = any(
        entry.shape == NO_CANDIDATES for side in sides for entry in side
    )
    folded = [fold_host(side, no_candidates) for side in sides]
    if [entry.shape for entry in folded[0]] == [entry.shape for entry in folded[1]]:
        return []
    findings = [f"sample output block holds {len(block)} lines, the run printed "
                f"{len(stdout)}"]
    for index in range(max(len(folded[0]), len(folded[1]))):
        left = folded[0][index] if index < len(folded[0]) else None
        right = folded[1][index] if index < len(folded[1]) else None
        if left is not None and right is not None and left.shape == right.shape:
            continue
        if left is None:
            findings.append(
                f"line {right.line}: the block omits this "
                f"{classify(right.shape)}: {right.text!r}"
            )
        elif right is None:
            findings.append(
                f"line {left.line}: the block carries an extra "
                f"{classify(left.shape)}: {left.text!r}"
            )
        else:
            findings.append(
                f"line {left.line}: {classify(left.shape)} differs: block "
                f"{left.text!r} against run {right.text!r}"
            )
    return findings


def sample_block(readme: Path, example: str | None) -> list[str]:
    """The one recorded run of the named example that the page quotes.

    A page carries one recorded run per binary the shape fold runs, so the
    selector names which of them this invocation owns; without it the page
    must hold exactly one recorded run, the shape of the main page.
    """
    blocks = [block for block in scan_blocks(readme, readme.parent) if block.sample]
    if example is not None:
        owner = f"{example}.expected"
        blocks = [
            block
            for block in blocks
            if block.region.split("#", 1)[0].rsplit("/", 1)[-1] == owner
        ]
    if len(blocks) != 1:
        named = f" of example '{example}'" if example is not None else ""
        raise Finding(
            f"{readme}: {len(blocks)} sample output blocks{named}, exactly one required"
        )
    return blocks[0].body


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--readme", type=Path, required=True)
    parser.add_argument("--example", help="the docs example whose recorded run owns the block")
    try:
        args = parser.parse_args(argv)
    except SystemExit as error:
        return 2 if error.code else 0
    if not args.binary.is_file():
        print(f"usage error: {args.binary} is not a file", file=sys.stderr)
        return 2
    if not args.readme.is_file():
        print(f"usage error: {args.readme} is not a file", file=sys.stderr)
        return 2

    try:
        block = sample_block(args.readme, args.example)
        run = subprocess.run(
            [str(args.binary)], capture_output=True, text=True, timeout=110, check=False
        )
    except Finding as error:
        print(str(error))
        return 1
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"{args.binary}: the run failed: {error}")
        return 1

    findings = [] if run.returncode == 0 else [
        f"{args.binary}: exited {run.returncode}\n{run.stderr.strip()}"
    ]
    findings.extend(compare(block, run.stdout.splitlines()))
    for line in findings:
        print(line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
