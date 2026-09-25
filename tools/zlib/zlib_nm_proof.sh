#!/usr/bin/env bash
# tools/zlib/zlib_nm_proof.sh
# Feature 005-vendor-hdrhistogram | FR-021 / SC-010 | privacy contract A6z.
#
# Proves, from the static archive alone, that the vendored zlib objects
# are merged INTO libspeedgun-ng.a and that the wrapper gate object
# (source/zlib/zlib_gate.cpp) links against them:
#
#   A. Vendored members present: > 0 defined symbols whose name starts
#      with z_ (the Z_PREFIX rename, research R-006; nm T/t), from
#      members whose name does not contain zlib_gate. The gate member
#      stays out of the vendored count, the same header-instantiation
#      caution the simdjson proof documents.
#   B. The gate contributes the reference: some member whose name
#      contains zlib_gate carries an UNDEFINED z_zlibVersion (nm U).
#      The zlib build turns on Z_PREFIX, so the C++ gate TU that takes
#      the address of &zlibVersion references the renamed symbol
#      z_zlibVersion.
#   C. Resolution inside the archive: a DEFINED z_zlibVersion (nm T/t)
#      lives in a different member than the U reference, i.e. the
#      merged vendored member satisfies the wrapper's reference.
#
# Semantics: merged vendored members satisfy the wrapper's reference
# (contract A6z boundary: member list is a build input, legal).
# RED on a thin archive (no vendored members); GREEN once the ADDLIB
# merge lands and z_-prefixed zlib members appear in the same archive.
#
# Usage: bash tools/zlib/zlib_nm_proof.sh <repo-root> [<binary-dir>]
#   <binary-dir> given -> <binary-dir>/libspeedgun-ng.a
#   else <repo-root>/build/dev/libspeedgun-ng.a
#   else first match <repo-root>/build/*/libspeedgun-ng.a
# Exit 0 only when A, B and C all hold. Exit 1 otherwise, naming the archive.
#
# Parsing: GNU nm (ubuntu CI) in bsd output format; per-member headers
# ("member.o:" or "archive.a[member.o]:") attribute symbols to members.

set -euo pipefail

if [ -z "${1:-}" ]; then
  echo "usage: bash tools/zlib/zlib_nm_proof.sh <repo-root> [<binary-dir>]" >&2
  exit 1
fi
REPO_ROOT=$(cd "$1" && pwd -P)

ARCHIVE=""
LOOKED=""
if [ -n "${2:-}" ]; then
  LOOKED="$2/libspeedgun-ng.a"
  [ -f "$LOOKED" ] && ARCHIVE="$LOOKED"
else
  LOOKED="$REPO_ROOT/build/dev/libspeedgun-ng.a"
  [ -f "$LOOKED" ] && ARCHIVE="$LOOKED"
  if [ -z "$ARCHIVE" ]; then
    for cand in "$REPO_ROOT"/build/*/libspeedgun-ng.a; do
      LOOKED="$LOOKED, $cand"
      if [ -z "$ARCHIVE" ] && [ -f "$cand" ]; then
        ARCHIVE="$cand"
      fi
    done
  fi
fi

if [ -z "$ARCHIVE" ]; then
  echo "FAIL: no libspeedgun-ng.a found. Looked for: $LOOKED" >&2
  exit 1
fi

if ! ar t "$ARCHIVE" >/dev/null 2>&1; then
  echo "FAIL: $ARCHIVE is not an ar archive." >&2
  exit 1
fi

echo "zlib nm proof (FR-021/SC-010, privacy contract A6z)"
echo "Archive: $ARCHIVE"
echo

# One nm pass; awk attributes symbols to members via per-member headers.
# The anchored prefix z_ matches the Z_PREFIX-renamed C symbols the
# vendored tree defines (z_deflate, z_zlibVersion...). Hidden-visibility
# members read as T or t. The gate symbol match is exact: the wrapper
# references the address of the renamed entry point.
facts=$(nm --format=bsd "$ARCHIVE" 2>/dev/null | awk '
  /^[^[:space:]].*:$/ {
    m = $0; sub(/:$/, "", m); sub(/^.*\[/, "", m); member = m
    next
  }
  {
    if ($0 ~ /^[[:space:]]/) { type = $1; name = $2 }
    else if (NF >= 3)        { type = $2; name = $3 }
    else next
    if (name ~ /^z_/ && (type == "T" || type == "t") && member !~ /zlib_gate/) {
      vend_count++
      if (!(member in seen) && anum < 5) { seen[member] = 1; anum++; amembers = amembers " " member }
    }
    if (name == "z_zlibVersion") {
      if (type == "U") uall = uall " " member
      if (type == "T" || type == "t") tall = tall " " member
    }
  }
  END {
    print "ACO=" vend_count + 0
    print "AMEM=" amembers
    print "UALL=" uall
    print "TALL=" tall
  }
')

vend_count=$(echo "$facts" | sed -n 's/^ACO=//p')
amembers=$(echo "$facts" | sed -n 's/^AMEM=//p')
uall=$(echo "$facts" | sed -n 's/^UALL=//p')
tall=$(echo "$facts" | sed -n 's/^TALL=//p')

rc=0

# A. Vendored members present.
if [ "$vend_count" -gt 0 ]; then
  echo "PASS A: $vend_count defined z_ symbols; example members:$amembers"
else
  echo "FAIL A: 0 defined z_ vendored symbols in $ARCHIVE"
  echo "        -> thin archive: no vendored zlib members merged in."
  rc=1
fi

# B. Gate object contributes the undefined reference.
gate_u=""
for m in $uall; do
  case "$m" in
    *zlib_gate*) [ -z "$gate_u" ] && gate_u="$m" ;;
  esac
done
if [ -z "$uall" ]; then
  echo "FAIL B: no member carries an undefined z_zlibVersion (U) reference."
  rc=1
elif [ -n "$gate_u" ]; then
  echo "PASS B: gate member '$gate_u' references UNDEFINED z_zlibVersion (U)."
else
  echo "FAIL B: U z_zlibVersion found in[$uall], but no member named *zlib_gate*."
  rc=1
fi

# C. Resolution: defined z_zlibVersion in a member other than the U ref.
resolved=""
for m in $tall; do
  if [ -z "$gate_u" ] || [ "$m" != "$gate_u" ]; then resolved="$m"; fi
done
if [ -n "$gate_u" ] && [ -n "$resolved" ]; then
  echo "PASS C: defined z_zlibVersion (T/t) in '$resolved', resolves the U ref in '$gate_u'."
elif [ -n "$gate_u" ]; then
  echo "FAIL C: U ref in '$gate_u' but no DEFINED z_zlibVersion (T/t) in another member of $ARCHIVE."
  rc=1
else
  echo "FAIL C: no gate U reference (see B), so resolution cannot be proven in $ARCHIVE."
  rc=1
fi

echo
if [ "$rc" -eq 0 ]; then
  echo "PASS: $ARCHIVE is self-contained: merged vendored members satisfy the gate reference (A6z)."
else
  echo "FAIL: $ARCHIVE does not satisfy A6z."
fi
exit "$rc"
