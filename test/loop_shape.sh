#!/usr/bin/env bash
# test/loop_shape.sh
# Codegen-shape gate for the timed loop (IF-04; 015 FR-004, FR-005, R-06).
#
# Usage: loop_shape.sh
#
# The timed loop is the region whose cost the harness reports, so its shape
# is a property of the generated code and the gate reads that code back.
# The shipped loop must hold no call into the contract dispatch and no flag
# load, and it must not be longer than a hand-written count-down loop with
# the same body. A loop that pays a check per iteration charges the cost to
# every benchmark and leaves the overhead floor blind to it.
#
# A liveness probe plants one extra load per iteration and requires the
# detector to fail on it. A clean run can then never come from a detector
# that counts nothing.
#
# Exit 0 clean, 1 on a violation. A missing compiler is skipped by name; the
# gate fails only where neither g++ nor clang++ is present, as
# test/barrier_shape.sh does.

set -uo pipefail

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

# benchmark.hpp reaches the generated export header of the counters
# library, so the gate needs the build tree's export directory.
EXPORT_DIR=$(find "$ROOT/build" -maxdepth 3 -type d -name export 2>/dev/null | head -1)
if [ -z "$EXPORT_DIR" ]; then
  echo "FAIL: no generated export directory under $ROOT/build" >&2
  exit 1
fi

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

# The body of the loop: the instructions from the target of the backward
# jump up to that jump. The backward jump is the one whose operand address
# sits at or below its own address.
loop_body() {
  printf '%s\n' "$1" | awk '
    {
      addr[NR] = $1
      ins[NR] = $2
      op[NR] = $3
      n = NR
    }
    END {
      for (i = n; i >= 1; --i) {
        if (ins[i] ~ /^j/) {
          target = op[i]
          gsub(/[^0-9a-f]/, "", target)
          if (length(target) > 0 && strtonum("0x" target) <= strtonum("0x" addr[i])) {
            for (k = 1; k <= n; ++k) {
              if (addr[k] == target) {
                for (m = k; m <= i; ++m) {
                  printf "%s\t%s\t%s\n", addr[m], ins[m], op[m]
                }
                exit
              }
            }
          }
        }
      }
    }
  '
}

shape_source() {
  cat <<'SOURCE'
#include <cstdint>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

volatile std::uint64_t extra = 0;

__attribute__((noinline)) auto shapeLoop(sg::State& state, std::uint64_t value)
    -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(value);
  }
}

__attribute__((noinline)) auto referenceLoop(std::uint64_t iterations,
                                             std::uint64_t value) -> void
{
  std::uint64_t remaining = iterations;
  while (remaining != 0) {
    sg::doNotOptimize(value);
    --remaining;
  }
}

__attribute__((noinline)) auto plantedLoop(sg::State& state, std::uint64_t value)
    -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(value);
    sg::doNotOptimize(extra);
  }
}
SOURCE
}

compile_shapes() {
  local cxx=$1
  printf '%s\n' "$(shape_source)" > "$WORKDIR/shape.cpp"
  "$cxx" -O2 -std=c++23 -I "$ROOT/include" -I "$EXPORT_DIR" \
    -c "$WORKDIR/shape.cpp" -o "$WORKDIR/shape-$cxx.o" 2> "$WORKDIR/err.txt"
}

# One loop: no call into the contract dispatch, and no longer than the
# hand-written count-down loop.
analyze() {
  local obj=$1 cxx=$2 label=$3
  local all body reference body_n

  all=$(dump_function "$obj" "$label")
  if [ -z "$all" ]; then
    echo "FAIL $cxx: could not locate $label in the object" >&2
    return 1
  fi
  body=$(loop_body "$all")
  if [ -z "$body" ]; then
    echo "FAIL $cxx: $label holds no loop" >&2
    return 1
  fi

  if printf '%s\n' "$body" | grep -qE $'\tcall'; then
    echo "FAIL $cxx: the $label loop calls out of line" >&2
    printf '%s\n' "$body" | sed 's/^/  /' >&2
    return 1
  fi

  body_n=$(printf '%s\n' "$body" | wc -l)
  reference=$(loop_body "$(dump_function "$obj" 'referenceLoop')")
  if [ -z "$reference" ]; then
    echo "FAIL $cxx: could not locate referenceLoop in the object" >&2
    return 1
  fi
  reference_n=$(printf '%s\n' "$reference" | wc -l)

  echo "  $cxx $label: loop body holds $body_n instructions, the count-down" \
       "reference $reference_n"
  # The two loops can be spelled with a count-up compare (inc, cmp, jne)
  # or a count-down test (sub, jne), one instruction apart. The bound
  # absorbs that spelling difference and still catches an extra load.
  if [ "$body_n" -gt $((reference_n + 1)) ]; then
    echo "FAIL $cxx: the $label loop is longer than the count-down reference" \
      >&2
    printf '%s\n' "$body" | sed 's/^/  /' >&2
    return 1
  fi
  return 0
}

# Liveness: the detector must fail on a loop that loads one extra value per
# iteration, or a clean run below proves nothing.
liveness_probe() {
  local cxx=$1 obj=$2

  if analyze "$obj" "$cxx" 'plantedLoop'; then
    echo "FAIL $cxx: a loop with one extra load per iteration passed, so the" \
      >&2
    echo "      detector below proves nothing" >&2
    return 1
  fi
  echo "  $cxx: detector fails on a loop with an extra load (live)"
  return 0
}

status=0
found=0

for cxx in g++ clang++; do
  command -v "$cxx" >/dev/null 2>&1 || { echo "skip $cxx (not present)"; continue; }
  found=1
  echo
  echo "========== $cxx at -O2 =========="

  if ! compile_shapes "$cxx"; then
    echo "FAIL $cxx: the shape translation unit did not compile" >&2
    sed 's/^/  /' "$WORKDIR/err.txt" >&2
    status=1
    continue
  fi

  if ! liveness_probe "$cxx" "$WORKDIR/shape-$cxx.o"; then
    status=1
    continue
  fi

  if ! analyze "$WORKDIR/shape-$cxx.o" "$cxx" 'shapeLoop'; then
    echo "      FR-004: the timed loop pays for more than the loop" >&2
    status=1
  fi
done

if [ "$found" -eq 0 ]; then
  echo "FAIL: neither g++ nor clang++ is present" >&2
  exit 1
fi

echo
if [ "$status" -eq 0 ]; then
  echo "loop_shape: the timed loop holds no out-of-line call and stays"
  echo "            within the count-down reference, in every compiler present"
else
  echo "loop_shape: FAILED"
fi
exit "$status"
