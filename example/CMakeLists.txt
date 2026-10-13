cmake_minimum_required(VERSION 3.20)

project(speedgun-ngExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(speedgun-ng REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_link_libraries("${NAME}" PRIVATE speedgun-ng::speedgun-ng)
  target_compile_features("${NAME}" PRIVATE cxx_std_23)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(empty_example)
add_example(counters_giraffe_example)
add_example(counters_standalone_example)

# The harness example is one speedgun executable (D-1): it links the
# harness archive beside the counters archive (T018, FR-033).
add_example(benchmark_example)
target_link_libraries(benchmark_example PRIVATE speedgun-ng::harness)

# The quick start program the root page quotes in full.
add_example(first_benchmark)
target_link_libraries(first_benchmark PRIVATE speedgun-ng::harness)

# The capability snippets the page quotes section by section build, so a
# block that drifted into a fragment fails in the build. The target stays
# out of `run-examples` and carries no `run_` target of its own: its entry
# point returns without entering the runner, so the binary prints no
# report and no page region shows any mechanism for that.
add_executable(docs_snippets docs_snippets.cpp)
target_link_libraries(docs_snippets
                      PRIVATE speedgun-ng::speedgun-ng speedgun-ng::harness)
target_compile_features(docs_snippets PRIVATE cxx_std_23)

add_folders(Example)
