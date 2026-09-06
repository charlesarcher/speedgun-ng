cmake_minimum_required(VERSION 3.14)

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

target_compile_features(speedgun-ng_speedgun-ng PUBLIC cxx_std_17)

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

include(cmake/dev-mode.cmake)
