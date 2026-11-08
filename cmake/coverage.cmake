# Coverage is a hard gate (constitution VI/VIII). lcov and genhtml are
# required in developer mode; missing tools are a configure error, never
# a skipped target.

find_program(LCOV_EXECUTABLE lcov)
if(NOT LCOV_EXECUTABLE)
  message(
      FATAL_ERROR
      "lcov is required for developer builds (constitution VI: 100% "
      "line and branch coverage). Install lcov and reconfigure. "
      "Coverage is not disabled when the tool is absent."
  )
endif()

find_program(GENHTML_EXECUTABLE genhtml)
if(NOT GENHTML_EXECUTABLE)
  message(
      FATAL_ERROR
      "genhtml (from the lcov package) is required for developer "
      "builds. Install lcov and reconfigure. HTML coverage output is "
      "not disabled when the tool is absent."
  )
endif()

# ---- Variables ----

# We use variables separate from what CTest uses, because those have
# customization issues
#
# The mismatch error class is ignored: a vendored header's inline code is
# emitted into
# this project's own instrumented translation units, so the capture sees it.
# Every vendored tree lands in the raw trace today (simdjson, hwloc,
# HdrHistogram_c, zlib, and yaml-cpp all appear in it), and the extract step
# below then removes every vendored path before the gate reads a number. The
# gate's metrics are therefore unaffected by anything a vendored header
# contributes. quill is the first vendored tree whose headers trip lcov's
# exception-tag consistency check, which would otherwise abort the capture
# and take the whole coverage target down with it. Narrowing this to the one
# error class keeps every other lcov check, and gcov's own diagnostics, live.
#
# `negative` is the second class, and this toolchain needs it. gcov 16 emits
# counters that lcov 2.3 misreads, so a branch the test never took arrives
# as a negative taken count and lcov aborts the capture. The class is
# narrowed for the same reason as `mismatch`: it names one parsing complaint,
# and every other lcov check stays live.
# `inconsistent` is the third class, and the capture needs it for one shape
# alone. `SG_BENCHMARK_F` and its siblings end with the header of the method
# the site then defines, so gcov records that method and the registration
# closures at the one macro invocation line, and lcov reads two functions at
# one line with different end lines as a contradiction and aborts the capture.
# Measured at the specs/045 phase 13 base on thrawn: seven such records, in
# `test/source/harness_callback_test.cpp`, `test/source/harness_fixture_test.cpp`
# and `example/docs/fixtures.cpp`. The extract roots below keep none of those
# files, so the class moves no figure the gate reads. specs/016 recorded the
# shape and `0eed39c2` carried the class for it.
#
# The other `inconsistent` cause, a contract closure recorded as a hit function
# with no hit contained line, stands repaired at its source, and the class is
# gone from the extract, list and summary reads (specs/045-merge-fixes phase
# 13, ruling G-25). The capture keeps this class until an owner rules on the
# fixture macro shape, because the repair of that shape reaches the public
# macro family in `include/speedgun-ng/benchmark.hpp`.
#
# The count arm of `topology.cpp` that filled the cpu set by index from
# the count when the /sys online line read empty is gone: the
# affinity-mask fallback owns the unreadable line, and the four
# `--omit-lines` patterns that named its lines left the set with the
# fix that replaced the arm. The `cpus.empty()` brace pattern stays:
# the fallback still branches at that line, and lcov names no pattern
# of the set unused.
set(
    COVERAGE_TRACE_COMMAND
    "${LCOV_EXECUTABLE}" -c -q
    --branch-coverage
    --no-external
    --ignore-errors mismatch,negative,inconsistent
    --base-directory "${PROJECT_SOURCE_DIR}"
    --rc geninfo_unexecuted_blocks=1
    -o "${PROJECT_BINARY_DIR}/coverage.raw.info"
    -d "${PROJECT_BINARY_DIR}"
    CACHE STRING
    "; separated command to generate a trace for the 'coverage' target"
    FORCE
)

# Keep application code only. include/* does not match nested
# include/speedgun-ng/*.hpp; extract from the raw trace after capture.
#
# `brace` is a tool rule for a known gcc record: gcov leaves an
# unexecutable count-zero record on the closing brace of a function that
# returns its local directly (the return object is constructed at the
# caller, so no code runs at the brace). The source returns its local
# plainly and this filter removes the false record; no copy is built in
# source to move a coverage count.
# The extract step ignores no error class. The record that once stopped it
# named a contract closure: gcov records a closure written inside an `SG_*`
# predicate at the macro invocation line, the `--omit-lines` pattern below
# removes that line's data, and the written trace then held a hit function
# with no hit contained line. `lcov --list` and `lcov --summary` refused that
# trace, which is where the continuous integration lanes of pull requests #86
# and #87 stopped. The closures stand in named helpers outside the macro now,
# so the pattern removes no line a function record still points at
# (specs/045-merge-fixes phase 13, ruling G-25).
#
# `tools/hostcheck/*` joins the two application roots, because specs/031 puts
# the mapfile matcher and the chooser builder there, outside the library on
# the ruling that keeps the chooser away from the library internals
# (plan R-04). A branch the measure cannot see is a branch no gate reads, so
# the four answers of the chooser stand inside the measure from here. The
# pattern `--extract` keeps is what a pattern names, which leaves
# `tools/dbc/overhead.cpp` and `tools/quill/quill_dependency_check.cpp` out of
# the trace: no test of this suite runs either tool.
#
set(
    COVERAGE_EXTRACT_COMMAND
    "${LCOV_EXECUTABLE}" --branch-coverage
    --filter exception,branch,brace
    --omit-lines "SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?"
    --omit-lines "if \\(cpus\\.empty\\(\\)\\) \\{"
    --extract "${PROJECT_BINARY_DIR}/coverage.raw.info"
    "${PROJECT_SOURCE_DIR}/include/speedgun-ng/*"
    "${PROJECT_SOURCE_DIR}/source/*"
    "${PROJECT_SOURCE_DIR}/tools/hostcheck/*"
    -o "${PROJECT_BINARY_DIR}/coverage.info"
    CACHE STRING
    "; separated command to keep application sources in the coverage trace"
    FORCE
)

set(
    COVERAGE_SUMMARY_COMMAND
    "${CMAKE_COMMAND}" -E env "LCOV=${LCOV_EXECUTABLE}"
    bash "${PROJECT_SOURCE_DIR}/tools/dbc/coverage_gate.sh"
    "${PROJECT_BINARY_DIR}/coverage.info"
    CACHE STRING
    "; separated command to fail the 'coverage' target under 100%"
    FORCE
)

set(
    COVERAGE_HTML_COMMAND
    "${GENHTML_EXECUTABLE}" --legend -f -q --branch-coverage
    "${PROJECT_BINARY_DIR}/coverage.info"
    -p "${PROJECT_SOURCE_DIR}"
    -o "${PROJECT_BINARY_DIR}/coverage_html"
    CACHE STRING
    "; separated command to generate an HTML report for the 'coverage' target"
    FORCE
)

# ---- Coverage target ----

add_custom_target(
    coverage
    COMMAND ${COVERAGE_TRACE_COMMAND}
    COMMAND ${COVERAGE_EXTRACT_COMMAND}
    COMMAND ${LCOV_EXECUTABLE} --branch-coverage
            --list "${PROJECT_BINARY_DIR}/coverage.info"
    COMMAND ${COVERAGE_SUMMARY_COMMAND}
    COMMAND ${COVERAGE_HTML_COMMAND}
    COMMENT "Generating coverage report (100% line and branch required)"
    VERBATIM
)
