cmake_minimum_required(VERSION 3.20)

project(speedgun-ngTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(speedgun-ng REQUIRED)
  enable_testing()
endif()

# ---- Tests ----

add_executable(speedgun-ng_test source/speedgun-ng_test.cpp)
target_link_libraries(speedgun-ng_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(speedgun-ng_test PRIVATE cxx_std_23)

add_test(NAME speedgun-ng_test COMMAND speedgun-ng_test)

# ---- DBC facility tests (feature 001) ----

add_executable(dbc_test source/dbc_test.cpp)
target_link_libraries(dbc_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(dbc_test PRIVATE cxx_std_23)
add_test(NAME dbc_test COMMAND dbc_test)

# The trap fixture is NOT a test (it aborts in checked builds); it is a driven
# executable used by dbc_trap_checked_test and the consumer-release CI job to
# prove the release-clean / always-on contrast (SC-002, FR-036).
add_executable(dbc_trap_fixture source/dbc_trap_fixture.cpp)
target_link_libraries(dbc_trap_fixture PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(dbc_trap_fixture PRIVATE cxx_std_23)

# Checked-build trap verification: spawns the fixture and asserts the gated
# site aborted (no marker) under the default (enforce) dev semantic (SC-002
# half (a)). The ignore-build contrast is the consumer-release job's job (T016).
add_executable(dbc_trap_checked_test source/dbc_trap_checked_test.cpp)
target_link_libraries(dbc_trap_checked_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(dbc_trap_checked_test PRIVATE cxx_std_23)
add_test(
  NAME dbc_trap_checked
  COMMAND dbc_trap_checked_test $<TARGET_FILE:dbc_trap_fixture>
)

# Negative-compile harness (T011): exercises compile-time contract layer
# (FR-020/022/023/024). Only the self-test TU is present until T012;
# the test is trivially green and proves the harness itself.
add_test(
  NAME dbc_compile_fail
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/compile-fail/run.sh -B ${CMAKE_BINARY_DIR}
)

# Standalone-compile test (T015): proves the public DBC header is consumable
# with *only* the public include/ directory (no library link, no export/ dir).
# Uses runtime (non-constant) predicates only.
add_executable(dbc_standalone_test source/dbc_standalone.cpp)
target_include_directories(dbc_standalone_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(dbc_standalone_test PRIVATE cxx_std_23)
add_test(NAME dbc_standalone_test COMMAND dbc_standalone_test)

# Dependency scan (T031 / FR-033 / SC-009): asserts the library target has
# zero external runtime dependencies (no find_package/FetchContent/external
# target_link_libraries on speedgun-ng_speedgun-ng). Registered so it is
# exercised in CI. Pure grep script; runs on any platform with bash.
add_test(
  NAME dbc_dependency_scan
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/dependency_scan.sh ${CMAKE_SOURCE_DIR}
)

# hwloc proof scripts (feature 003-vendor-hwloc, T008; FR-004/FR-007,
# SC-009/SC-011): purity scan and the nm symbol-isolation proof. Same
# pure-grep-script registration pattern as dbc_dependency_scan.
add_test(NAME hwloc_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/hwloc/hwloc_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME hwloc_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/hwloc/hwloc_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# simdjson purity scan (feature 004-vendor-simdjson, T006; FR-004/FR-013,
# SC-008): privacy contract A7/A8. Same pure-grep-script registration
# pattern as the hwloc pair.
add_test(NAME simdjson_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/simdjson/simdjson_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME simdjson_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/simdjson/simdjson_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# hdrhistogram and zlib proof scripts (feature 005-vendor-hdrhistogram,
# T009/T010/T013; FR-004/FR-007/FR-014/FR-019/FR-021, SC-008/SC-009/SC-010):
# purity scans and nm symbol-isolation proofs, privacy contract
# A7/A8/A7z/A8z/A6/A6z. Same pure-grep-script registration pattern as the
# simdjson pair. The nm proofs are TDD red until the ADDLIB merge lands.
add_test(NAME hdrhistogram_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/hdrhistogram/hdrhistogram_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME hdrhistogram_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/hdrhistogram/hdrhistogram_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
add_test(NAME zlib_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/zlib/zlib_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME zlib_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/zlib/zlib_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # The following two tests are registered for tasks T010/T012 (parallel);
  # the scripts are not present yet and these tests are not executed here.
  add_test(
    NAME dbc_asm_smoke
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/asm_smoke.sh ${CMAKE_SOURCE_DIR}/include
  )
  add_test(
    NAME dbc_semantics_matrix
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR}
  )
endif()

# Gate-effectiveness harness (T024 / SC-006). Self-contained: invokes the
# Python driver directly (no dbc-gate CMake target). TDD red until T025/T026
# land tools/dbc/dbc_doc_gate.py and dbc_pair_gate.py: while they are missing
# the harness exits with "gate scripts not present (TDD red)".
add_test(
  NAME dbc_gate_fixtures
  COMMAND
    python3
    ${CMAKE_CURRENT_SOURCE_DIR}/dbc-gate-fixture/run_gate_fixtures.py
    --gate both
    --registry ${CMAKE_SOURCE_DIR}/tools/dbc/macros.yaml
    --out ${CMAKE_BINARY_DIR}/dbc-gate-fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/dbc-gate-fixture
)

# Gate-effectiveness harness for the prose and commit lint gate (feature 002,
# T009). Self-contained: invokes the Python driver directly. TDD red until
# tools/prose/prose_gate.py lands: while it is missing the harness exits with
# "gate script not present (TDD red)".
add_test(
    NAME prose_gate_fixtures
    COMMAND
      python3
      ${CMAKE_CURRENT_SOURCE_DIR}/prose-gate-fixture/run_prose_gate_fixtures.py
      --check both
      --out ${CMAKE_BINARY_DIR}/prose-gate-fixtures
      ${CMAKE_CURRENT_SOURCE_DIR}/prose-gate-fixture
)

# ---- End-of-file commands ----

add_folders(Test)
