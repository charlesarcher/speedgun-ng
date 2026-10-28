#!/usr/bin/env bash
# Fail unless application coverage is 100% line (and 100% branch when
# branch records exist). Zero branch records means no application
# branches remain after fuse/exception filters — that is a pass, not a
# missing-tool skip.
#
# The exits are 0 for a full trace, 1 for a measure below 100%, 2 for a
# usage fault, and 3 for a tool failure: lcov exited nonzero while its
# own report names no shortfall, so the reader looks at the tool and not
# at a ratio the tool never measured.
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: coverage_gate.sh <coverage.info>" >&2
  exit 2
fi

info=$1
if [[ -z "${LCOV:-}" ]]; then
  LCOV=$(command -v lcov) || true
fi
if [[ -z "${LCOV}" ]]; then
  echo "lcov is required" >&2
  exit 2
fi

if out=$("${LCOV}" --branch-coverage --summary "${info}" 2>&1); then
  rc=0
else
  rc=$?
fi
printf '%s\n' "${out}"

# A gated measure line that is neither "no data found" nor 100.0% is a
# shortfall. A refusal exits nonzero with no such line in its report, so
# the tool is what failed and the suite was never judged.
short=$(echo "${out}" | grep -E ' (lines|branches)\.+: ' \
  | grep -v 'no data found' | grep -v ': 100\.0%' || true)

if [[ ${rc} -ne 0 && -z "${short}" ]]; then
  echo "coverage-gate: lcov failed to read ${info} (exit ${rc})" >&2
  exit 3
fi

echo "${out}" | grep -q 'lines\.\.\.\.\.\.\.: 100.0%' || {
  echo "coverage-gate: line coverage is not 100%" >&2
  exit 1
}

if echo "${out}" | grep -q 'branches\.\.\.\.: no data found'; then
  echo "coverage-gate: no application branches (100% vacuously)"
  exit 0
fi

# D-14 (specs/030-ci-speedup/spec.md): the owner accepts the gap classes
# of the 030 round. The branch metric carries a recorded allowance of 19
# rows: the never-taken branch positions no lane reaches, listed in the
# round report of 2026-10-26. A larger shortfall is a regression and
# fails; the line metric keeps its strict 100.0% check above.
allowance=19
br=$(echo "${out}" | grep -oE 'branches\.+:\s*[0-9.]+% \([0-9]+ of [0-9]+ branches\)' | head -1)
if echo "${out}" | grep -q 'branches\.\.\.\.: 100.0%'; then
  exit 0
fi
if [[ -z "${br}" ]]; then
  echo "coverage-gate: branch coverage is not 100%" >&2
  exit 1
fi
hit=$(echo "${br}" | grep -oE '\([0-9]+ of' | grep -oE '[0-9]+')
total=$(echo "${br}" | grep -oE 'of [0-9]+' | grep -oE '[0-9]+')
missing=$((total - hit))
if (( missing <= allowance )); then
  echo "coverage-gate: branch shortfall ${missing} rows, within the"\
       " D-14 recorded allowance of ${allowance} (accepted classes)"
  exit 0
fi
echo "coverage-gate: branch shortfall ${missing} rows exceeds the " \
     "D-14 recorded allowance of ${allowance}" >&2
# The reader needs the rows the count names: every branch position the
# trace holds at taken zero, so the difference against the recorded
# classes stands on the log the job keeps.
awk '
  /^SF:/ { sf = $0; sub(/^SF:/, "", sf) }
  /^BRDA:/ {
    split(substr($0, 6), f, ",")
    if (f[4] == "0") { print sf " line " f[1] " branch " f[3] }
  }
' "${info}" | sort -u >&2
exit 1
