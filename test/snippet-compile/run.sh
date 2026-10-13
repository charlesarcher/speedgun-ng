#!/usr/bin/env bash
# test/snippet-compile/run.sh
# Snippet compile gate for the main page (spec 018 FR-011, SC-003; contract
# example-drift R-10).
#
# Usage: run.sh <source-dir> [<compiler>]
#
# Every C++ block the main page quotes wraps a complete top-level
# declaration, so a reader can paste it into the program the quick start
# builds. This gate proves that claim. It emits each marked cpp fence of the
# main page through the emit-blocks mode of example_drift.py, wraps each
# one in the canonical include prelude the quick start shows, and
# syntax-compiles the
# result. A block that drifted into a fragment fails here, and so does an API
# name the public headers no longer declare.
#
# Liveness: one planted probe snippet calls a name that does not exist, and
# the run requires the compiler to reject it before any real snippet is
# compiled. A run in which the probe compiles would be a gate that checks
# nothing, so it is a failure by itself.
#
# The public headers include the export header CMake generates into
# build/export, so -I <source>/include alone cannot expand them. The gate
# takes that generated header from a configured tree when one is present and
# states the same macros itself otherwise, which keeps the gate usable in a
# tree nobody has configured.
#
# Exit 0 clean, 1 on a finding, 2 on a usage error.

set -uo pipefail

readonly source_dir="${1:-}"
if [[ -z "${source_dir}" || ! -d "${source_dir}" ]]; then
  echo "snippet-compile: usage error: run.sh needs an existing source directory" >&2
  exit 2
fi

cxx="${2:-}"
if [[ -z "${cxx}" ]]; then
  for candidate in g++ clang++; do
    if command -v "${candidate}" >/dev/null 2>&1; then
      cxx="${candidate}"
      break
    fi
  done
fi
if [[ -z "${cxx}" ]]; then
  echo "snippet-compile: usage error: no C++ compiler found" >&2
  exit 2
fi

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

readonly drift="${source_dir}/tools/docs/example_drift.py"
if [[ ! -f "${drift}" ]]; then
  echo "snippet-compile: usage error: ${drift} is missing" >&2
  exit 2
fi

gen="${work}/generated/speedgun-ng"
mkdir -p "${gen}"
includes=(-I "${source_dir}/include")
export_header=""
for candidate in "${source_dir}/build/export" "${source_dir}/build"/*/export; do
  if [[ -f "${candidate}/speedgun-ng/speedgun-ng_export.hpp" ]]; then
    export_header="${candidate}"
    break
  fi
done
if [[ -n "${export_header}" ]]; then
  includes+=(-I "${export_header}")
  echo "snippet-compile: export header from ${export_header}"
else
  includes+=(-I "${work}/generated")
  printf '%s\n' '#pragma once' '#define SPEEDGUN_NG_EXPORT' \
    '#define SPEEDGUN_NG_NO_EXPORT' '#define SPEEDGUN_NG_DEPRECATED' \
    '#define SPEEDGUN_NG_DEPRECATED_EXPORT' \
    '#define SPEEDGUN_NG_DEPRECATED_NO_EXPORT' \
    '#define SPEEDGUN_NG_SUPPRESS_C4251' >"${gen}/speedgun-ng_export.hpp"
  echo "snippet-compile: export header stated by this gate"
fi

readonly prelude="${work}/prelude.txt"
printf '%s\n' '#include <cstdint>' '#include <vector>' \
  '#include "speedgun-ng/barrier.hpp"' '#include "speedgun-ng/benchmark.hpp"' \
  >"${prelude}"

compile_unit() {
  local unit="$1"
  cat "${prelude}" "${unit}" >"${work}/unit.cpp"
  local output
  output="$("${cxx}" -std=c++23 -fsyntax-only "${includes[@]}" \
    "${work}/unit.cpp" 2>&1)"
  local status="$?"
  if [[ "${status}" -ne 0 ]]; then
    printf '%s\n' "${output}" | sed -e 's/^/    /'
  fi
  return "${status}"
}

printf '%s\n' 'auto brokenProbe(sg::State& state) -> void' '{' \
  '  state.thisNameDoesNotExist();' '}' >"${work}/probe.snippet"
if compile_unit "${work}/probe.snippet" >/dev/null 2>&1; then
  echo "snippet-compile: the planted probe compiled, so the gate checks nothing" >&2
  exit 1
fi
echo "snippet-compile: planted probe rejected"

emitted="$(python3 "${drift}" "${source_dir}" --emit-blocks "${work}/blocks")"
echo "${emitted}"
blocks="${work}/blocks"
if [[ ! -d "${blocks}" ]]; then
  echo "snippet-compile: finding: --emit-blocks wrote no directory" >&2
  exit 1
fi
count="$(find "${blocks}" -name '*.snippet' | wc -l | tr -d ' ')"
if [[ ! "${emitted}" =~ emitted\ ${count}\ cpp\ snippet\ of\ ([0-9]+)\ cpp\ fence ]] ||
  [[ ! "${BASH_REMATCH[1]}" -eq "${count}" || "${count}" -eq 0 ]]; then
  echo "snippet-compile: finding: emitted ${count} snippet of the cpp fences" \
    "on the main page, and the counts must agree and stay above zero" >&2
  exit 1
fi

status=0
for snippet in "${blocks}"/*.snippet; do
  name="$(basename "${snippet}")"
  if compile_unit "${snippet}" >/dev/null 2>&1; then
    echo "snippet-compile: pass ${name}"
  else
    echo "snippet-compile: finding: ${name} does not compile over the" \
      "canonical prelude" >&2
    compile_unit "${snippet}" >&2 || true
    status=1
  fi
done

if [[ "${status}" -eq 0 ]]; then
  echo "snippet-compile: ${count} snippet compile"
fi
exit "${status}"
