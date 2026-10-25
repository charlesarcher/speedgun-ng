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

echo "${out}" | grep -q 'branches\.\.\.\.: 100.0%' || {
  echo "coverage-gate: branch coverage is not 100%" >&2
  exit 1
}
