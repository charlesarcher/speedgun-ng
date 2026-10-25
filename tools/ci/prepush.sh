#!/usr/bin/env bash
# tools/ci/prepush.sh
# T003 / FR-029, FR-030: the local mirror of the CI checks, one timed
# line per step, as contracts/prepush-script.md fixes it. The scaffold
# carries the options, the exit codes and the step() timing; the rest
# of the step bodies and the fault paths follow in T022 to T024.
#
# Usage: tools/ci/prepush.sh [--full]
#   no option     the steps through the quick tier, the two cpu
#                 pinning runs and the naming check
#   full          also the slow tier, the consumer-release build and
#                 the tsan configure and link
#   help          print this usage and exit 0
#   anything else exit 2 before any step runs
#
# Each step prints one line when it finishes:
#   prepush: <step name> <seconds> s
#
# Exit 0 when every step passed, 1 when a step failed or a required
# tool is missing or the wrong version, 2 for an unknown option.

set -euo pipefail

# Run one step and print its wall time when it finishes. A failing
# command prints its own command line and exit status, and stops the
# script with that status.
step() {
  local name=$1
  shift
  local started=$SECONDS
  local status=0
  "$@" || status=$?
  if [ "$status" -ne 0 ]; then
    printf 'prepush: %s failed with %d: %s\n' "$name" "$status" "$*" >&2
    exit "$status"
  fi
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

# Steps 1 to 5, the gates that read the tree and need no build
# directory, in the order of the table in contracts/prepush-script.md.
step 'drift gate' python3 tools/docs/example_drift.py .
step 'format gate' cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
step 'time-source gate' bash test/time_source_gate.sh .
step 'spell gate' cmake -P cmake/spell.cmake
step 'prose gate' python3 tools/prose/prose_gate.py --base origin/master

# Steps 6 to 9, the build tree of the prepush preset and the quick
# tier twice: once with the whole host, once pinned to two cpus, which
# is what a four-vCPU runner does to the same tier. The build names
# the tree by directory: plan.md adds no build preset and the
# contracts/presets.md verify line builds the same way, so there is
# no prepush build preset to name.
step 'configure' cmake --preset=prepush
step 'build' cmake --build build/prepush -j "$(nproc)"
step 'quick tier, full affinity' ctest --preset prepush-quick -j "$(nproc)"
step 'quick tier, two cpus' taskset -c 0,1 ctest --preset prepush-quick -j 2

# Step 10, the naming check on the changed units, mirrors the shape
# CI fails on: the one check .clang-tidy pins as WarningsAsErrors at
# line 21, with the two header filters of the clang-tidy preset in
# CMakePresets.json. The units are the .cpp files the diff against
# origin/master names that still exist. A header the diff names is
# analyzed through a named unit that includes it, because clang-tidy
# needs a translation unit; a diff that names only headers, or no C++
# at all, passes with a note and CI's static-analysis job, which
# builds every unit, keeps the full header verdict. The reconfigure
# exports the compile database of the step 6 tree; the preset does
# not write one, and without compile flags clang-tidy cannot parse a
# unit and fails on the missing includes alone.
naming_check() {
  local root=$PWD
  local units=()
  local unit
  while IFS= read -r unit; do
    case $unit in
      *.cpp) [ -f "$unit" ] && units+=("$unit") ;;
    esac
  done < <(git diff --name-only origin/master)
  if [ "${#units[@]}" -eq 0 ]; then
    echo 'naming check: the diff names no C++ unit, nothing to analyze'
    return 0
  fi
  cmake --preset=prepush -D CMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
  clang-tidy \
    --checks="-*,readability-identifier-naming" \
    --header-filter="^$root/" \
    --exclude-header-filter="^$root/external/" \
    -p build/prepush \
    "${units[@]}"
}
step 'naming check on the changed units' naming_check

if [ "$FULL" = 1 ]; then
  # Steps 11 to 13, only with the full option, in the order of the
  # contract table, one timing line each. The two build pairs are
  # functions for that reason, the shape of the naming check above.
  #
  # Step 11 is the whole suite: the prepush-full test preset carries no
  # label filter, so it adds what the quick tier leaves out. Steps 12
  # and 13 mirror their CI jobs line for line, the configure arguments
  # and the build directory included, because a shorter pair would not
  # be the build the job proves. The tsan configure carries the
  # clang++ the tsan job sets in its env: gcc is the host default here,
  # and a configure that switches the compiler of a tree already under
  # build/tsan fails on the cached one.
  consumer_release_build() {
    cmake --preset=ci-linux-ignore -B build/consumer-release \
      -D CMAKE_BUILD_TYPE=Release \
      -D speedgun-ng_DEVELOPER_MODE=OFF
    cmake --build build/consumer-release -j "$(nproc)"
  }
  tsan_configure_and_link() {
    CXX=clang++ cmake --preset=ci-tsan
    cmake --build build/tsan -j "$(nproc)"
  }
  step 'slow tier' ctest --preset prepush-full -j "$(nproc)"
  step 'consumer release build' consumer_release_build
  step 'tsan configure and link' tsan_configure_and_link
fi

exit 0
