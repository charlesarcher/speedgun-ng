#!/usr/bin/env bash
# test/loop_shape.sh
# Codegen-shape gate for the timed loop (IF-04; 015 FR-004, FR-005, R-06).
#
# Usage: loop_shape.sh [build-dir]
#
# The build directory holds the generated export header of the counters
# library. The CMake test passes the build directory as the argument; a
# manual run can set SG_BUILD_DIR instead. With neither, the gate fails
# and names both.
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
BUILD_DIR=${1:-${SG_BUILD_DIR:-}}
if [ -z "$BUILD_DIR" ]; then
  echo "FAIL: no build directory: pass it as the first argument or set" >&2
  echo "      SG_BUILD_DIR" >&2
  exit 1
fi

EXPORT_DIR=$(find "$BUILD_DIR" -maxdepth 3 -type d -name export 2>/dev/null | head -1)
if [ -z "$EXPORT_DIR" ]; then
  echo "FAIL: no generated export directory under $BUILD_DIR" >&2
  exit 1
fi

dump_function() {
  local flags="-d --no-show-raw-insn -C"
  if [ "${3:-}" = "relocs" ]; then
    # A relocatable object names the callee of a call only in its relocation
    # entries, so the counter rule reads the dump with them.
    flags="-dr --no-show-raw-insn -C"
  fi
  objdump $flags "$1" 2>/dev/null | awk -v rx="$2" '
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
      printf "%s\t%s\t%s\t%s\n", tolower(addr), parts[1], (n > 1 ? parts[2] : ""),
             (n > 2 ? parts[3] : "")
      next
    }
    p && /^Disassembly of section/ { exit }
  '
}

# The body of the loop: the instructions from the target of the backward
# jump up to that jump. The backward jump chosen is the one with the
# shortest span, the cycle the iterations travel. A wider back edge can
# belong to a cold tail the compiler parks after the loop. That tail
# runs once per run; the iterations travel the short cycle.
loop_body() {
printf '%s\n' "$1" | awk '
{
addr[NR] = $1
ins[NR] = $2
op[NR] = $3
tgt[NR] = $4
n = NR
}
END {
best = 0
bestk = 0
bestspan = -1
bestdec = 0
for (i = 1; i <= n; ++i) {
  if (ins[i] !~ /^j/) {
    continue
  }
  target = op[i]
  gsub(/[^0-9a-f]/, "", target)
  if (length(target) == 0) {
    continue
  }
  if (strtonum("0x" target) > strtonum("0x" addr[i])) {
    continue
  }
  k = 0
  for (m = 1; m <= n; ++m) {
    if (addr[m] == target) {
      k = m
      break
    }
  }
  if (k == 0) {
    continue
  }
  span = strtonum("0x" addr[i]) - strtonum("0x" target)
  # specs/021 FR-001: a chunked cursor leaves a second cycle, the refill the
  # run count enters once per chunk. The iterations travel the cycle that
  # holds the count-down by one, so that cycle wins over a shorter one that
  # does not; among cycles that agree, the shortest still wins.
  dec = 0
  for (m = k; m <= i; ++m) {
    if (ins[m] ~ /^dec/ || (ins[m] ~ /^sub/ && op[m] ~ /^\$0x1/)) {
      dec = 1
      break
    }
  }
  if (best == 0 ||
      (dec > bestdec) ||
      (dec == bestdec && span < bestspan)) {
    bestspan = span
    best = i
    bestk = k
    bestdec = dec
  }
}
if (best == 0) {
exit
}
for (m = bestk; m <= best; ++m) {
printf "%s\t%s\t%s\t%s\n", addr[m], ins[m], op[m], tgt[m]
}
}
'
}

shape_source() {
  cat <<'SOURCE'
#include <cstddef>
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

// specs/021 T010: a cursor that samples every iteration instead of every
// chunk. The cycle it leaves holds the call into the recorder, which is
// what the call rule of the shape check is for.
__attribute__((noinline)) static auto plantedSample() -> void
{
  extra = extra + 1;
}

__attribute__((noinline)) auto plantedSampleLoop(sg::State& state,
                                                 std::uint64_t value) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(value);
    plantedSample();
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

// specs/021 FR-001, D-01: the count-down of a run that captures chunks
// carries one instruction more than the plain count-down: the test the
// refill re-enters. The gate allows that one instruction and no more, so a
// per-iteration load or call still fails the shape.

__attribute__((noinline)) auto plantedLoop(sg::State& state, std::uint64_t value)
    -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(value);
    sg::doNotOptimize(extra);
  }
}

// FR-007: the body of a benchmark that counts its own work. The counter store
// stands in fixed storage with inline name buffers, so the one out-of-line call
// the loop holds is the body's own call into `setCounter`.
__attribute__((noinline)) auto counterLoop(sg::State& state, std::uint64_t value)
    -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(value);
    state.setCounter("loop", {1.0});
  }
}

// The liveness shape of the rule below: a body that allocates on every
// iteration, so the detector has to reject it.
__attribute__((noinline)) auto plantedCounterLoop(sg::State& state,
                                                  std::uint64_t value) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(value);
    state.setCounter("loop", {1.0});
    sg::doNotOptimize(*new std::uint64_t[1]);
  }
}

// FR-007, E-06: a body that pauses its own timing with the explicit pair. The
// pause is an out-of-line call into the harness and the fold path copies a
// plain recorder view when it resets its slot, so the timed loop still reaches
// no allocation.
__attribute__((noinline)) auto pausePairLoop(sg::State& state,
                                             std::uint64_t value) -> void
{
  for (auto _ : state) {
    state.pauseTiming();
    sg::doNotOptimize(value);
    state.resumeTiming();
  }
}

// FR-007, E-06: the same proof for the scope guard, whose destructor closes the
// pair through the fold sink.
__attribute__((noinline)) auto scopedPauseLoop(sg::State& state,
                                               std::uint64_t value) -> void
{
  for (auto _ : state) {
    sg::ScopedPauseTiming guard {state};
    sg::doNotOptimize(value);
  }
}
SOURCE
}

compile_shapes() {
  local cxx=$1
  printf '%s\n' "$(shape_source)" > "$WORKDIR/shape.cpp"
  # A hardening build wraps a function whose frame holds an address-taken
  # temporary in a stack canary, and the cold block of that check sits between
  # the blocks of the loop with a back edge shorter than the loop's own. The
  # canary costs one compare per call, so it is not the cost per iteration the
  # gate reads, and the gate reads the loop without it.
  "$cxx" -O2 -fno-stack-protector -std=c++23 -I "$ROOT/include" -I "$EXPORT_DIR" \
    -c "$WORKDIR/shape.cpp" -o "$WORKDIR/shape-$cxx.o" 2> "$WORKDIR/err.txt"
}

# One loop: no call into the contract dispatch, and no longer than the
# hand-written count-down loop.
analyze() {
  local obj=$1 cxx=$2 label=$3
  local all body reference body_n call_n

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

  echo "  $cxx $label: loop body holds $body_n instructions, the count-down"
  echo "      reference $reference_n plus the one instruction of the chunk"
  echo "      boundary"
  # The shipped loop counts down the remaining iterations, exactly as the
  # reference does, so the two bodies must match instruction for
  # instruction. A per-iteration check or load makes the shipped loop
  # longer, which is what the gate is for. specs/021 FR-001, D-01: the
  # cursor counts a chunk down and re-enters the count-down at the refill,
  # and that re-entry test is the one instruction the shape allows.
  if [ "$body_n" -gt "$((reference_n + 1))" ]; then
    echo "FAIL $cxx: the $label loop is longer than the count-down reference"
    echo "      plus the chunk boundary" >&2
    printf '%s\n' "$body" | sed 's/^/  /' >&2
    return 1
  fi
  # specs/021 FR-001, D-01: the sample belongs to the chunk boundary, so a
  # call in the cycle the iterations travel names a cursor that samples per
  # iteration. The refill stands outside that cycle.
  call_n=$(printf '%s\n' "$body" | awk -F'\t' '$2 ~ /^call/' | wc -l)
  if [ "$call_n" -ne 0 ]; then
    echo "FAIL $cxx: the $label loop holds $call_n out-of-line calls; the timed"
    echo "      loop may hold none (FR-001)" >&2
    printf '%s\n' "$body" | sed 's/^/  /' >&2
    return 1
  fi
  return 0
}

# The counter loop of FR-007: the body owns one out-of-line call, the call into
# `State::setCounter`, and nothing else. An allocation would add a second call
# and would name the allocation symbol in the function, so both readings are
# checked.
analyze_counter() {
  local obj=$1 cxx=$2 label=$3
  local all body call_n

  all=$(dump_function "$obj" "$label" relocs)
  if [ -z "$all" ]; then
    echo "FAIL $cxx: could not locate $label in the object" >&2
    return 1
  fi
  body=$(loop_body "$all")
  if [ -z "$body" ]; then
    echo "FAIL $cxx: $label holds no loop" >&2
    return 1
  fi

  call_n=$(printf '%s\n' "$body" | awk -F'\t' '$2 ~ /^call/' | wc -l)
  if [ "$call_n" -ne 1 ]; then
    echo "FAIL $cxx: the $label loop holds $call_n out-of-line calls; the body's" \
      >&2
    echo "      own call into setCounter is the one it may hold (FR-007)" >&2
    printf '%s\n' "$body" | sed 's/^/  /' >&2
    return 1
  fi

  # The function reaches the counter store exactly once and reaches no
  # allocation. A relocatable object names the callee of a call in its
  # relocation entries, and a compiler is free to rotate the loop, so these
  # two readings cover the whole function while the count above reads the
  # cycle the iterations travel.
  if [ "$(printf '%s\n' "$all" | grep -c 'R_X86_64_PLT32.*setCounter')" -ne 1 ]; then
    echo "FAIL $cxx: $label does not reach State::setCounter exactly once" >&2
    printf '%s\n' "$all" | sed 's/^/  /' >&2
    return 1
  fi

  if printf '%s\n' "$all" | grep -qE '_Znwm|_ZnaYm|operator[[:space:]]+new|malloc|calloc|memcpy|memmove'; then
    echo "FAIL $cxx: the $label function reaches an allocation, so the counter" \
      >&2
    echo "      store would allocate inside the timed loop (FR-007)" >&2
    return 1
  fi

  echo "  $cxx $label: the loop holds the body's own call and allocates nothing"
  return 0
}

# The pause paths of E-06 hold calls into the harness, so the count of calls
# says nothing there. What has to hold is that the function reaches no
# allocation: the fold path resets its slot by copying a plain recorder view,
# which is a fixed-size store and not a request for memory.
analyze_no_alloc() {
  local obj=$1 cxx=$2 label=$3
  local all body

  all=$(dump_function "$obj" "$label" relocs)
  if [ -z "$all" ]; then
    echo "FAIL $cxx: could not locate $label in the object" >&2
    return 1
  fi
  body=$(loop_body "$all")
  if [ -z "$body" ]; then
    echo "FAIL $cxx: $label holds no loop" >&2
    return 1
  fi

  if printf '%s\n' "$all" | grep -qE '_Znwm|_ZnaYm|operator[[:space:]]+new|malloc|calloc|memcpy|memmove'; then
    echo "FAIL $cxx: the $label function reaches an allocation, so the pause" \
      >&2
    echo "      path would allocate inside the timed loop (FR-007)" >&2
    return 1
  fi

  echo "  $cxx $label: the pause path reaches no allocation"
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

  if analyze "$obj" "$cxx" 'plantedSampleLoop'; then
    echo "FAIL $cxx: a loop that samples every iteration passed, so the call"
    echo "      rule proves nothing (T010)" >&2
    return 1
  fi
  echo "  $cxx: detector fails on a loop that samples per iteration (live)"
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

  if ! analyze_counter "$WORKDIR/shape-$cxx.o" "$cxx" 'counterLoop'; then
    echo "      FR-007: the counter loop pays for more than its own call" >&2
    status=1
  fi

  # Both pause paths, the explicit pair and the scope guard.
  for pause_label in pausePairLoop scopedPauseLoop; do
    if ! analyze_no_alloc "$WORKDIR/shape-$cxx.o" "$cxx" "$pause_label"; then
      status=1
    fi
  done

  # The allocation reading is live for the pause check too.
  if analyze_no_alloc "$WORKDIR/shape-$cxx.o" "$cxx" 'plantedCounterLoop'; then
    echo "FAIL $cxx: the pause allocation check accepted a loop that allocates" \
      >&2
    status=1
  fi

  # The counter rule is live: a body that allocates per iteration has to be
  # rejected, or the clean reading above proves nothing.
  if analyze_counter "$WORKDIR/shape-$cxx.o" "$cxx" 'plantedCounterLoop'; then
    echo "FAIL $cxx: a loop that allocates per iteration passed the counter" \
      >&2
    echo "      check, so that check proves nothing" >&2
    status=1
  else
    echo "  $cxx: counter check rejects a loop that allocates (live)"
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
