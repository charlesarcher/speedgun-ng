#!/usr/bin/env bash
# tools/ci/host_record_gate.sh
# The committed-record check of the host record (specs/031-host-validation,
# the design ruling of 2026-10-23): the reader runs over the records under
# validation/ against the schema under docs/schema. The gate writes no record
# and reads no host, so it stands free of the runner's CPU (FR-035); there is
# no hardware CI tier. A record that does not validate fails the gate with the
# one line the reader prints for it, the line that quotes the file and the
# reason.
#
# Usage: tools/ci/host_record_gate.sh <reader> <tree-root> <records-dir>
set -euo pipefail

if [ $# -ne 3 ]; then
  printf 'usage: host_record_gate.sh <reader> <tree-root> <records-dir>\n' >&2
  exit 2
fi

reader=$1
root=$2
records=$3

status=0
"$reader" --root "$root" --records "$records" || status=$?
exit "$status"
