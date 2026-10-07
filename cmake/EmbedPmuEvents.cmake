# Compiles the vendored event tables into the library (specs/012, US5,
# FR-036). Include this module, then call
# speedgun_embed_pmu_events(<target> <vendored-root>).
#
# The tables reach the library as static data generated at build time, so
# an installed package carries them and the provider performs no run-time
# path lookup for them. The embedding is unconditional: no option turns it
# off, because a package without its tables publishes no counters.

# Resolved at include time, where CMAKE_CURRENT_LIST_DIR names the
# directory holding this module.
set(SPEEDGUN_EMBED_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/embed_pmu_blob.cmake")
set(SPEEDGUN_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

function(speedgun_embed_pmu_events target vendored_root)
  if(NOT IS_DIRECTORY "${vendored_root}/arch/x86")
    message(FATAL_ERROR "no vendored event tree at ${vendored_root}")
  endif()

  set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/pmu_events")
  file(MAKE_DIRECTORY "${gen_dir}")

  # The generator emits one unit per vendored directory beside the
  # registry, so their names are settled here: the tree's own directories
  # decide them.
  file(GLOB arch_dirs "${vendored_root}/arch/x86/*")
  list(SORT arch_dirs)
  set(generated "${gen_dir}/x86_mapfile.cpp" "${gen_dir}/registry.cpp")
  foreach(arch_dir IN LISTS arch_dirs)
    if(NOT IS_DIRECTORY "${arch_dir}")
      continue()
    endif()
    get_filename_component(arch_name "${arch_dir}" NAME)
    # Two vendored directories carry a hyphen and a symbol cannot, so the
    # symbol folds it to an underscore while the path keeps its own name.
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" arch_symbol "${arch_name}")
    list(APPEND generated "${gen_dir}/${arch_symbol}.cpp")
  endforeach()

  add_custom_command(
    OUTPUT ${generated}
    COMMAND ${CMAKE_COMMAND} -DEMBED_ROOT=${vendored_root}
            -DEMBED_OUT=${gen_dir} -P "${SPEEDGUN_EMBED_SCRIPT}"
    DEPENDS "${SPEEDGUN_EMBED_SCRIPT}" ${arch_dirs}
    COMMENT "Embedding the vendored PMU event tables"
    VERBATIM)

  target_sources(${target} PRIVATE ${generated})
  # The generated units reach the private seam header by the same path the
  # hand-written units use, and the generated source is exempt from the
  # lint gates: a tool has no verdict on a file no hand wrote.
  set_source_files_properties(
    ${generated}
    PROPERTIES SKIP_UNITY_BUILD ON SKIP_LINTING ON
               INCLUDE_DIRECTORIES "${SPEEDGUN_SOURCE_ROOT}/source")
  # Alder Lake's embedded literal is longer than the 65536 characters a
  # C++ compiler must accept. Clang promotes that to an error under
  # -Werror. The bytes are generated, so the warning is off for Clang.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(${generated}
      PROPERTIES COMPILE_OPTIONS "-Wno-overlength-strings")
  endif()
endfunction()
