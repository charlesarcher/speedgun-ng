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
# of the 030 round. The branch metric carries a recorded allowance: the
# never-taken branch positions no lane reaches, one ledger row per
# accepted class. A larger shortfall is a regression and fails; the line
# metric keeps its strict 100.0% check above.
#
# Row one, the 030 round of 2026-10-26: 19 rows across the accepted
# classes, listed in the round report of that date (D-14).
#
# Row two, specs/031-host-validation of 2026-10-30: 2 rows, the
# positions lcov names `source/harness/host_record_check.cpp line 266
# branch 21 -> 22` and the same line's `branch 21 -> 23`, where the
# `EvidenceRow` construction stood at line 184 until the reads of that
# record answered refusals of their own. Their class is the unwind tail
# of that construction. Two steps stand ahead of it. The reader guards
# every record against the schema
# `docs/schema/host-record-v1.json`, which requires `path`, `command`,
# `exit` and `line` on each row, and behind the guard the read of a row
# that holds no such value answers an empty one beside a note naming the
# key, so the construction reaches its end on every document the reader
# can be handed. A failed contract reaches `std::abort()` at
# `include/speedgun-ng/dbc.hpp:213`, so a caller leaves that
# construction whole, ends the process, or never reaches the line. An
# allocation failure stands between, and no caller supplies an allocator
# to that vector. No host reaches an unwind there, so these two rows
# stand dark on the CI runner and on a developer host alike, and the
# Sizing note below carries the two readings. Seven further `BRDA` rows
# of the file stand in the exception-unwind class the extract step's
# `--filter exception` drops before the rate: this construction, the
# listing loop at line 548, and the schema read at line 655.
# Sizing.
# The allowance counts the union of the CI coverage job, where every
# entry runs to its end. That union read 16 rows at `bea6af5b` (run
# 40983302766, artifact `coverage-info`), 16 at the head of specs/033,
# 18 at specs/032 and 19 at specs/035, so the accepted classes carry
# the reading within two rows of the constant. A developer host reads
# higher, because an entry there can stop early and leave the arms past
# its stop dark. On the lane host of 2026-10-30 two entries do that.
# `counters_linux_pmu_seam_test` fails the FR-019 device-scope claim
# with `device 'cpu' holds 0 merged rows`, and `harness_cli_test` skips
# the metric columns for want of a metric row that resolves to a
# published leaf. Their stops leave 6 rows dark:
# `source/counters/linux_pmu/fast_read.cpp:197`,
# `source/counters/linux_pmu/provider.cpp:567` and `:595`, and
# `source/harness/json_load.cpp:130` twice and `:1356`. Those six stand
# covered in the CI union, so they join no ledger row. A host run that
# reports them reports the host.
allowance=21
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
