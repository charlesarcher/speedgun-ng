# ---- vendored static archive merge ----
#
# Contract: specs/005-vendor-hdrhistogram (research R-006, which
# generalizes the simdjson merge mechanism of specs/004 R-009)
#
# This module owns the post-build archiver mechanism. It lives under
# cmake/ because the root CMakeLists.txt call site is held to the
# call-site prohibition list of specs/003 (hwloc_purity_scan SC-010):
# the archiver and add_custom_command belong in a module, beside the
# module that owns the same mechanism for hwloc.
#
# Usage:
#
#   include(cmake/VendoredArchiveMerge.cmake)
#   vendored_archive_merge(<static-library-target> <archived-target>...)
#
# A static archive never absorbs the members of another archive at
# creation, so a post-build archiver step merges each named vendored
# archive into the target archive: the installed static library is
# self-contained, gate references resolve in-archive, and the package
# files name nothing foreign. Shared configurations absorb the
# PRIVATE links at link time, so the step registers only for static
# library targets.
#
# Ordering: the PRIVATE link edges build each vendored archive before
# the library target links, and a POST_BUILD step runs after the
# link, so every archived target exists when the merge reads it (the
# reasoning mirrors ImportAutotoolsSubmodule).

function(vendored_archive_merge merge_into)
  if(NOT TARGET ${merge_into})
    message(
        FATAL_ERROR
        "vendored_archive_merge: target ${merge_into} does not exist"
    )
  endif()
  get_target_property(_vam_merge_type ${merge_into} TYPE)
  if(NOT _vam_merge_type STREQUAL "STATIC_LIBRARY")
    message(
        STATUS
        "vendored_archive_merge: ${merge_into} is ${_vam_merge_type}; "
        "the archive merge applies to static libraries and is skipped"
    )
    return()
  endif()

  # The merge runs in a clean temp directory under simple file names:
  # GNU ar tokenizes MRI script paths at '=', so a build tree whose
  # path contains '=' (scratch cell dirs such as NDEBUG=on_-O0)
  # breaks a direct CREATE/ADDLIB line. Copying the archives under
  # /tmp keeps every member and behaves the same under GNU ar and
  # llvm-ar (current macOS): CREATE a fresh archive, ADDLIB the
  # target's own members first, ADDLIB each archived target in
  # argument order, SAVE. A hash of the target path isolates
  # concurrent merges of different targets; ranlib refreshes the
  # index.
  #
  # The step script is generated per merge call: keyed by the
  # merge-into target and the archived-target list, so two merge
  # calls naming the same library (simdjson's wrapper call and a
  # second call for the remaining vendored archives) never generate
  # to one path, while repeat calls with an identical signature reuse
  # one generated file.
  set(_vam_step_dir "${CMAKE_BINARY_DIR}/_vendored-merge")
  file(MAKE_DIRECTORY "${_vam_step_dir}")
  string(SHA1 _vam_call_sha "${merge_into}|${ARGN}")
  string(SUBSTRING "${_vam_call_sha}" 0 8 _vam_call_sha8)
  set(_vam_merge_script
      "${_vam_step_dir}/${merge_into}-${_vam_call_sha8}-step.cmake"
  )

  # One copy-and-ADDLIB block per archived target, in argument
  # order. The generator expressions resolve per configuration at
  # generate time; the -P script below sees plain paths.
  set(_vam_vendored_blocks "")
  set(_vam_index 0)
  foreach(_vam_archived ${ARGN})
    string(APPEND _vam_vendored_blocks
        "set(_vam_vendored_${_vam_index} \"$<TARGET_FILE:${_vam_archived}>\")
file(COPY \"\${_vam_vendored_${_vam_index}}\" DESTINATION \"\${_vam_dir}\")
get_filename_component(_vam_vendored_name_${_vam_index} \"\${_vam_vendored_${_vam_index}}\" NAME)
file(APPEND \"\${_vam_dir}/merge.mri\"
  \"ADDLIB \${_vam_dir}/\${_vam_vendored_name_${_vam_index}}
\")
")
    math(EXPR _vam_index "${_vam_index} + 1")
  endforeach()

  file(GENERATE
      OUTPUT "${_vam_merge_script}"
      CONTENT
      "set(_vam_target \"$<TARGET_FILE:${merge_into}>\")
set(_vam_ar \"${CMAKE_AR}\")
set(_vam_ranlib \"${CMAKE_RANLIB}\")
string(SHA1 _vam_sha \"\${_vam_target}\")
string(SUBSTRING \"\${_vam_sha}\" 0 8 _vam_sha8)
set(_vam_dir \"/tmp/vam-merge-\${_vam_sha8}\")
file(REMOVE_RECURSE \"\${_vam_dir}\")
file(MAKE_DIRECTORY \"\${_vam_dir}\")
file(COPY \"\${_vam_target}\" DESTINATION \"\${_vam_dir}\")
get_filename_component(_vam_target_name \"\${_vam_target}\" NAME)
file(WRITE \"\${_vam_dir}/merge.mri\"
  \"CREATE \${_vam_dir}/out.a
ADDLIB \${_vam_dir}/\${_vam_target_name}
\")
${_vam_vendored_blocks}file(APPEND \"\${_vam_dir}/merge.mri\"
  \"SAVE
END
\")
execute_process(COMMAND \"\${_vam_ar}\" -M
    INPUT_FILE \"\${_vam_dir}/merge.mri\"
    OUTPUT_QUIET
    RESULT_VARIABLE _vam_rc
    ERROR_VARIABLE _vam_err)
if(NOT _vam_rc EQUAL 0)
  message(FATAL_ERROR
    \"vendored_archive_merge: archive merge failed for \${_vam_target}: \${_vam_err}\")
endif()
execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy
    \"\${_vam_dir}/out.a\" \"\${_vam_target}\"
    RESULT_VARIABLE _vam_cp_rc
    ERROR_VARIABLE _vam_cp_err)
if(NOT _vam_cp_rc EQUAL 0)
  message(FATAL_ERROR
    \"vendored_archive_merge: could not move the merged archive into place: \${_vam_cp_err}\")
endif()
if(_vam_ranlib)
  execute_process(COMMAND \"\${_vam_ranlib}\" \"\${_vam_target}\")
else()
  execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E touch \"\${_vam_target}\")
endif()
file(REMOVE_RECURSE \"\${_vam_dir}\")
")
  add_custom_command(
      TARGET ${merge_into} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -P "${_vam_merge_script}"
      COMMENT
          "Merging vendored archives into ${merge_into}"
      VERBATIM
  )
endfunction()
