cmake_minimum_required(VERSION 3.20)

include(cmake/prelude.cmake)

project(
    speedgun-ng
    VERSION 0.1.0
    DESCRIPTION "Speedgun Next Generation - a modern C++ benchmarking framework"
    HOMEPAGE_URL "https://github.com/charlesarcher/speedgun-ng"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    speedgun-ng_speedgun-ng
    source/speedgun-ng.cpp
)
add_library(speedgun-ng::speedgun-ng ALIAS speedgun-ng_speedgun-ng)

include(GenerateExportHeader)
generate_export_header(
    speedgun-ng_speedgun-ng
    BASE_NAME speedgun-ng
    EXPORT_FILE_NAME export/speedgun-ng/speedgun-ng_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(speedgun-ng_speedgun-ng PUBLIC SPEEDGUN_NG_STATIC_DEFINE)
endif()

set_target_properties(
    speedgun-ng_speedgun-ng PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME speedgun-ng
    OUTPUT_NAME speedgun-ng
)

target_include_directories(
    speedgun-ng_speedgun-ng ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    speedgun-ng_speedgun-ng SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(speedgun-ng_speedgun-ng PUBLIC cxx_std_23)

# Vendored hwloc ingestion. hwloc is a private internal dependency:
# the link below is PRIVATE and the imported target is never exported,
# so no consumer of speedgun-ng::speedgun-ng can observe it (FR-007).
# Re-pinning the submodule: follow the re-pinning section of README.md
# (checkout the new tag, commit the submodule pointer, bump the
# version assertion in source/hwloc/hwloc_gate.cpp) (FR-006).
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ImportAutotoolsSubmodule)

target_sources(speedgun-ng_speedgun-ng PRIVATE source/hwloc/hwloc_gate.cpp)

import_autotools_submodule(
    NAME hwloc_vendor
    SUBMODULE_DIR external/hwloc
    BOOTSTRAP autogen.sh
    CONFIGURE_ARGS --enable-embedded-mode
        --with-hwloc-symbol-prefix=sg_
        --disable-libxml2 --disable-opencl --disable-cairo
        --disable-cuda --disable-levelzero --disable-nvml
        --disable-rsmi --disable-libudev --disable-pci
        --disable-plugin-dlopen --disable-plugin-ltdl
    ARCHIVE hwloc/libhwloc_embedded.a
    MERGE_INTO speedgun-ng_speedgun-ng
)

# BUILD_INTERFACE keeps the vendored target out of the exported
# package files: the merged members ship inside libspeedgun-ng.a, so
# consumers must never resolve the name hwloc_vendor (privacy A2).
target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:hwloc_vendor>)

# Vendored simdjson ingestion. simdjson is a private internal
# dependency with a native CMake build, ingested through
# add_subdirectory below. A CMake child inherits the parent's compile
# flags, sanitizer and coverage instrumentation, clang-tidy and
# cppcheck; an autotools ExternalProject child inherits none of them.
# The exemptions FR-009 and FR-010 are therefore set explicitly, and
# the function scope keeps them off the rest of the build
# (research R-006). The link added beside this bracket is PRIVATE and
# BUILD_INTERFACE only: consumers of speedgun-ng::speedgun-ng observe
# nothing of simdjson (FR-007). Re-pinning the submodule: follow the
# re-pinning section of README.md (checkout the new tag, commit the
# submodule pointer, bump the version assertion in
# source/simdjson/simdjson_gate.cpp) (FR-006).
function(import_simdjson)
  # FR-002: abort configure with the init command, no fallback path.
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/simdjson/CMakeLists.txt")
    message(
        FATAL_ERROR
        "external/simdjson is uninitialized; run: "
        "git submodule update --init external/simdjson"
    )
  endif()
  # FR-009/FR-010: the vendored compile carries no parent warning
  # set, no warnings-as-errors promotion, no sanitizer or coverage
  # instrumentation, no clang-tidy, no cppcheck. FR-016: hidden
  # symbols baked in at compile time (R-007). FR-007: a static
  # archive in every configuration (R-005). FR-014: simdjson defines
  # no install rule (R-004). SIMDJSON_DEVELOPER_MODE stays unset: a
  # subproject is library-only without it (R-004).
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
  set(CMAKE_CXX_FLAGS_SANITIZE "")
  set(CMAKE_CXX_FLAGS_COVERAGE "")
  set(CMAKE_CXX_CLANG_TIDY "")
  set(CMAKE_CXX_CPPCHECK "")
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  # The shared configuration absorbs the static vendored archive at
  # link time, so every vendored object must be position-independent
  # (SC-004); the property covers all vendored targets and is inert
  # for a static parent.
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_SHARED_LIBS OFF)
  set(SIMDJSON_INSTALL OFF)
  add_subdirectory(
      external/simdjson "${CMAKE_BINARY_DIR}/_simdjson" EXCLUDE_FROM_ALL
  )
  # FR-013/R-008: the wrapper compiles <simdjson.h> with -isystem, so
  # header diagnostics stay out of the strict set while the wrapper's
  # own lines stay gated.
  get_target_property(_simdjson_inc simdjson INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(
      simdjson PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_simdjson_inc}"
  )
endfunction()

import_simdjson()

# The wrapper is the only TU that includes <simdjson.h> (FR-013); its
# static_assert is the version tripwire and its link-proof reference
# is the object-level proof (FR-003, FR-007). BUILD_INTERFACE keeps
# the vendored name out of the exported package files: the merged
# members ship inside libspeedgun-ng.a, so consumers must never
# resolve the name simdjson::simdjson (privacy A2).
target_sources(speedgun-ng_speedgun-ng PRIVATE source/simdjson/simdjson_gate.cpp)
target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>)

include(cmake/dbc.cmake)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${speedgun-ng_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT speedgun-ng_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of speedgun-ng"
  )
endif()

include(cmake/dbc-gate.cmake)
include(cmake/dev-mode.cmake)
