#!/usr/bin/env bash
# test/docs_mask_guard.sh
# The mask guard of a docs example gate (027 FR-034, FR-035).
#
# The recorded run of the jitter probe example needs a host whose mask
# gives the idle resolution a free cpu behind the two planned contexts
# (FR-017), so a mask that holds fewer than three cpus cannot serve it.
# The gate that wraps this script skips with the reason on such a host,
# the way counters_overhead skips on a host without a probe-passing
# fast mechanism. The count stands beside the recorded run it serves:
# the `example-run: needs-cpus` declaration of
# example/docs/jitter_probe.cpp names the same need.
#
# Usage: docs_mask_guard.sh <min-cpus> <command> [args...]
#
# Exit 2 on a mask below <min-cpus>, the skip code the SKIP_RETURN_CODE
# test property reads; the command's own status otherwise.

set -uo pipefail

if [ "$#" -lt 2 ]; then
  echo "usage: docs_mask_guard.sh <min-cpus> <command> [args...]" >&2
  exit 2
fi

need=$1
shift
held=$(python3 -c 'import os; print(len(os.sched_getaffinity(0)))')
if [ "$held" -lt "$need" ]; then
  printf 'the mask holds %s cpus, the recorded run needs %s\n' \
    "$held" "$need" >&2
  exit 2
fi
exec "$@"
