#!/usr/bin/env bash
# Fail unless application coverage is 100% line (and 100% branch when
# branch records exist). Zero branch records means no application
# branches remain after fuse/exception filters — that is a pass, not a
# missing-tool skip.
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: coverage_gate.sh <coverage.info>" >&2
  exit 2
fi

info=$1
if [[ -z "${LCOV:-}" ]]; then
  LCOV=$(command -v lcov)
fi
if [[ -z "${LCOV}" ]]; then
  echo "lcov is required" >&2
  exit 2
fi

out=$("${LCOV}" --branch-coverage --summary "${info}" 2>&1) || true
printf '%s\n' "${out}"

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
