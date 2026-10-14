#!/usr/bin/env bash
# test/example_run.sh
# Run gate for the docs examples (019 T035; FR-034, FR-035, SC-14, D-12, D-15).
#
# Usage: example_run.sh [build-dir]
#
# Every docs_ target of example/docs/ runs through its own binary with
# run with a min-time floor of 0.01 seconds. A file passes when every benchmark
# result row, no row prints FAILED, and every SKIPPED row is one the file
# is one the file declares beside its registration with an
# `// example-run: expect-skip <instance>: <message>` marker. The list flag
# gives the registered set, so a row that vanished and a benchmark that never
# ran are both visible. A declared skip that never prints is a lie about the
# example, so the pairing is checked in both directions.
#
# The build directory holds the docs_ binaries. The CMake test passes it as the
# first argument; a manual run can set SG_BUILD_DIR instead. With neither, the
# gate fails and names both.
#
# Exit 0 clean, 1 on a violation.

set -uo pipefail

ROOT=$(cd "${BASH_SOURCE[0]%/*}/.." && pwd)

BUILD_DIR=${1:-${SG_BUILD_DIR:-}}
if [ -z "$BUILD_DIR" ]; then
  echo "FAIL: no build directory: pass it as the first argument or set" >&2
  echo "      SG_BUILD_DIR" >&2
  exit 1
fi

status=0
for source in "$ROOT"/example/docs/*.cpp; do
  name=$(basename "$source" .cpp)
  binary="$BUILD_DIR/example/docs_$name"
  if [ ! -x "$binary" ]; then
    echo "FAIL: docs_$name has no built binary at $binary" >&2
    status=1
    continue
  fi

  listed=$("$binary" --list 2>/dev/null)
  if [ -z "$listed" ]; then
    echo "FAIL: docs_$name lists no registered benchmark" >&2
    status=1
    continue
  fi

  output=$("$binary" --min-time=0.01 2>&1)
  code=$?

  if printf '%s\n' "$output" | grep -q 'FAILED'; then
    echo "FAIL: docs_$name printed a FAILED row" >&2
    printf '%s\n' "$output" | grep 'FAILED' >&2
    status=1
    continue
  fi
  if [ "$code" -ne 0 ]; then
    echo "FAIL: docs_$name exited $code" >&2
    status=1
    continue
  fi

  declared_pairs=$(grep -o 'example-run: expect-skip .*' "$source" \
    | sed 's/^example-run: expect-skip //')
  printed_pairs=$(printf '%s\n' "$output" \
    | sed -n 's/^\([^ ]*\)[[:space:]]\+SKIPPED:[[:space:]]*\(.*[^[:space:]]\)[[:space:]]*$/\1: \2/p')
  declared=$(printf '%s\n' "$declared_pairs" | sed 's/: .*//')
  printed=$(printf '%s\n' "$printed_pairs" | sed 's/: .*//')

  ok=1
  while IFS= read -r instance; do
    [ -z "$instance" ] && continue
    if ! printf '%s\n' "$output" | grep -q "^$instance"; then
      echo "FAIL: docs_$name registered $instance but printed no row" >&2
      ok=0
      continue
    fi
    if printf '%s\n' "$printed" | grep -qxF "$instance"; then
      if ! printf '%s\n' "$declared" | grep -qxF "$instance"; then
        echo "FAIL: docs_$name skipped $instance without declaring it" >&2
        ok=0
      fi
    else
      if printf '%s\n' "$declared" | grep -qxF "$instance"; then
        echo "FAIL: docs_$name declares a skip for $instance that did not print" >&2
        ok=0
      fi
    fi
  done <<EOF
$listed
EOF

  while IFS= read -r expectation; do
    [ -z "$expectation" ] && continue
    instance=${expectation%%: *}
    if ! printf '%s\n' "$printed_pairs" | grep -qxF "$expectation"; then
      if printf '%s\n' "$printed" | grep -qxF "$instance"; then
        echo "FAIL: docs_$name printed another message for $instance" >&2
      else
        echo "FAIL: docs_$name declares a skip for $instance that did not print" >&2
      fi
      ok=0
    fi
  done <<EOF
$declared_pairs
EOF

  if [ "$ok" -ne 1 ]; then
    status=1
  else
    echo "PASS: docs_$name ($(printf '%s\n' "$listed" | grep -c .) instances)"
  fi
done

exit "$status"
