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
exact bytes. Exit 0 clean, 1 with any finding, 2 on a usage error.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

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


def mask(line: str) -> str:
    """Fold the values a run owns: digits, whitespace runs, the build field and,
    on a line carrying the fit column, the curve the fit chose. The pre-check
    block's host-owned words fold beside them (specs/034 FR-017, FR-018): the
    float edge words an unlucky window prints, the governor a host runs, the
    kernel setting values and the irq affinity list the host states, each
    beside the source token that keeps its bytes."""
    folded = BUILD_FIELD.sub("build=#", line)
    if FIT_FIELD.search(folded):
        folded = FIT_CLASS.sub("#", folded)
    folded = re.sub(r"\s+", " ", re.sub(r"\d+", "#", folded))
    folded = FLOAT_EDGE.sub("#.#", folded)
    folded = SETTING_VALUE.sub(r"\1=#", folded)
    folded = GOVERNOR_VALUE.sub(r"\1 # \2", folded)
    folded = IRQ_AFFINITY_VALUE.sub(r"\1 # \2", folded)
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


def compare(block: list[str], stdout: list[str]) -> list[str]:
    folded = [mask(line) for line in block], [mask(line) for line in stdout]
    if folded[0] == folded[1]:
        return []
    findings = [f"sample output block holds {len(block)} lines, the run printed "
                f"{len(stdout)}"]
    for index in range(max(len(block), len(stdout))):
        left = folded[0][index] if index < len(folded[0]) else None
        right = folded[1][index] if index < len(folded[1]) else None
        if left == right:
            continue
        if left is None:
            findings.append(
                f"line {index + 1}: the block omits this {classify(right)}: "
                f"{stdout[index]!r}"
            )
        elif right is None:
            findings.append(
                f"line {index + 1}: the block carries an extra {classify(left)}: "
                f"{block[index]!r}"
            )
        else:
            findings.append(
                f"line {index + 1}: {classify(left)} differs: block "
                f"{block[index]!r} against run {stdout[index]!r}"
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
