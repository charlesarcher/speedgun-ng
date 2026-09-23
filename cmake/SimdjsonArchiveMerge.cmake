# ---- simdjson static archive merge ----
#
# Contract: specs/004-vendor-simdjson/contracts/build-integration.md
# Research: specs/004-vendor-simdjson/research.md (R-009)
#
# This module lives under cmake/ because the root CMakeLists.txt call
# site is held to the call-site prohibition list of specs/003
# (hwloc_purity_scan SC-010): the archiver and add_custom_command
# belong here, beside the module that owns the same mechanism for
# hwloc.
#
# Usage:
#
#   include(cmake/SimdjsonArchiveMerge.cmake)
#   simdjson_archive_merge(<static-library-target>)
#
# A static archive never absorbs the members of another archive at
# creation, so a post-build archiver step merges the vendored
# libsimdjson.a into libspeedgun-ng.a: the installed static library
# is self-contained, the gate's get_active_implementation reference
# resolves in-archive (SC-009), and the package files name nothing
# foreign (FR-015). Shared configurations absorb the PRIVATE link at
# link time, so the step registers only for static library targets.
#
# Ordering: the PRIVATE link edge builds the simdjson archive before
# the library target links, and a POST_BUILD step runs after the
# link, so the vendored archive exists when the merge reads it
# (FR-015 reasoning mirrors ImportAutotoolsSubmodule).

function(simdjson_archive_merge merge_into)
  if(NOT TARGET ${merge_into})
    message(
        FATAL_ERROR
        "simdjson_archive_merge: target ${merge_into} does not exist"
    )
  endif()
  get_target_property(_sj_merge_type ${merge_into} TYPE)
  if(NOT _sj_merge_type STREQUAL "STATIC_LIBRARY")
    message(
        STATUS
        "simdjson_archive_merge: ${merge_into} is ${_sj_merge_type}; "
        "the archive merge applies to static libraries and is skipped"
    )
    return()
  endif()

  # The merge runs inside a clean temp directory: GNU ar tokenizes
  # MRI script paths at '=', so a build tree whose path contains '='
  # (scratch cell dirs such as NDEBUG=on_-O0) breaks a direct
  # CREATE/ADDLIB line. Copying both archives to simple names under
  # /tmp keeps every member and behaves the same under GNU ar and
  # llvm-ar (current macOS): CREATE a fresh archive, ADDLIB the
  # target's own members first, ADDLIB the vendored archive, SAVE.
  # A hash of the target path isolates concurrent merges of
  # different targets; ranlib refreshes the index.
  set(_sj_merge_script "${CMAKE_BINARY_DIR}/_simdjson/merge-step.cmake")
  file(GENERATE
      OUTPUT "${_sj_merge_script}"
      CONTENT
      "set(_sj_target \"$<TARGET_FILE:${merge_into}>\")
set(_sj_vendored \"$<TARGET_FILE:simdjson>\")
set(_sj_ar \"${CMAKE_AR}\")
set(_sj_ranlib \"${CMAKE_RANLIB}\")
string(SHA1 _sj_sha \"\${_sj_target}\")
string(SUBSTRING \"\${_sj_sha}\" 0 8 _sj_sha8)
set(_sj_dir \"/tmp/sj-merge-\${_sj_sha8}\")
file(REMOVE_RECURSE \"\${_sj_dir}\")
file(MAKE_DIRECTORY \"\${_sj_dir}\")
file(COPY \"\${_sj_target}\" DESTINATION \"\${_sj_dir}\")
file(COPY \"\${_sj_vendored}\" DESTINATION \"\${_sj_dir}\")
get_filename_component(_sj_target_name \"\${_sj_target}\" NAME)
get_filename_component(_sj_vendored_name \"\${_sj_vendored}\" NAME)
file(WRITE \"\${_sj_dir}/merge.mri\"
  \"CREATE \${_sj_dir}/out.a
ADDLIB \${_sj_dir}/\${_sj_target_name}
ADDLIB \${_sj_dir}/\${_sj_vendored_name}
SAVE
END
\")
execute_process(COMMAND \"\${_sj_ar}\" -M
    INPUT_FILE \"\${_sj_dir}/merge.mri\"
    OUTPUT_QUIET
    RESULT_VARIABLE _sj_rc
    ERROR_VARIABLE _sj_err)
if(NOT _sj_rc EQUAL 0)
  message(FATAL_ERROR
    \"simdjson_archive_merge: archive merge failed for \${_sj_target}: \${_sj_err}\")
endif()
execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy
    \"\${_sj_dir}/out.a\" \"\${_sj_target}\"
    RESULT_VARIABLE _sj_cp_rc
    ERROR_VARIABLE _sj_cp_err)
if(NOT _sj_cp_rc EQUAL 0)
  message(FATAL_ERROR
    \"simdjson_archive_merge: could not move the merged archive into place: \${_sj_cp_err}\")
endif()
if(_sj_ranlib)
  execute_process(COMMAND \"\${_sj_ranlib}\" \"\${_sj_target}\")
else()
  execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E touch \"\${_sj_target}\")
endif()
file(REMOVE_RECURSE \"\${_sj_dir}\")
")
  add_custom_command(
      TARGET ${merge_into} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -P "${_sj_merge_script}"
      COMMENT
          "Merging simdjson members into ${merge_into}"
      VERBATIM
  )
endfunction()
