include(cmake/folders.cmake)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

option(BUILD_MCSS_DOCS "Build documentation using Doxygen and m.css" OFF)
if(BUILD_MCSS_DOCS)
  include(cmake/docs.cmake)
endif()

# Coverage tooling is required in developer mode. ENABLE_COVERAGE turns
# on instrumentation (via the coverage preset); the coverage target and
# lcov/genhtml requirement are unconditional. Never skip if missing.
option(ENABLE_COVERAGE "Enable coverage instrumentation (gcov flags)" OFF)
include(cmake/coverage.cmake)

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)
include(cmake/prose-lint-targets.cmake)

# Host record validation (specs/031-host-validation): the writer runs over the
# tree root, the reader reads the record back from the same output root, and a
# status short of zero from either fails the target. The output root stands as
# a cache variable, so a run can point the write at the build tree and leave a
# committed record where it stands (FR-001 to FR-003, FR-034, R-10).
# The two programs the target depends on register in the test directory, so
# the target rides that guard: a developer tree configured with BUILD_TESTING
# off holds no such target, and CMake still generates the tree.
if(BUILD_TESTING)
  set(VALIDATE_OUT
      "${CMAKE_SOURCE_DIR}/validation"
      CACHE PATH "Where the host record is written and where the reader looks")

  add_custom_target(
      validate
      DEPENDS host_record_write host_record_check
      COMMAND "${CMAKE_COMMAND}"
      -D "VALIDATE_ROOT=${CMAKE_SOURCE_DIR}"
      -D "VALIDATE_OUT=${VALIDATE_OUT}"
      -D "VALIDATE_BIN=$<TARGET_FILE_DIR:host_record_write>"
      -P "${CMAKE_SOURCE_DIR}/cmake/validate.cmake"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Writing and checking the host record"
      VERBATIM
  )
endif()

add_folders(Project)
