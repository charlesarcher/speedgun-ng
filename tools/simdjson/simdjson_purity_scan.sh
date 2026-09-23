#!/usr/bin/env bash
# tools/simdjson/simdjson_purity_scan.sh
# Feature 004-vendor-simdjson | FR-004 / FR-013 / SC-008 | privacy
# contract A7/A8. Modeled on tools/hwloc/hwloc_purity_scan.sh.
#
# Contract audits (specs/004-vendor-simdjson/contracts/privacy-contract.md):
#   A7: zero simdjson discovery calls in project build files. Pattern
#       find_package( *simdjson / pkg_check_modules( *simdjson,
#       case-insensitive on simdjson, across all CMakeLists.txt and
#       *.cmake under the repo root, excluding external/, build/,
#       prefix, node_modules/, .git/, .specify/, .omo/.
#   A8: zero simdjson references under include/ (public headers stay
#       clean; the wrapper under source/ is the sole includer).
#
# Every hit prints as `path:line: text`. Exit 0 (PASS) only when the
# finding count is zero; exit 1 (FAIL) naming every hit.
#
# Usage: bash tools/simdjson/simdjson_purity_scan.sh [repo-root]
#   repo-root defaults to two levels up or $1 (for ctest: ${CMAKE_SOURCE_DIR})
#
# The scan set is CMakeLists.txt/*.cmake/include/ only; this .sh file
# is never scanned, so its own pattern strings cannot self-match.

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
if [ -n "${1:-}" ]; then
  REPO_ROOT=$(cd "$1" && pwd -P)
else
  REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd -P)
fi

# grep -E patterns, used with -ni (case-insensitive simdjson, line numbers).
DISCOVERY_PATTERN='find_package\( *simdjson|pkg_check_modules\( *simdjson'
INCLUDE_PATTERN='simdjson'

# Findings accumulator: each entry is `path:line: text`.
FINDINGS=()

# Record every grep -niE $pattern hit in file $2 as `$1:line: text`.
record_hits() {
  local rel=$1 file=$2 pattern=$3
  local l lno text
  while IFS= read -r l || [ -n "$l" ]; do
    lno=${l%%:*}
    text=${l#*:}
    FINDINGS+=("${rel}:${lno}: ${text}")
  done < <(grep -niE "$pattern" "$file" || true)
}

# A7: zero simdjson discovery calls in project build files (FR-004, SC-008).
check_discovery_calls() {
  local f rel
  while IFS= read -r f; do
    [ -f "$f" ] || continue
    rel=${f#"$REPO_ROOT"/}
    record_hits "$rel" "$f" "$DISCOVERY_PATTERN"
  done < <(find "$REPO_ROOT" \
      \( -name CMakeLists.txt -o -name '*.cmake' \) \
      -not -path '*/external/*' \
      -not -path '*/build/*' \
      -not -path '*/prefix*' \
      -not -path '*/node_modules/*' \
      -not -path '*/.git/*' \
      -not -path '*/.specify/*' \
      -not -path '*/.omo/*' \
      -print)
}

# A8: zero simdjson references under include/ (FR-013).
check_include_references() {
  local dir="$REPO_ROOT/include" f rel
  [ -d "$dir" ] || return 0
  while IFS= read -r f; do
    rel=${f#"$REPO_ROOT"/}
    record_hits "$rel" "$f" "$INCLUDE_PATTERN"
  done < <(find "$dir" -type f -print)
}

echo "simdjson purity scan (FR-004 / FR-013 / SC-008)"
echo "Repo root: $REPO_ROOT"
echo "Checks: A7 build-file discovery calls, A8 include/ references"
echo

check_discovery_calls
check_include_references

if [ "${#FINDINGS[@]}" -gt 0 ]; then
  for f in "${FINDINGS[@]}"; do
    echo "$f"
  done
fi

echo
if [ "${#FINDINGS[@]}" -eq 0 ]; then
  echo "Summary: 0 findings."
  echo "PASS: zero discovery calls in build files, zero simdjson in include/ (A7/A8)."
  exit 0
else
  echo "Summary: ${#FINDINGS[@]} finding(s)."
  echo "FAIL: ${#FINDINGS[@]} simdjson purity violation(s) (A7/A8)."
  exit 1
fi
