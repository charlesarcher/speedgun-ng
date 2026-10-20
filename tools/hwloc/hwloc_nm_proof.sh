#!/usr/bin/env bash
# tools/hwloc/hwloc_nm_proof.sh
# Feature 003-vendor-hwloc | FR-007 / SC-011 | privacy contract A6.
#
# Proves, from the static archive alone, that the vendored hwloc objects
# are merged INTO libspeedgun-ng.a and that the wrapper gate object
# (source/hwloc/hwloc_gate.cpp) links against them:
#
#   A. Vendored members present: > 0 defined sg_hwloc_* symbols (nm T/t).
#      The sg_ prefix comes from the hwloc configure symbol-prefix
#      option (research R-003), so a case-insensitive
#      'hwloc' audit still matches.
#   B. The gate contributes the reference: some member whose name contains
#      hwloc_gate carries an UNDEFINED sg_hwloc_get_api_version (nm U).
#   C. Resolution inside the archive: a DEFINED sg_hwloc_get_api_version
#      (nm T) lives in a different member than the U reference, i.e. the
#      merged vendored member satisfies the wrapper's prefixed reference.
#
# Feature 022-topology-placement (T031, R-12, FR-031) adds the harness
# archive as a second subject with its own assertions:
#
#   D. The harness archive holds UNDEFINED sg_hwloc_* references (its
#      calls reach the vendored members across the archive boundary) and
#      zero DEFINED ones (no vendored member merged into it).
#   E. An executable that links both archives resolves them: the
#      first_benchmark executable carries DEFINED sg_hwloc_* symbols from
#      the vendored archive on its link line and no UNDEFINED ones.
#   F. No unprefixed hwloc C symbol is exported from an installed
#      artifact: no global (uppercase) symbol named hwloc_* appears in
#      either archive or in the executable.
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
#   The harness archive and the example executable are looked for beside
#   the library archive, in the same binary directory.
# Exit 0 only when A, B, C, D, E and F all hold. Exit 1 otherwise, naming
# the archive.
#
# Parsing: GNU nm (ubuntu CI) in bsd output format; per-member headers
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

echo "hwloc nm proof (FR-007/SC-011, privacy contract A6; FR-031/R-12)"
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

# D and E: the harness archive as second subject (T031, R-12, FR-031).
HARCHIVE="$(dirname "$ARCHIVE")/libspeedgun-ng-harness.a"
EXE="$(dirname "$ARCHIVE")/example/first_benchmark"
if [ ! -f "$HARCHIVE" ] || [ ! -f "$EXE" ]; then
  echo "FAIL D/E: missing second subject: $HARCHIVE and $EXE must both exist."
  rc=1
else
  hfacts=$(nm --format=bsd "$HARCHIVE" 2>/dev/null | awk '
    {
      if ($0 ~ /^[[:space:]]/) { type = $1; name = $2 }
      else if (NF >= 3)        { type = $2; name = $3 }
      else next
      if (name ~ /^sg_hwloc_/) {
        if (type == "U") hu++
        if (type == "T" || type == "t") hd++
      }
      if (name ~ /^hwloc_/ && type ~ /^[TDBW]$/) bad++
    }
    END { print "HU=" hu + 0; print "HD=" hd + 0; print "HBAD=" bad + 0 }')
  h_u=$(echo "$hfacts" | sed -n 's/^HU=//p')
  h_d=$(echo "$hfacts" | sed -n 's/^HD=//p')
  if [ "$h_u" -gt 0 ] && [ "$h_d" -eq 0 ]; then
    echo "PASS D: harness archive '$HARCHIVE' carries $h_u UNDEFINED and 0 DEFINED sg_hwloc_*."
  else
    echo "FAIL D: harness archive carries $h_u UNDEFINED and $h_d DEFINED sg_hwloc_*."
    rc=1
  fi

  efacts=$(nm "$EXE" 2>/dev/null | awk '
    {
      if ($0 ~ /^[[:space:]]/) { type = $1; name = $2 }
      else if (NF >= 3)        { type = $2; name = $3 }
      else next
      if (name ~ /^sg_hwloc_/) {
        if (type == "U") eu++
        if (type == "T" || type == "t") ed++
      }
      if (name ~ /^hwloc_/ && type ~ /^[TDBW]$/) bad++
    }
    END { print "EU=" eu + 0; print "ED=" ed + 0; print "EBAD=" bad + 0 }')
  e_u=$(echo "$efacts" | sed -n 's/^EU=//p')
  e_d=$(echo "$efacts" | sed -n 's/^ED=//p')
  if [ "$e_d" -gt 0 ] && [ "$e_u" -eq 0 ]; then
    echo "PASS E: executable '$EXE' resolves the names: $e_d DEFINED, 0 UNDEFINED sg_hwloc_*."
  else
    echo "FAIL E: executable carries $e_d DEFINED and $e_u UNDEFINED sg_hwloc_*."
    rc=1
  fi

  # F: no exported unprefixed hwloc C symbol in any installed artifact.
  l_bad=$(nm --format=bsd "$ARCHIVE" 2>/dev/null | awk '
    {
      if ($0 ~ /^[[:space:]]/) { type = $1; name = $2 }
      else if (NF >= 3)        { type = $2; name = $3 }
      else next
      if (name ~ /^hwloc_/ && type ~ /^[TDBW]$/) bad++
    }
    END { print bad + 0 }')
  h_bad=$(echo "$hfacts" | sed -n 's/^HBAD=//p')
  e_bad=$(echo "$efacts" | sed -n 's/^EBAD=//p')
  if [ "$l_bad" -eq 0 ] && [ "$h_bad" -eq 0 ] && [ "$e_bad" -eq 0 ]; then
    echo "PASS F: no exported unprefixed hwloc_* symbol in the two archives or the executable."
  else
    echo "FAIL F: exported unprefixed hwloc_* symbols: library $l_bad, harness $h_bad, executable $e_bad."
    rc=1
  fi
fi

if [ "$rc" -eq 0 ]; then
  echo "PASS: the harness archive references the vendored members across the archive boundary and the executable resolves them (FR-031)."
fi
exit "$rc"
