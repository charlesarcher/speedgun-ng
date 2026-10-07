#!/usr/bin/env bash
# test/counters_header_purity.sh
# Vocabulary purity scan (T010, T102; FR-010, C-SYS-6): the public counters
# headers carry no platform-counter, clock-syscall, or counter-instruction
# names. Violations are printed as file:line and the script exits 1;
# a clean scan exits 0.
#
# The scan covers four terms across every public counters header and
# across the public header declaring the trace-start marker:
# perf_event, clock_gettime, rdpmc, rdtsc. The second of those two
# include patterns is `simulation.hpp`, which the counters glob misses
# because the header is named for what it declares (specs/011 R-013).
# A fifth term, the platform acronym PMU, is scanned across the core
# vocabulary headers only, and its scope is the second half of this
# record. The platform vocabulary
# the headers do use is outside the first four terms, and saying so
# here is part of the record: sysfs (the PMU catalog's own source path)
# and pmu (the provider name and the counters_pmu.hpp header). The
# calibration that spelled CPUID in a header left with the raw
# time-stamp counter (specs/008-timestamp-counter, FR-011), and no
# header spells it now, and none spells CLOCK_MONOTONIC. `tsc` is not
# a term either, because FR-023 mandates `FAST_TSC` as a read-mode
# name, so the term is required vocabulary, which the scan must not
# flag.
#
# A term matches as a substring, so perf_event_open and rdtscp are
# violations the whole-word form missed. The exception is rdpmc, which
# keeps the word boundary: FR-023 mandates the read-mode name
# FAST_RDPMC, and scanning the bare term would flag that required name,
# exactly as scanning tsc would flag FAST_TSC.
#
# The PMU term matches a standalone uppercase acronym in a core
# vocabulary header, which is the shape a prose violation takes:
# "a core-PMU group" names the concept, and that is what FR-010 puts
# outside the core vocabulary. Case sensitivity and the word boundary
# are what keep the required vocabulary legal. The mandated enumerator
# is spelled `ReadMode::FAST_RDPMC` and lives in a
# scanned header. The shipped provider header is `counters_pmu.hpp` in
# lowercase, and its include guard is
# `SPEEDGUN_NG_COUNTERS_PMU_HPP`, where `PMU` sits between word
# characters and the word boundary rejects it. A bare `pmu` term
# matches all three, so the term is the uppercase acronym and the
# boundary keeps the guard.
#
# The term is scoped to the core vocabulary headers because FR-010
# places the platform names inside provider implementations, and the
# four shipped providers are the one place the acronym belongs. The
# core list is spelled out and every entry is checked for existence,
# so renaming a core header leaves a named gap and exits 2 instead of
# quietly scanning less.
#
# The two include patterns are the widened set. Dropping either one
# leaves this scan inspecting fewer headers and still exiting 0, so a
# probe copies the scanned headers into a scratch tree, requires this
# scan to report nothing in that copy, plants one banned term in the
# copy of simulation.hpp, and requires this scan to report it there. A
# clean run on the repository's headers then carries the widened set.
# The scratch tree lives in a temporary directory this script removes
# on exit, and the repository's headers are never written.
#
# Usage: counters_header_purity.sh <repo-root>

set -u

ROOT=${1:-.}
HEADERS_DIR="$ROOT/include/speedgun-ng"
CORE_HEADERS="counters_core.hpp counters_measurement.hpp counters_provider.hpp
counters_system.hpp counters.hpp"
TERMS="perf_event clock_gettime rdpmc rdtsc"
PROBE_TERM=perf_event

if [ ! -d "$HEADERS_DIR" ]; then
  echo "counters_header_purity: no header dir at $HEADERS_DIR"
  exit 2
fi

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

# Widened-set probe: a scratch copy of the scanned headers carrying one
# planted banned term. This scan must report nothing in the copy before
# the plant and the planted term after it, so a clean run on the real
# headers cannot come from a scan that inspects nothing.
widened_set_probe() {
  local copy="$WORKDIR/probe/include/speedgun-ng"
  local out status

  mkdir -p "$copy" || return 1
  cp "$HEADERS_DIR"/*.hpp "$copy"/ || return 1

  out=$(SG_HEADER_PURITY_PROBE=1 bash "$0" "$WORKDIR/probe" 2>&1)
  status=$?
  if [ $status -ne 0 ]; then
    echo "FAIL: the unplanted copy of the scanned headers reported $status," >&2
    echo "      so a clean run cannot come from this copy" >&2
    printf '%s\n' "$out" >&2
    return 1
  fi

  printf '\n// planted by the widened-set probe\nconst char* probe = "%s";\n' \
    "$PROBE_TERM" >>"$copy/simulation.hpp" || return 1

  out=$(SG_HEADER_PURITY_PROBE=1 bash "$0" "$WORKDIR/probe" 2>&1)
  status=$?
  if [ $status -ne 1 ] || ! printf '%s\n' "$out" | grep -q "$PROBE_TERM"; then
    echo "FAIL: the planted term '$PROBE_TERM' went unreported, scan status" >&2
    echo "      $status, so a clean run on the real headers proves nothing" >&2
    printf '%s\n' "$out" >&2
    return 1
  fi
  echo "  planted '$PROBE_TERM' in the scratch copy, the scan reported it"
  return 0
}

status=0
if [ "${SG_HEADER_PURITY_PROBE:-}" != 1 ]; then
  echo "=== widened-set probe (scan liveness) ==="
  if ! widened_set_probe; then
    status=1
  fi
  echo
fi

for term in $TERMS; do
  if [ "$term" = rdpmc ]; then
    hits=$(grep -rn --include='counters*.hpp' --include='simulation.hpp' -w \
             -e "$term" "$HEADERS_DIR" || true)
  else
    hits=$(grep -rn --include='counters*.hpp' --include='simulation.hpp' \
             -e "$term" "$HEADERS_DIR" || true)
  fi
  if [ -n "$hits" ]; then
    echo "counters_header_purity: platform term '$term' in public headers:"
    printf '%s\n' "$hits"
    status=1
  fi
done

# The platform acronym, over the core vocabulary headers (see the record
# above). The shipped provider headers are the one place FR-010 puts it,
# so they are outside this term.
for header in $CORE_HEADERS; do
  path="$HEADERS_DIR/$header"
  if [ ! -f "$path" ]; then
    echo "counters_header_purity: core vocabulary header missing: $path"
    status=2
    continue
  fi
  hits=$(grep -Hn -E '\bPMU\b' "$path" || true)
  if [ -n "$hits" ]; then
    echo "counters_header_purity: platform concept 'PMU' in $header:"
    printf '%s\n' "$hits"
    status=1
  fi
done

if [ $status -eq 0 ]; then
  echo "counters_header_purity: clean"
fi
exit $status
