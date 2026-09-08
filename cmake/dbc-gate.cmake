# ---- DBC coverage gate (feature 001) ----
#
# Developer-mode target: a minimal XML-only Doxygen pass over the public
# headers, then both Phase 0 gate scripts. Not part of ALL. Does not
# reuse the m.css docs target (no FetchContent, no network).

find_package(Python3 REQUIRED)

find_program(DOXYGEN_EXECUTABLE NAMES doxygen)
if(NOT DOXYGEN_EXECUTABLE)
  message(FATAL_ERROR "dbc-gate requires doxygen on PATH")
endif()

set(_dbc_gate_dir "${CMAKE_CURRENT_BINARY_DIR}/dbc-gate")
set(_dbc_gate_xml "${_dbc_gate_dir}/xml")
set(_dbc_gate_doxyfile "${_dbc_gate_dir}/Doxyfile")
set(_dbc_gate_doc_matrix "${_dbc_gate_dir}/doc-matrix.json")
set(_dbc_gate_pair_matrix "${_dbc_gate_dir}/pair-matrix.json")
set(_dbc_gate_matrix "${_dbc_gate_dir}/matrix.json")

file(MAKE_DIRECTORY "${_dbc_gate_dir}")

file(
    WRITE "${_dbc_gate_doxyfile}"
    "PROJECT_NAME = ${PROJECT_NAME}\n"
    "PROJECT_NUMBER = ${PROJECT_VERSION}\n"
    "OUTPUT_DIRECTORY = \"${_dbc_gate_dir}\"\n"
    "INPUT = \"${PROJECT_SOURCE_DIR}/include/speedgun-ng\"\n"
    "RECURSIVE = YES\n"
    "EXTRACT_ALL = YES\n"
    "GENERATE_XML = YES\n"
    "GENERATE_HTML = NO\n"
    "GENERATE_LATEX = NO\n"
    "XML_PROGRAMLISTING = NO\n"
    "CREATE_SUBDIRS = NO\n"
    "HAVE_DOT = NO\n"
    "QUIET = YES\n"
)

add_custom_target(
    dbc-gate
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${_dbc_gate_xml}"
    COMMAND "${DOXYGEN_EXECUTABLE}" "${_dbc_gate_doxyfile}"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/dbc/dbc_doc_gate.py"
        --xml "${_dbc_gate_xml}"
        --registry "${PROJECT_SOURCE_DIR}/tools/dbc/macros.yaml"
        --out "${_dbc_gate_doc_matrix}"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/dbc/dbc_pair_gate.py"
        --src "${PROJECT_SOURCE_DIR}/include/speedgun-ng"
        --src2 "${PROJECT_SOURCE_DIR}/source"
        --registry "${PROJECT_SOURCE_DIR}/tools/dbc/macros.yaml"
        --doc-matrix "${_dbc_gate_doc_matrix}"
        --out "${_dbc_gate_pair_matrix}"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${_dbc_gate_pair_matrix}"
        "${_dbc_gate_matrix}"
    COMMENT "Running DBC documentation and pairing gates"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
)
