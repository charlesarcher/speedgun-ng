#!/usr/bin/env bash
# test/compile-fail/run.sh
# Negative-compile harness for DBC compile-time layer (US1 / T011).
# POSIX-ish bash. Per-TU verdicts. Supports // platforms: and // expect: mappings.
#
# Usage: run.sh [-B <builddir>]   (default: build/dev)
#   -B : build directory; used to locate the generated export header dir
#        containing speedgun-ng_export.hpp (searched under the builddir).
#
# For each sibling *.cpp (not under positive/):
#   - must compile to FAILURE
#   - compiler diagnostic must contain the expect substring (from // expect:)
#   - // platforms: gcc,clang or all (default all); skip if active not listed
# For positive/*.cpp: must compile to exit 0 (clean).
#
# Compile flags (per spec): -std=c++23 + strict warnings + -I <repo>/include -I<exportdir>
#
# Exit non-zero if any verdict is FAIL.

set -u
set -o pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd -P)
cd "$SCRIPT_DIR" || { echo "FAIL harness (cannot cd to script dir)"; exit 1; }

BUILDDIR=build/dev
while getopts ":B:" opt; do
  case $opt in
    B) BUILDDIR=$OPTARG ;;
    \?) echo "Unknown option: -$OPTARG" >&2; exit 2 ;;
  esac
done
shift $((OPTIND-1))

if [ "${BUILDDIR#/}" = "$BUILDDIR" ]; then
  BUILDDIR="$REPO_ROOT/$BUILDDIR"
fi

if [ ! -d "$BUILDDIR" ]; then
  echo "FAIL harness (build dir not found: $BUILDDIR)"
  exit 1
fi

# Locate export include dir by finding the generated header.
# The -I path must be the one that makes #include <speedgun-ng/speedgun-ng_export.hpp> work,
# which is ${build}/export (the parent of the speedgun-ng/ subdir).
found=$(find "$BUILDDIR" -name 'speedgun-ng_export.hpp' 2>/dev/null | head -1 || true)
if [ -z "$found" ]; then
  echo "FAIL harness (could not find speedgun-ng_export.hpp under $BUILDDIR)"
  exit 1
fi
export_incdir=$(dirname "$(dirname "$found")")
if [ ! -f "$export_incdir/speedgun-ng/speedgun-ng_export.hpp" ]; then
  # Fallback: if structure different, use the dir containing the file's parent
  export_incdir=$(dirname "$found")
fi

INC_FLAGS="-I $REPO_ROOT/include -I $export_incdir"
WARN_FLAGS="-std=c++23 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wold-style-cast"
CXX=${CXX:-c++}

# Determine active platform for SKIP logic.
cc_version=$($CXX --version 2>&1 | head -1 || true)
if echo "$cc_version" | grep -qi 'clang'; then
  ACTIVE=clang
else
  ACTIVE=gcc
fi
echo "compile-fail: active platform=$ACTIVE (CXX=$CXX, builddir=$BUILDDIR)"
echo "compile-fail: export_incdir=$export_incdir"

overall=0

# --- Negative TUs (direct siblings, not positive/) ---
for tu in *.cpp; do
  [ -f "$tu" ] || continue
  # Skip if under positive (defensive; glob shouldn't match but be sure)
  case $tu in
    positive/*) continue ;;
  esac

  # Parse mappings (last occurrence wins; tolerate spaces)
  platforms_line=$(grep -i '^//[[:space:]]*platforms:' "$tu" | tail -1 || true)
  expect_line=$(grep -i '^//[[:space:]]*expect:' "$tu" | tail -1 || true)

  platforms=$(printf '%s\n' "$platforms_line" | sed -n 's|.*[Pp]latforms:[[:space:]]*||p' | tr -d ' \t\r' | tr '[:upper:]' '[:lower:]')
  expect=$(printf '%s\n' "$expect_line" | sed -n 's|.*[Ee]xpect:[[:space:]]*||p' | sed 's/[[:space:]]*$//')

  if [ -z "$platforms" ]; then
    platforms=all
  fi
  if [ -z "$expect" ]; then
    echo "FAIL $tu (missing // expect: diagnostic substring mapping)"
    overall=1
    continue
  fi

  # Platform scope check
  if [ "$platforms" != "all" ]; then
    # platforms is comma list e.g. gcc,clang
    if ! echo ",$platforms," | grep -q ",$ACTIVE,"; then
      echo "SKIP $tu (platform $ACTIVE not in scope)"
      continue
    fi
  fi

  # Compile (expect failure). Use -c to stop at compile, no link.
  # Capture combined output.
  set +e
  diag=$($CXX $WARN_FLAGS $INC_FLAGS -c "$tu" -o /tmp/cf_$$.o 2>&1)
  exitcode=$?
  set -e
  rm -f /tmp/cf_$$.o 2>/dev/null || true

  if [ $exitcode -eq 0 ]; then
    echo "FAIL $tu (expected FAILURE, but compiled successfully)"
    overall=1
    continue
  fi

  # Substring match (case-sensitive first, then try insensitive for robustness)
  if printf '%s\n' "$diag" | grep -qF "$expect" || printf '%s\n' "$diag" | grep -qiF "$expect"; then
    echo "PASS $tu (matched '$expect')"
  else
    echo "FAIL $tu (diagnostic did not contain '$expect'; output was:"
    printf '%s\n' "$diag" | sed 's/^/  | /'
    echo ")"
    overall=1
  fi
done

# --- Positive TUs (must succeed) ---
if [ -d positive ]; then
  for tu in positive/*.cpp; do
    [ -f "$tu" ] || continue

    set +e
    diag=$($CXX $WARN_FLAGS $INC_FLAGS -c "$tu" -o /tmp/cf_$$.o 2>&1)
    exitcode=$?
    set -e
    rm -f /tmp/cf_$$.o 2>/dev/null || true

    if [ $exitcode -eq 0 ]; then
      echo "PASS $tu (clean)"
    else
      echo "FAIL $tu (expected SUCCESS, but failed to compile; output was:"
      printf '%s\n' "$diag" | sed 's/^/  | /'
      echo ")"
      overall=1
    fi
  done
fi

if [ $overall -ne 0 ]; then
  echo "compile-fail: one or more verdicts FAILED"
else
  echo "compile-fail: all verdicts PASS"
fi
exit $overall
