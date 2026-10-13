# sg-example:begin package
cmake_minimum_required(VERSION 3.20)

project(speedgun-ngQuickStart CXX)

find_package(speedgun-ng REQUIRED)

add_executable(first_benchmark first_benchmark.cpp)
target_link_libraries(first_benchmark
                      PRIVATE speedgun-ng::speedgun-ng speedgun-ng::harness)
# sg-example:end package
