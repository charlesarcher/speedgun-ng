#!/usr/bin/env bash
# tools/ci/prepush.sh
# T003 / FR-029, FR-030: the local mirror of the CI checks, one timed
# line per step, as contracts/prepush-script.md fixes it. The scaffold
# carries the options, the exit codes and the step() timing; the step
# bodies and fault paths follow in T020 to T024.
#
# Usage: tools/ci/prepush.sh [--full]
#   no option     the steps through the quick tier, the two cpu
#                 pinning runs and the naming check
#   --full        also the slow tier, the consumer-release build and
#                 the tsan configure and link
#   --help        print this usage and exit 0
#   anything else exit 2 before any step runs
#
# Each step prints one line when it finishes:
#   prepush: <step name> <seconds> s
#
# Exit 0 when every step passed, 1 when a step failed or a required
# tool is missing or the wrong version, 2 for an unknown option.

set -euo pipefail

# Run one step and print its wall time when it finishes. A failing
# command stops the script with its own status under set -e.
step() {
  local name=$1
  shift
  local started=$SECONDS
  "$@"
  printf 'prepush: %s %d s\n' "$name" $((SECONDS - started))
}

usage() {
  cat <<'EOF'
Usage: tools/ci/prepush.sh [OPTION]...

Run the CI checks locally, one timed line per step.

Options:
  (none)    the steps through the quick tier, the two cpu pinning
            runs and the naming check
  --full    also the slow tier, the consumer-release build and the
            tsan configure and link
  --help    print this usage and exit 0

Exit status:
  0  every step passed
  1  a step failed, or a required tool is missing or the wrong version
  2  an unknown option
EOF
}

FULL=0
while [ $# -gt 0 ]; do
  case "$1" in
    --full) FULL=1 ;;
    --help) usage; exit 0 ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

# The step commands of the contract table are relative to the
# repository root, so run from there whatever the calling directory.
cd "$(dirname "$0")/../.."

# Steps 1 to 10, as ordered calls to step() in the order of the table
# in contracts/prepush-script.md: drift gate, format gate,
# time-source gate, spell gate, prose gate, configure, build, quick
# tier at full affinity, quick tier on two cpus, naming check on the
# changed units.

if [ "$FULL" = 1 ]; then
  # Steps 11 to 13, only under --full: slow tier, consumer release
  # build, tsan configure and link.
  :
fi

exit 0
