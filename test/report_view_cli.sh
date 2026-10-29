#!/usr/bin/env bash
# test/report_view_cli.sh
# The report view driver gate (specs/038-report-view-generators T017;
# FR-014 to FR-017, contracts/views-api.md B-6 and the refusal and
# disclosure table C): the built binary drives the fixture documents
# and every row of the table answers the contract.
#
# The rows: the four views over multi.json answer their bytes on
# standard output at status 0; a document the reader refuses, a
# benchmark the document does not carry, a column outside the
# selector, a view word outside the four, a missing required argument
# and a repeated required argument each answer one refusal line on
# standard error at status 2 with no view output ahead of them; the
# disclosure rows, the one-value histogram, the empty plot series and
# the absent page identity, print the disclosure on standard output at
# status 0; the help exit prints one line per argument row at status 0.
#
# The empty-series row needs a document whose every repetition value
# stands unavailable, and no fixture carries one, so the script builds
# a scratch document from single.json with the one repetition row
# marked unavailable, the way test/time_source_gate.sh plants its
# probe. The scratch documents and the captures live in one temporary
# directory; the repository tree is never written.
#
# Usage: report_view_cli.sh <report_view-binary> <fixture-dir>

set -u

BIN=${1:-}
FIXTURES=${2:-}

if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
  echo "report_view_cli: usage: report_view_cli.sh <report_view binary> <fixture dir>" >&2
  exit 2
fi
if [ -z "$FIXTURES" ] || [ ! -d "$FIXTURES" ]; then
  echo "report_view_cli: the fixture directory '$FIXTURES' answers nothing" >&2
  exit 2
fi

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

failures=0

# One diagnostic line per failed row, the shape of the time-source
# gate's hits, and one verdict line at the end.
note() {
  echo "report_view_cli: $1" >&2
  failures=$((failures + 1))
}

# drive <arguments...>: runs the binary once and leaves the status in
# $status, the captured standard output in $out with its line count in
# $out_lines, and the captured standard error in $err with its line
# count in $err_lines.
drive() {
  "$BIN" "$@" >"$WORKDIR/stdout" 2>"$WORKDIR/stderr"
  status=$?
  out=$(cat "$WORKDIR/stdout")
  err=$(cat "$WORKDIR/stderr")
  out_lines=$(wc -l <"$WORKDIR/stdout")
  err_lines=$(wc -l <"$WORKDIR/stderr")
}

# ---- The four views over multi.json ----

for view in histogram plot page text; do
  drive "$FIXTURES/multi.json" --benchmark='bmViews/threads:2' \
        --metric=time --view="$view"
  if [ "$status" -ne 0 ] || [ -z "$out" ] || [ "$err_lines" -ne 0 ]; then
    note "the $view view over multi.json failed (status $status," \
         "view bytes ${#out}, stderr lines $err_lines)"
  fi
done

# ---- The reader fault ----

printf 'this file holds no JSON the parser reads' >"$WORKDIR/malformed.json"
drive "$WORKDIR/malformed.json" --benchmark=bmViews --metric=time \
      --view=histogram
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the reader fault failed (status $status, stdout bytes ${#out}," \
       "stderr lines $err_lines): $err"
fi

# ---- The refusal rows of table C ----

drive "$FIXTURES/multi.json" --benchmark=absent --metric=time \
      --view=histogram
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the benchmark-missing refusal failed (status $status," \
       "stdout bytes ${#out}, stderr lines $err_lines): $err"
fi

drive "$FIXTURES/multi.json" --benchmark='bmViews/threads:2' \
      --metric=instructions --view=histogram
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the counter-label refusal failed (status $status," \
       "stdout bytes ${#out}, stderr lines $err_lines): $err"
fi

drive "$FIXTURES/multi.json" --benchmark='bmViews/threads:2' \
      --metric=time --view=sound
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the view-word refusal failed (status $status," \
       "stdout bytes ${#out}, stderr lines $err_lines): $err"
fi

drive
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the missing-argument refusal over no arguments failed" \
       "(status $status, stdout bytes ${#out}," \
       "stderr lines $err_lines): $err"
fi

drive "$FIXTURES/multi.json" --benchmark='bmViews/threads:2' --metric=time
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the missing-argument refusal over the view argument failed" \
       "(status $status, stdout bytes ${#out}," \
       "stderr lines $err_lines): $err"
fi

drive "$FIXTURES/multi.json" --benchmark   # an option naming no value
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the missing-argument refusal over a valueless option failed" \
       "(status $status, stdout bytes ${#out}," \
       "stderr lines $err_lines): $err"
fi

drive "$FIXTURES/multi.json" --benchmark=absent --benchmark=bmViews \
      --metric=time --view=histogram
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the repeated-argument refusal failed (status $status," \
       "stdout bytes ${#out}, stderr lines $err_lines): $err"
fi

drive "$FIXTURES/multi.json" "$FIXTURES/single.json" --benchmark=bmViews \
      --metric=time --view=histogram
if [ "$status" -ne 2 ] || [ -n "$out" ] || [ "$err_lines" -ne 1 ]; then
  note "the repeated-report-path refusal failed (status $status," \
       "stdout bytes ${#out}, stderr lines $err_lines): $err"
fi

# ---- The disclosure rows of table C ----

drive "$FIXTURES/single.json" --benchmark=bmViewsSingle --metric=time \
      --view=histogram
if [ "$status" -ne 0 ] || [ "$err_lines" -ne 0 ] \
   || ! printf '%s' "$out" | grep -q 'not available'; then
  note "the one-value histogram disclosure failed (status $status," \
       "stderr lines $err_lines): $out"
fi

python3 - "$FIXTURES/single.json" "$WORKDIR/empty_series.json" <<'EOF'
import json, sys

document = json.load(open(sys.argv[1]))
row = document["benchmarks"][0]["rows"][0]
row["timeAvailability"] = "GAP"
row["timePerIterationNs"] = 0.0
with open(sys.argv[2], "w") as stream:
    json.dump(document, stream)
EOF
drive "$WORKDIR/empty_series.json" --benchmark=bmViewsSingle --metric=time \
      --view=plot
if [ "$status" -ne 0 ] || [ "$err_lines" -ne 0 ] \
   || ! printf '%s' "$out" | grep -q 'not available'; then
  note "the empty-series plot disclosure failed (status $status," \
       "stderr lines $err_lines): $out"
fi

drive "$FIXTURES/nospeedgun.json" --benchmark=bmViewsNoHeader --metric=time \
      --view=page
if [ "$status" -ne 0 ] || [ "$err_lines" -ne 0 ] \
   || ! printf '%s' "$out" | grep -q 'not available'; then
  note "the absent page identity disclosure failed (status $status," \
       "stderr lines $err_lines): $out"
fi

# ---- The help exit ----

drive --help
if [ "$status" -ne 0 ] || [ "$err_lines" -ne 0 ] || [ "$out_lines" -ne 4 ] \
   || ! printf '%s' "$out" | grep -q 'report_view <report>' \
   || ! printf '%s' "$out" | grep -q 'benchmark=' \
   || ! printf '%s' "$out" | grep -q 'metric=' \
   || ! printf '%s' "$out" | grep -q 'view='; then
  note "the help exit failed (status $status, stderr lines $err_lines," \
       "stdout lines $out_lines): $out"
fi

# ---- The verdict ----

if [ "$failures" -eq 0 ]; then
  echo "report_view_cli: clean"
  exit 0
fi
echo "report_view_cli: $failures row(s) failed"
exit 1
