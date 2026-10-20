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

# A docs example is one complete program per main-page section (D-12): the page
# copies its marked regions and `test/example_run.sh` runs it, so the target
# carries the `docs_` prefix and a `run_docs_` runner beside the `run_`
# convention of `add_example` (FR-032, FR-033).
function(add_docs_example NAME)
  add_executable("docs_${NAME}" "docs/${NAME}.cpp")
  target_link_libraries("docs_${NAME}"
                        PRIVATE speedgun-ng::speedgun-ng speedgun-ng::harness)
  target_compile_features("docs_${NAME}" PRIVATE cxx_std_23)
  add_custom_target("run_docs_${NAME}" COMMAND "docs_${NAME}" VERBATIM)
  add_dependencies("run_docs_${NAME}" "docs_${NAME}")
  add_dependencies(run-examples "run_docs_${NAME}")
endfunction()

add_docs_example(run_loop)
add_docs_example(registration)
add_docs_example(argument_families)
add_docs_example(fixtures)
add_docs_example(run_control)
add_docs_example(timing_modes)
add_docs_example(pause_and_resume)
add_docs_example(time_unit)
add_docs_example(counter_metrics)
add_docs_example(skips)
add_docs_example(command_line)
add_docs_example(reading_the_report)
add_docs_example(user_counters)
add_docs_example(throughput)
add_docs_example(labels)
add_docs_example(custom_statistics)
add_docs_example(aggregates_only)
add_docs_example(complexity_fit)
add_docs_example(execution_contexts)
add_docs_example(threads)
add_docs_example(topology_placement)
add_docs_example(percentiles)


add_folders(Example)
