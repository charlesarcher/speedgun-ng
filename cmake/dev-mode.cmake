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

add_folders(Project)
