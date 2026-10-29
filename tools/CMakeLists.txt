cmake_minimum_required(VERSION 3.20)

project(speedgun-ngTools LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(speedgun-ng REQUIRED)
endif()

# The report view driver (specs/038-report-view-generators FR-014, D-2):
# one thin program over the library surface, linking the harness the
# view functions compile into.
add_executable(report_view report_view.cpp)
target_link_libraries(report_view PRIVATE speedgun-ng::harness)
target_compile_features(report_view PRIVATE cxx_std_23)

add_folders(Tools)
