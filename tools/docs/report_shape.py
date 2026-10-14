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
Exit 0 clean, 1 with any finding, 2 on a usage error.
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


def mask(line: str) -> str:
    """Fold the values a run owns: digits, whitespace runs, the build field and,
    on a line carrying the fit column, the curve the fit chose."""
    folded = BUILD_FIELD.sub("build=#", line)
    if FIT_FIELD.search(folded):
        folded = FIT_CLASS.sub("#", folded)
    folded = re.sub(r"\s+", " ", re.sub(r"\d+", "#", folded))
    # Two field paddings are dropped rather than collapsed: a value wide enough
    # to consume the padding after an equals sign, or after the open paren of the
    # coefficient field of a fit row, prints without it.
    return re.sub(r"\( +", "(", re.sub(r"= ", "=", folded)).strip()


def classify(shape: str) -> str:
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


def sample_block(readme: Path) -> list[str]:
    blocks = [block for block in scan_blocks(readme, readme.parent) if block.sample]
    if len(blocks) != 1:
        raise Finding(
            f"{readme}: {len(blocks)} sample output blocks, exactly one required"
        )
    return blocks[0].body


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--readme", type=Path, required=True)
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
        block = sample_block(args.readme)
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
