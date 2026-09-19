cmake_minimum_required(VERSION 3.14)

# Script-mode entry for the prose and commit gate (specs/002-prose-commit-lint).
# CMake ignores -D arguments written after -P, so every definition must
# precede the -P argument. Any nonzero gate status becomes a fatal error:
# the wrapper exits 8 for findings and for usage errors alike. The gate
# offers no auto-fix, so this wrapper carries no FIX flag.

macro(default name)
  if(NOT DEFINED "${name}")
    set("${name}" "${ARGN}")
  endif()
endmacro()

# PROSE_BASE stays undefined unless the caller defines it; the gate then
# resolves its own left edge from the merge base with origin/master.
default(PROSE_MODE range)
default(PROSE_HEAD HEAD)
default(PROSE_CHECK all)
default(PROSE_RULES tools/prose/prose_rules.yaml)
default(PYTHON_COMMAND python3)

set(gate_command
    "${PYTHON_COMMAND}" "${CMAKE_SOURCE_DIR}/tools/prose/prose_gate.py"
    --check "${PROSE_CHECK}"
    --mode "${PROSE_MODE}"
    --rules "${PROSE_RULES}"
    --head "${PROSE_HEAD}"
)
if(DEFINED PROSE_BASE)
  list(APPEND gate_command --base "${PROSE_BASE}")
endif()

execute_process(
    COMMAND ${gate_command}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE result
)

if(result EQUAL "1")
  message(FATAL_ERROR "Prose gate raised findings (status ${result}).")
elseif(result EQUAL "2")
  message(FATAL_ERROR "Prose gate usage error: bad arguments, unreadable rule data, or git failure (status ${result}).")
elseif(NOT result EQUAL "0")
  message(FATAL_ERROR "Prose gate returned with ${result}")
endif()
