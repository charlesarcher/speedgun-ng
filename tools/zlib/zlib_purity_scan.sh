#!/usr/bin/env bash
# tools/zlib/zlib_purity_scan.sh
# Feature 005-vendor-hdrhistogram | FR-014 / FR-019 / SC-008 | privacy
# contract A7z/A8z. Modeled on tools/simdjson/simdjson_purity_scan.sh.
#
# Contract audits (specs/005-vendor-hdrhistogram/contracts/privacy-contract.md):
#   A8z: zero zlib/libz references under include/, case-insensitive
#       (public headers stay clean; the wrapper under source/ is the sole
#       includer).
#   A7z: zero discovery calls naming zlib in project build files. Pattern
#        find_package( *libz|zlib / pkg_check_modules( *libz|zlib,
#        case-insensitive, across all CMakeLists.txt and *.cmake under the
#        repo root, excluding external/, build/, prefix, node_modules/,
#        .git/, .specify/, .omo/.
#
# A find_package(ZLIB) call inside the vendored external/hdrhistogram_c
# subtree is permitted and never scanned: external/ is excluded from the
# scan set. The host-resolution proof lives with the CI configure-record
# check (research R-013, SC-010); this scan does not duplicate it.
#
# Every hit prints as `path:line: text`. Exit 0 (PASS) only when the
# finding count is zero; exit 1 (FAIL) naming every hit.
#
# Usage: bash tools/zlib/zlib_purity_scan.sh [repo-root]
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

# grep -E patterns, used with -ni (case-insensitive zlib/libz, line numbers).
INCLUDE_PATTERN='zlib|libz'
DISCOVERY_PATTERN='find_package\( *(libz|zlib)|pkg_check_modules\( *(libz|zlib)'

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

# A7z: zero zlib discovery calls in project build files (FR-019, SC-008).
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

# A8z: zero zlib/libz references under include/ (FR-014).
check_include_references() {
  local dir="$REPO_ROOT/include" f rel
  [ -d "$dir" ] || return 0
  while IFS= read -r f; do
    rel=${f#"$REPO_ROOT"/}
    record_hits "$rel" "$f" "$INCLUDE_PATTERN"
  done < <(find "$dir" -type f -print)
}

echo "zlib purity scan (FR-014 / FR-019 / SC-008)"
echo "Repo root: $REPO_ROOT"
echo "Checks: A7z build-file discovery calls, A8z include/ references"
echo

check_include_references
check_discovery_calls

if [ "${#FINDINGS[@]}" -gt 0 ]; then
  for f in "${FINDINGS[@]}"; do
    echo "$f"
  done
fi

echo
if [ "${#FINDINGS[@]}" -eq 0 ]; then
  echo "Summary: 0 findings."
  echo "PASS: zero zlib/libz in include/, zero discovery calls in build files (A7z/A8z)."
  exit 0
else
  echo "Summary: ${#FINDINGS[@]} finding(s)."
  echo "FAIL: ${#FINDINGS[@]} zlib purity violation(s) (A7z/A8z)."
  exit 1
fi
