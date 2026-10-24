#!/usr/bin/env python3
"""Replay the base command-line matrix against a built harness binary.

One row of test/data/base_cli_matrix.txt names a case, the argv of one
command line, the exit status the base answers and the shortest
digit-masked standard error line that names the rejection, or a single
dash when the base prints no fault line. Each row is replayed against the
binary the caller names, and a row whose status or fault fragment moved
prints one line naming the case. The last line counts the rows that
match. (FR-003, SC-013)

One row departs from the base commit: case17 follows the counter ruling of
specs/028-full-command-line, where an address this host does not find exits 2
(FR-087).

One row of test/data/base_cli_console.txt records, for the case its name
names, the digit-masked console text that binary printed at the base
commit and the sorted key set of the JSON document it wrote, each a
single dash when the answer is empty. Console lines are joined with a
literal backslash-n, and the record holds the standard-output lines
before the standard-error ones with the version banner left out. A case
the record holds is compared line by line, and the document it wrote is
read back: its sorted key set, with `schemaVersion` left out, has to be
the recorded set, which is also what refuses a `run` object the base
record does not name, and the unmeasured values the key set cannot hold
have to be the ones the record and the run's own banner name. A case the
record does not hold is compared by status and fault line alone. Every
row that answers a fault line has to answer the recorded line and no
other, and a usage fault answers exactly one line. (FR-005, FR-007 to
FR-009, FR-059, FR-086, SC-007, AC-7)
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

# One replayed row: the exit status, the standard-error lines, the masked
# console text, the sorted key set of the document the run wrote, that
# document, and the version and build type the run's banner names.
Answer = tuple[int, list[str], list[str], list[str], object, tuple[str, str]]


def mask(text: str) -> str:
    """Replace every run of digits with a single #, the matrix mask."""
    return re.sub(r"\d+", "#", text)


def collapse(text: str) -> str:
    """Squeeze every run of blanks to one space, then drop the blanks that
    follow an `=`.

    The report right-aligns each number to the widest value of its run, so
    the padding of a column follows the magnitude the host happened to
    measure. Without this the console record would name a difference every
    time a value changed its number of digits: the same command answers
    `iterations=614064197` in a release build and `iterations= 614064197`
    under the thread sanitizer, where a narrower value stands in the same
    column. Both print the same words and the same numbers.
    """
    return re.sub(r"=\s+", "=", " ".join(text.split()))


def is_banner(line: str) -> bool:
    """Report the version line a run opens with, which may change freely."""
    return line.startswith("speedgun-ng ") and " build=" in line


def strip_program(line: str, prefixes: list[str], name: str) -> str:
    """Drop the program prefix getopt writes before its own diagnostic.

    getopt_long prints the path the process was started with, so that
    prefix spells whichever path the caller used. The two spellings this
    run used come first; the name rule also covers a matrix row that
    carries a prefix from another invocation. The prefix goes before the
    digit mask, so a path with digits in it is still recognized.
    """
    for prefix in prefixes:
        if line.startswith(prefix):
            return line[len(prefix):]
    head, found, rest = line.partition(name + ": ")
    if found and " " not in head and (head == "" or "/" in head):
        return rest
    return line


def read_rows(path: Path) -> tuple[list[tuple[str, list[str], int, str]], int]:
    rows: list[tuple[str, list[str], int, str]] = []
    broken = 0
    for text in path.read_text().splitlines():
        if not text:
            continue
        fields = text.split(" | ", 3)
        if len(fields) != 4 or not fields[2].isdigit():
            broken += 1
            print(f"{fields[0]}: the row carries no four fields with a "
                  "numeric status")
            continue
        case, argv_text, status_text, fragment = fields
        rows.append((case, argv_text.split(), int(status_text), fragment))
    return rows, broken


def fault_lines(stderr: str, prefixes: list[str],
                name: str) -> list[str]:
    """Every standard-error line the run printed, prefix left out.

    The shortest line rule is gone: it compared one of several lines with
    the record, so a row that answers a second fault line passed. Every
    line is held to the record now, and the caller counts them. (FR-007 to
    FR-009, SC-007)
    """
    return [mask(strip_program(line, prefixes, name))
            for line in stderr.splitlines() if line.strip()]


def console_of(stdout: str, stderr: str, prefixes: list[str],
               name: str) -> list[str]:
    """The masked console text: standard output, then standard error."""
    out = [collapse(mask(line)) for line in stdout.splitlines()
           if line.strip() and not is_banner(line)]
    out += [collapse(mask(strip_program(line, prefixes, name)))
            for line in stderr.splitlines() if line.strip()]
    return out


def banner_of(stdout: str) -> tuple[str, str]:
    """Report the version and the build type this run's banner names.

    The banner stands in no console record, because it moves with the
    release and with the preset. It is read from the run instead, so the
    two `speedgun` members of the document that run wrote have something
    to agree with. (SC-007, AC-7)
    """
    for line in stdout.splitlines():
        if not is_banner(line):
            continue
        words = line.split()
        build = words[2].partition("=")[2] if len(words) > 2 else ""
        return words[1], build
    return "", ""


def read_console(path: Path) -> tuple[dict[str, tuple[list[str], list[str]]],
                                      int]:
    records: dict[str, tuple[list[str], list[str]]] = {}
    broken = 0
    for text in path.read_text().splitlines():
        if not text or text.startswith("#"):
            continue
        fields = text.split(" | ", 2)
        if len(fields) != 3:
            broken += 1
            print(f"{fields[0]}: the console record carries no three fields")
            continue
        case, console, keys = fields
        records[case] = (
            [] if console == "-"
            else [collapse(line) for line in console.split("\\n")],
            [] if keys == "-" else keys.split(";"))
    return records, broken


def key_set(node: object, path: str):
    """Every key path of a document, array items reached through []."""
    if isinstance(node, dict):
        for key, value in node.items():
            child = f"{path}.{key}" if path else key
            yield child
            yield from key_set(value, child)
    elif isinstance(node, list):
        for item in node:
            yield from key_set(item, f"{path}[]" if path else "[]")


def written_document(target: Path) -> tuple[list[str], object]:
    """The sorted key set of the written document and the document.

    `schemaVersion` is left out of the set, because SC-007 states that the
    document of a run with no new option differs from the base document in
    that one value alone: every other path the record names has to stand.
    """
    with target.open() as stream:
        document = json.load(stream)
    keys = set(key_set(document, ""))
    keys.discard("schemaVersion")
    return sorted(keys), document


def reroute(argv: list[str]) -> tuple[list[str], Path | None]:
    """Send a written document to a private path.

    Two ctest runs of this matrix may go at the same time, and a shared
    path lets one read the other's half-written document.
    """
    if not any(word.startswith("--json=") for word in argv):
        return argv, None
    target = Path(tempfile.gettempdir()) / f"sg028_console_{os.getpid()}.json"
    out = [f"--json={target}" if word.startswith("--json=") else word
           for word in argv]
    return out, target


def run_row(binary: Path, argv: list[str],
            prefixes: list[str]) -> Answer:
    argv, target = reroute(argv)
    done = subprocess.run([str(binary), *argv], cwd=REPO,
                          capture_output=True, text=True)
    keys: list[str] = []
    document: object = None
    if target is not None:
        try:
            keys, document = written_document(target)
        except (OSError, ValueError):
            keys = [f"no readable document at {target}"]
        target.unlink(missing_ok=True)
    return (done.returncode,
            fault_lines(done.stderr, prefixes, binary.name),
            console_of(done.stdout, done.stderr, prefixes, binary.name),
            keys, document, banner_of(done.stdout))


# The document word of one printed column unit: the console prints the
# label of `kUnitScales` (source/harness/report.cpp) and the document the
# word of `kTimeUnitWords` (source/harness/detail/json_report.hpp), both
# indexed by the same enumerator.
UNIT_WORDS = {"ns": "NANOSECONDS", "us": "MICROSECONDS",
              "ms": "MILLISECONDS", "s": "SECONDS"}


def value_problems(record: list[str], banner: tuple[str, str],
                   doc: object) -> list[str]:
    """One phrase per unmeasured document value that moved.

    The field set, and the reason each member is here: every path of the
    document is held by the key set rule, but a key set carries no value,
    so the values a run does not measure are compared on their own.
    `benchmarks[].name` has to be the row name the recorded console text
    prints, and has to split into the recorded `benchmarks[].suite` and
    `benchmarks[].case`; `benchmarks[].timeUnit` has to be the unit that
    text prints the time column in; and `speedgun.version` and
    `speedgun.buildType` have to be what this very run's banner names.
    The key set rule alone is what refuses a `run` object the record does
    not name. Nothing a run measures enters here: iterations, times,
    percentiles, statistics and the sample and pass counts answer again
    every run, which is why the console mask hides their digits. (SC-007,
    AC-7)
    """
    if not isinstance(doc, dict):
        return []          # the key set rule already named the unreadable one
    problems: list[str] = []
    rows = [row for row in doc.get("benchmarks", [])
            if isinstance(row, dict)]
    printed: list[str] = []
    for line in record:
        head = line.split(" ")[0] if line else ""
        if head and head not in printed:
            printed.append(head)
    named = [mask(row.get("name") or "") for row in rows]
    if named != printed:
        problems.append("document names '" + "|".join(named)
                        + "', the record prints '" + "|".join(printed) + "'")
    for row in rows:
        name, suite, case = row.get("name"), row.get("suite"), row.get("case")
        if f"{suite}/{case}" != name:
            problems.append(f"benchmark '{name}' is named '{suite}' "
                            f"of case '{case}'")
    unit = ""
    for line in record:
        found = re.search(r"time/iter \((\w+)\)", line)
        if found is not None:
            unit = UNIT_WORDS.get(found.group(1), "")
            break
    for row in rows:
        if unit and row.get("timeUnit") != unit:
            problems.append(f"benchmark '{row.get('name')}' carries the "
                            f"unit '{row.get('timeUnit')}', the record "
                            f"prints '{unit}'")
    version, build = banner
    config = doc.get("speedgun")
    if not isinstance(config, dict):
        config = {}
    if not version or not build:
        problems.append("the run printed no version banner to read the "
                        "two config members against")
    elif config.get("version") != version:
        problems.append(f"document version '{config.get('version')}', the "
                        f"banner names '{version}'")
    elif config.get("buildType") != build:
        problems.append(f"document build type '{config.get('buildType')}', "
                        f"the banner names '{build}'")
    return problems


def console_problems(want: list[str], got: list[str]) -> list[str]:
    """One phrase per console line that moved, and one per extra line."""
    problems = []
    for index, line in enumerate(want):
        if index >= len(got):
            problems.append(f"console line {index + 1} expected '{line}', "
                            "observed no line")
        elif got[index] != line:
            problems.append(f"console line {index + 1} expected '{line}', "
                            f"observed '{got[index]}'")
    if len(got) > len(want):
        problems.append(f"console holds {len(got)} lines, "
                        f"expected {len(want)}")
    return problems


def key_problems(want: list[str], got: list[str]) -> list[str]:
    """One phrase naming the keys that moved, if any did."""
    if want == got:
        return []
    missing = "|".join(set(want) - set(got)) or "-"
    added = "|".join(set(got) - set(want)) or "-"
    return [f"document keys missing {missing}, added {added}"]


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: run_matrix.py <harness-binary> [matrix-file] "
              "[console-file]")
        return 1
    given = sys.argv[1]
    binary = Path(given).resolve()
    prefixes = [given + ": ", str(binary) + ": "]
    matrix = (Path(sys.argv[2]).resolve() if len(sys.argv) > 2
              else REPO / "test" / "data" / "base_cli_matrix.txt")
    console = (Path(sys.argv[3]).resolve() if len(sys.argv) > 3
               else REPO / "test" / "data" / "base_cli_console.txt")
    for path in (binary, matrix, console):
        if not path.is_file():
            print(f"missing input: {path}")
            return 1

    rows, broken = read_rows(matrix)
    records, broken_records = read_console(console)
    total = len(rows) + broken + broken_records
    passed = 0
    for case, argv, want_status, want_fragment in rows:
        status, faults, text, keys, doc, banner = run_row(binary, argv,
                                                          prefixes)
        want = ("" if want_fragment == "-"
                else mask(strip_program(want_fragment, prefixes, binary.name)))
        problems = []
        if status != want_status:
            problems.append(f"status expected {want_status}, "
                            f"observed {status}")
        # Every line of a fault row is held to the recorded line, so a
        # second line of its own is a mismatch rather than the line the
        # shortest rule happened to leave out. A row the base refuses,
        # the status 2 a rejected command line answers, has to answer that
        # one line and nothing beside it. A row that runs on to a normal
        # end may print one warning per column it refused, so there it is
        # the distinct lines that have to be the recorded line alone.
        # (FR-007 to FR-009, FR-046, SC-007)
        if want_fragment == "-":
            if faults:
                problems.append("fault expected no line, observed "
                                + str(len(faults)) + ": '"
                                + "' '".join(faults) + "'")
        else:
            distinct = sorted(set(faults))
            if distinct != [want]:
                problems.append("fault expected '" + want + "', observed "
                                + str(len(faults)) + " line(s): '"
                                + "' '".join(distinct) + "'")
            if want_status == 2 and len(faults) != 1:
                problems.append(f"the fault answers {len(faults)} lines, "
                                "a usage fault prints exactly one line")
        record = records.pop(case, None)
        if record is not None:
            problems += console_problems(record[0], text)
            problems += key_problems(record[1], keys)
            problems += value_problems(record[0], banner, doc)
        if problems:
            print(f"{case}: " + "; ".join(problems))
        else:
            passed += 1
    for case in sorted(records):
        total += 1
        print(f"{case}: the console record names no matrix case")

    tail = (" with all rows matching" if passed == total
            else f" with {total - passed} row(s) mismatching")
    print(f"{passed} of {total}{tail}")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
