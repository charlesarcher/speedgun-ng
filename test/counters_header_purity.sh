#!/usr/bin/env bash
# test/counters_header_purity.sh
# Vocabulary purity scan (T010, T102; FR-010, C-SYS-6): the public counters
# headers carry no platform-counter, clock-syscall, or counter-instruction
# names. Violations are printed as file:line and the script exits 1;
# a clean scan exits 0.
#
# The scan covers four terms only: perf_event, clock_gettime, rdpmc, rdtsc.
# The platform vocabulary the headers do use is outside it, and saying so
# here is part of the record: CLOCK_MONOTONIC (the POSIX clock constant),
# CPUID (the tsc calibration and the fast tick read are spelled CPUID),
# sysfs (the PMU catalog's own source path), and pmu (the provider name
# and the counters_pmu.hpp header). `tsc` is not a term either, because
# FR-023 mandates `fast_tsc` as a read-mode name, so the term is required
# vocabulary, which the scan must not flag.
#
# A term matches as a substring, so perf_event_open and rdtscp are
# violations the whole-word form missed. The exception is rdpmc, which
# keeps the word boundary: FR-023 mandates the read-mode name
# fast_rdpmc, and scanning the bare term would flag that required name,
# exactly as scanning tsc would flag fast_tsc.
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
  if [ "$term" = rdpmc ]; then
    hits=$(grep -rn --include='counters*.hpp' -w -e "$term" "$HEADERS_DIR" || true)
  else
    hits=$(grep -rn --include='counters*.hpp' -e "$term" "$HEADERS_DIR" || true)
  fi
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
