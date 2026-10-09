#!/usr/bin/env bash
# test/barrier_shape.sh
# Codegen-shape gate for the optimization barrier (T036; 015 FR-029, FR-030,
# US6 scenarios 1 and 2, D-4).
#
# Usage: barrier_shape.sh
#
# The barrier's whole contract is a property of the generated code, so the
# gate compiles the shape itself at -O2 and reads the instructions back. One
# translation unit keeps the measured work alive through sg::doNotOptimize
# from the shipped header; the other drops that call. The first must retain
# the computation, the second must lose it, and the gap between the two is
# what the gate reports. A barrier that the optimizer can see through costs
# nothing and measures nothing, and the harness would then publish a number
# for work that is not there (FR-030).
#
# The measured work is a dead-by-construction chain: its only observable use
# is the barrier itself, so an absent barrier leaves the function with no
# work at all. That is the shape FR-030 talks about, and it is why the
# without-barrier count must stay near the return instruction alone.
#
# A liveness probe plants a no-op barrier, an empty asm statement that does
# not name the value, and requires this gate's detector to fail on it. A
# clean run can then never come from a detector that counts nothing.
#
# Exit 0 clean, 1 on a violation. A missing compiler is skipped by name; the
# gate fails only where neither g++ nor clang++ is present, as
# test/counters_tsc_read_shape.sh does.

set -uo pipefail

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

dump_function() {
  objdump -d --no-show-raw-insn -C "$1" 2>/dev/null | awk -v rx="$2" '
    /^[[:space:]]*[0-9a-f]+ <.*>:/ {
      if (p) { exit }
      if ($0 ~ rx && $0 !~ /\[clone/) { p = 1 }
      next
    }
    p && /^[[:space:]]*[0-9a-f]+:/ {
      colon = index($0, ":")
      addr = substr($0, 1, colon - 1)
      gsub(/[[:space:]]/, "", addr)
      rest = substr($0, colon + 1)
      gsub(/^[[:space:]]+/, "", rest)
      gsub(/[[:space:]]*$/, "", rest)
      n = split(rest, parts, /[[:space:]]+/)
      printf "%s\t%s\t%s\n", tolower(addr), parts[1], (n > 1 ? parts[2] : "")
      next
    }
    p && /^Disassembly of section/ { exit }
  '
}

# The measured work, spelled three ways. The barrier form includes the
# shipped header; the bare form names no barrier; the planted form names an
# empty asm statement that leaves the value unmentioned, which is the
# no-op barrier the liveness probe needs.
shape_source() {
  local form=$1
  cat <<SOURCE
#include <cstdint>

$BARRIER_DEFINE

__attribute__((noinline)) auto bmShape(std::uint64_t seed) -> void
{
  std::uint64_t accumulator = seed;
  for (int index = 0; index < 8; ++index) {
    accumulator = accumulator * 6364136223846793005ULL
                  + 1442695040888963407ULL;
    accumulator ^= accumulator >> 31;
  }
  SG_SHAPE_BARRIER(accumulator);
}
SOURCE
}

compile_shape() {
  local cxx=$1 form=$2 out=$3
  case "$form" in
    barrier)
      BARRIER_DEFINE='#include "speedgun-ng/barrier.hpp"
#define SG_SHAPE_BARRIER(v) ::sg::doNotOptimize(v)'
      ;;
    bare)
      BARRIER_DEFINE='#define SG_SHAPE_BARRIER(v) static_cast<void>(v)'
      ;;
    planted)
      BARRIER_DEFINE='#define SG_SHAPE_BARRIER(v) asm volatile("" ::: "memory")'
      ;;
  esac
  printf '%s\n' "$(shape_source "$form")" > "$WORKDIR/shape.cpp"
  "$cxx" -O2 -std=c++23 -I "$ROOT/include" -c "$WORKDIR/shape.cpp" \
    -o "$out" 2> "$WORKDIR/err.txt"
}

# Instructions of the shape function. The count is the claim: the barrier
# keeps the chain, the absence of it leaves the return alone.
analyze() {
  local obj=$1 label=$2 floor=$3 ceiling=$4
  local ins n

  ins=$(dump_function "$obj" 'bmShape')
  if [ -z "$ins" ]; then
    echo "FAIL $label: could not locate bmShape in the object" >&2
    return 1
  fi
  n=$(printf '%s\n' "$ins" | wc -l)
  echo "  $label: bmShape holds $n instructions"
  if [ "$n" -lt "$floor" ]; then
    echo "FAIL $label: bmShape holds $n instructions, expected at least $floor" \
      >&2
    return 1
  fi
  if [ "$n" -gt "$ceiling" ]; then
    echo "FAIL $label: bmShape holds $n instructions, expected at most $ceiling" \
      >&2
    return 1
  fi
  return 0
}

# Liveness: the detector must fail on a planted no-op barrier, or a clean
# run below proves nothing.
liveness_probe() {
  local cxx=$1
  local obj="$WORKDIR/probe-$cxx.o"

  if ! compile_shape "$cxx" planted "$obj"; then
    echo "FAIL $cxx: the planted shape did not compile" >&2
    sed 's/^/  /' "$WORKDIR/err.txt" >&2
    return 1
  fi
  if analyze "$obj" "$cxx planted no-op barrier" 10 4096; then
    echo "FAIL $cxx: a planted no-op barrier kept the measured work, so the" \
      >&2
    echo "      detector below proves nothing" >&2
    return 1
  fi
  echo "  $cxx: detector fails on a planted no-op barrier (live)"
  return 0
}

status=0
found=0

for cxx in g++ clang++; do
  command -v "$cxx" >/dev/null 2>&1 || { echo "skip $cxx (not present)"; continue; }
  found=1
  echo
  echo "========== $cxx at -O2 =========="

  if ! liveness_probe "$cxx"; then
    status=1
    continue
  fi

  if ! compile_shape "$cxx" barrier "$WORKDIR/with-$cxx.o"; then
    echo "FAIL $cxx: the barrier shape did not compile" >&2
    sed 's/^/  /' "$WORKDIR/err.txt" >&2
    status=1
    continue
  fi
  if ! compile_shape "$cxx" bare "$WORKDIR/without-$cxx.o"; then
    echo "FAIL $cxx: the bare shape did not compile" >&2
    sed 's/^/  /' "$WORKDIR/err.txt" >&2
    status=1
    continue
  fi

  # FR-029: the barrier keeps the chain, so the function holds the eight
  # multiplies, the eight xors, and their shifts. FR-030: without it the
  # function holds nothing but the return, which is the ceiling below.
  if ! analyze "$WORKDIR/with-$cxx.o" "$cxx with sg::doNotOptimize" 10 4096; then
    echo "      FR-029: the barrier let the measured work go" >&2
    status=1
  fi
  if ! analyze "$WORKDIR/without-$cxx.o" "$cxx without the barrier" 1 3; then
    echo "      FR-030: the work survived with no barrier at all" >&2
    status=1
  fi
done

if [ "$found" -eq 0 ]; then
  echo "FAIL: neither g++ nor clang++ is present" >&2
  exit 1
fi

echo
if [ "$status" -eq 0 ]; then
  echo "barrier_shape: the barrier keeps the measured work and its absence"
  echo "               eliminates it, in every compiler present"
else
  echo "barrier_shape: FAILED"
fi
exit "$status"
