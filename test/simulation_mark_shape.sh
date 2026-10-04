#!/usr/bin/env bash
# test/simulation_mark_shape.sh
# Codegen gate for the trace-start marker (specs/011 FR-012, FR-023,
# FR-024, FR-025).
#
# Usage: simulation_mark_shape.sh <repo-root> <binary-dir>
#
# The marker at source/simulation/marker.cpp has no observable effect at
# run time, so nothing in the ordinary suite can tell a correct marker
# from a broken one. A tracer pattern-matches eight contiguous bytes at an
# instruction boundary, and a marker that stops matching starts every
# trace in the wrong place with no failing test anywhere. This gate reads
# the compiled bytes instead: it compiles the translation unit at -O2 for
# each available compiler crossed with each contract semantics, and
# asserts the window appears exactly once.
#
# The claim is about optimized code, so the gate compiles the unit itself
# rather than reading the build tree's object. Both contract semantics are
# checked: the release-consumer configuration, where SG_CONTRACTS_SEMANTIC
# is 0, and the default, where it is 2 and the contract checks stay
# inline. An unoptimized object would not answer the question, and a build
# type this script does not control would.
#
# The unit holds exactly one function, so the window's count over the
# object's whole text is the in-function count FR-023 asks for.
#
# A negative probe plants a wrong window in a scratch copy of the same
# unit and fails when the detector misses it, so a clean run cannot come
# from a detector that inspects nothing.
#
# Exit 0 clean, 1 on a violation, 0 with a skip line where no compiler on
# PATH targets x86, because FR-025 is decided by the compiler and a
# marker instruction exists only there.

set -uo pipefail

ROOT=${1:-.}
BINARY_DIR=${2:-}

if [ -z "$BINARY_DIR" ]; then
  echo "FAIL: binary dir not given; pass the CMake binary dir as \$2" >&2
  exit 1
fi

# The eight bytes an attached tracer matches: a 32-bit immediate move into
# the 32-bit view of EBX, the tag little-endian in bytes 1 through 4, then
# the FS segment-override prefix, the address-size-override prefix, and the
# one-byte no-operation (specs/011 contracts/simulation-start.md).
WINDOW="bbcefa0000646790"

# The wrong window the negative probe plants: the last byte is a different
# one-byte no-operation form, so the eight bytes no longer match while the
# tag and the register stay as they are.
WRONG_WINDOW="bbcefa0000646791"

if [ "$(uname -s)" != "Linux" ]; then
  echo "skip: $(uname -s) is not Linux; the gate is registered in the Linux-only region"
  exit 0
fi

EXPORT_DIR="$BINARY_DIR/export"
if [ ! -f "$EXPORT_DIR/speedgun-ng/speedgun-ng_export.hpp" ]; then
  echo "FAIL: generated export header missing at $EXPORT_DIR" >&2
  echo "      build the library target before this gate" >&2
  exit 1
fi

TU="$ROOT/source/simulation/marker.cpp"
if [ ! -f "$TU" ]; then
  echo "FAIL: $TU not found" >&2
  exit 1
fi

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

# Every byte of the object's disassembled text, in address order, as one
# hex string. objdump splits a long instruction across its byte column, so
# the window spans more than one printed line and a per-line match would
# find nothing.
byte_stream() {
  objdump -d "$1" 2>/dev/null \
    | awk -F'\t' '$1 ~ /^[[:space:]]*[0-9a-f]+:$/ { printf "%s", $2 }' \
    | tr -d ' \n'
}

count_window() {
  local stream
  stream=$(byte_stream "$1")
  if [ -z "$stream" ]; then
    return 1
  fi
  # awk, because a run in which the window is absent is exactly what the
  # negative probe produces, and grep's exit status there would reach the
  # caller through `pipefail` as if the detector had failed.
  printf '%s' "$stream" | awk -v window="$WINDOW" '{ print gsub(window, "@") }'
}

compile_unit() {
  local cxx=$1 semantic=$2 source=$3 object=$4
  "$cxx" -O2 -std=c++23 -DSG_CONTRACTS_SEMANTIC="$semantic" \
    -I "$ROOT/include" -I "$EXPORT_DIR" -c "$source" -o "$object"
}

# The gate itself: the window appears exactly once in the compiled unit.
analyze() {
  local obj=$1 label=$2
  local count

  count=$(count_window "$obj") || {
    echo "FAIL $label: no bytes were disassembled, so the count below would" >&2
    echo "      pass without inspecting anything" >&2
    return 1
  }

  if [ "$count" -ne 1 ]; then
    echo "FAIL $label: the marker window appears $count times, expected" >&2
    echo "      exactly 1; an attached tracer matches the eight bytes as one" >&2
    echo "      window and every other count starts a trace in the wrong place" >&2
    return 1
  fi
  echo "  $label: marker window $WINDOW appears exactly once"
  return 0
}

# Negative probe: the same unit with a deliberately wrong window. The
# detector above must fail to find it, or a clean run on the real unit
# proves nothing.
negative_probe() {
  local cxx=$1 semantic=$2
  local dir="$WORKDIR/probe/$cxx-$semantic"
  local object="$dir/marker.o"
  local count

  mkdir -p "$dir"
  if ! sed 's/\.byte 0x64, 0x67, 0x90/.byte 0x64, 0x67, 0x91/' "$TU" \
    >"$dir/marker.cpp"; then
    echo "FAIL: the negative probe could not plant a wrong window" >&2
    return 1
  fi

  if ! compile_unit "$cxx" "$semantic" "$dir/marker.cpp" "$object" \
    2>"$dir/err.txt"; then
    echo "FAIL: the negative probe did not compile" >&2
    sed 's/^/  /' "$dir/err.txt" >&2
    return 1
  fi

  count=$(count_window "$object") || return 1
  if [ "$count" -ne 0 ]; then
    echo "FAIL: the probe planted the wrong window $WRONG_WINDOW and the" >&2
    echo "      detector still found $count occurrences of $WINDOW, so a" >&2
    echo "      clean run on the real unit cannot come from this detector" >&2
    return 1
  fi
  echo "  $cxx semantic=$semantic: wrong window planted, detector found 0"
  return 0
}

# FR-025 reads of the compiler's target, so the skip is decided by what the
# compiler emits. The host's own architecture is a separate question.
targets_x86() {
  local triple
  triple=$("$1" -dumpmachine 2>/dev/null) || return 1
  case "$triple" in
    *x86_64* | *i?86* | *amd64*) return 0 ;;
  esac
  return 1
}

status=0
found=0

echo "=== negative probe (detector liveness) ==="
for cxx in g++ clang++; do
  command -v "$cxx" >/dev/null 2>&1 || continue
  targets_x86 "$cxx" || continue
  for semantic in 0 2; do
    found=1
    if ! negative_probe "$cxx" "$semantic"; then
      status=1
    fi
  done
done

if [ "$found" -eq 0 ]; then
  echo "skip: no compiler on PATH targets an x86 instruction set, so no" >&2
  echo "      build of the marker unit emits a marker window to count" >&2
  exit 0
fi

for cxx in g++ clang++; do
  command -v "$cxx" >/dev/null 2>&1 || { echo "skip $cxx (not present)"; continue; }
  if ! targets_x86 "$cxx"; then
    echo "skip $cxx (target $($cxx -dumpmachine 2>/dev/null) is not x86)"
    continue
  fi
  for semantic in 0 2; do
    found=1
    object="$WORKDIR/$cxx-$semantic.o"
    echo
    echo "========== $cxx, SG_CONTRACTS_SEMANTIC=$semantic =========="
    if ! compile_unit "$cxx" "$semantic" "$TU" "$object" 2>"$WORKDIR/err.txt"; then
      echo "FAIL $cxx semantic=$semantic: the translation unit did not compile" >&2
      sed 's/^/  /' "$WORKDIR/err.txt" >&2
      status=1
      continue
    fi
    if ! analyze "$object" "$cxx semantic=$semantic"; then
      status=1
    fi
  done
done

echo
if [ "$status" -eq 0 ]; then
  echo "simulation_mark_shape: the marker window appears exactly once in"
  echo "                      every compiler and every contract semantic"
  echo "                      exercised above, and the planted wrong window"
  echo "                      was detected"
else
  echo "simulation_mark_shape: FAILED"
fi
exit "$status"