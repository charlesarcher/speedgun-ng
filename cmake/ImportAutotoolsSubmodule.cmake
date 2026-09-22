# ---- Autotools submodule ingestion module ----
#
# Contract: specs/003-vendor-hwloc/contracts/ingestion-module.md
# Research: specs/003-vendor-hwloc/research.md (R-001, R-004, R-006,
# R-007, R-009, R-010, R-011, R-014, R-015)
#
# Usage:
#
#   include(ImportAutotoolsSubmodule)
#   import_autotools_submodule(
#       NAME <imported-target-name>
#       SUBMODULE_DIR <repo-relative-vendored-path>
#       BOOTSTRAP autogen.sh | PREGENERATED
#       CONFIGURE_ARGS <arg> [<arg> ...]
#       ARCHIVE <archive-path-relative-to-vendor-build-dir>
#       [MERGE_INTO <static-library-target>]
#   )
#
# Input table (contract section 1):
#
#   NAME           identifier, required. Name of the imported target
#                  produced; must not collide with an existing target.
#   SUBMODULE_DIR  path, required. Must exist and be non-empty at
#                  configure time; a missing or empty tree aborts
#                  naming the git submodule update init command.
#   BOOTSTRAP      autogen.sh or PREGENERATED, required. autogen.sh
#                  runs the script in the copied tree before configure
#                  (the git checkout case: no pre-generated
#                  configure). PREGENERATED configures directly and
#                  demands no autoconf-family host tools beyond make
#                  and sh.
#   CONFIGURE_ARGS list, required. Passed verbatim to configure after
#                  the module's fixed inputs (prefix, compiler
#                  selection, curated flags).
#   ARCHIVE        relative path, required. The archive the module
#                  must find after make; absence fails the build
#                  naming the path.
#   MERGE_INTO     target, optional. When given and that target is a
#                  static library, a post-build step merges the staged
#                  archive members into it (research R-010). Ignored
#                  for shared configurations.
#
# Guarantees (contract section 2), one line each:
#
# * Correct build ordering for the consumer: the imported target
#   carries an internal dependency edge on the external project, and
#   BUILD_BYPRODUCTS names the archive for Ninja (FR-013, FR-015).
# * Pristine submodule worktree: bootstrap runs in a build-tree copy;
#   there is no download, update, patch or git step against the
#   source tree; the copy is the only thing the child writes (FR-014,
#   SC-007).
# * The configure step does not re-run per build:
#   CONFIGURE_HANDLED_BY_BUILD ON (CMake 3.20 floor, FR-015).
# * Input changes force a full vendor rebuild: the vendor prefix
#   directory is keyed on a SHA1 of the C compiler, its version
#   output, CMAKE_BUILD_TYPE, the sanitizer-flag state, and
#   CONFIGURE_ARGS; a change redirects to a fresh prefix (FR-016,
#   SC-008).
# * Curated child flags: CC and CXX explicit; child CFLAGS is the
#   build-type pair plus hidden visibility; zero sanitizer flags, zero
#   parent warning sets, zero warnings-as-errors (FR-010, R-008).
# * Private prefix only: staging lives under the build tree; the
#   module defines no install rule; CMAKE_INSTALL_PREFIX receives
#   nothing (FR-017, SC-002).
# * Early, named toolchain failure: configure-time probes locate
#   make, sh and (autogen.sh case) autoconf, automake,
#   patch and the libtool alias set; a miss aborts listing the tools
#   and the apt-get, dnf and brew package sets (FR-018, R-011).
# * Archive contents independent of host packages: the caller's
#   disable list plus embedded mode keep feature detection out
#   (FR-020).
# * Platform confinement: shared logic first, then per-platform
#   blocks; POSIX implemented; unsupported platforms abort pointing
#   at the upstream contrib/windows-cmake/ on-ramp (FR-019, R-014).
# * Symbol prefix consistency: the configure-time prefix flows into
#   hwloc's generated public config header, which this module stages
#   with the public headers, so every TU including hwloc.h references
#   the renamed symbols automatically (FR-025, R-003).
#
# Sanitizer exclusion policy (FR-010): the vendored C archive builds
# without sanitizer instrumentation on every preset. Rationale:
# hwloc's C code sits outside the project's defect surface, and
# instrumented callers still get out-of-bounds detection on
# hwloc-allocated memory (spec clarification 2026-09-20). The
# exclusion is structural: ExternalProject children inherit no parent
# CMAKE_*_FLAGS; the child sees only the curated CFLAGS this module
# hands configure, and that string never carries a sanitize flag.
# Hidden visibility (appended always) keeps merged hwloc objects out
# of any speedgun-ng shared library dynamic table (FR-024).

include_guard(GLOBAL)
include(ExternalProject)

function(import_autotools_submodule)
  # ---- Shared logic: argument parse and validation (FR-019) ----

  cmake_parse_arguments(IAS ""
      "NAME;SUBMODULE_DIR;BOOTSTRAP;ARCHIVE;MERGE_INTO"
      "CONFIGURE_ARGS" ${ARGN})

  if(IAS_UNPARSED_ARGUMENTS)
    message(
        FATAL_ERROR
        "import_autotools_submodule: unknown arguments: "
        "${IAS_UNPARSED_ARGUMENTS}"
    )
  endif()

  foreach(_ias_required NAME SUBMODULE_DIR BOOTSTRAP CONFIGURE_ARGS ARCHIVE)
    if(NOT IAS_${_ias_required})
      message(
          FATAL_ERROR
          "import_autotools_submodule: ${_ias_required} is a required "
          "argument (contract: ingestion-module.md section 1)"
      )
    endif()
  endforeach()

  if(NOT IAS_BOOTSTRAP STREQUAL "autogen.sh"
     AND NOT IAS_BOOTSTRAP STREQUAL "PREGENERATED")
    message(
        FATAL_ERROR
        "import_autotools_submodule(${IAS_NAME}): BOOTSTRAP must be "
        "exactly autogen.sh or PREGENERATED, got '${IAS_BOOTSTRAP}'"
    )
  endif()

  if(TARGET ${IAS_NAME})
    message(
        FATAL_ERROR
        "import_autotools_submodule(${IAS_NAME}): target ${IAS_NAME} "
        "already exists; the imported target name must not collide"
    )
  endif()

  if(WIN32)
    # ---- Platform block: Windows, unsupported (FR-019, R-014) ----
    message(
        FATAL_ERROR
        "import_autotools_submodule(${IAS_NAME}): this module supports "
        "Linux and macOS only. The documented Windows on-ramp is the "
        "upstream contrib/windows-cmake/ wrapper at the vendored tag."
    )
  elseif(UNIX OR APPLE)
    # ---- Platform block: POSIX, implemented (FR-019) ----

    # (a) Guard: the vendored tree must exist and be non-empty (FR-002)
    set(_ias_sub_dir "${IAS_SUBMODULE_DIR}")
    if(NOT IS_ABSOLUTE "${_ias_sub_dir}")
      set(_ias_sub_dir "${CMAKE_SOURCE_DIR}/${_ias_sub_dir}")
    endif()
    file(GLOB _ias_entries "${_ias_sub_dir}/*" "${_ias_sub_dir}/.*")
    if(NOT IS_DIRECTORY "${_ias_sub_dir}" OR _ias_entries STREQUAL "")
      message(
          FATAL_ERROR
          "import_autotools_submodule(${IAS_NAME}): the vendored tree "
          "'${IAS_SUBMODULE_DIR}' is missing or empty. Run: "
          "git submodule update --init ${IAS_SUBMODULE_DIR}"
      )
    endif()

    # (b) Toolchain probe (FR-018, R-011). PREGENERATED demands make
    # and sh only; autogen.sh also demands the autoreconf family.
    find_program(IAS_PROG_MAKE make)
    find_program(IAS_PROG_SH sh)
    if(IAS_BOOTSTRAP STREQUAL "autogen.sh")
      find_program(IAS_PROG_AUTOCONF autoconf)
      find_program(IAS_PROG_AUTOMAKE automake)
      find_program(IAS_PROG_PATCH patch)
      # Homebrew names the libtool programs glibtool and glibtoolize;
      # autoreconf finds either alias.
      find_program(IAS_PROG_LIBTOOL
          NAMES libtoolize glibtoolize libtool glibtool)
    endif()

    set(_ias_missing "")
    if(NOT IAS_PROG_MAKE)
      list(APPEND _ias_missing make)
    endif()
    if(NOT IAS_PROG_SH)
      list(APPEND _ias_missing sh)
    endif()
    if(IAS_BOOTSTRAP STREQUAL "autogen.sh")
      if(NOT IAS_PROG_AUTOCONF)
        list(APPEND _ias_missing autoconf)
      endif()
      if(NOT IAS_PROG_AUTOMAKE)
        list(APPEND _ias_missing automake)
      endif()
      if(NOT IAS_PROG_PATCH)
        list(APPEND _ias_missing patch)
      endif()
      if(NOT IAS_PROG_LIBTOOL)
        list(APPEND _ias_missing libtool)
      endif()
    endif()
    if(_ias_missing)
      message(
          FATAL_ERROR
          "import_autotools_submodule(${IAS_NAME}): host tools "
          "missing: ${_ias_missing}. Install them. Debian-family: "
          "apt-get install autoconf automake libtool patch. "
          "RPM-family: dnf install autoconf automake libtool patch. "
          "macOS: brew install autoconf automake libtool (BSD patch "
          "ships with macOS)."
      )
    endif()

    # The merge step (f) drives the archiver directly (R-010).
    if(IAS_MERGE_INTO AND NOT CMAKE_AR)
      message(
          FATAL_ERROR
          "import_autotools_submodule(${IAS_NAME}): MERGE_INTO needs "
          "an archiver, CMAKE_AR is empty; enable the C or CXX "
          "language"
      )
    endif()

    # (c) Hash-keyed private prefix (FR-016, R-006)
    #
    # The C language lets CMake detect the compiler this module hands
    # to configure. enable_language is idempotent; the enclosing project
    # declares CXX only.
    enable_language(C)
    if(NOT CMAKE_C_COMPILER)
      message(
          FATAL_ERROR
          "import_autotools_submodule(${IAS_NAME}): "
          "CMAKE_C_COMPILER is empty after enable_language(C)"
      )
    endif()
    execute_process(
        COMMAND "${CMAKE_C_COMPILER}" --version
        OUTPUT_VARIABLE _ias_cc_version
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX REPLACE "[ \t\r\n]+" "" _ias_cc_version "${_ias_cc_version}")

    # Sanitizer state: whether any parent C/CXX flag string, the
    # config-specific one included, carries -fsanitize. The child
    # never receives these flags; the state only keys the prefix
    # hash, so a sanitizer toggle lands in a fresh prefix.
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _ias_build_type_upper)
    string(JOIN " " _ias_parent_flags
        "${CMAKE_C_FLAGS}" "${CMAKE_CXX_FLAGS}"
        "${CMAKE_C_FLAGS_${_ias_build_type_upper}}")
    if(_ias_parent_flags MATCHES "-fsanitize")
      set(_ias_sanitizer_state "ON")
    else()
      set(_ias_sanitizer_state "OFF")
    endif()

    string(JOIN " " _ias_args_joined "${IAS_CONFIGURE_ARGS}")
    string(JOIN "|" _ias_hash_input
        "${CMAKE_C_COMPILER}" "${_ias_cc_version}"
        "${CMAKE_BUILD_TYPE}" "${_ias_sanitizer_state}"
        "${_ias_args_joined}")
    string(SHA1 _ias_hash "${_ias_hash_input}")
    string(SUBSTRING "${_ias_hash}" 0 8 _ias_hash8)

    # Everything vendor-side lives under this hash-keyed root in the
    # build tree: src (copy), build (out-of-tree), stage (the private
    # prefix). Nothing ever touches CMAKE_INSTALL_PREFIX (FR-017).
    set(_ias_prefix "${CMAKE_BINARY_DIR}/${IAS_NAME}-prefix-${_ias_hash8}")
    set(_ias_src_dir "${_ias_prefix}/src")
    set(_ias_build_dir "${_ias_prefix}/build")
    set(_ias_stage_dir "${_ias_prefix}/stage")

    # Curated child CFLAGS (FR-010, R-008).
    #
    # Policy: the child gets exactly the build-type optimization and
    # debug pair below, plus hidden visibility. Never a sanitizer
    # flag, never the parent warning sets, never warnings-as-errors.
    # ExternalProject children inherit no parent CMAKE_*_FLAGS, so
    # this string is the whole child flag surface. Hidden visibility
    # keeps merged hwloc objects out of any speedgun-ng shared
    # library dynamic table (FR-024).
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(_ias_cflags "-g")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
      set(_ias_cflags "-O2 -DNDEBUG")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
      set(_ias_cflags "-Og -g")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
      set(_ias_cflags "-O2 -g")
    else()
      set(_ias_cflags "")
    endif()
    string(STRIP "${_ias_cflags} -fvisibility=hidden" _ias_cflags)

    # CC and CXX explicit: the child compiler is the parent's, the
    # child flags are the curated set above (R-008, R-015).
    set(_ias_child_env "CC=${CMAKE_C_COMPILER}")
    if(CMAKE_CXX_COMPILER)
      list(APPEND _ias_child_env "CXX=${CMAKE_CXX_COMPILER}")
    endif()
    list(APPEND _ias_child_env "CFLAGS=${_ias_cflags}")

    get_filename_component(_ias_archive_name "${IAS_ARCHIVE}" NAME)
    set(_ias_build_archive "${_ias_build_dir}/${IAS_ARCHIVE}")
    set(_ias_staged_archive "${_ias_stage_dir}/lib/${_ias_archive_name}")

    # (d) External project: copy, bootstrap, configure, build, stage.
    #
    # No URL, no git step, no update, no patch: the submodule is the
    # only source and the bootstrap writes only into the copy (R-007,
    # SC-007). BUILD_COMMAND names the probed make: under the Ninja
    # generator CMAKE_MAKE_PROGRAM is ninja, which cannot drive an
    # autotools Makefile.
    #
    # One sh -c carries the whole bootstrap-and-configure sequence:
    # ExternalProject runs a single command per step, and out-of-tree
    # configure needs the cwd moved after autogen. CONFIGURE_ARGS carry
    # no spaces (hwloc flags), so the space-joined string is exact.
    # CMake has no CONFIGURE_COMMAND_ENV keyword: the child environment
    # rides as shell exports at the head of the same sh -c string.
    set(_ias_env_exports "")
    foreach(_ias_env ${_ias_child_env})
      string(APPEND _ias_env_exports "export \"${_ias_env}\" && ")
    endforeach()
    string(JOIN " " _ias_args_str ${IAS_CONFIGURE_ARGS})
    if(IAS_BOOTSTRAP STREQUAL "autogen.sh")
      set(_ias_conf_script
          "${_ias_env_exports}cd \"${_ias_src_dir}\" && ./autogen.sh"
          "&& cd \"${_ias_build_dir}\""
          "&& sh \"${_ias_src_dir}/configure\" --prefix=\"${_ias_stage_dir}\" ${_ias_args_str}")
    else()
      set(_ias_conf_script
          "${_ias_env_exports}cd \"${_ias_build_dir}\""
          "&& sh \"${_ias_src_dir}/configure\" --prefix=\"${_ias_stage_dir}\" ${_ias_args_str}")
    endif()
    string(JOIN " " _ias_conf_script ${_ias_conf_script})

    # Staging script: verify the artifact, then populate the private
    # prefix (lib archive, public headers, generated config header
    # tree from the out-of-tree build). copy_directory merges, so the
    # generated include/hwloc/autogen/config.h lands beside the
    # public headers (R-004, FR-025). Absence of the archive aborts
    # naming the expected path and the vendor prefix (contract
    # failure mode 3). libtool leaves the real archive for a
    # convenience library in the .libs/ sibling of the .la, so the
    # check accepts the named path or that .libs/ location.
    set(_ias_stage_script "${_ias_prefix}/stage-script.cmake")
    get_filename_component(_ias_archive_dir "${IAS_ARCHIVE}" DIRECTORY)
    set(_ias_libs_archive
        "${_ias_build_dir}/${_ias_archive_dir}/.libs/${_ias_archive_name}")
    file(GENERATE
        OUTPUT "${_ias_stage_script}"
        CONTENT
        "set(_ias_archive \"${_ias_build_archive}\")
if(NOT EXISTS \"\${_ias_archive}\")
  if(EXISTS \"${_ias_libs_archive}\")
    set(_ias_archive \"${_ias_libs_archive}\")
  else()
    message(FATAL_ERROR
      \"import_autotools_submodule(${IAS_NAME}): expected archive not found at ${_ias_build_archive} or ${_ias_libs_archive}. Inspect the vendor prefix ${_ias_prefix} for the failed vendor build.\")
  endif()
endif()
file(MAKE_DIRECTORY \"${_ias_stage_dir}/lib\" \"${_ias_stage_dir}/include\")
file(COPY \"\${_ias_archive}\" DESTINATION \"${_ias_stage_dir}/lib\")
execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy_directory
  \"${_ias_src_dir}/include\" \"${_ias_stage_dir}/include\"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy_directory
  \"${_ias_build_dir}/include\" \"${_ias_stage_dir}/include\"
  COMMAND_ERROR_IS_FATAL ANY)
")

    ExternalProject_Add(
        ${IAS_NAME}-ep
        SOURCE_DIR "${_ias_src_dir}"
        BINARY_DIR "${_ias_build_dir}"
        PREFIX "${_ias_stage_dir}"
        TMP_DIR "${_ias_prefix}/tmp"
        STAMP_DIR "${_ias_prefix}/stamp"
        DOWNLOAD_COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${_ias_sub_dir}" "${_ias_src_dir}"
        DOWNLOAD_NO_PROGRESS 1
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        CONFIGURE_COMMAND "${IAS_PROG_SH}" -c "${_ias_conf_script}"
        CONFIGURE_HANDLED_BY_BUILD ON
        BUILD_COMMAND "${IAS_PROG_MAKE}"
        INSTALL_COMMAND
            "${CMAKE_COMMAND}" -P "${_ias_stage_script}"
        BUILD_BYPRODUCTS
            "${_ias_staged_archive}"
            "${_ias_build_archive}"
    )

    # (e) Imported target (FR-013, R-009).
    #
    # dl and thread libraries: hwloc core references dlopen/dlsym
    # probing and pthread primitives on Linux (R-009).
    # INTERFACE_SYSTEM_INCLUDE_DIRECTORIES makes the staged headers
    # arrive SYSTEM at consumers (R-013).
    find_package(Threads REQUIRED)
    set_target_properties(Threads::Threads PROPERTIES IMPORTED_GLOBAL TRUE)

    add_library(${IAS_NAME} STATIC IMPORTED GLOBAL)
    # Imported-target interface directories must exist at generate time;
    # the staging step populates them later (contract section 2).
    file(MAKE_DIRECTORY "${_ias_stage_dir}/include")
    set_target_properties(
        ${IAS_NAME}
        PROPERTIES
        IMPORTED_LOCATION "${_ias_staged_archive}"
        INTERFACE_INCLUDE_DIRECTORIES "${_ias_stage_dir}/include"
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_ias_stage_dir}/include"
        INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS};Threads::Threads"
    )
    add_dependencies(${IAS_NAME} ${IAS_NAME}-ep)

    # (f) MERGE_INTO: static archive slurp (R-010).
    #
    # Static archives never absorb members of other archives at
    # creation; the archiver MRI ADDLIB step is the upstream slurping
    # pattern. The merged archive is self-contained, so package files
    # carry zero hwloc (FR-023) while nm finds the members (SC-011).
    # Ignored for shared configurations (contract).
    if(IAS_MERGE_INTO)
      if(NOT TARGET ${IAS_MERGE_INTO})
        message(
            FATAL_ERROR
            "import_autotools_submodule(${IAS_NAME}): MERGE_INTO "
            "target ${IAS_MERGE_INTO} does not exist"
        )
      endif()
      get_target_property(_ias_merge_type ${IAS_MERGE_INTO} TYPE)
      if(_ias_merge_type STREQUAL "STATIC_LIBRARY")
        # The merge rewrites the archive through a temporary: CREATE a
        # fresh archive, ADDLIB the target's own members first, ADDLIB
        # the vendored archive, SAVE. MRI CREATE truncates, so
        # rebuilding from both sources keeps every member and behaves
        # the same under GNU ar and llvm-ar. Rename and ranlib refresh
        # the index in place.
        set(_ias_mri_script "${_ias_prefix}/merge-script.mri")
        set(_ias_merge_tmp "${_ias_prefix}/merge-output.tmp")
        file(GENERATE
            OUTPUT "${_ias_mri_script}"
            CONTENT
            "CREATE ${_ias_merge_tmp}
ADDLIB $<TARGET_FILE:${IAS_MERGE_INTO}>
ADDLIB ${_ias_staged_archive}
SAVE
END
")
        # Ordering holds: the imported target's add_dependencies edge
        # runs the external project before any consumer links, and a
        # POST_BUILD step runs after the consumer link. No DEPENDS
        # here: TARGET-mode add_custom_command rejects it under
        # CMP0175, and the target-level edge above already orders the
        # staged archive ahead of this step on Make and Ninja (FR-015).
        if(CMAKE_RANLIB)
          set(_ias_reindex_commands
              COMMAND "${CMAKE_RANLIB}" "$<TARGET_FILE:${IAS_MERGE_INTO}>")
        else()
          set(_ias_reindex_commands
              COMMAND "${CMAKE_COMMAND}" -E touch
                      "$<TARGET_FILE:${IAS_MERGE_INTO}>")
        endif()
        add_custom_command(
            TARGET ${IAS_MERGE_INTO} POST_BUILD
            COMMAND "${IAS_PROG_SH}" -c
                    "${CMAKE_AR} -M < '${_ias_mri_script}'"
            COMMAND "${CMAKE_COMMAND}" -E rename
                    "${_ias_merge_tmp}" "$<TARGET_FILE:${IAS_MERGE_INTO}>"
            ${_ias_reindex_commands}
            COMMENT
                "Merging ${IAS_NAME} members into ${IAS_MERGE_INTO}"
            VERBATIM
        )
      else()
        message(
            STATUS
            "import_autotools_submodule(${IAS_NAME}): MERGE_INTO "
            "target ${IAS_MERGE_INTO} is ${_ias_merge_type}; the "
            "archive merge applies to static libraries and is skipped"
        )
      endif()
    endif()
  else()
    # ---- Platform block: everything else, unsupported (FR-019) ----
    message(
        FATAL_ERROR
        "import_autotools_submodule(${IAS_NAME}): unsupported "
        "platform '${CMAKE_SYSTEM_NAME}'. Supported set: Linux and "
        "macOS. The documented Windows on-ramp is the upstream "
        "contrib/windows-cmake/ wrapper at the vendored tag."
    )
  endif()
endfunction()
