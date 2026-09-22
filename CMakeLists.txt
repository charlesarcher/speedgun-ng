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

target_link_libraries(speedgun-ng_speedgun-ng PRIVATE hwloc_vendor)

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
