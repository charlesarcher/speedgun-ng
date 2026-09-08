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
set(
    COVERAGE_TRACE_COMMAND
    "${LCOV_EXECUTABLE}" -c -q
    --branch-coverage
    --no-external
    --base-directory "${PROJECT_SOURCE_DIR}"
    --rc geninfo_unexecuted_blocks=1
    -o "${PROJECT_BINARY_DIR}/coverage.raw.info"
    -d "${PROJECT_BINARY_DIR}"
    CACHE STRING
    "; separated command to generate a trace for the 'coverage' target"
)

# Keep application code only. include/* does not match nested
# include/speedgun-ng/*.hpp; extract from the raw trace after capture.
set(
    COVERAGE_EXTRACT_COMMAND
    "${LCOV_EXECUTABLE}" --branch-coverage
    --filter exception,branch
    --omit-lines "SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?"
    --extract "${PROJECT_BINARY_DIR}/coverage.raw.info"
    "${PROJECT_SOURCE_DIR}/include/speedgun-ng/*"
    "${PROJECT_SOURCE_DIR}/source/*"
    -o "${PROJECT_BINARY_DIR}/coverage.info"
    CACHE STRING
    "; separated command to keep application sources in the coverage trace"
)

set(
    COVERAGE_SUMMARY_COMMAND
    "${CMAKE_COMMAND}" -E env "LCOV=${LCOV_EXECUTABLE}"
    bash "${PROJECT_SOURCE_DIR}/tools/dbc/coverage_gate.sh"
    "${PROJECT_BINARY_DIR}/coverage.info"
    CACHE STRING
    "; separated command to fail the 'coverage' target under 100%"
)

set(
    COVERAGE_HTML_COMMAND
    "${GENHTML_EXECUTABLE}" --legend -f -q --branch-coverage
    "${PROJECT_BINARY_DIR}/coverage.info"
    -p "${PROJECT_SOURCE_DIR}"
    -o "${PROJECT_BINARY_DIR}/coverage_html"
    CACHE STRING
    "; separated command to generate an HTML report for the 'coverage' target"
)

# ---- Coverage target ----

add_custom_target(
    coverage
    COMMAND ${COVERAGE_TRACE_COMMAND}
    COMMAND ${COVERAGE_EXTRACT_COMMAND}
    COMMAND ${LCOV_EXECUTABLE} --branch-coverage --list
            "${PROJECT_BINARY_DIR}/coverage.info"
    COMMAND ${COVERAGE_SUMMARY_COMMAND}
    COMMAND ${COVERAGE_HTML_COMMAND}
    COMMENT "Generating coverage report (100% line and branch required)"
    VERBATIM
)
