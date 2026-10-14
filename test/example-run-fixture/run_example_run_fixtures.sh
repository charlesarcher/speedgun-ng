#!/usr/bin/env bash
# test/example-run-fixture/run_example_run_fixtures.sh
# Liveness fixtures for the docs-example run gate (019 T035; FR-035, SC-14).
#
# Usage: run_example_run_fixtures.sh [repo-root]
#
# A gate that inspects nothing passes on a broken tree, so each outcome the
# gate claims to tell apart is planted in a scratch tree of fake docs_ binaries
# and the gate must name it: a clean file passes, a registered benchmark with
# no row fails, a skip nobody declared fails, a declared skip passes, and an
# expectation the run never prints fails. Exit 0 when every case behaves, 1
# otherwise.

set -uo pipefail

REPO=${1:-}
if [ -z "$REPO" ]; then
  REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
fi
GATE="$REPO/test/example_run.sh"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

LISTED="'bmA' 'bmB'"
rowA='bmA                 1.20 ns         1.19 ns'
rowB='bmB                 2.40 ns         2.38 ns'
skipB='bmB                 SKIPPED: no cells configured'

# build_tree <case> <marker-line> <row-for-bmA> <row-for-bmB-or-empty>
# The fake binary answers the list query with the registered set and prints
# the rows the case says print.
build_tree() {
  case=$1 marker=$2 first=$3 second=$4
  tree="$WORK/$case"
  mkdir -p "$tree/test" "$tree/example/docs" "$tree/build/example"
  cp "$GATE" "$tree/test/example_run.sh"
  {
    echo 'auto main() -> int { return 0; }'
    if [ -n "$marker" ]; then echo "// $marker"; fi
  } > "$tree/example/docs/demo.cpp"
  cat > "$tree/build/example/docs_demo" <<SCRIPT
#!/usr/bin/env bash
if [ "\$1" = "--list" ]; then printf '%s\n' $LISTED; exit 0; fi
printf '%s\n' '$first'
if [ -n '$second' ]; then printf '%s\n' '$second'; fi
exit 0
SCRIPT
  chmod +x "$tree/build/example/docs_demo"
  printf '%s' "$tree"
}

expect() {
  want=$1 tree=$2 pattern=$3 label=$4
  out=$(bash "$tree/test/example_run.sh" "$tree/build" 2>&1)
  code=$?
  if [ "$code" -ne "$want" ]; then
    echo "FAIL: $label exited $code, want $want"
    printf '%s\n' "$out"
    return 1
  fi
  if [ -n "$pattern" ]; then
    if ! printf '%s\n' "$out" | grep -q "$pattern"; then
      echo "FAIL: $label did not name the violation"
      printf '%s\n' "$out"
      return 1
    fi
  fi
  echo "ok: $label"
  return 0
}

failed=0

tree=$(build_tree clean '' "$rowA" "$rowB")
expect 0 "$tree" 'PASS: docs_demo' 'clean file' || failed=1

tree=$(build_tree missing '' "$rowA" '')
expect 1 "$tree" 'printed no row' 'registered benchmark with no row' || failed=1

tree=$(build_tree undeclared '' "$rowA" "$skipB")
expect 1 "$tree" 'skipped bmB without declaring' 'skip nobody declared' || failed=1

tree=$(build_tree declared 'example-run: expect-skip bmB: no cells configured' "$rowA" "$skipB")
expect 0 "$tree" 'PASS: docs_demo' 'declared skip' || failed=1

tree=$(build_tree stale 'example-run: expect-skip bmB: no cells configured' "$rowA" "$rowB")
expect 1 "$tree" 'did not print' 'expectation the run never prints' || failed=1

exit "$failed"
