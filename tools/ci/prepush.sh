#!/usr/bin/env bash
# tools/ci/prepush.sh
# T003, T024 / FR-029, FR-030: the local mirror of the CI checks, one
# timed line per step, as contracts/prepush-script.md fixes it, with
# the fault paths of FR-030: the unknown option, the missing tool, the
# formatter version stop before formatting, the failing-step line.
#
# Usage: tools/ci/prepush.sh [--full] [--self-check]
#   no option     the steps through the quick tier, the two cpu
#                 pinning runs and the naming check
#   full          also the slow tier, the consumer-release build and
#                 the tsan configure and link
#   self-check    drive each fault path in place and print one
#                 self-check line per path, touching no repository
#                 file
#   help          print this usage and exit 0
#   anything else exit 2 before any step runs
#
# Each step prints one line when it finishes:
#   prepush: <step name> <seconds> s
#
# Exit 0 when every step passed, or under the self-check option when
# every fault path behaved, 1 when a step failed or a required tool is
# missing or the wrong version, 2 for an unknown option.

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
  (none)       the steps through the quick tier, the two cpu pinning
               runs and the naming check
  --full       also the slow tier, the consumer-release build and the
               tsan configure and link
  --self-check drive each fault path in place, one self-check line
               per path, exit 0 only when every path behaved
  --help       print this usage and exit 0

Exit status:
  0  every step passed, or every self-check passed
  1  a step failed, or a required tool is missing or the wrong version
  2  an unknown option
EOF
}

FULL=0
SELF_CHECK=0
while [ $# -gt 0 ]; do
  case "$1" in
    --full) FULL=1 ;;
    --self-check) SELF_CHECK=1 ;;
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

# The tools every step calls, checked before any step runs. A missing
# one stops the run with a line that names the tool; a bare 127 from
# inside a step names nothing (FR-030).
require_tools() {
  local tool
  for tool in python3 clang-format cmake ctest taskset git clang-tidy nproc; do
    if ! command -v "$tool" >/dev/null; then
      printf 'prepush: missing tool: %s\n' "$tool" >&2
      exit 1
    fi
  done
}
require_tools

# Drive one fault path by running this script itself under an altered
# PATH, compare the status and the fault it printed, and print one
# self-check line. The arguments are the path's name, the PATH for
# the run, the exit status and the text the fault path must produce,
# then the options to run with. Details of a failure go to stderr so
# stdout keeps one line per path.
check_fault() {
  local name=$1 path=$2 expected=$3 pattern=$4
  shift 4
  local script=$PWD/tools/ci/prepush.sh
  local found status=0
  if found=$(PATH="$path" "$BASH" "$script" "$@" 2>&1 >/dev/null); then
    status=0
  else
    status=$?
  fi
  if [ "$status" -ne "$expected" ]; then
    printf 'self-check: %s fail\n' "$name"
    printf '  exit %d, expected %d\n' "$status" "$expected" >&2
    return 1
  fi
  if ! printf '%s\n' "$found" | grep -qF -- "$pattern"; then
    printf 'self-check: %s fail\n' "$name"
    printf '  fault output lacks: %s\n' "$pattern" >&2
    return 1
  fi
  printf 'self-check: %s pass\n' "$name"
}

# The fault-path proof the Test Plan names for shell code: each fault
# of FR-030 driven for real, the script run against a bad option, an
# empty PATH, a clang-format stub of another version and a python3
# stub that fails step 1 on purpose. The stubs live in a scratch
# directory, so no repository file is touched, and the formatter and
# step faults stop the child runs before any step could write.
self_check() {
  local stub fail=0
  stub=$(mktemp -d)
  mkdir "$stub/empty" "$stub/old-format" "$stub/step-failure"
  printf '#!/bin/sh\necho "clang-format version 9.9.9"\n' \
    > "$stub/old-format/clang-format"
  printf '#!/bin/sh\nexit 3\n' > "$stub/step-failure/python3"
  chmod +x "$stub/old-format/clang-format" "$stub/step-failure/python3"

  check_fault 'unknown option' "$PATH" 2 'unknown option: --nope' --nope \
    || fail=1
  check_fault 'missing tool' "$stub/empty" 1 'missing tool: python3' \
    || fail=1
  check_fault 'formatter version' "$stub/old-format:$PATH" 1 \
    'version found 9.9.9' || fail=1
  check_fault 'failing step' "$stub/step-failure:$PATH" 3 \
    'drift gate failed with 3: python3 tools/docs/example_drift.py .' \
    || fail=1

  rm -rf "$stub"
  return "$fail"
}

if [ "$SELF_CHECK" = 1 ]; then
  self_check && exit 0
  exit 1
fi

# Steps 1 to 5, the gates that read the tree and need no build
# directory, in the order of the table in contracts/prepush-script.md.
step 'drift gate' python3 tools/docs/example_drift.py .

# The formatter version gate belongs at step 2, before lint.cmake
# runs: CI pins clang-format 18.1.8 and any other version would
# reformat the tree differently, so the run stops here, before
# formatting anything, printing the version found (FR-030).
format_version_gate() {
  local found
  found=$(clang-format --version |
    sed -n 's/.*version \([0-9][0-9.]*\).*/\1/p')
  if [ "$found" != 18.1.8 ]; then
    printf 'prepush: clang-format version found %s, expected 18.1.8\n' \
      "$found" >&2
    exit 1
  fi
}
format_version_gate
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
