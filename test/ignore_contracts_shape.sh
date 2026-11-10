#!/usr/bin/env bash
# test/ignore_contracts_shape.sh
# Ignore-semantic compile gate for names that feed a postcondition
# (052 T004; FR-002, FR-003, FR-005, FR-006, D-6, D-8, R-8).
#
# Usage: ignore_contracts_shape.sh <repo-root> <binary-dir>
#
# A helper whose only call site stands inside SG_ENSURE is a used name at
# the enforce semantic and an unused one at the ignore semantic, where the
# macro expands to ((void)0) at include/speedgun-ng/dbc.hpp:463. The
# library target builds with SG_CONTRACTS_SEMANTIC=2, so the ordinary
# build reads such a name as used and prints nothing, and only a release
# consumer's compile, where the define is 0, reaches the warning. This
# gate runs that compile over the two units that hold the shape:
# source/harness/noise_tracer.cpp and example/docs/hunt_ranks.cpp. Each
# file compiles in its own step, so a failing first step cannot hide the
# reading of the second.
#
# -Wall and -Wextra carry the two unused warnings, and -Werror on each one
# is the promotion a top-level build makes through
# CMAKE_COMPILE_WARNING_AS_ERROR at cmake/variables.cmake:28. The two
# defines the build writes beside the semantic come along: SG_BUILD_TYPE
# and SG_PROJECT_VERSION reach the report text as string literals at
# source/harness/report.cpp:493, so a compile that leaves one out answers
# for a translation unit other than the one the build reads. The version
# comes from the project() line of $1/CMakeLists.txt and the export
# directory from $2, so no version string and no build directory stand
# baked in here.
#
# The four include directories carry the form R-8 measured: the public
# headers, the harness-internal headers beside the sources, and the
# generated export header. hunt_ranks.cpp reaches every header through the
# public and export directories alone, because it declares the one harness
# function it calls by hand at example/docs/hunt_ranks.cpp:56, and
# noise_tracer.cpp reaches its internal headers through paths relative to
# itself, so the gate adds no include directory of its own.
#
# A planted probe keeps the reading live: the gate copies
# noise_tracer.cpp into a scratch directory and plants one helper whose
# only call site stands inside SG_ENSURE, the shape of the two names the
# gate reads. That copy must fail the semantic-0 compile with an
# unused-function diagnostic naming the planted helper, and must pass the
# semantic-2 compile. Where it does not, a clean run would rest on a scan
# that inspects nothing, so the gate counts the step as failed and names
# the probe. The repository file keeps its bytes: the probe stands in the
# copy alone.
#
# Each step prints the compiler, the unit, the semantic, the exit status
# and the seconds it took, read from the SECONDS builtin, and each
# compiler crossed with each semantic prints the wall time of its steps.
#
# Exit 0 clean, 1 with the count of failed steps, 0 with one skip line
# where no compiler on PATH targets x86-64 on Linux, the platform the
# registration names in test/CMakeLists.txt.

set -uo pipefail

ROOT=${1:-.}
BINARY_DIR=${2:-}

if [ -z "$BINARY_DIR" ]; then
  echo "FAIL: binary dir not given; pass the CMake binary dir as \$2" >&2
  exit 1
fi

UNITS=(source/harness/noise_tracer.cpp example/docs/hunt_ranks.cpp)

# The name the probe plants, so the check below can ask the compiler's
# text for it. The caller of the helper carries external linkage, which
# keeps the warning off the enforce semantic, where the call stands.
PROBE_HELPER=probeAnswered

# The line the plant sits above: inside namespace sg::detail, where a
# helper in an anonymous namespace takes the shape of the two names the
# gate reads.
NAMESPACE_CLOSE='}  // namespace sg::detail'

probe_source() {
  cat <<'CPP'
// The planted probe of specs/052 FR-006: one helper whose only call site
// stands inside a postcondition. The scratch copy carries it, and the
// repository file does not.
namespace
{
auto probeAnswered(int value) -> bool
{
  return value >= 0;
}
}  // namespace

auto probeRecord(int value) -> int
{
  SG_ENSURE(probeAnswered(value),
            "the planted probe calls its helper from one postcondition");
  return value;
}
CPP
}

# The plant writes the copy to its argument. A closer that has moved makes
# the plant fail here, because a probe that planted nothing would answer a
# clean run for nothing.
plant_probe() {
  local target=$1
  awk -v marker="$NAMESPACE_CLOSE" -v block="$WORKDIR/probe_block.txt" '
    $0 == marker && !planted {
      while ((getline line < block) > 0) { print line }
      planted = 1
    }
    { print }
    END { if (!planted) exit 1 }
  ' "$ROOT/source/harness/noise_tracer.cpp" >"$target"
}

# The three tests below read the driver alone, so a host with no compiler
# reaches the skip line without any other program on PATH.

# Candidate spellings: the three plain drivers, a version suffix on one,
# and a target prefix on one. A program whose name only looks like a
# driver, flex++ and c++filt among them, reaches no rule here, so a cross
# wrapper that prints its host triple, wineg++ among them, cannot enter.
candidate_name() {
  case $1 in
    c++ | g++ | clang++ | c++-* | g++-* | clang++-* | *-c++ | *-g++)
      return 0 ;;
  esac
  return 1
}

# A driver that compiles an empty unit is a compiler. The unit goes to
# /dev/null, because the question here is whether the driver works.
compiles_empty_unit() {
  printf 'int main() { return 0; }\n' |
    "$1" -x c++ -c - -o /dev/null 2>/dev/null
}

# The gate reads an x86-64 build of a Linux program, and a driver that
# emits another target's code answers nothing about it.
targets_x86_64_linux() {
  local triple
  triple=$("$1" -dumpmachine 2>/dev/null) || return 1
  case $triple in
    x86_64*linux* | *amd64*linux*) return 0 ;;
  esac
  return 1
}

COMPILERS=()
DUPLICATES=()

# One program reaches the gate under several names: /usr/bin/c++ and
# /usr/bin/g++ carry one inode here. The -ef test reads the file behind a
# symlink, so the several spellings of one compiler cost one set of
# compiles.
add_compiler() {
  local path=$1 name=$2 other
  for other in "${COMPILERS[@]}"; do
    if [[ $path -ef $other ]]; then
      DUPLICATES+=("$name is $other")
      return
    fi
  done
  COMPILERS+=("$path")
}

IFS=':' read -ra PATH_DIRS <<<"$PATH"
for dir in "${PATH_DIRS[@]}"; do
  [ -d "$dir" ] || continue
  for path in "$dir"/*++; do
    if [ ! -f "$path" ] || [ ! -x "$path" ]; then
      continue
    fi
    name=${path##*/}
    candidate_name "$name" || continue
    targets_x86_64_linux "$path" || continue
    compiles_empty_unit "$path" || continue
    add_compiler "$path" "$name"
  done
done

if [ "${#COMPILERS[@]}" -eq 0 ]; then
  echo "skip: no compiler on PATH targets x86-64 on Linux, so neither unit" \
    "reaches a compile whose unused names this gate reads"
  exit 0
fi

NAMES=()
for path in "${COMPILERS[@]}"; do
  NAMES+=("${path##*/}")
  echo "compiler: ${path##*/} ($("$path" -dumpmachine))"
done
for note in "${DUPLICATES[@]}"; do
  echo "one program, several names: $note"
done

EXPORT_DIR="$BINARY_DIR/export"
if [ ! -f "$EXPORT_DIR/speedgun-ng/speedgun-ng_export.hpp" ]; then
  echo "FAIL: generated export header missing at $EXPORT_DIR" >&2
  echo "      build the library target before this gate" >&2
  exit 1
fi

# The version the project() line states, the one the build hands the
# harness as SG_PROJECT_VERSION.
VERSION=$(sed -n 's/^[[:space:]]*VERSION \([0-9][0-9.]*\)[[:space:]]*$/\1/p' \
  "$ROOT/CMakeLists.txt" | head -n 1)
if [ -z "$VERSION" ]; then
  echo "FAIL: $ROOT/CMakeLists.txt holds no indented VERSION line with a" >&2
  echo "      number, so the version the build defines stands unreadable" >&2
  exit 1
fi

for unit in "${UNITS[@]}"; do
  if [ ! -f "$ROOT/$unit" ]; then
    echo "FAIL: $ROOT/$unit not found" >&2
    exit 1
  fi
done

WORKDIR=$(mktemp -d)
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

FLAGS=(-O2 -std=c++23 -Wall -Wextra -Werror=unused-function
  -Werror=unused-variable)
DEFINES=(-DSG_BUILD_TYPE='"Release"' -DSG_PROJECT_VERSION="\"$VERSION\"")
INCLUDES=(-I"$ROOT/include" -I"$ROOT/source" -I"$ROOT/source/harness"
  -I"$EXPORT_DIR")

probe_source >"$WORKDIR/probe_block.txt"
PROBE_COPY="$WORKDIR/probe/noise_tracer.cpp"
mkdir -p "$WORKDIR/probe"
if ! plant_probe "$PROBE_COPY"; then
  echo "FAIL: no line reading }  // namespace sg::detail stood in" >&2
  echo "      $ROOT/source/harness/noise_tracer.cpp, so the probe of" >&2
  echo "      FR-006 stands unplanted and a clean run would mean nothing" >&2
  exit 1
fi

steps=0
failures=0

# One compile, one line. The last argument names what the step must
# answer: pass for a clean compile, and unused-function for the planted
# copy at the ignore semantic, where the compiler must report the planted
# helper by name.
run_step() {
  local cxx=$1 label=$2 source=$3 semantic=$4 expect=$5
  local object="$WORKDIR/${label//\//_}-${cxx##*/}-$semantic.o"
  local err="$WORKDIR/stderr.txt"
  local start=$SECONDS rc elapsed verdict

  "$cxx" "${FLAGS[@]}" "${DEFINES[@]}" "${INCLUDES[@]}" \
    "-DSG_CONTRACTS_SEMANTIC=$semantic" -c "$source" -o "$object" 2>"$err"
  rc=$?
  elapsed=$(( SECONDS - start ))
  steps=$(( steps + 1 ))

  verdict=failed
  if [ "$expect" = pass ]; then
    [ "$rc" -eq 0 ] && verdict=ok
  elif [ "$rc" -ne 0 ] && grep -q "unused-function" "$err" &&
    grep -q "$PROBE_HELPER" "$err"; then
    verdict=ok
  fi

  echo "step: $cxx $label semantic=$semantic exit=$rc ${elapsed}s verdict=$verdict"
  [ "$verdict" = ok ] && return 0

  failures=$(( failures + 1 ))
  if [ "$expect" = unused-function ]; then
    echo "        the planted probe did not bite: the ignore-semantic" \
      "compile must report $PROBE_HELPER as an unused function" >&2
  fi
  sed 's/^/        /' "$err"
  return 0
}

for cxx in "${COMPILERS[@]}"; do
  for semantic in 0 2; do
    pair_start=$SECONDS
    for unit in "${UNITS[@]}"; do
      run_step "$cxx" "$unit" "$ROOT/$unit" "$semantic" pass
    done
    if [ "$semantic" -eq 0 ]; then
      run_step "$cxx" "probe/noise_tracer.cpp" "$PROBE_COPY" "$semantic" \
        unused-function
    else
      run_step "$cxx" "probe/noise_tracer.cpp" "$PROBE_COPY" "$semantic" pass
    fi
    echo "time: $cxx semantic=$semantic $(( SECONDS - pair_start ))s"
  done
done

echo
echo "ignore_contracts_shape: failed steps $failures of $steps at semantics" \
  "0 and 2, compilers ${NAMES[*]}, version $VERSION"

if [ "$failures" -gt 0 ]; then
  exit 1
fi
exit 0
