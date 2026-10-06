#!/usr/bin/env bash
# test/counters_tsc_read_shape.sh
# Codegen-shape gate for the raw time-stamp read (T026; 008 FR-003, FR-022).
#
# Usage: counters_tsc_read_shape.sh <repo-root> <binary-dir>
#
# The read at source/counters/clock_provider.cpp is one instruction. This
# gate proves the compiled read arm carries no function call, so the only
# cost between the catalog entry and the instruction is arithmetic, a store
# into the sample ring, and the index bump. A call on that arm would cost a
# frame, a register spill, and a return on every sampling action, and the
# published budget would no longer describe the shipped code.
#
# The claim is about optimized code, so the gate compiles the translation
# unit at -O2 itself. An unoptimized object calls a helper for the read,
# which makes the property false in a build configuration where it does not
# matter. Both contract semantics are checked: the release-consumer
# configuration, where SG_CONTRACTS_SEMANTIC is 0, and the default, where it
# is 2 and the contract compares stay inline.
#
# The arm is bounded the way the compiler closes it: from the rdtsc through
# the first branch whose target lies below its own address, which is the
# loop back-edge. A forward branch leaves the arm, so the violation path and
# the other read modes stay outside the window.
#
# The window is a lower bound on the arm. The whole iteration reaches past
# it. A compiler that factors the store and the index bump into shared code
# ends the window early, and clang at -O2 reports four instructions where
# gcc reports fifteen. The reported size shows which happened. A factored
# store cannot hide a call between the read and the store, because
# factoring moves code downstream of the read, and a read routed through a
# helper leaves no rdtsc in this function at all, which the count below
# catches.
#
# Two detectors guard against each other. The function must hold exactly one
# rdtsc and no rdtscp or rdpid, which pins the recorded P2 choice at
# clock_provider.cpp:154, and the arm must hold no call. A negative probe
# then plants a call in an arm of the same shape and fails when this
# detector misses it, so a clean run cannot come from a detector that
# extracts nothing.
#
# Exit 0 clean, 1 on a violation, 0 with a skip line where the build target
# no x86 instruction, because 008 FR-001 publishes the entry only where the
# instruction exists.

set -uo pipefail

ROOT=${1:-.}
BINARY_DIR=${2:-}

if [ -z "$BINARY_DIR" ]; then
  echo "FAIL: binary dir not given; pass the CMake binary dir as \$2" >&2
  exit 1
fi

if [ "$(uname -s)" != "Linux" ]; then
  echo "skip: $(uname -s) is not Linux; the read is gated on the Linux CI job"
  exit 0
fi

case "$(uname -m)" in
  x86_64 | amd64 | i386 | i686) ;;
  *)
    echo "skip: $(uname -m) executes no x86 time-stamp instruction, so the"
    echo "      entry publishes nowhere and there is no read arm to gate"
    exit 0
    ;;
esac

EXPORT_DIR="$BINARY_DIR/export"
if [ ! -f "$EXPORT_DIR/speedgun-ng/speedgun-ng_export.hpp" ]; then
  echo "FAIL: generated export header missing at $EXPORT_DIR" >&2
  echo "      build the library target before this gate" >&2
  exit 1
fi

TU="$ROOT/source/counters/clock_provider.cpp"
if [ ! -f "$TU" ]; then
  echo "FAIL: $TU not found" >&2
  exit 1
fi

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT


# Instruction lines of the one function whose demangled header matches the
# regex. Emits "addr<TAB>mnemonic<TAB>operand1" per instruction.
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

# The read arm: from the rdtsc through the loop back-edge, the first branch
# whose target lies below its own address. A forward branch leaves the arm.
read_arm() {
  printf '%s\n' "$1" | awk -F'\t' '
    function h2d(s,   i, c, d, v) {
      if (s == "") { return -1 }
      v = 0
      for (i = 1; i <= length(s); i++) {
        c = tolower(substr(s, i, 1))
        d = index("0123456789abcdef", c) - 1
        if (d < 0) { return -1 }
        v = v * 16 + d
      }
      return v
    }
    $2 == "rdtsc" { inarm = 1; print; next }
    inarm == 1 {
      print
      if ($2 ~ /^j/) {
        t = h2d($3)
        if (t >= 0 && t < h2d($1)) { exit }
      }
    }
  '
}

count_in() { printf '%s\n' "$1" | grep -cE "$2" || true; }

# Analyze one compiled object. Prints the arm and reports findings.
analyze() {
  local obj=$1 label=$2
  local ins arm n_rdtsc n_other n_call n_fence n_insn

  ins=$(dump_function "$obj" 'read_points.*point_sink')

  if [ -z "$ins" ]; then
    echo "FAIL $label: could not locate read_points in the object" >&2
    return 1
  fi

  n_rdtsc=$(printf '%s\n' "$ins" | awk -F'\t' '$2 == "rdtsc"' | wc -l)
  n_other=$(printf '%s\n' "$ins" \
    | awk -F'\t' '$2 == "rdtscp" || $2 == "rdpid" || $2 == "rdrand"' | wc -l)
  arm=$(read_arm "$ins")

  if [ "$n_rdtsc" -ne 1 ]; then
    echo "FAIL $label: the function holds $n_rdtsc rdtsc instructions, expected 1" >&2
    return 1
  fi
  if [ "$n_other" -ne 0 ]; then
    echo "FAIL $label: found $n_other rdtscp/rdpid/rdrand, and the recorded" >&2
    echo "      P2 choice at clock_provider.cpp:154 names __rdtsc" >&2
    printf '%s\n' "$ins" | awk -F'\t' '$2 ~ /^rd(tscp|pid|rand)/'
    return 1
  fi
  if [ -z "$arm" ]; then
    echo "FAIL $label: the rdtsc arm extracted empty, so the checks below" >&2
    echo "      would pass without inspecting any instruction" >&2
    return 1
  fi

  n_call=$(count_in "$arm" '[[:space:]]call[q]?[[:space:]]')
  n_fence=$(printf '%s\n' "$arm" \
    | awk -F'\t' '$2 ~ /^(lfence|mfence|sfence)$/' | wc -l)
  n_insn=$(printf '%s\n' "$arm" | wc -l)

  echo "  $label: read arm is $n_insn instructions, $n_call calls, $n_fence fences"
  printf '%s\n' "$arm" | sed 's/^/    /'

  if [ "$n_call" -ne 0 ]; then
    echo "FAIL $label: the read arm holds $n_call call instructions:" >&2
    printf '%s\n' "$arm" | grep -E '[[:space:]]call[q]?[[:space:]]' >&2
    return 1
  fi
  # FR-031: the leaf's value is ordered only where one thread takes both
  # window endpoints, and that precondition is on the caller. A fence in
  # the read would cost every timestamp for no order the caller lacks, so
  # the arm must hold none.
  if [ "$n_fence" -ne 0 ]; then
    echo "FAIL $label: the read arm holds $n_fence ordering fences, and" >&2
    echo "      FR-031 gives the leaf a caller-side precondition instead" >&2
    printf '%s\n' "$arm" | awk -F'\t' '$2 ~ /^(lfence|mfence|sfence)$/' >&2
    return 1
  fi
  return 0
}

# Negative probe: an arm of the same shape with a call planted in it. The
# detector above must report the call, or the clean runs mean nothing.
negative_probe() {
  local obj="$WORKDIR/probe/probe.o"
  local ins arm n_call

  mkdir -p "$WORKDIR/probe"
  cat >"$WORKDIR/probe/probe.cpp" <<'PROBE'
#include <x86intrin.h>
extern void opaque(void);
__attribute__((noinline)) auto probe_arm(unsigned long long* out) -> unsigned long long
{
  unsigned long long total = 0;
  for (int i = 0; i < 4; ++i) {
    total += __rdtsc();
    opaque();
  }
  *out = total;
  return total;
}
PROBE

  if ! g++ -O2 -std=c++23 -c "$WORKDIR/probe/probe.cpp" -o "$obj" 2>/dev/null; then
    echo "FAIL: the negative probe did not compile" >&2
    return 1
  fi

  ins=$(dump_function "$obj" 'probe_arm')
  arm=$(read_arm "$ins")

  if [ -z "$arm" ]; then
    echo "FAIL: the negative probe arm extracted empty; the extractor needs" >&2
    echo "      the same shape on a probe as it found on the real arm" >&2
    return 1
  fi

  n_call=$(count_in "$arm" '[[:space:]]call[q]?[[:space:]]')
  if [ "$n_call" -lt 1 ]; then
    echo "FAIL: negative probe planted a call and the detector missed it," >&2
    echo "      so a clean run on the real arm proves nothing" >&2
    return 1
  fi
  echo "  negative probe: detector found $n_call call in a planted arm (live)"
  return 0
}

status=0
found=0

echo "=== negative probe (detector liveness) ==="
if ! negative_probe; then
  status=1
fi

for cxx in g++ clang++; do
  command -v "$cxx" >/dev/null 2>&1 || { echo "skip $cxx (not present)"; continue; }
  for semantic in 0 2; do
    found=1
    obj="$WORKDIR/${cxx}-${semantic}.o"
    echo
    echo "========== $cxx, SG_CONTRACTS_SEMANTIC=$semantic =========="
    if ! "$cxx" -O2 -std=c++23 -DSG_CONTRACTS_SEMANTIC="$semantic" \
      -I "$ROOT/include" -I "$EXPORT_DIR" -c "$TU" -o "$obj" 2>"$WORKDIR/err.txt"; then
      echo "FAIL $cxx semantic=$semantic: the translation unit did not compile" >&2
      sed 's/^/  /' "$WORKDIR/err.txt" >&2
      status=1
      continue
    fi
    if ! analyze "$obj" "$cxx semantic=$semantic"; then
      status=1
    fi
  done
done

if [ "$found" -eq 0 ]; then
  echo "FAIL: neither g++ nor clang++ is present" >&2
  exit 1
fi

echo
if [ "$status" -eq 0 ]; then
  echo "counters_tsc_read_shape: the read arm holds no call in any compiler"
  echo "                        and any contract semantic"
else
  echo "counters_tsc_read_shape: FAILED"
fi
exit "$status"
