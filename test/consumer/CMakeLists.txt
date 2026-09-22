# Downstream consumer test project (FR-026, SC-005).
# Driven by the downstream-consumer CI job: install speedgun-ng to a
# prefix, configure this project against it with CMAKE_PREFIX_PATH,
# build, run, then grep the configure and build logs for the vendored
# topology library. This project must never name that dependency;
# that absence is the point.

cmake_minimum_required(VERSION 3.20)

project(speedgun-ng-consumer LANGUAGES CXX)

find_package(speedgun-ng REQUIRED)

add_executable(consumer main.cpp)

target_link_libraries(consumer PRIVATE speedgun-ng::speedgun-ng)

target_compile_features(consumer PRIVATE cxx_std_23)
