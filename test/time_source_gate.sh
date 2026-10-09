#!/usr/bin/env bash
# test/time_source_gate.sh
# The time-source gate (specs/015-benchmark-harness-core T008, T011;
# FR-039, FR-040, SC-013, D-6): no C++ source or header outside the
# counters library reaches a time or time-stamp source the counters
# library does not own.
#
# The scanned set is the D-6 scope: source/, include/, example/, test/,
# and tools/, over the C and C++ extensions .c, .cc, .cpp, .cxx, .h,
# .hh, .hpp, .hxx, .ipp, .inl, and .tpp. The counters library sits
# outside the rule and keeps its own clock reads (FR-040):
# source/counters/, the
# include/speedgun-ng/counters*.hpp headers, and the counters_ tests and
# gate scripts under test/. Code under external/ is outside the rule.
#
# The banned list is the constitutional list of D-6, and a term matches
# as a substring, so a qualified or intrinsics spelling the table names
# is a hit. The matching discipline is test/counters_header_purity.sh:
# substring terms, one printed file:line per hit, exit 1 on any hit,
# exit 0 on a clean scan.
#
# One allowance exists, and D-6 names it: tools/dbc/overhead.cpp may
# hold the <chrono> include and std::chrono::steady_clock, because that
# tool measures contract overhead against an independent reference clock
# on purpose. Every other banned term in that file is a hit, and a spec,
# a plan, a local override, or a suppression comment creates no other
# exception.
#
# The scan carries its own liveness probe, for the reason the purity
# scan records: a clean run must not come from a scan that inspects
# nothing. A scratch root holding one planted call must exit 1 with the
# hit printed, and the same scratch root without the plant must exit 0.
# The repository is never written.
#
# Usage: time_source_gate.sh <repo-root>

set -u

ROOT=${1:-.}

TERMS=(
  'std::chrono::system_clock'
  'std::chrono::steady_clock'
  'std::chrono::high_resolution_clock'
  '<chrono>'
  'std::clock'
  'std::time'
  'timespec_get'
  'clock_gettime'
  'clock_getres'
  'gettimeofday'
  'time('
  'times('
  'getrusage'
  'rdtsc'
  'rdtscp'
  '__rdtsc'
  '__rdtscp'
  '<x86intrin.h>'
  '<immintrin.h>'
  '<ia32intrin.h>'
)

# The D-6 exception, and the only two terms it admits.
EXEMPT_FILE='tools/dbc/overhead.cpp'
EXEMPT_TERMS='<chrono> std::chrono::steady_clock'

scan_root() {
  local root=$1
  local status=0
  local files=()
  local term

  while IFS= read -r file; do
    files+=("$file")
  done < <(
    cd "$root" || return 2
    find source include example test tools -type f \
         \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \
            -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \
            -o -name '*.ipp' -o -name '*.inl' -o -name '*.tpp' \) 2>/dev/null \
      | grep -v '^source/counters/' \
      | grep -v '^include/speedgun-ng/counters' \
      | grep -v '^test/source/counters_' \
      | grep -v '^test/counters_' \
      | sort
  )

  if [ ${#files[@]} -eq 0 ]; then
    echo "time_source_gate: the scan found no files under $root" >&2
    return 2
  fi

  for term in "${TERMS[@]}"; do
    local hits
    hits=$(cd "$root" && grep -FHn -- "$term" "${files[@]}" 2>/dev/null || true)
    if [[ " $EXEMPT_TERMS " == *" $term "* ]]; then
      hits=$(printf '%s\n' "$hits" | grep -v "^$EXEMPT_FILE:" || true)
    fi
    if [ -n "$hits" ]; then
      echo "time_source_gate: banned time source '$term':"
      printf '%s\n' "$hits"
      status=1
    fi
  done

  return $status
}

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

# Liveness probe: the same scan over a scratch root, unplanted and then
# planted. A clean repository run proves nothing if the scan cannot see
# a planted call at all.
probe() {
  local probe_root="$WORKDIR/probe"
  local out status

  mkdir -p "$probe_root/source/harness" \
    || return 1
  printf 'int probe() { return 0; }\n' >"$probe_root/source/harness/probe.cpp"
  mkdir -p "$probe_root/include/speedgun-ng" || return 1

  out=$(scan_root "$probe_root" 2>&1)
  status=$?
  if [ $status -ne 0 ]; then
    echo "FAIL: the unplanted scratch root reported $status" >&2
    printf '%s\n' "$out" >&2
    return 1
  fi

  printf 'void probe2() { std::chrono::steady_clock::now(); }\n' \
    >>"$probe_root/source/harness/probe.cpp"

  out=$(scan_root "$probe_root" 2>&1)
  status=$?
  if [ $status -ne 1 ] || ! printf '%s\n' "$out" | grep -q 'probe.cpp:'; then
    echo "FAIL: the planted std::chrono::steady_clock call went unreported" >&2
    echo "      (status $status), so a clean run on the repository proves" >&2
    echo "      nothing" >&2
    printf '%s\n' "$out" >&2
    return 1
  fi
  echo "  planted the banned call in the scratch root, the gate reported it"

  printf 'void probe3() { clock_gettime(0, 0); }\n' \
    >"$probe_root/include/speedgun-ng/probe.h"

  out=$(scan_root "$probe_root" 2>&1)
  status=$?
  if [ $status -ne 1 ] || ! printf '%s\n' "$out" | grep -q 'probe.h:'; then
    echo "FAIL: the planted clock_gettime in a .h header went unreported" >&2
    echo "      (status $status), so a clean run on the repository proves" >&2
    echo "      nothing" >&2
    printf '%s\n' "$out" >&2
    return 1
  fi
  echo "  planted the banned call in a .h header, the gate reported it"
  return 0
}

if [ "${SG_TIME_GATE_PROBE:-}" != 1 ]; then
  echo "=== time-source gate liveness probe ==="
  if ! probe; then
    exit 1
  fi
  echo
fi

if scan_root "$ROOT"; then
  echo "time_source_gate: clean"
  exit 0
fi
exit 1
