#!/usr/bin/env bash
# The acceptance runs of specs/039-context-groups (T033; FR-038).
#
# Five runs, in their two kinds. A host run is a CLI command over the shape
# the lane host states, and a seam run is a named `ctest` invocation whose
# test installs a shape no lane host states. Each run writes a transcript
# beside its artifacts: the command line it ran, the exit code it answered,
# and where the captured bytes stand. The reports and the captured test
# output stand under `results/reports/`, and each record names the kind of
# every run it quotes.
#
# The runs:
#   preset-core  the preset grouping over a multi-group shape
#   preset-node  the `node` grouping
#   layout       one file naming four groups over eight contexts
#   refusals     the 16 fault shapes of FR-018 with the two pairings of
#                FR-019: one line, exit status 2, no report file (SC-004)
#   ungrouped    the same fixture run with no partition (FR-033, SC-006)
#
# Usage: run_groups.sh [`--build` <dir>] [`--master-build` <dir>]
#
# The release tree the CLI and `ctest` read defaults to `build-w7a`, the
# tree this unit built. A fresh clone reaches the same reading with:
#
#   `cmake --preset=ci-rocky -B build-w7a && cmake --build build-w7a`
#
# `--master-build` names a second tree that carries master, the far side of
# the ungrouped comparison. Where none stands named, that run states the
# gap in its transcript and the reading it did perform.
#
# The CLI commands read `docs_threads`, whose `bmSharded` states no
# placement, so `--placement` settles the run's preset and `--threads`
# states its context count. `--iterations=1` keeps each run to one
# iteration and skips calibration.
#
# The layout files stand written into the fixtures directory of the reports
# on every pass, so the bytes that refused a case stay readable beside the
# line they produced. The run overwrites what it wrote, so a second reading
# answers the same shape.
#
# Exit 0 when every reading matched, 1 on a finding, 2 on a usage error.

set -u

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT" || exit 1

R=specs/039-context-groups/results
OUT=$R/reports
FIX=$OUT/fixtures
BUILD=build-w7a
MASTER=
TEST=harness_group_run_test

usage() {
  printf 'usage: run_groups.sh [--build <dir>] [--master-build <dir>]\n'
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --build)
      [ "$#" -ge 2 ] || { usage >&2; exit 2; }
      BUILD=$2
      shift 2
      ;;
    --master-build)
      [ "$#" -ge 2 ] || { usage >&2; exit 2; }
      MASTER=$2
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      exit 2
      ;;
  esac
done

BENCH=$BUILD/example/docs_threads
if [ ! -x "$BENCH" ]; then
  printf 'error: no benchmark binary at %s\n' "$BENCH" >&2
  printf 'error: build the tree the script reads first:\n' >&2
  printf '       cmake --preset=ci-rocky -B %s && cmake --build %s\n' \
    "$BUILD" "$BUILD" >&2
  exit 1
fi

mkdir -p "$OUT/refusals" "$FIX" || exit 1

status=0
TRANSCRIPT=

# say <text>: report one reading to the person running the script.
say() {
  printf '%s\n' "$*"
}

# finding <text>: mark a reading that differs from what the spec states, in
# the transcript of the run in progress and on standard error.
finding() {
  printf 'finding: %s\n' "$*" >> "$TRANSCRIPT"
  printf 'finding: %s\n' "$*" >&2
  status=1
}

# begin <run name> <one line naming the run>: start the run's transcript.
begin() {
  TRANSCRIPT=$OUT/$1.log
  : > "$TRANSCRIPT"
  printf '%b\n' "$2" >> "$TRANSCRIPT"
}

# argv <command...>: the command line as a reader can paste it back.
argv() {
  local line
  line=$(printf '%q ' "$@")
  printf '%s' "${line% }"
}

# expect <exit code> <capture file> <command...>: run the command from the
# repository root, keep its combined output in the capture file, and write
# the command line, the exit code and the capture's path into the
# transcript. A status beside the named one is a finding (FR-038).
expect() {
  local want=$1 capture=$2
  shift 2
  printf '$ %s\n' "$(argv "$@")" >> "$TRANSCRIPT"
  local code=0
  "$@" > "$capture" 2>&1 || code=$?
  printf 'exit %s\n' "$code" >> "$TRANSCRIPT"
  printf 'the bytes stand in %s\n' "$capture" >> "$TRANSCRIPT"
  if [ "$code" -ne "$want" ]; then
    finding "exit $code answers where $want stood named for $capture"
  fi
  return 0
}

# refuse <capture file> <report path> <command...>: the reading the refusal
# shapes share. Exit status 2, the one line captured, and no report file
# written (FR-018, FR-019, SC-004).
refuse() {
  local capture=$1 report=$2
  shift 2
  rm -f "$report"
  printf '$ %s\n' "$(argv "$@")" >> "$TRANSCRIPT"
  local code=0
  "$@" > "$capture" 2>&1 || code=$?
  printf 'exit %s\n' "$code" >> "$TRANSCRIPT"
  printf 'the line stands captured in %s\n' "$capture" >> "$TRANSCRIPT"
  if [ "$code" -ne 2 ]; then
    finding "exit $code answers where 2 stood named for $capture"
  fi
  if [ -e "$report" ]; then
    finding "a report file stands written for $capture"
    rm -f "$report"
  else
    printf 'no report file stands written\n' >> "$TRANSCRIPT"
  fi
  return 0
}

# refusal <case name> <layout file> [option...]: one case of SC-004, eight
# contexts of the named layout file beside any extra option.
refusal() {
  local name=$1 file=$2
  shift 2
  refuse "$OUT/refusals/$name.txt" "$OUT/refusals/$name.json" \
    "$BENCH" --threads=8 --iterations=1 --no-percentiles \
    --json="$OUT/refusals/$name.json" --group-layout="$FIX/$file" "$@"
}

# disclosure <capture file> <out file>: cut the context disclosure line out
# of a console capture, the region FR-033 compares. The reading holds no
# timing figure, so a second pass answers the same bytes.
disclosure() {
  printf '$ grep -m1 %s %s\n' "'^  contexts='" "$1" >> "$TRANSCRIPT"
  if grep -m1 '^  contexts=' "$1" > "$2"; then
    printf 'the disclosure line stands in %s\n' "$2" >> "$TRANSCRIPT"
  else
    finding "the capture holds no context disclosure line: $1"
  fi
}

# fixture <file name>: read the document from standard input into the
# fixtures directory.
fixture() {
  cat > "$FIX/$1"
}

# The document the layout runs accept: four groups over eight contexts, each
# naming two context positions, the shape SC-005's file case states. Slots
# name context positions, so the file reads the same on any host (FR-012,
# FR-013).
fixture four-groups.yml <<'YAML'
groups:
  - name: first
    slots: [0, 1]
  - name: second
    slots: [2, 3]
  - name: third
    slots: [4, 5]
  - name: fourth
    slots: [6, 7]
YAML

# The 16 fault shapes of FR-018, in the order contracts/group-layout.md
# table C lists them. Each document breaks the one rule its row names, above
# the eight contexts the case states.
# The unreadable path stands as a fixture name no pass writes, so the reader
# meets the open fault FR-018's first row names.

fixture unparsable-document.yml <<'YAML'
groups: [
  - name: first
YAML

fixture no-groups-key.yml <<'YAML'
notgroups:
  - name: first
    slots: [0, 1, 2, 3, 4, 5, 6, 7]
YAML

fixture root-sequence.yml <<'YAML'
- name: first
  slots: [0, 1, 2, 3, 4, 5, 6, 7]
YAML

fixture empty-groups.yml <<'YAML'
groups: []
YAML

fixture entry-not-mapping.yml <<'YAML'
groups:
  - 3
YAML

fixture no-slots-key.yml <<'YAML'
groups:
  - name: first
YAML

fixture empty-slots.yml <<'YAML'
groups:
  - name: first
    slots: []
YAML

fixture member-wrong-kind.yml <<'YAML'
groups:
  - name: first
    slots: [zero]
YAML

fixture member-out-of-range.yml <<'YAML'
groups:
  - name: first
    slots: [0, 1, 2, 3, 4, 5, 6, 9]
YAML

fixture repeated-slot.yml <<'YAML'
groups:
  - name: first
    slots: [0, 1, 2, 3, 4, 5, 6, 7]
  - name: second
    slots: [7]
YAML

fixture uncovered-context.yml <<'YAML'
groups:
  - name: first
    slots: [0, 1, 2, 3, 4, 5, 6]
YAML

fixture cpus-key.yml <<'YAML'
groups:
  - name: first
    cpus: [0]
    slots: [0, 1, 2, 3, 4, 5, 6, 7]
YAML

fixture key-outside-grammar.yml <<'YAML'
groups:
  - name: first
    slots: [0, 1, 2, 3, 4, 5, 6, 7]
    nodes: [0]
YAML

fixture repeated-name.yml <<'YAML'
groups:
  - name: first
    slots: [0, 1, 2, 3]
  - name: first
    slots: [4, 5, 6, 7]
YAML

fixture name-outside-grammar.yml <<'YAML'
groups:
  - name: 'bad name'
    slots: [0, 1, 2, 3, 4, 5, 6, 7]
YAML

# Run 1, preset-core: the grouping one preset names, over the shape the lane
# host states and over the shape the seam installs.
begin preset-core 'preset-core: the preset grouping over a multi-group shape,\nin both kinds (FR-038)'

expect 0 "$OUT/preset-core.core16.console.txt" \
  "$BENCH" --placement=core --threads=16 --iterations=1 --no-percentiles \
  --json="$OUT/preset-core.core16.json"
printf 'host run: the host states two L3 groups of eight cores, so 16\n' \
  >> "$TRANSCRIPT"
printf 'contexts under `core` disclose `l3-0` and `l3-1` of eight each\n' \
  >> "$TRANSCRIPT"

expect 0 "$OUT/preset-core.l3two.console.txt" \
  "$BENCH" --placement=l3 --threads=2 --iterations=1 --no-percentiles \
  --json="$OUT/preset-core.l3two.json"
printf 'host run: the `l3` arm seats one context per L3 unit, so two\n' \
  >> "$TRANSCRIPT"
printf 'contexts disclose two single-member groups\n' >> "$TRANSCRIPT"

expect 0 "$OUT/preset-core.seam-ctest.txt" \
  ctest --test-dir "$BUILD" -R "^${TEST}\$" --verbose
printf 'seam run: %s, scenario `core-four-groups`, eight contexts over four\n' \
  "$TEST" >> "$TRANSCRIPT"
printf 'L3 groups of two cores each, a shape the lane host states nowhere\n' \
  >> "$TRANSCRIPT"

say "preset-core: 3 runs, transcript $TRANSCRIPT"

# Run 2, preset-node: the `node` grouping over the host's one memory node,
# beside the seam's two nodes.
begin preset-node 'preset-node: the `node` grouping in both kinds (FR-038)'

expect 0 "$OUT/preset-node.host.console.txt" \
  "$BENCH" --placement=node --threads=4 --iterations=1 --no-percentiles \
  --json="$OUT/preset-node.host.json"
printf 'host run: one memory node covers the host, so four contexts disclose\n' \
  >> "$TRANSCRIPT"
printf 'the one group `node-0`\n' >> "$TRANSCRIPT"

expect 0 "$OUT/preset-node.seam-ctest.txt" \
  ctest --test-dir "$BUILD" -R "^${TEST}\$" --verbose
printf 'seam run: %s, scenario `node-two-nodes`, four contexts over two\n' \
  "$TEST" >> "$TRANSCRIPT"
printf 'memory nodes with alternating memberships, no lane host carries it\n' \
  >> "$TRANSCRIPT"

say "preset-node: 2 runs, transcript $TRANSCRIPT"

# Run 3, layout: the file route end to end, the partition one file states.
begin layout 'layout: the partition one file names, in both kinds (FR-038)'

expect 0 "$OUT/layout.host.console.txt" \
  "$BENCH" --threads=8 --iterations=1 --no-percentiles \
  --group-layout="$FIX/four-groups.yml" --json="$OUT/layout.host.json"
printf 'host run: the file names four groups of two over eight contexts and\n' \
  >> "$TRANSCRIPT"
printf 'carries no preset, so the run groups by the file and prints no\n' \
  >> "$TRANSCRIPT"
printf 'machine line (FR-028)\n' >> "$TRANSCRIPT"
if grep -q '"group"' "$OUT/layout.host.json"; then
  printf 'the document names the group of every context\n' >> "$TRANSCRIPT"
else
  finding "the layout document carries no group key"
fi

expect 0 "$OUT/layout.seam-ctest.txt" \
  ctest --test-dir "$BUILD" -R "^${TEST}\$" --verbose
printf 'seam run: %s, scenario `file-four-groups`, eight contexts over the\n' \
  "$TEST" >> "$TRANSCRIPT"
printf 'four groups one file states, a four-group host shape no lane host\n' \
  >> "$TRANSCRIPT"
printf 'states\n' >> "$TRANSCRIPT"

say "layout: 2 runs, transcript $TRANSCRIPT"

# Run 4, refusals: the 16 fault shapes of FR-018 in the contract's order,
# then the two pairings FR-019 names.
begin refusals 'refusals: every refusal shape answers one line with status 2,\nthe 18 cases of SC-004 (FR-018, FR-019)'

refusal 01-unreadable-path unreadable-path.yml
refusal 02-unparsable-document unparsable-document.yml
refusal 03-no-groups-key no-groups-key.yml
refusal 04-root-sequence root-sequence.yml
refusal 05-empty-groups empty-groups.yml
refusal 06-entry-not-mapping entry-not-mapping.yml
refusal 07-no-slots-key no-slots-key.yml
refusal 08-empty-slots empty-slots.yml
refusal 09-member-wrong-kind member-wrong-kind.yml
refusal 10-member-out-of-range member-out-of-range.yml
refusal 11-repeated-slot repeated-slot.yml
refusal 12-uncovered-context uncovered-context.yml
refusal 13-cpus-key cpus-key.yml
refusal 14-key-outside-grammar key-outside-grammar.yml
refusal 15-repeated-name repeated-name.yml
refusal 16-name-outside-grammar name-outside-grammar.yml
refusal 17-beside-preset four-groups.yml --placement=core
refusal 18-beside-hunt four-groups.yml --hunt=all
printf 'the two pairings read the accepted file beside `--placement=core`\n' \
  >> "$TRANSCRIPT"
printf 'and beside `--hunt`, each naming both rows in its line (FR-019)\n' \
  >> "$TRANSCRIPT"

say "refusals: 18 runs, transcript $TRANSCRIPT"

# Run 5, ungrouped: the same fixture with no preset and no file. The
# comparison region is the context disclosure line, whose bytes carry the
# group segment and hold no timing figure a second run can move (FR-033).
begin ungrouped 'ungrouped: one fixture run with no partition, over the region\nFR-033 compares (SC-006)'

expect 0 "$OUT/ungrouped.branch.console.txt" \
  "$BENCH" --threads=8 --iterations=1 --no-percentiles \
  --json="$OUT/ungrouped.branch.json"
disclosure "$OUT/ungrouped.branch.console.txt" \
  "$OUT/ungrouped.branch.disclosure.txt"
if grep -q '"group"' "$OUT/ungrouped.branch.json"; then
  finding 'the ungrouped document carries a group key'
else
  printf 'the document carries no group key (FR-033)\n' >> "$TRANSCRIPT"
fi

if [ -n "$MASTER" ]; then
  expect 0 "$OUT/ungrouped.master.console.txt" \
    "$MASTER/example/docs_threads" --threads=8 --iterations=1 \
    --no-percentiles --json="$OUT/ungrouped.master.json"
  disclosure "$OUT/ungrouped.master.console.txt" \
    "$OUT/ungrouped.master.disclosure.txt"
  printf '$ diff -u %s %s\n' "$OUT/ungrouped.branch.disclosure.txt" \
    "$OUT/ungrouped.master.disclosure.txt" >> "$TRANSCRIPT"
  diff -u "$OUT/ungrouped.branch.disclosure.txt" \
    "$OUT/ungrouped.master.disclosure.txt" \
    > "$OUT/ungrouped.disclosure.diff"
  if [ -s "$OUT/ungrouped.disclosure.diff" ]; then
    finding 'the two disclosure lines differ, the bytes stand in the diff'
  else
    printf 'the two disclosure lines stand byte-identical (FR-033)\n' \
      >> "$TRANSCRIPT"
  fi
else
  rm -f "$OUT/ungrouped.master.console.txt" "$OUT/ungrouped.master.json" \
    "$OUT/ungrouped.master.disclosure.txt" "$OUT/ungrouped.disclosure.diff"
  printf 'the far side stands open: no master tree stood named by\n' \
    >> "$TRANSCRIPT"
  printf '`--master-build`, so this run recorded the branch bytes alone\n' \
    >> "$TRANSCRIPT"
fi

say "ungrouped: 1 run, transcript $TRANSCRIPT"

if [ "$status" -eq 0 ]; then
  say 'run_groups: 5 runs, every reading matched'
fi
exit "$status"
