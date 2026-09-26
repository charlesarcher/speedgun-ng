#!/usr/bin/env bash
# test/counters_header_purity.sh
# Vocabulary purity scan (T010; FR-010, C-SYS-6): the public counters
# headers carry no platform-counter, clock-syscall, or counter-instruction
# names. Violations are printed as file:line and the script exits 1;
# a clean scan exits 0.
#
# Usage: counters_header_purity.sh <repo-root>

set -u

ROOT=${1:-.}
HEADERS_DIR="$ROOT/include/speedgun-ng"

if [ ! -d "$HEADERS_DIR" ]; then
  echo "counters_header_purity: no header dir at $HEADERS_DIR"
  exit 2
fi

status=0
for term in perf_event clock_gettime rdpmc rdtsc; do
  hits=$(grep -rn --include='counters*.hpp' -w "$term" "$HEADERS_DIR" || true)
  if [ -n "$hits" ]; then
    echo "counters_header_purity: platform term '$term' in public headers:"
    printf '%s\n' "$hits"
    status=1
  fi
done

if [ $status -eq 0 ]; then
  echo "counters_header_purity: clean"
fi
exit $status
