#!/usr/bin/env bash
# test/counters_push_atomic_scan.sh
# Atomic read-modify-write scan of the push path (T100; FR-035, R-008):
# the hot-path increment, the sampled load, and the fold-time read are
# plain accesses on a thread-confined cell, and no atomic spelling
# appears on any of them. Violations are printed as file:line under the
# spelling that matched, and the script exits 1; a clean scan exits 0.
#
# The scanned files are the push path: the handle that adds
# (include/speedgun-ng/counters_measurement.hpp), the provider that
# declares it (include/speedgun-ng/counters_push.hpp), the group read
# that samples the cell (source/counters/push_provider.cpp), and the
# fold that reads it again (source/counters/fold.cpp).
#
# The terms are code spellings, so the prose these four files use to state
# the rule itself ("non-atomic", "never an atomic RMW") is legal.
# std::atomic covers the std types including atomic_ref and atomic_flag,
# atomic_ catches the C11 typedefs and the std free functions, _Atomic is
# the C keyword, memory_order_ is any ordering argument, and the two
# builtin prefixes close the spellings that reach an atomic RMW without
# naming an atomic at all. A comment that names one of the terms is a
# finding too, and the author resolves it by rewording.
#
# A grep scan is a lexical gate, so the properties it cannot reach are
# stated here: it does not see an atomic reached through a type alias,
# and it does not see a non-atomic read-modify-write on the cell, which
# FR-035 permits only because the cell is thread-confined. A planted
# std::atomic is the case it does catch, and the check names it.
#
# Usage: counters_push_atomic_scan.sh <repo-root>

set -u

ROOT=${1:-.}
FILES="include/speedgun-ng/counters_measurement.hpp
include/speedgun-ng/counters_push.hpp
source/counters/push_provider.cpp
source/counters/fold.cpp"

# Code-shaped spellings of an atomic access. Extended regex, one per
# term; the order runs from the specific to the general.
TERMS='std::atomic
include <atomic>
\<atomic_[a-z]
\<_Atomic\>
memory_order_
__atomic_
__sync_'

paths=""
for file in $FILES; do
  if [ ! -f "$ROOT/$file" ]; then
    echo "counters_push_atomic_scan: no such push path file: $ROOT/$file"
    exit 2
  fi
  paths="$paths $ROOT/$file"
done

# A grep that rejects the word-boundary syntax would report every
# anchored term clean and turn this gate into a no-op, so prove the
# syntax works here (a match means a working grep) and stop otherwise.
if ! printf 'atomic_x\n' | grep -qE -e '\<atomic_[a-z]'; then
  echo "counters_push_atomic_scan: grep rejects the word-boundary syntax"
  exit 2
fi

status=0
# Read line by line: one term may carry a space, and word splitting
# would cut it in two.
while IFS= read -r term; do
  [ -n "$term" ] || continue
  hits=$(grep -rnE -e "$term" $paths || true)
  if [ -n "$hits" ]; then
    echo "counters_push_atomic_scan: atomic spelling '$term' on the push path:"
    printf '%s\n' "$hits"
    status=1
  fi
done <<TERMS_EOF
$TERMS
TERMS_EOF

if [ $status -eq 0 ]; then
  echo "counters_push_atomic_scan: clean"
fi
exit $status
