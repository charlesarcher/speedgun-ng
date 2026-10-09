#!/usr/bin/env bash
# test/cli_shape.sh
# Run-control shape gate for the entry point (FU-1; 015 FR-032, FR-036, R-06).
#
# Usage: cli_shape.sh [source-file]
#
# The entry point reads the interrupt flag once per pass and carries that
# one reading to the status update, the per-pass check and the break. A
# second read leaves a window between two readings in which a signal can
# arrive, and the postcondition then compares a status that never saw the
# signal against a flag that did. The final postcondition therefore reads
# no flag at all: it compares the status with the two run-wide flags.
#
# Exit 0 clean, 1 on a violation.

set -uo pipefail

SOURCE=${1:-}
if [ -z "$SOURCE" ]; then
  ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
  SOURCE=$ROOT/source/harness/cli.cpp
fi

if [ ! -f "$SOURCE" ]; then
  echo "FAIL: no entry-point source at $SOURCE" >&2
  exit 1
fi

status=0

# The loop body: from the run call to the end of the for loop.
loop=$(awk '/const auto result = runner\.run\(\*entry\);/,/^  \}$/' "$SOURCE")
if [ -z "$loop" ]; then
  echo "FAIL: could not locate the benchmark loop of speedgunMain" \
    "in $SOURCE" >&2
  exit 1
fi

reads=$(printf '%s\n' "$loop" | grep -c 'interruptFlag()\.load()')
if [ "$reads" -ne 1 ]; then
  echo "FAIL: the benchmark loop reads the interrupt flag $reads times," \
    "expected one reading per pass" >&2
  printf '%s\n' "$loop" | grep -n 'interruptFlag()' | sed 's/^/  /' >&2
  status=1
fi

if ! printf '%s\n' "$loop" | grep -q 'bool interrupted ='; then
  echo "FAIL: the loop holds no local reading of the flag" >&2
  status=1
fi

# The postcondition after the loop: it compares the status with the
# run-wide flags and reaches no flag itself.
post=$(grep -A1 'SG_ENSURE((status != 0)' "$SOURCE")
if [ -z "$post" ]; then
  echo "FAIL: no run-wide postcondition in $SOURCE" >&2
  exit 1
fi
if printf '%s\n' "$post" | grep -q 'interruptFlag'; then
  echo "FAIL: the run-wide postcondition reads the interrupt flag, so a" \
    "signal between the last reading and the check aborts the run" >&2
  printf '%s\n' "$post" | sed 's/^/  /' >&2
  status=1
fi
if ! printf '%s\n' "$post" | grep -q 'anyFailed || anyInterrupted'; then
  echo "FAIL: the run-wide postcondition does not compare the status with" \
    "anyFailed and anyInterrupted" >&2
  status=1
fi

if grep -q 'bool anyInterrupted' "$SOURCE" && [ "$status" -eq 0 ]; then
  echo "cli_shape: the entry point reads the flag once per pass and the"
  echo "           run-wide postcondition reads no flag"
fi
exit "$status"
