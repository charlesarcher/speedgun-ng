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
# The capture ignores no error class. It reads the three application roots
# the extract step below keeps, and no other path. The `mismatch` class it
# once ignored came from vendored headers alone: the inline code of
# yaml-cpp, simdjson and quill is emitted into this project's own
# translation units, and lcov saw one header line with an exception tag in
# one unit and none in another. No such header is part of any gated figure,
# because the extract step always removed every vendored path, so the
# capture now leaves those paths out at the read, ahead of any merge. The
# `negative` class named a gcov counter lcov misread; no capture of the
# coverage job reports one, so it is gone with `mismatch`.
# The `inconsistent` class stands gone from every read here. Two
# shapes produced it. A contract closure written inside an `SG_*` predicate
# was recorded as a hit function with no hit contained line, and each such
# closure body now stands in a named helper outside the macro.
# `SG_BENCHMARK_F` and its siblings registered the case callable and the
# fixture factory with two lambdas written at the macro invocation line,
# which is also the line of the header of the method the site then defines,
# so lcov read two functions at one line with two different end lines and
# aborted the capture; the macros now pass the named entry functions
# `::sg::detail::fixtureCaseEntry` and `::sg::detail::fixtureFactoryEntry`,
# and each record keeps its own line. specs/016 recorded the second shape
# and `0eed39c2` carried the class for it; this phase removed the shape and
# the class together (specs/045-merge-fixes phase 13, ruling G-25).
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
    --include "${PROJECT_SOURCE_DIR}/include/speedgun-ng/*"
    --include "${PROJECT_SOURCE_DIR}/source/*"
    --include "${PROJECT_SOURCE_DIR}/tools/hostcheck/*"
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
