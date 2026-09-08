#!/usr/bin/env bash
# tools/dbc/asm_smoke.sh
# T017 / FR-039: Linux assembly smoke test for the DBC satisfied hot path.
#
# Usage: asm_smoke.sh [include-dir]
#   include-dir defaults to include/ (repo-relative, or $1 from ctest).
#
# For each of g++ and clang++ that is present:
#   compile a hot function containing a satisfied SG_REQUIRE with a
#   *runtime* (non-constant) predicate at -O2 -std=c++20, objdump it,
#   and assert the satisfied path is predicate + one branch, with no
#   call to sg::dbc and no EH personality in that function.
#
# A negative probe TU (unguarded throw) must exhibit EH so the detector
# is discriminating. Fresh temp dir per run.
#
# If the header-inline primary is rejected (calls / personality on the
# hot path), report the fallback decision loudly and exit non-zero.
# This script does not implement source/dbc/dbc.cpp.

set -euo pipefail

if [ -n "${1:-}" ]; then
  INCLUDE_DIR=$1
elif [ -d include ]; then
  INCLUDE_DIR=include
else
  INCLUDE_DIR="$(cd "$(dirname "$0")/../.." && pwd)/include"
fi

if [ ! -d "$INCLUDE_DIR" ]; then
  echo "FAIL: include dir not found: $INCLUDE_DIR" >&2
  exit 1
fi
if [ ! -f "$INCLUDE_DIR/speedgun-ng/dbc.hpp" ]; then
  echo "FAIL: $INCLUDE_DIR/speedgun-ng/dbc.hpp not found" >&2
  exit 1
fi

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

count_lines() {
  if [ -z "${1:-}" ]; then
    printf '0'
    return
  fi
  printf '%s\n' "$1" | grep -c . || true
}

# Exact objdump symbol `<name>:` — not `<name [clone .cold]>:`.
extract_objdump_symbol() {
  local dump=$1
  local name=$2
  printf '%s\n' "$dump" | awk -v name="$name" '
    BEGIN { start = "<" name ">:" }
    index($0, start) { p = 1 }
    p {
      if ($0 ~ /^[[:space:]]*[0-9a-f]+ </ && index($0, start) == 0) exit
      if ($0 ~ /^Disassembly of section/) exit
      print
    }
  '
}

# Instructions from function entry through the first ret (satisfied path).
satisfied_path() {
  printf '%s\n' "$1" | awk '
    { print }
    /[[:space:]]retq?[[:space:]]*$/ { exit }
  '
}

# Assembler CFI block of _Z3hoti / _Z10unguardedi through first .cfi_endproc.
extract_s_func() {
  local asm=$1
  local label=$2
  printf '%s\n' "$asm" | awk -v lab="$label" '
    $0 ~ "^" lab ":" { p = 1 }
    p { print }
    p && /\.cfi_endproc/ { exit }
  '
}

reject_primary() {
  local cxx=$1
  local why=$2
  echo
  echo "============================================================"
  echo "REJECT PRIMARY: $why ($cxx)"
  echo "Fallback decision: engage out-of-line source/dbc/dbc.cpp"
  echo "This script does NOT implement the fallback."
  echo "============================================================"
}

EH_RX='personality|cfi_lsda|gcc_except|_Unwind|__cxa_throw|__cxa_allocate_exception|__gxx_personality'
CALL_RX='[[:space:]]callq?[[:space:]]'
DBC_RX='sg::dbc|_ZN2sg3dbc'
PRED_RX='[[:space:]](test|cmp)[a-z]*[[:space:]]'
JCC_RX='[[:space:]]j[a-z]+[[:space:]]'

analyze_compiler() {
  local cxx=$1
  local slot=$WORKDIR/$cxx
  mkdir -p "$slot"

  local ver
  ver=$("$cxx" --version)
  ver=${ver%%$'\n'*}
  echo
  echo "========== $cxx =========="
  echo "compiler: $ver"
  echo "include:  $INCLUDE_DIR"

  cat >"$slot/hot.cpp" <<'EOF'
#include <speedgun-ng/dbc.hpp>
int hot(int x)
{
  SG_REQUIRE(x > 0, "hot: x > 0");
  return x * 2;
}
EOF

  cat >"$slot/throw.cpp" <<'EOF'
int unguarded(int x)
{
  throw x;
}
EOF

  "$cxx" -O2 -std=c++20 -I "$INCLUDE_DIR" -c -o "$slot/hot.o" "$slot/hot.cpp"
  "$cxx" -O2 -std=c++20 -I "$INCLUDE_DIR" -S -o "$slot/hot.s" "$slot/hot.cpp"
  "$cxx" -O2 -std=c++20 -c -o "$slot/throw.o" "$slot/throw.cpp"
  "$cxx" -O2 -std=c++20 -S -o "$slot/throw.s" "$slot/throw.cpp"

  local dump asm
  dump=$(objdump -d -C -r "$slot/hot.o")
  asm=$(cat "$slot/hot.s")

  local func sat cfi
  func=$(extract_objdump_symbol "$dump" 'hot(int)')
  sat=$(satisfied_path "$func")
  cfi=$(extract_s_func "$asm" '_Z3hoti')

  echo
  echo "--- function hot(int) ---"
  printf '%s\n' "$func"
  echo
  echo "--- satisfied path (entry through first ret) ---"
  printf '%s\n' "$sat"

  local call_hits dbc_hits pred_hits jcc_hits eh_hits
  call_hits=$(printf '%s\n' "$sat" | grep -E "$CALL_RX" || true)
  dbc_hits=$(printf '%s\n' "$sat" | grep -E "$DBC_RX" || true)
  pred_hits=$(printf '%s\n' "$sat" | grep -E "$PRED_RX" || true)
  jcc_hits=$(printf '%s\n' "$sat" | grep -E "$JCC_RX" | grep -vE '[[:space:]]jmp[[:space:]]' || true)
  eh_hits=$(printf '%s\n' "$cfi" | grep -E "$EH_RX" || true)

  echo
  echo "--- grep: call (satisfied path) ---"
  if [ -n "$call_hits" ]; then printf '%s\n' "$call_hits"; else echo "(none)"; fi
  echo "--- grep: sg::dbc (satisfied path) ---"
  if [ -n "$dbc_hits" ]; then printf '%s\n' "$dbc_hits"; else echo "(none)"; fi
  echo "--- grep: predicate test/cmp (satisfied path) ---"
  if [ -n "$pred_hits" ]; then printf '%s\n' "$pred_hits"; else echo "(none)"; fi
  echo "--- grep: conditional branch (satisfied path) ---"
  if [ -n "$jcc_hits" ]; then printf '%s\n' "$jcc_hits"; else echo "(none)"; fi
  echo "--- grep: personality/EH in hot() CFI ---"
  if [ -n "$eh_hits" ]; then printf '%s\n' "$eh_hits"; else echo "(none)"; fi

  local n_pred n_jcc n_call
  n_pred=$(count_lines "$pred_hits")
  n_jcc=$(count_lines "$jcc_hits")
  n_call=$(count_lines "$call_hits")

  echo
  echo "counts: predicate=$n_pred branch=$n_jcc call=$n_call"

  if [ -z "$func" ] || [ -z "$sat" ]; then
    echo "FAIL $cxx: could not extract hot(int) from objdump"
    return 1
  fi

  local rc=0
  if [ "$n_call" -ne 0 ] || [ -n "$dbc_hits" ]; then
    reject_primary "$cxx" "sg::dbc call on satisfied hot path"
    rc=1
  fi
  if [ -n "$eh_hits" ]; then
    reject_primary "$cxx" "EH personality/landing-pad in hot()"
    rc=1
  fi
  if [ "$n_pred" -lt 1 ]; then
    echo "FAIL $cxx: satisfied path has no predicate (test/cmp)"
    rc=1
  fi
  if [ "$n_jcc" -ne 1 ]; then
    echo "FAIL $cxx: satisfied path must have exactly one branch (got $n_jcc)"
    rc=1
  fi

  # Negative probe: unguarded throw must exhibit EH (detector self-check).
  local tdump tasm tfunc teh
  tdump=$(objdump -d -C -r "$slot/throw.o")
  tasm=$(cat "$slot/throw.s")
  tfunc=$(extract_objdump_symbol "$tdump" 'unguarded(int)')
  teh=$(printf '%s\n' "$tasm" "$tdump" | grep -E "$EH_RX" || true)

  echo
  echo "--- negative probe: unguarded(int) ---"
  printf '%s\n' "$tfunc"
  echo "--- grep: EH in throwing TU ---"
  if [ -n "$teh" ]; then printf '%s\n' "$teh"; else echo "(none)"; fi

  if [ -z "$teh" ]; then
    echo "FAIL $cxx: negative probe did not find EH (detector is not discriminating)"
    rc=1
  else
    echo "negative probe: EH found (detector is live)"
  fi

  if [ "$rc" -eq 0 ]; then
    echo "PASS $cxx: predicate + one branch; no sg::dbc call; no personality in hot()"
  fi
  return "$rc"
}

found=0
status=0
for cxx in g++ clang++; do
  if command -v "$cxx" >/dev/null 2>&1; then
    found=1
    if ! analyze_compiler "$cxx"; then
      status=1
    fi
  else
    echo "skip $cxx (not present)"
  fi
done

if [ "$found" -eq 0 ]; then
  echo "FAIL: neither g++ nor clang++ is present"
  exit 1
fi

echo
if [ "$status" -eq 0 ]; then
  echo "asm_smoke: all present compilers PASS"
else
  echo "asm_smoke: one or more compilers FAILED"
fi
exit "$status"
