#!/usr/bin/env bash
# tools/dbc/dependency_scan.sh
# T031 / FR-033 / SC-009: zero external runtime dependencies for the
# speedgun-ng_speedgun-ng library target.
#
# Scans CMakeLists.txt + cmake/ (per plan) for:
#   find_package, FetchContent, target_link_libraries touching the library.
# Prints a classification table:
#   library-runtime : would ship to consumers / affect lib (FAIL)
#   dev-tooling     : docs, gates, CI/dev only (documented exceptions, OK)
#
# Documented exceptions (per plan):
#   - m.css FetchContent in cmake/docs.cmake (developer-mode docs tooling)
#   - Python3 / doxygen (gate/CI/dev tooling, not library link deps)
#
# Usage: bash tools/dbc/dependency_scan.sh [repo-root]
#   repo-root defaults to two levels up or $1 (for ctest: ${CMAKE_SOURCE_DIR})
#
# Exit 0 only when library target has zero runtime external deps.
# Exit 1 on any library-runtime classification.

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
if [ -n "${1:-}" ]; then
  REPO_ROOT=$(cd "$1" && pwd -P)
else
  REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd -P)
fi

CMAKELISTS="$REPO_ROOT/CMakeLists.txt"
CMAKE_DIR="$REPO_ROOT/cmake"

echo "DBC dependency scan (FR-033/SC-009)"
echo "Scanning: CMakeLists.txt + cmake/"
echo

# Collect candidate lines (portable, no process sub if possible)
candidates=()

# find_package / FetchContent anywhere in the scan set
if [ -f "$CMAKELISTS" ]; then
  while IFS= read -r l || [ -n "$l" ]; do
    candidates+=("CMakeLists.txt:$l")
  done < <(grep -n -E 'find_package|FetchContent' "$CMAKELISTS" || true)
fi
if [ -d "$CMAKE_DIR" ]; then
  for f in "$CMAKE_DIR"/*.cmake; do
    [ -f "$f" ] || continue
    bn=$(basename "$f")
    while IFS= read -r l || [ -n "$l" ]; do
      candidates+=("$bn:$l")
    done < <(grep -n -E 'find_package|FetchContent' "$f" || true)
  done
fi

# target_link_libraries that directly name the library target (the only way
# an external dep would attach at library definition time)
if [ -f "$CMAKELISTS" ]; then
  while IFS= read -r l || [ -n "$l" ]; do
    candidates+=("CMakeLists.txt:$l")
  done < <(grep -n -E 'target_link_libraries.*speedgun-ng_speedgun-ng' "$CMAKELISTS" || true)
fi
if [ -d "$CMAKE_DIR" ]; then
  for f in "$CMAKE_DIR"/*.cmake; do
    [ -f "$f" ] || continue
    bn=$(basename "$f")
    while IFS= read -r l || [ -n "$l" ]; do
      candidates+=("$bn:$l")
    done < <(grep -n -E 'target_link_libraries.*speedgun-ng_speedgun-ng' "$f" || true)
  done
fi

printf "%-35s | %-16s | %s\n" "Finding" "Classification" "Note"
printf '%.0s-' {1..90}; echo

lib_runtime=0
dev_tooling=0
printed=0

for entry in "${candidates[@]}"; do
  file=${entry%%:*}
  rest=${entry#*:}
  lno=${rest%%:*}
  text=${rest#*:}
  loc="$file:$lno"
  class="dev-tooling"
  note=""

  if echo "$text" | grep -q 'speedgun-ng_speedgun-ng'; then
    class="library-runtime"
    note="external target_link_libraries on library target"
    lib_runtime=$((lib_runtime + 1))
  elif echo "$file" | grep -q 'docs.cmake'; then
    if echo "$text" | grep -qi 'mcss\|FetchContent'; then
      note="m.css FetchContent (developer-mode docs tooling - out of scope per plan)"
    elif echo "$text" | grep -qi 'Python3'; then
      note="Python3 (docs tooling)"
    fi
  elif echo "$text" | grep -qi 'Python3'; then
    note="Python3 (dbc-gate / CI tooling)"
  elif echo "$file" | grep -q 'dbc-gate.cmake'; then
    note="gate tooling (not a library link dep)"
  fi

  if [ "$class" = "dev-tooling" ]; then
    dev_tooling=$((dev_tooling + 1))
  fi

  printf "%-35s | %-16s | %s\n" "$loc" "$class" "$note"
  printed=1
done

if [ "$printed" -eq 0 ]; then
  echo "(no candidate find_package/FetchContent/target_link lines in scan scope)"
fi

echo
echo "Summary: $lib_runtime library-runtime, $dev_tooling dev-tooling"

if [ "$lib_runtime" -eq 0 ]; then
  echo "Library target speedgun-ng_speedgun-ng has ZERO external runtime dependencies."
  echo "PASS (SC-009)"
  exit 0
else
  echo "FAIL: $lib_runtime library-runtime finding(s) would introduce external runtime deps."
  exit 1
fi
