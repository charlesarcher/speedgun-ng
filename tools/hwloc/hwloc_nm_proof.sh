#!/usr/bin/env bash
# tools/hwloc/hwloc_nm_proof.sh
# Feature 003-vendor-hwloc | FR-007 / SC-011 | privacy contract A6.
#
# Proves, from the static archive alone, that the vendored hwloc objects
# are merged INTO libspeedgun-ng.a and that the wrapper gate object
# (source/hwloc/hwloc_gate.cpp) links against them:
#
#   A. Vendored members present: > 0 defined sg_hwloc_* symbols (nm T/t).
#      The sg_ prefix comes from hwloc configure
#      --with-hwloc-symbol-prefix=sg_ (research R-003), so a case-insensitive
#      'hwloc' audit still matches.
#   B. The gate contributes the reference: some member whose name contains
#      hwloc_gate carries an UNDEFINED sg_hwloc_get_api_version (nm U).
#   C. Resolution inside the archive: a DEFINED sg_hwloc_get_api_version
#      (nm T) lives in a different member than the U reference, i.e. the
#      merged vendored member satisfies the wrapper's prefixed reference.
#
# Semantics: merged vendored members satisfy the wrapper's prefixed
# reference (contract A6 boundary: member list is a build input, legal).
# RED on a thin archive (no vendored members); GREEN once the ADDLIB
# merge lands and hwloc members appear in the same archive.
#
# Usage: bash tools/hwloc/hwloc_nm_proof.sh <repo-root> [<binary-dir>]
#   <binary-dir> given -> <binary-dir>/libspeedgun-ng.a
#   else <repo-root>/build/dev/libspeedgun-ng.a
#   else first match <repo-root>/build/*/libspeedgun-ng.a
# Exit 0 only when A, B and C all hold. Exit 1 otherwise, naming the archive.
#
# Parsing: GNU nm (ubuntu CI) --format=bsd; per-member headers
# ("member.o:" or "archive.a[member.o]:") attribute symbols to members.

set -euo pipefail

if [ -z "${1:-}" ]; then
  echo "usage: bash tools/hwloc/hwloc_nm_proof.sh <repo-root> [<binary-dir>]" >&2
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

echo "hwloc nm proof (FR-007/SC-011, privacy contract A6)"
echo "Archive: $ARCHIVE"
echo

# One nm pass; awk attributes symbols to members via per-member headers.
facts=$(nm --format=bsd "$ARCHIVE" 2>/dev/null | awk '
  /^[^[:space:]].*:$/ {
    m = $0; sub(/:$/, "", m); sub(/^.*\[/, "", m); member = m
    next
  }
  {
    if ($0 ~ /^[[:space:]]/) { type = $1; name = $2 }
    else if (NF >= 3)        { type = $2; name = $3 }
    else next
    if (name ~ /^sg_hwloc_/ && (type == "T" || type == "t")) {
      vend_count++
      if (!(member in seen) && anum < 5) { seen[member] = 1; anum++; amembers = amembers " " member }
    }
    if (name == "sg_hwloc_get_api_version") {
      if (type == "U") uall = uall " " member
      if (type == "T") tall = tall " " member
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
  echo "PASS A: $vend_count defined sg_hwloc_* symbols; example members:$amembers"
else
  echo "FAIL A: 0 defined sg_hwloc_* vendored symbols in $ARCHIVE"
  echo "        -> thin archive: no vendored hwloc members merged in."
  rc=1
fi

# B. Gate object contributes the undefined reference.
gate_u=""
for m in $uall; do
  case "$m" in
    *hwloc_gate*) [ -z "$gate_u" ] && gate_u="$m" ;;
  esac
done
if [ -z "$uall" ]; then
  echo "FAIL B: no member carries an undefined sg_hwloc_get_api_version (U) reference."
  rc=1
elif [ -n "$gate_u" ]; then
  echo "PASS B: gate member '$gate_u' references UNDEFINED sg_hwloc_get_api_version (U)."
else
  echo "FAIL B: U sg_hwloc_get_api_version found in[$uall], but no member named *hwloc_gate*."
  rc=1
fi

# C. Resolution: defined T sg_hwloc_get_api_version in a member other than the U ref.
resolved=""
for m in $tall; do
  if [ -z "$gate_u" ] || [ "$m" != "$gate_u" ]; then resolved="$m"; fi
done
if [ -n "$gate_u" ] && [ -n "$resolved" ]; then
  echo "PASS C: defined sg_hwloc_get_api_version (T) in '$resolved', resolves the U ref in '$gate_u'."
elif [ -n "$gate_u" ]; then
  echo "FAIL C: U ref in '$gate_u' but no DEFINED sg_hwloc_get_api_version (T) in another member of $ARCHIVE."
  rc=1
else
  echo "FAIL C: no gate U reference (see B), so resolution cannot be proven in $ARCHIVE."
  rc=1
fi

echo
if [ "$rc" -eq 0 ]; then
  echo "PASS: $ARCHIVE is self-contained: merged vendored members satisfy the gate reference (A6)."
else
  echo "FAIL: $ARCHIVE does not satisfy A6."
fi
exit "$rc"
