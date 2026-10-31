cmake_minimum_required(VERSION 3.14)

# Script-mode entry for the host record target of specs/031-host-validation
# (FR-001 to FR-003, FR-005, R-10). CMake ignores -D arguments written after
# -P, so every definition must precede the -P argument.
#
# Both programs print their own lines, the record line and one line per
# record, so this script prints none of its own and maps a status the way
# cmake/prose-lint.cmake does: a status short of zero ends the run as a
# fatal error naming that status.

macro(default name)
  if(NOT DEFINED "${name}")
    set("${name}" "${ARGN}")
  endif()
endmacro()

# FR-002: the tree root answers the mapfile the identity reads, the schema
# that guards the bytes and the records themselves, and the default output
# root is the `validation` directory under that root, where a writer run with
# no output argument stands its record.
#
# FR-003, FR-034: the output root is the only place this run writes, so a
# narrow-mask run can point it under the build tree, and the developer build
# puts both executables in the test directory of the tree that built them.
default(VALIDATE_ROOT "${CMAKE_SOURCE_DIR}")
default(VALIDATE_OUT "${CMAKE_SOURCE_DIR}/validation")
default(VALIDATE_BIN "${CMAKE_BINARY_DIR}/test")

# FR-002, FR-011: the writer creates nothing beyond the record file it names,
# so the output root stands here first, the way cmake/dbc-gate.cmake stands
# its directory before its first step.
file(MAKE_DIRECTORY "${VALIDATE_OUT}")

execute_process(
    COMMAND "${VALIDATE_BIN}/host_record_write"
        --root "${VALIDATE_ROOT}"
        --out "${VALIDATE_OUT}"
    RESULT_VARIABLE write_status
)

if(NOT write_status EQUAL "0")
  message(FATAL_ERROR "host_record_write returned with ${write_status}")
endif()

execute_process(
    COMMAND "${VALIDATE_BIN}/host_record_check"
        --root "${VALIDATE_ROOT}"
        --records "${VALIDATE_OUT}"
    RESULT_VARIABLE check_status
)

if(NOT check_status EQUAL "0")
  message(FATAL_ERROR "host_record_check returned with ${check_status}")
endif()
