#!/usr/bin/env bash
# test/tier_labels.sh
# Tier and budget audit of the test registration (T010; 030 FR-026, FR-031,
# FR-047, FR-048).
#
# Usage: tier_labels.sh <build-dir> <ctest-program>
#
# Every entry is meant to carry one tier label and one time budget. The tier
# is what the pre-push path filters on, and the budget is what brings a hung
# entry down at 300 s, a fifth of the 1500 s CTest default. Both are set
# by blocks at the end of the registration files, where no compiler reads
# them, so this entry is the check that the blocks did their work.
#
# Six faults are named, one line each, with the entry that carries it:
#   both-labels     a name in the quick list and in the slow list
#   unlabeled       an entry of the -N list in neither label list
#   stale-slow-row  a row of SPEEDGUN_SLOW_TESTS that names no entry
#   no-budget       an entry with no TIMEOUT property and no exception row
#   wrong-budget    an entry whose exception row lists a value the entry
#                   does not carry as its TIMEOUT
#   determinism     a registration that sleeps, reads a clock for its
#                   verdict, retries, or sets a skip rule master does not set
#
# The reading is taken from the ctest program the registration passes, run
# against the build directory it was configured from, so the entry runs from
# any working directory. The full listing is read as json, which is the one
# form that carries each entry's properties. The two label lists come from
# the two filtered listings, which are the commands the pre-push path runs.
#
# Exit 0 clean, 1 with one failure line per fault.

set -uo pipefail

BUILD_DIR=${1:-}
CTEST=${2:-}

if [ -z "$BUILD_DIR" ] || [ -z "$CTEST" ]; then
  echo "FAIL: usage: tier_labels.sh <build-dir> <ctest-program>" >&2
  exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
  echo "FAIL: build dir $BUILD_DIR is not a directory" >&2
  exit 1
fi

HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REG_TEST=$HERE/CMakeLists.txt
REG_EXAMPLE=$HERE/../example/CMakeLists.txt

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for listing in all quick slow; do
  case $listing in
    all) set -- -N --show-only=json-v1 ;;
    quick) set -- -N -L quick ;;
    slow) set -- -N -L slow ;;
  esac
  if ! "$CTEST" --test-dir "$BUILD_DIR" "$@" >"$TMP/$listing"; then
    echo "FAIL: the listing $listing failed against $BUILD_DIR" >&2
    exit 1
  fi
done

python3 - "$TMP" "$REG_TEST" "$REG_EXAMPLE" <<'PY'
import json
import re
import sys

tmp, reg_test, reg_example = sys.argv[1:4]

# The two registrations hold the rules the suite keeps by hand: the slow
# tier list, the budget exceptions, and the skip rules. The tier list is
# T011's and the exceptions block is T044's, so a missing one reads as an
# empty list here, which is what the audit says about a tree without it.
MASTER_SKIP = {
    "counters_pmu_test": "2",
    "counters_metric_formula_test": "2",
    "counters_metric_lookup_test": "2",
    "counters_metric_trees_test": "2",
    "counters_metric_threadless_test": "2",
    "counters_overhead": "2",
    "counters_counter_limits_test": "2",
    "counters_objects_limit_test": "2",
}

# FR-048 closes the list of constructs a registration may not carry, and
# MASTER_SKIP above is the baseline for the skip rule, read from the two
# registration files at the commit this branch started from.
FORBIDDEN = (
    ("sleeps", re.compile(r"\bsleep", re.I)),
    ("reads a clock for its verdict",
     re.compile(r"\bdate\b|string\s*\(\s*timestamp|clock_gettime", re.I)),
    ("retries", re.compile(r"\bwhile\b|\bretry|\brepeat\b|until.pass", re.I)),
)

LABEL_LINE = re.compile(r"^\s*Test\s+#\d+:\s+(.*?)\s*$")
SLOW_LIST = re.compile(r"set\s*\(\s*SPEEDGUN_SLOW_TESTS\b(.*?)\)", re.I | re.S)
PROPS_BLOCK = re.compile(r"set_tests_properties\s*\(([^)]*)\)", re.I | re.S)
SKIP_RULE = re.compile(r"SKIP_RETURN_CODE\s+(\S+)", re.I)
# The exceptions block of FR-032 names one entry per row, and the row opens
# with the words budget exception. That marker form is set by this script.
# The row also states the seconds the entry keeps, and the budget check
# compares that number with the TIMEOUT property of the listing.
EXCEPTION_ROW = re.compile(
    r"budget[ -]exception[s]?[:\s]+([A-Za-z0-9_.:-]+)(?:\D*(\d+)\s*s)?",
    re.I)
MATRIX_PREFIX = "dbc_matrix_"

FAULTS = []


def fault(kind, detail):
    FAULTS.append("FAIL: %s: %s" % (kind, detail))


def read_lines(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read().splitlines()


def is_comment(line):
    return line.lstrip().startswith("#")


def code_text(path):
    """The file without comment lines, where prose about a construct sits."""
    return "\n".join(ln for ln in read_lines(path) if not is_comment(ln))


suite = json.load(open(tmp + "/all", encoding="utf-8"))["tests"]
names = [entry["name"] for entry in suite]
registered = set(names)


def prop(entry, wanted):
    for one in entry.get("properties", []):
        if one.get("name") == wanted:
            return one.get("value")
    return None


def label_list(path):
    return [m.group(1) for m in
            (LABEL_LINE.match(ln) for ln in read_lines(path)) if m]


quick = label_list(tmp + "/quick")
slow = label_list(tmp + "/slow")
quick_set, slow_set = set(quick), set(slow)

print("read: %d entries, %d quick, %d slow"
      % (len(names), len(quick), len(slow)))

for name in sorted(quick_set & slow_set):
    fault("both-labels", "%s carries the quick label and the slow label" % name)

for name in names:
    if name not in quick_set and name not in slow_set:
        fault("unlabeled", "%s carries no tier label" % name)

# The matrix entries are registered only on the lanes whose preset turns
# SPEEDGUN_DBC_MATRIX on, so a slow row for one of them is stale only where
# the lane registers at least one.
matrix_registered = any(name.startswith(MATRIX_PREFIX) for name in names)
slow_rows = []
listed = SLOW_LIST.search(code_text(reg_test))
if listed is not None:
    slow_rows = [token for token in listed.group(1).split()
                 if token not in ("PARENT_SCOPE", "CACHE")]
for row in slow_rows:
    if row in registered:
        continue
    if row.startswith(MATRIX_PREFIX) and not matrix_registered:
        continue
    fault("stale-slow-row",
          "SPEEDGUN_SLOW_TESTS names %s, which this suite does not register"
          % row)

# name -> the seconds its row lists, absent when the row lists none
exceptions = {}
for path in (reg_test, reg_example):
    for line in read_lines(path):
        if is_comment(line):
            found = EXCEPTION_ROW.search(line)
            if found:
                exceptions[found.group(1)] = found.group(2)

for entry in suite:
    name = entry["name"]
    timeout = prop(entry, "TIMEOUT")
    if timeout is None and name not in exceptions:
        fault("no-budget", "%s holds no TIMEOUT and no exception row" % name)
    listed = exceptions.get(name)
    if listed is not None and (timeout is None
                               or float(timeout) != float(listed)):
        carries = ("no TIMEOUT" if timeout is None
                   else "TIMEOUT %g" % float(timeout))
        fault("wrong-budget",
              "%s carries %s, its exception row lists %s s"
              % (name, carries, listed))

for path in (reg_test, reg_example):
    for number, line in enumerate(read_lines(path), 1):
        if is_comment(line):
            continue
        for label, pattern in FORBIDDEN:
            hit = pattern.search(line)
            if hit:
                fault("determinism", "%s:%d %s: %s"
                      % (path, number, label, line.strip()))
    for block in PROPS_BLOCK.findall(code_text(path)):
        words = block.split()
        entry = words[0] if words else ""
        rule = SKIP_RULE.search(block)
        if rule and MASTER_SKIP.get(entry) != rule.group(1):
            baseline = MASTER_SKIP.get(entry, "no skip rule")
            fault("skip-rule", "%s:%s sets SKIP_RETURN_CODE %s, master sets %s"
                  % (path, entry, rule.group(1), baseline))

for line in FAULTS:
    print(line)

if FAULTS:
    count = len(FAULTS)
    print("test_tier_labels: %d fault%s" % (count, "" if count == 1 else "s"))
    raise SystemExit(1)

print("ok: every entry carries one label and a budget, and neither "
      "registration breaks FR-048")
PY
