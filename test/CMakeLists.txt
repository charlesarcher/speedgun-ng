cmake_minimum_required(VERSION 3.20)

project(speedgun-ngTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(speedgun-ng REQUIRED)
  enable_testing()
endif()

# The compile concurrency one matrix entry may start: each dbc matrix entry
# hands this value to its own build as JOBS, so it bounds that entry's
# compiler fleet and never the concurrency of the whole suite (030 P-04).
set(
  SPEEDGUN_TEST_JOBS "2"
  CACHE STRING
  "Compile concurrency one dbc matrix entry may start"
)

# The slots the cross-processor scenario of counters_clock_raw_test needs:
# it starts one worker per logical processor of the host it runs on, so the
# entry reserves that many (030 FR-003).
cmake_host_system_information(
  RESULT SPEEDGUN_HOST_PROCESSORS
  QUERY NUMBER_OF_LOGICAL_CORES
)

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

# Literal survives the rename fixer (D-01). Not a behavior check.
add_executable(dbc_literal_fixture source/dbc_literal_fixture.cpp)
target_link_libraries(dbc_literal_fixture PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(dbc_literal_fixture PRIVATE cxx_std_23)
add_test(NAME dbc_literal_fixture COMMAND dbc_literal_fixture)

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
# 005 T009/T010/T013; FR-004/FR-007/FR-014/FR-019/FR-021,
# SC-008/SC-009/SC-010):
# purity scans and nm symbol-isolation proofs, privacy contract
# A7/A8/A7z/A8z/A6/A6z. Same pure-grep-script registration pattern as the
# simdjson pair. The nm proofs were TDD red until the ADDLIB merge landed.
add_test(NAME hdrhistogram_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/hdrhistogram/hdrhistogram_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME hdrhistogram_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/hdrhistogram/hdrhistogram_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
add_test(NAME zlib_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/zlib/zlib_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME zlib_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/zlib/zlib_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# yaml-cpp proof scripts (feature 006-vendor-yaml-cpp; FR-004/FR-007/FR-014,
# SC-008/SC-009): purity scan and nm symbol-isolation proof, privacy contract
# A7/A8/A6. Same pure-grep-script registration pattern as the simdjson pair.
add_test(NAME yaml_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/yaml/yaml_purity_scan.sh ${CMAKE_SOURCE_DIR})
add_test(NAME yaml_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/yaml/yaml_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# quill proofs (feature 009-vendor-quill; FR-005/FR-008/FR-008a/FR-014,
# SC-008/SC-009, privacy contract A6/A7/A8).
#
# The runnable check replaces the nm symbol-isolation proof the five
# compiled dependencies carry, and it is deliberately not named
# *_nm_proof. quill is header-only: it compiles no target, so no archive
# is merged, and every symbol it declares is defined as a weak symbol in
# the same translation unit that declares it, leaving zero undefined
# symbols for an archive scan to find (specs/009 research R-007, R-008).
# Naming a runnable check *_nm_proof would assert an archive scan that
# cannot exist here. The check links the library and quill, drives a real
# counter, emits its value, and asserts the value reached the log, which
# is the only available proof for a header-only dependency.
add_executable(quill_dependency_check ../tools/quill/quill_dependency_check.cpp)
target_link_libraries(
  quill_dependency_check PRIVATE speedgun-ng_speedgun-ng quill::quill
)
add_test(NAME quill_dependency_check COMMAND quill_dependency_check)

# Purity scan uses the same pure-grep-script registration pattern as the
# five existing pairs (specs/009 T013/T014).
add_test(NAME quill_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/quill/quill_purity_scan.sh ${CMAKE_SOURCE_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # The following two tests are registered for tasks T010/T012 (parallel);
  # the scripts are not present yet and these tests are not executed here.
  add_test(
    NAME dbc_asm_smoke
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/asm_smoke.sh ${CMAKE_SOURCE_DIR}/include
  )
  # Only the lanes whose preset sets SPEEDGUN_DBC_MATRIX carry the matrix
  # entries: each configures and builds a tree of its own, so coverage and
  # sanitize leave them unregistered (030 FR-019, D-5, P-01).
  if(SPEEDGUN_DBC_MATRIX)
    # NDEBUG off at -O0: assert aborts and contract semantic stays 2
    add_test(
      NAME dbc_matrix_off_o0
      COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR} dbc_matrix_off_o0 ${SPEEDGUN_TEST_JOBS}
    )
    # NDEBUG off at -O2: the same semantic value under optimization
    add_test(
      NAME dbc_matrix_off_o2
      COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR} dbc_matrix_off_o2 ${SPEEDGUN_TEST_JOBS}
    )
    # NDEBUG on at -O0: assert elided and contract semantic stays 2
    add_test(
      NAME dbc_matrix_on_o0
      COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR} dbc_matrix_on_o0 ${SPEEDGUN_TEST_JOBS}
    )
    # NDEBUG on at -O2: assert elided under optimization, semantic 2
    add_test(
      NAME dbc_matrix_on_o2
      COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR} dbc_matrix_on_o2 ${SPEEDGUN_TEST_JOBS}
    )
    # observe mode: the negative probe that must print contract semantic 1
    add_test(
      NAME dbc_matrix_observe_off_o0
      COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR} dbc_matrix_observe_off_o0 ${SPEEDGUN_TEST_JOBS}
    )
    # Each cell hands SPEEDGUN_TEST_JOBS to its own build as its compile
    # concurrency, so each of the five entries holds that many slots
    # while its compiler fleet runs (030 FR-003).
    set_tests_properties(
      dbc_matrix_off_o0
      dbc_matrix_off_o2
      dbc_matrix_on_o0
      dbc_matrix_on_o2
      dbc_matrix_observe_off_o0
      PROPERTIES PROCESSORS ${SPEEDGUN_TEST_JOBS}
    )
  endif()

  # Codegen-shape gate for the raw time-stamp read (T026; 008 FR-003,
  # FR-022): the optimized read arm carries no function call, so nothing
  # stands between the catalog entry and the instruction. The script
  # compiles the translation unit at -O2 itself, because the property holds
  # in optimized code only and an unoptimized object calls a helper for the
  # read. Same pure-script registration pattern; the script exits 0 clean, 1
  # naming the violation, and 0 with a skip line off x86.
  add_test(
    NAME counters_tsc_read_shape
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/counters_tsc_read_shape.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
  )

  # Codegen gate for the trace-start marker (011 T009; FR-012, FR-023,
  # FR-024, FR-025): the marker has no run-time effect, so nothing in the
  # ordinary suite can tell a correct marker from a broken one. The script
  # compiles the translation unit at -O2 itself, for each compiler crossed
  # with each contract semantic, and asserts the eight-byte window a tracer
  # matches appears exactly once, after planting a wrong window to prove
  # the detector bites. Same pure-script registration pattern; the script
  # exits 0 clean, 1 naming the count, and 0 with a skip line where no
  # compiler on PATH targets x86.
  add_test(
    NAME simulation_mark_shape
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/simulation_mark_shape.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
  )

  # Tier and budget audit of the registration (030 T010; FR-026, FR-031,
  # FR-047, FR-048): the tier label and the time budget are set by the two
  # blocks at the end of the file, where no compiler reads them, so this
  # entry reads the suite back from ctest and fails when an entry carries
  # both labels or none, when a slow row names no entry, when an entry holds
  # no budget, or when a registration sleeps, reads a clock for its verdict,
  # retries, or sets a skip rule master does not set. The build directory and
  # the ctest program come in as arguments, so the entry runs from any
  # working directory, and the two blocks give it its label and its budget
  # with the rest of the suite (Principle III: seen red before they land).
  add_test(
    NAME test_tier_labels
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tier_labels.sh ${CMAKE_BINARY_DIR} ${CMAKE_CTEST_COMMAND}
  )
endif()

# Gate-effectiveness harness (001 T024 / SC-006). Self-contained: invokes
# the Python driver directly (no dbc-gate CMake target). It was TDD red
# until 001 T025 and T026 landed tools/dbc/dbc_doc_gate.py and
# dbc_pair_gate.py, and it still exits with "gate scripts not present (TDD
# red)" while either script is missing.
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
# T009). Self-contained: invokes the Python driver directly. It was TDD red
# until tools/prose/prose_gate.py landed, and it still exits with "gate
# script not present (TDD red)" while that script is missing.
add_test(
    NAME prose_gate_fixtures
    COMMAND
      python3
      ${CMAKE_CURRENT_SOURCE_DIR}/prose-gate-fixture/run_prose_gate_fixtures.py
      --check both
      --out ${CMAKE_BINARY_DIR}/prose-gate-fixtures
      ${CMAKE_CURRENT_SOURCE_DIR}/prose-gate-fixture
)

# ---- Counters library tests (feature 007) ----

# Raw time-stamp counter and its accessor (specs/008-timestamp-counter
# FR-001..FR-008; US1, US2): publication wherever the build executes the
# instruction, a count carrying no rate, and an accessor interchangeable
# with the uniform lookup.
add_executable(counters_tsc_test source/counters_tsc_test.cpp)
target_link_libraries(counters_tsc_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_tsc_test PRIVATE cxx_std_23)
add_test(NAME counters_tsc_test COMMAND counters_tsc_test)

add_executable(counters_core_test source/counters_core_test.cpp)
target_link_libraries(counters_core_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_core_test PRIVATE cxx_std_23)
add_test(NAME counters_core_test COMMAND counters_core_test)

add_executable(counters_fake_test source/counters_fake_test.cpp)
target_link_libraries(counters_fake_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_fake_test PRIVATE cxx_std_23)
add_test(NAME counters_fake_test COMMAND counters_fake_test)

# Dimension-algebra negative-compile cases (T012 / FR-014, FR-015).
add_test(
  NAME counters_compile_fail
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/compile-fail/run.sh -B ${CMAKE_BINARY_DIR}
)

# Vocabulary purity scan (T010; FR-010): platform terms stay out of the
# public counters headers. Pure-grep-script registration pattern, same
# as the vendored-dependency purity scans.
add_test(
  NAME counters_header_purity
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/counters_header_purity.sh ${CMAKE_SOURCE_DIR}
)

# Atomic read-modify-write scan of the push path (T100; FR-035, R-008):
# the hot-path add, the sampled load, and the fold-time read are plain
# accesses on a thread-confined cell. Same pure-grep-script registration
# pattern; the script exits 0 clean and 1 naming each violation.
add_test(
  NAME counters_push_atomic_scan
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/counters_push_atomic_scan.sh ${CMAKE_SOURCE_DIR}
)

# Recorder arena, overflow policies, and independent cursors (T028;
# FR-025..FR-029).
add_executable(counters_recorder_test source/counters_recorder_test.cpp)
target_link_libraries(counters_recorder_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_recorder_test PRIVATE cxx_std_23)
add_test(NAME counters_recorder_test COMMAND counters_recorder_test)
# The concurrent-resolution scenario resolves, walks and compiles on eight
# threads at once (kThreadCount of source/counters_recorder_test.cpp), so
# eight slots keep those workers off cpus a sibling entry holds (030 FR-003).
set_tests_properties(counters_recorder_test PROPERTIES PROCESSORS 8)

# Allocation-free sampling proof (T027; FR-026): counting global
# operator new/delete in this TU, sampling under the counter.
# The thread sanitizer runtime defines those operators too. The two
# definitions cannot link, so this proof stays out of that preset.
if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "fsanitize=thread")
  add_executable(counters_noalloc_test source/counters_noalloc_test.cpp)
  target_link_libraries(counters_noalloc_test PRIVATE speedgun-ng::speedgun-ng)
  target_compile_features(counters_noalloc_test PRIVATE cxx_std_23)
  add_test(NAME counters_noalloc_test COMMAND counters_noalloc_test)
endif()

# The recorder trap fixture is NOT a test (it aborts); it is a driven
# executable used by counters_trap_checked_test and the consumer-release
# CI job (T023): scope misuse and fold-range abort only in checked
# builds, the FR-027 overrun (always-on) aborts in every configuration.
add_executable(counters_trap_fixture source/counters_trap_fixture.cpp)
target_link_libraries(counters_trap_fixture PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_trap_fixture PRIVATE cxx_std_23)

# A fixture with no mode body, which stands in for a mode the checker
# drives and the fixture does not implement. It is never run as a check
# of its own; the CTest below asserts that the checker reports it.
add_executable(counters_trap_noguard_fixture
               source/counters_trap_noguard_fixture.cpp)
target_compile_features(counters_trap_noguard_fixture PRIVATE cxx_std_23)

# Trap verification, both directions (T023, T090, T105). The expectations
# follow SG_CONTRACTS_SEMANTIC, the PUBLIC definition of the library this
# test links, so one target covers both configurations. In a checked build
# every mode must abort before printing its marker. In an ignore build the
# gated modes must run through and print theirs while the always-on
# overrun still aborts; that is the FR-027 release proof, and it is
# reproducible locally in a release-configured binary directory:
#   cmake -S . -B build/release-dev -D CMAKE_BUILD_TYPE=Release
#     -D speedgun-ng_DEVELOPER_MODE=ON -D speedgun-ng_CONTRACTS=ignore
#   ctest --test-dir build/release-dev -R counters_trap --output-on-failure
add_executable(counters_trap_checked_test source/counters_trap_checked_test.cpp)
target_link_libraries(counters_trap_checked_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_trap_checked_test PRIVATE cxx_std_23)
add_test(
  NAME counters_trap_checked
  COMMAND counters_trap_checked_test $<TARGET_FILE:counters_trap_fixture>
)
# The checker redirects every run into one fixed scratch file, so the two
# tests below share it and must not run at the same time.
set_tests_properties(counters_trap_checked PROPERTIES
                     RESOURCE_LOCK counters_trap_checked_out)

# The check above trusts that every mode it drives reached its guard, so
# the trust is itself covered: run the same checker against a fixture that
# implements no mode, where every mode lands on a refusal that exits
# non-zero with the marker absent. The test passes on the checker's own
# wording for that outcome and on nothing else, so a missing fixture
# binary and a checker that stopped demanding the contract facility's
# report both turn it red. The two patterns are the aborting
# configuration's report and the surviving configuration's report.
add_test(
  NAME counters_trap_checked_rejects_unknown_mode
  COMMAND counters_trap_checked_test $<TARGET_FILE:counters_trap_noguard_fixture>
)
set_tests_properties(
  counters_trap_checked_rejects_unknown_mode
  PROPERTIES PASS_REGULAR_EXPRESSION
             "no guard of that mode was reached;where the gated check must have emitted no code"
             RESOURCE_LOCK counters_trap_checked_out
)

# Multi-object tree, selection, cross-object composition, and fan-out
# (T029, T033; FR-002, FR-003, FR-047, SC-007).
add_executable(counters_objects_test source/counters_objects_test.cpp)
target_link_libraries(counters_objects_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_objects_test PRIVATE cxx_std_23)
add_test(NAME counters_objects_test COMMAND counters_objects_test)

# Per-window disclosure column under both provider registration orders
# (T012, T013; FR-002, FR-004, FR-005, SC-002, SC-003, D-03). Two binaries
# over one source, because provider registration happens once per process
# and the two orders cannot share one binary. Each order is a test that can
# fail.
foreach(order RANGE 1)
  if(order EQUAL 0)
    set(order_suffix "_clock_first")
  else()
    set(order_suffix "_pmu_first")
  endif()
  add_executable(counters_registration_order_test${order_suffix}
                 source/counters_registration_order_test.cpp)
  target_link_libraries(counters_registration_order_test${order_suffix}
                        PRIVATE speedgun-ng::speedgun-ng)
  target_compile_features(counters_registration_order_test${order_suffix}
                          PRIVATE cxx_std_23)
  target_compile_definitions(
    counters_registration_order_test${order_suffix}
    PRIVATE SG_REGISTRATION_ORDER=${order})
  add_test(NAME counters_registration_order_test${order_suffix}
           COMMAND counters_registration_order_test${order_suffix})
endforeach()

# Shipped clock and push providers (T034, T038; FR-033..FR-035): clock
# windows, push hot-path adds, composites with standard disclosure, and
# the raw time-stamp entry publishing under the build guard with no rate
# attached.
add_executable(counters_clock_push_test source/counters_clock_push_test.cpp)
target_link_libraries(counters_clock_push_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_clock_push_test PRIVATE cxx_std_23)
add_test(NAME counters_clock_push_test COMMAND counters_clock_push_test)

# Out-of-tree provider extension from public headers only (T039;
# FR-011, FR-012, FR-047): the giraffe counts honks through a mini
# provider defined in the test TU.
add_executable(counters_provider_ext_test
               source/counters_provider_ext_test.cpp)
target_link_libraries(counters_provider_ext_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_provider_ext_test PRIVATE cxx_std_23)
add_test(NAME counters_provider_ext_test COMMAND counters_provider_ext_test)

# Gate-effectiveness fixtures for the pmu-events integrity tool (feature
# 007, T057; FR-045). Self-contained: the Python driver builds synthetic
# repo-like roots under the build dir (clean tree, hash drift, gate
# disagreement, missing and extra files, offline replace-last re-pin) and
# asserts the tool's exit codes and named files in both directions. No
# network, and the real external/pmu-events tree is never touched.
add_test(
    NAME pmu_events_check
    COMMAND
      python3
      ${CMAKE_CURRENT_SOURCE_DIR}/pmu-events-gate-fixture/run_pmu_events_fixtures.py
      --out ${CMAKE_BINARY_DIR}/pmu-events-gate-fixtures
      ${CMAKE_CURRENT_SOURCE_DIR}/pmu-events-gate-fixture
)

# Linux PMU provider: merged catalog, probed availability at the
# host's paranoia level, and the syscall-mode group read with the
# enabled/running ratio pair (T043; FR-037..FR-041, SC-002). Privilege
# gates skip with the reason printed; the test never fails an
# unprivileged host.
add_executable(counters_pmu_test source/counters_pmu_test.cpp)
target_link_libraries(counters_pmu_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_pmu_test PRIVATE cxx_std_23)
# The fast-window lifetime scenario needs a granted perf_event_open, which a
# runner refuses and a developer host grants, so the scenario prints its
# reason and exits 2 where the kernel refuses (SC-004).
add_test(NAME counters_pmu_test COMMAND counters_pmu_test)
set_tests_properties(counters_pmu_test PROPERTIES SKIP_RETURN_CODE 2)

# Synthetic-fixture coverage for the Linux PMU provider's pure seam
# functions (T066, plan.md Test Plan; FR-037, FR-040, R-010, R-011).
# The config encoder and the mapped-page decode are covered here with
# injected inputs, so CI reaches them without privileges; the kernel glue
# that feeds them runs on a fast-capable developer host. This is one of the
# two families that include a private seam header, because a pure function
# behind it is otherwise reachable only from a host that owns the
# hardware; the metric tests reach the formula seam the same way.
add_executable(counters_linux_pmu_seam_test
               source/counters_linux_pmu_seam_test.cpp)
target_link_libraries(counters_linux_pmu_seam_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_linux_pmu_seam_test PRIVATE cxx_std_23)
# The synthetic event-table fixtures are generated into the build tree at
# run time, following the `pmu-events-gate-fixture` precedent (T057).
target_compile_definitions(
    counters_linux_pmu_seam_test
    PRIVATE
        "SG_SEAM_TABLE_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/counters_pmu_seam_tables\""
)
add_test(NAME counters_linux_pmu_seam_test
         COMMAND counters_linux_pmu_seam_test)

# Metric formulas of the vendored event table (T013..T019, T027..T034;
# FR-003..FR-021). The grammar test drives the parse seam as a pure
# function; the lookup test resolves the fixture table against the fixture
# host and folds the result, so the assertion is a value.
add_executable(counters_metric_formula_test
               source/counters_metric_formula_test.cpp)
target_link_libraries(counters_metric_formula_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_metric_formula_test PRIVATE cxx_std_23)
add_test(NAME counters_metric_formula_test COMMAND counters_metric_formula_test)
set_tests_properties(counters_metric_formula_test PROPERTIES
                     SKIP_RETURN_CODE 2)

add_executable(counters_metric_lookup_test
               source/counters_metric_lookup_test.cpp)
target_link_libraries(counters_metric_lookup_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_metric_lookup_test PRIVATE cxx_std_23)
add_test(NAME counters_metric_lookup_test COMMAND counters_metric_lookup_test)
set_tests_properties(counters_metric_lookup_test PROPERTIES
                     SKIP_RETURN_CODE 2)

add_executable(counters_metric_trees_test
               source/counters_metric_trees_test.cpp)
target_link_libraries(counters_metric_trees_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_metric_trees_test PRIVATE cxx_std_23)
add_test(NAME counters_metric_trees_test COMMAND counters_metric_trees_test)
set_tests_properties(counters_metric_trees_test PROPERTIES
                     SKIP_RETURN_CODE 2)

add_executable(counters_metric_threadless_test
               source/counters_metric_threadless_test.cpp)
target_link_libraries(counters_metric_threadless_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_metric_threadless_test PRIVATE cxx_std_23)
add_test(NAME counters_metric_threadless_test
         COMMAND counters_metric_threadless_test)
set_tests_properties(counters_metric_threadless_test PROPERTIES
                     SKIP_RETURN_CODE 2)

# Sampling-cost benchmark and mode disclosure (T051; US7 scenarios 1
# and 6; SC-004, SC-010, quickstart 12). Publishes the min/median/max
# per-action distribution of each regime this host probes, the achieved
# read mode per catalog entry with the refusal reason where a fast
# mechanism was rejected, and the fold cost measured off the sampling
# path. Exit 2 skips with the reason printed, so a host without a
# probe-passing fast mechanism reports its skip and stays green
# (Principle VII, FR-023, FR-032).
add_executable(counters_overhead source/counters_overhead.cpp)
target_link_libraries(counters_overhead PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_overhead PRIVATE cxx_std_23)
add_test(NAME counters_overhead COMMAND counters_overhead)
set_tests_properties(counters_overhead PROPERTIES SKIP_RETURN_CODE 2)

# The two counters examples as gates (T151; SC-001, SC-003, FR-049): the
# standalone example is the SC-001 acceptance artifact, and the giraffe
# example proves the extension seam from public headers alone. Neither
# takes an argument, reads a hardware counter, or needs a privilege, so
# every host in the matrix runs both, no SKIP_RETURN_CODE applies, and
# their exit 1 is a real failure. Both are registered on the example
# targets, so a configuration built without BUILD_EXAMPLES registers
# neither.
if(TARGET counters_standalone_example)
  add_test(NAME counters_standalone_example
           COMMAND counters_standalone_example)
  add_test(NAME counters_giraffe_example COMMAND counters_giraffe_example)
endif()

# ---- Programmable-counter limit (feature 024) ----

# The leaf-0AH decode of the published counter count (T006 registers the
# skeleton so the build stays green ahead of the first assertion; T009 fills
# it; FR-005, FR-032, research D-01, D-04). The decode is a pure function of
# synthetic registers, reached through the relative include of a private
# header the way counters_metric_lookup_test.cpp reaches
# linux_pmu/metric_tables.hpp, so this binary carries no include directory of
# its own. Exit 2 skips with the reason printed, the pattern of the metric
# blocks above.
add_executable(counters_counter_limits_test
               source/counters_counter_limits_test.cpp)
target_link_libraries(counters_counter_limits_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_counter_limits_test PRIVATE cxx_std_23)
add_test(NAME counters_counter_limits_test
         COMMAND counters_counter_limits_test)
set_tests_properties(counters_counter_limits_test PROPERTIES
                     SKIP_RETURN_CODE 2)

# The same count as a fact an object publishes (T006 registers the skeleton,
# T010 fills it; FR-005, FR-006, data-model E-02). Every case reads a scripted
# fake, so no case needs a kernel counter or a privilege.
add_executable(counters_objects_limit_test
               source/counters_objects_limit_test.cpp)
target_link_libraries(counters_objects_limit_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_objects_limit_test PRIVATE cxx_std_23)
add_test(NAME counters_objects_limit_test COMMAND counters_objects_limit_test)
set_tests_properties(counters_objects_limit_test PROPERTIES
                     SKIP_RETURN_CODE 2)

# ---- Nanosecond-rate counter (feature 011) ----

# The nanosecond-rate clock leaf (specs/011 FR-001..FR-008; US2):
# publication at `machine/monotonic_raw` on every supported build, the
# existing unit token and read mode, ten million consecutive reads that
# never decrease, the same claim across every logical processor under a
# join, and no observed step finer than the resolution the platform
# reports. It prints the per-sampling-action cost distribution beside the
# resolution so the published row has a run to check against.
add_executable(counters_clock_raw_test source/counters_clock_raw_test.cpp)
target_link_libraries(counters_clock_raw_test
                      PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(counters_clock_raw_test PRIVATE cxx_std_23)
add_test(NAME counters_clock_raw_test COMMAND counters_clock_raw_test)
# The cross-processor scenario starts one worker per logical processor of
# the host and joins them, so the entry takes as many slots as the host has
# cpus (030 FR-003, SPEEDGUN_HOST_PROCESSORS above).
set_tests_properties(counters_clock_raw_test PROPERTIES
                     PROCESSORS ${SPEEDGUN_HOST_PROCESSORS})

# ---- Trace-start marker (feature 011) ----

# The marker's runtime properties (specs/011 FR-013, FR-026, FR-027; US1):
# the published tag carries the documented value, every general-purpose
# register is bit-identical across a call with the register's upper 32
# bits set, and repeated calls terminate with no tracer attached. No
# tracer is required to run it, which is what keeps the marker covered on
# a build machine with no tracer installed.
add_executable(simulation_test source/simulation_test.cpp)
target_link_libraries(simulation_test PRIVATE speedgun-ng::speedgun-ng)
target_compile_features(simulation_test PRIVATE cxx_std_23)
add_test(NAME simulation_test COMMAND simulation_test)

# ---- Benchmark harness core (feature 015) ----

# The time-source gate (T008, T011; FR-039, FR-040, SC-013, D-6): no C++
# source or header outside the counters library reaches a time or
# time-stamp source the counters library does not own. Same pure-scan
# registration pattern as the vendored purity scans.
add_test(
  NAME time_source_gate
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/time_source_gate.sh ${CMAKE_SOURCE_DIR}
)

# The capture path over a scripted provider (T012; US1, SC-001, SC-004,
# SC-005, SC-014, FR-017, FR-022, FR-024, FR-025, FR-026).
# The capture proof replaces the global operator new and delete in this
# translation unit. The thread sanitizer runtime defines those operators
# too, so the two definitions cannot link, and this test stays out of
# that preset for the same reason as counters_noalloc_test above.
if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "fsanitize=thread")
add_executable(harness_capture_test source/harness_capture_test.cpp)
target_link_libraries(harness_capture_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_capture_test PRIVATE cxx_std_23)
add_test(NAME harness_capture_test COMMAND harness_capture_test)
endif()

# The run-control calibration on the real clock (T019; US2, FR-010 to
# FR-014, FR-016, SC-014).
add_executable(harness_calibration_test source/harness_calibration_test.cpp)
target_link_libraries(harness_calibration_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_calibration_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_calibration_test PRIVATE cxx_std_23)
add_test(NAME harness_calibration_test COMMAND harness_calibration_test)
set_tests_properties(harness_calibration_test PROPERTIES TIMEOUT 120)

# IF-09: the growth rule and its stop outcomes, over the calibration
# header the runner shares with the warm-up phase (FR-008, FR-010,
# FR-016).
add_executable(harness_growth_test source/harness_growth_test.cpp)
target_link_libraries(harness_growth_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_growth_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_growth_test PRIVATE cxx_std_23)
add_test(NAME harness_growth_test COMMAND harness_growth_test)

# The rank rule, the availability bound and the point list over scripted
# columns of chunk values (T012; US1, US2, FR-010 to FR-013, FR-016, FR-024).
add_executable(harness_percentile_test source/harness_percentile_test.cpp)
target_link_libraries(harness_percentile_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_percentile_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_percentile_test PRIVATE cxx_std_23)
add_test(NAME harness_percentile_test COMMAND harness_percentile_test)

# The written document against the run it carries (T016, T021; US1,
# FR-001, FR-003, FR-029, FR-031, SC-001, SC-003).
add_executable(harness_json_report_test source/harness_json_report_test.cpp)
target_link_libraries(harness_json_report_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_json_report_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_json_report_test PRIVATE cxx_std_23)
target_compile_definitions(
    harness_json_report_test
    PRIVATE SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"
)
add_test(NAME harness_json_report_test COMMAND harness_json_report_test)

# The schema file and its checker over a written document and five planted
# disagreements (T017; US1, FR-008, FR-010, D-05).
add_executable(harness_json_schema_test source/harness_json_schema_test.cpp)
target_link_libraries(harness_json_schema_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_json_schema_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_definitions(
    harness_json_schema_test
    PRIVATE SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"
)
target_compile_features(harness_json_schema_test PRIVATE cxx_std_23)
add_test(NAME harness_json_schema_test COMMAND harness_json_schema_test)

# The reader over the writer's own file, and the version rule (T022, T023,
# T026; US2, FR-010, FR-011, FR-022, FR-023, FR-026, SC-002, SC-010).
add_executable(harness_json_load_test source/harness_json_load_test.cpp)
target_link_libraries(harness_json_load_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_json_load_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_json_load_test PRIVATE cxx_std_23)
add_test(NAME harness_json_load_test COMMAND harness_json_load_test)

# The rule grammar and the compare mode of the regression gate (specs/026
# T007; FR-071, SC-007, D-20, PC-10). The fixture documents reach each
# test through the path argument, the shape of `hwloc_purity_scan`.
add_executable(harness_compare_test source/harness_compare_test.cpp)
target_link_libraries(harness_compare_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_compare_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_compare_test PRIVATE cxx_std_23)
add_test(NAME harness_compare_test
         COMMAND harness_compare_test ${CMAKE_SOURCE_DIR}/test/data)

add_executable(harness_compare_cli_test source/harness_compare_cli_test.cpp)
target_link_libraries(harness_compare_cli_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_compare_cli_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_compare_cli_test PRIVATE cxx_std_23)
add_test(NAME harness_compare_cli_test
         COMMAND harness_compare_cli_test ${CMAKE_SOURCE_DIR}/test/data)

# The command line over a scripted provider (T020; US2, FR-009, FR-015,
# FR-021, FR-022, FR-034, FR-036, SC-007).
add_executable(harness_cli_test source/harness_cli_test.cpp)
target_link_libraries(harness_cli_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_cli_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_test COMMAND harness_cli_test)
# The suite's heaviest test: `captureRun` runs the harness in-process behind
# a `freopen`, so its two windows of 1,000 runs each hold one core for the
# whole entry. Two slots keep a sibling off that core; CI needs no TIMEOUT,
# since `sanitize` and `tsan` finish this test inside `--timeout 3600`.
# Every scratch name carries the process id, so a second copy shares no file.
set_tests_properties(harness_cli_test PROPERTIES PROCESSORS 2)

# The one option table of the contract, read out of the source that holds
# it: 28 rows in the fixed order, each with its argument kind and its
# value, the seven new names at the constants 290 to 296 and the one row
# this feature brings at 260 (T004; FR-001,
# FR-003, FR-004, FR-079, SC-001). The directory that holds the table
# reaches the test by argument, the way the fixture directory reaches
# `harness_compare_test`. The test is red until Phase 1 lands the seven
# rows, and it is registered as a plain test so the same registration
# turns green with them.
# it: 35 rows in the fixed order, each with its argument kind and its value,
# specs/028's seven names at the constants 290 to 296 and the eight hunt
# names at 270 to 277 (T004; FR-001, FR-003, FR-004, FR-079, FR-080,
# SC-001). The directory that holds the table reaches the test by argument,
# the way the fixture directory reaches `harness_compare_test`. Phase 1
# landed the hunt rows, and the test is registered as a plain test so the
# same registration that was red with master stands green with them.
add_executable(harness_cli_table_test source/harness_cli_table_test.cpp)
target_link_libraries(harness_cli_table_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_cli_table_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_cli_table_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_table_test
         COMMAND harness_cli_table_test ${CMAKE_SOURCE_DIR}/source/harness)

# The public view surface over the report value `loadReport` answered:
# `ViewSeries` default-constructs empty and the six declarations
# type-check through the header's declares (specs/038 T001; FR-001,
# FR-004, FR-005). The unit runs red until the header lands, and the
# same registration turns green with it.
add_executable(views_test source/views_test.cpp)
target_link_libraries(views_test PRIVATE speedgun-ng::harness)
# The detail escaper the escape block calls directly (specs/038 T006),
# the shape harness_cli_table_test gives its source directory.
target_include_directories(views_test PRIVATE ${CMAKE_SOURCE_DIR}/source/views)
target_compile_features(views_test PRIVATE cxx_std_23)
# The fixture directory the resolver block loads its documents from, the
# shape harness_compare_test gives its fixture directory.
add_test(NAME views_test
         COMMAND views_test ${CMAKE_SOURCE_DIR}/test/data/report_views)

# The driver's command line over the fixture documents (specs/038
# T017; FR-014 to FR-017, contracts/views-api.md B-6 and the refusal
# and disclosure table C): the gate script drives the built binary
# over the fixtures and asserts every row of the table, one verdict
# line per run. The binary and the fixture directory travel as
# arguments, the shape harness_cli_table_test gives its source
# directory. The registration runs red until the driver's task T018
# fills the program in.
add_test(
    NAME report_view_cli_test
    COMMAND
        bash ${CMAKE_CURRENT_SOURCE_DIR}/report_view_cli.sh
        $<TARGET_FILE:report_view> ${CMAKE_SOURCE_DIR}/test/data/report_views
)

# The usage faults of the command-line scan, each driven by an `argv`
# array built in the test and compared byte for byte against the fault
# line of the contract (T010; FR-007 to FR-012, SC-003,
# contracts/option-table.md). T009 owns this registration because the
# same pass defines `detail::scanCommandLine` in
# `source/harness/options.cpp` and so first makes the target linkable.
# The target runs red until T011 splits the parse from the exit: the scan
# still answers the faults the base prints, so each of the five contract
# texts mismatches and the five checks report by name.
add_executable(harness_cli_sources_test source/harness_cli_sources_test.cpp)
target_link_libraries(harness_cli_sources_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_cli_sources_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_cli_sources_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_sources_test
         COMMAND harness_cli_sources_test ${CMAKE_SOURCE_DIR}/test/data)

# The thread list of `--threads` read as a comma-separated sequence of
# items: a single count, a `min:max` range expanded through
# `detail::buildThreadCounts`, and the refusal line for a count below one,
# a low above its high or a malformed item (T040; FR-044, FR-045, FR-046).
# This is the pass that defines `detail::readThreadList` in
# `source/harness/options.cpp`, so it is the pass that first makes the
# target linkable and registers it.
add_executable(harness_cli_threads_test source/harness_cli_threads_test.cpp)
target_link_libraries(harness_cli_threads_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_cli_threads_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_cli_threads_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_threads_test COMMAND harness_cli_threads_test)

# The interleaved walk: the seeded order `detail::shuffledOrder` answers,
# the seed forms `--interleave` accepts and refuses, the rows a run holds
# back and prints after the walk, and the interrupted run that prints the
# rows it ran (T044; FR-051 to FR-060, SC-005).
# This is the pass that defines `detail::shuffledOrder` in
# `source/harness/options.cpp`, so it is the pass that first makes the
# target linkable and registers it.
add_executable(harness_cli_interleave_test source/harness_cli_interleave_test.cpp)
target_link_libraries(harness_cli_interleave_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_cli_interleave_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_cli_interleave_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_interleave_test
         COMMAND harness_cli_interleave_test ${CMAKE_SOURCE_DIR})

# The base command line replayed over the built example binary: each row
# of the matrix file names a command line, the status the base answers
# and the shortest fault line it prints, and the runner prints one line
# per row that moved (T003; FR-003, FR-013, SC-007, SC-013). Only a tree
# that builds the examples holds the binary the runner drives, so this
# registration is conditional the way `report_shape` is.
if(TARGET docs_command_line)
  add_test(
      NAME base_cli_matrix
      COMMAND
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/base_cli/run_matrix.py
        $<TARGET_FILE:docs_command_line>
        ${CMAKE_SOURCE_DIR}/test/data/base_cli_matrix.txt
  )
  set_tests_properties(base_cli_matrix PROPERTIES TIMEOUT 120)
endif()

# The catalog listing over a scripted leaf set (T025; US3, FR-036, FR-037,
# SC-009).
add_executable(harness_catalog_test source/harness_catalog_test.cpp)
target_link_libraries(harness_catalog_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_catalog_test PRIVATE cxx_std_23)
add_test(NAME harness_catalog_test COMMAND harness_catalog_test)

# The harness paths the feature's own tests leave unmeasured (T071; PR-4b,
# FR-005, FR-017, FR-021, FR-023, FR-024, FR-032, FR-034, FR-038).
add_executable(harness_gap_test source/harness_gap_test.cpp)
target_link_libraries(harness_gap_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_gap_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_gap_test PRIVATE cxx_std_23)
add_test(NAME harness_gap_test COMMAND harness_gap_test)

# The foundational jitter probe surface: the option defaults of data-model.md
# E-01 (FR-003, R-15), the sixteen-field stats aggregate in the E-03 order
# (FR-002), the three mode enumerators (E-02, R-10), the declared entry point
# and seam signatures (contracts/public-api.md, R-02), and the record and
# per-cpu state shapes (E-04, E-05).
add_executable(harness_jitter_probe_test
               source/harness_jitter_probe_test.cpp)
target_link_libraries(harness_jitter_probe_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_jitter_probe_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_jitter_probe_test PRIVATE cxx_std_23)
add_test(NAME harness_jitter_probe_test COMMAND harness_jitter_probe_test)

# The no-allocation proof of the probe loop (T003; FR-041, SC-005, Q-2,
# K-13): the proof replaces the global operator new and delete - the
# basic four forms and the four aligned forms std::align_val_t names -
# and the thread sanitizer runtime defines those operators too, so the
# two definitions cannot link, and this proof stays out of that preset
# for the same reason as counters_noalloc_test above. The threaded
# probe scenarios stand in harness_jitter_probe_test, which replaces
# nothing, so that preset builds them.
if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "fsanitize=thread")
  add_executable(harness_jitter_probe_alloc_test
                 source/harness_jitter_probe_alloc_test.cpp)
  target_link_libraries(harness_jitter_probe_alloc_test
                        PRIVATE speedgun-ng::harness)
  target_include_directories(harness_jitter_probe_alloc_test
                             PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
  target_compile_features(harness_jitter_probe_alloc_test PRIVATE cxx_std_23)
  add_test(NAME harness_jitter_probe_alloc_test
           COMMAND harness_jitter_probe_alloc_test)
endif()

# The probe's report surfaces (T007): the console block of a run with the
# probe (FR-023, R-08), the run-level and per-entry JSON keys (FR-026,
# FR-027), the round trip (FR-029), the raw gap list (FR-028), and the
# reader error codes on the new keys.
add_executable(harness_jitter_window_test
               source/harness_jitter_window_test.cpp)
target_link_libraries(harness_jitter_window_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_jitter_window_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_jitter_window_test PRIVATE cxx_std_23)
add_test(NAME harness_jitter_window_test
         COMMAND harness_jitter_window_test)

# The option grammar of contracts/cli-options.md (T008): every
# status-2 case, the standing getopt paths, the option-value uniqueness
# over the parser's own table (FR-011, R-06), and the probe-free shape
# (FR-013, SC-003). The uniqueness scan reads the parser's source
# through the path the registration hands the binary, the way the
# schema tests read their schema path.
add_executable(harness_precheck_test source/harness_precheck_test.cpp)
target_link_libraries(harness_precheck_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_precheck_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_precheck_test PRIVATE cxx_std_23)
add_test(NAME harness_precheck_test COMMAND harness_precheck_test)

add_executable(harness_jitter_cli_test source/harness_jitter_cli_test.cpp)
target_link_libraries(harness_jitter_cli_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_jitter_cli_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_jitter_cli_test PRIVATE cxx_std_23)
target_compile_definitions(
    harness_jitter_cli_test
    PRIVATE SG_OPTIONS_SOURCE="${CMAKE_SOURCE_DIR}/source/harness/options.cpp")
add_test(NAME harness_jitter_cli_test COMMAND harness_jitter_cli_test)

# The public surface of the noise tracer (specs/036 T001, T002): the
# struct defaults of data-model.md E-16 and E-17, and the source-order
# binding rows of FR-005. The carrier rows drive the scan and the
# gather the way `harness_cli_sources_test` drives them, and they stand
# red until T007 lands the option row and the value parser; the
# suite-order row runs `speedgunMain` in a forked child the way
# `harness_jitter_cli_test` does, and it stands red until T024 lands
# the integration, the staging the tasks table records.
add_executable(harness_noise_test source/harness_noise_test.cpp)
target_link_libraries(harness_noise_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_noise_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_noise_test PRIVATE cxx_std_23)
add_test(NAME harness_noise_test COMMAND harness_noise_test)

# The record surfaces of the tracer (specs/036 T013 to T016): the console
# lines of the tracer record (FR-013, FR-016, R-14), the JSON round trip
# (FR-014, R-08, R-11), the schema shape (FR-006, D-3), and the byte
# identity (FR-012, FR-015, R-10) over hand-built records. The schema path
# enters the way `harness_json_report_test` takes it.
add_executable(harness_noise_report_test
               source/harness_noise_report_test.cpp)
target_link_libraries(harness_noise_report_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_noise_report_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_noise_report_test PRIVATE cxx_std_23)
target_compile_definitions(
    harness_noise_report_test
    PRIVATE SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"
)
add_test(NAME harness_noise_report_test COMMAND harness_noise_report_test)

# The idle resolver, the overlap check and the mask decision over
# installed layouts (T017; US2, FR-016 to FR-020, D-3 to D-5, D-9, R-07,
# PC-2).
add_executable(harness_jitter_preset_test
               source/harness_jitter_preset_test.cpp)
target_link_libraries(harness_jitter_preset_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_jitter_preset_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_jitter_preset_test PRIVATE cxx_std_23)
add_test(NAME harness_jitter_preset_test
         COMMAND harness_jitter_preset_test)
# The widest preset instance of the fit scenarios names four threads
# (instance.threadCount of source/harness_jitter_preset_test.cpp), so four
# slots cover one threaded run (030 FR-003).
set_tests_properties(harness_jitter_preset_test PROPERTIES PROCESSORS 4)

# The machine layout read, the two install seams and the mask intersection
# (T007; US1, FR-001, FR-002, FR-003, FR-005, FR-006, R-04, R-11, R-13,
# SC-04, Q-5, Q-8).
add_executable(harness_topology_test source/harness_topology_test.cpp)
target_link_libraries(harness_topology_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_topology_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_topology_test PRIVATE cxx_std_23)
add_test(NAME harness_topology_test COMMAND harness_topology_test)

# The preset table, the fit rule and the runner wiring over installed
# layouts (T008, T009; US1, FR-008 to FR-019, FR-021, E-09, SC-02, SC-03,
# SC-04, R-05).
add_executable(harness_placement_preset_test
               source/harness_placement_preset_test.cpp)
target_link_libraries(harness_placement_preset_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_placement_preset_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_placement_preset_test PRIVATE cxx_std_23)
add_test(NAME harness_placement_preset_test
         COMMAND harness_placement_preset_test)

# specs/035-numa-placement: the node walk, the binder seam and the node
# plan over synthetic layouts of one, two and four nodes (T003, T005,
# T007, T008, T009, T016; FR-005, FR-009 to FR-013, FR-016).
add_executable(harness_numa_placement_test
               source/harness_numa_placement_test.cpp)
target_link_libraries(harness_numa_placement_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_numa_placement_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_numa_placement_test PRIVATE cxx_std_23)
add_test(NAME harness_numa_placement_test
         COMMAND harness_numa_placement_test)

# The placement option, the handle preset and the stop rule (T019; US3,
# FR-015, FR-016, FR-017, FR-036, contracts/cli-placement.md. prose-lint: allow reason="verbatim file name"
add_executable(harness_cli_placement_test
               source/harness_cli_placement_test.cpp)
target_link_libraries(harness_cli_placement_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_cli_placement_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_cli_placement_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_placement_test
         COMMAND harness_cli_placement_test)

# The topology line, the cpu@group field, the cut mark and the byte-for-
# byte no-preset block (T015; US2, FR-020 to FR-022, R-06, R-09, SC-05,
# contracts/report-topology.md).
add_executable(harness_placement_report_test
               source/harness_placement_report_test.cpp)
target_link_libraries(harness_placement_report_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_placement_report_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_placement_report_test PRIVATE cxx_std_23)
add_test(NAME harness_placement_report_test
         COMMAND harness_placement_report_test)

# The group name of the public disclosure record, read off the aggregate
# the header owns (specs/039 T001, T002; FR-005, FR-031, FR-040, A-1 of
# the partition contract). Wave 5 adds the console segment and the report
# key to this same entry (FR-028, FR-029).
add_executable(harness_group_report_test
               source/harness_group_report_test.cpp)
target_link_libraries(harness_group_report_test
                      PRIVATE speedgun-ng::harness)
# The console block reads the printers, the fit walk and the layout reader at
# the internal seam, and installs the machine layout the machine line names.
# The document block reads a written document against the shipped schema.
target_include_directories(harness_group_report_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_definitions(
    harness_group_report_test
    PRIVATE SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"
)
target_compile_features(harness_group_report_test PRIVATE cxx_std_23)
add_test(NAME harness_group_report_test
         COMMAND harness_group_report_test)

# The group layout reader: the partitions the grammar accepts and every fault
# of the layout contract's table C (specs/039-context-groups T007, T008;
# FR-012 to FR-018, FR-021). The reader reads one file it is handed, so the
# test needs the harness internal header and no topology seam.
add_executable(harness_group_layout_test
               source/harness_group_layout_test.cpp)
target_link_libraries(harness_group_layout_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_group_layout_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_group_layout_test PRIVATE cxx_std_23)
add_test(NAME harness_group_layout_test
         COMMAND harness_group_layout_test)

# The partition of one instance's contexts into the groups one preset
# names, over the installed shapes and the reads that name no unit
# (specs/039 T005, T006; FR-001 to FR-010, A-1 to A-6, B-1, B-2).
add_executable(harness_group_partition_test
               source/harness_group_partition_test.cpp)
target_link_libraries(harness_group_partition_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_group_partition_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_group_partition_test PRIVATE cxx_std_23)
add_test(NAME harness_group_partition_test
         COMMAND harness_group_partition_test)

# The `--group-layout` row: the word the scan recognizes, the record the row
# carries, the line the help answer prints and the source the row refuses
# (specs/039-context-groups T010, T012; FR-011; specs/028 FR-015, FR-018,
# FR-026). The option table and the run record stand behind the harness source
# directory, the way harness_cli_table_test reaches them.
add_executable(harness_group_cli_test source/harness_group_cli_test.cpp)
target_link_libraries(harness_group_cli_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_group_cli_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_group_cli_test PRIVATE cxx_std_23)
add_test(NAME harness_group_cli_test
         COMMAND harness_group_cli_test)

# The rounds of a grouped threaded attempt: the partition each run carries,
# the rounds each group meets on, the fixture pair of a split attempt, and the
# ledger fault of a stranded member (specs/039-context-groups T017; FR-001,
# FR-004, FR-010, FR-022 to FR-027, D-7, D-9).
add_executable(harness_group_run_test
               source/harness_group_run_test.cpp)
target_link_libraries(harness_group_run_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_group_run_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_group_run_test PRIVATE cxx_std_23)
add_test(NAME harness_group_run_test COMMAND harness_group_run_test)
# The grouped scenarios run eight contexts at once, the widest the file
# installs, so eight slots cover one attempt (030 FR-003).
set_tests_properties(harness_group_run_test PROPERTIES PROCESSORS 8)

# The barrier codegen shape at -O2 in every compiler present (T036; US6,
# FR-029, FR-030).
add_test(NAME barrier_shape
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/barrier_shape.sh)

# The timed-loop codegen shape at -O2 in every compiler present (IF-04;
# FR-004, FR-005, R-06). The build directory is passed so the gate finds
# the generated export header in any build tree (FU-4). This directory is
# a project of its own, so the top-level build tree is CMAKE_BINARY_DIR.
add_test(NAME loop_shape
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/loop_shape.sh ${CMAKE_BINARY_DIR})

# The run-control shape of the entry point: one flag reading per pass and
# a run-wide postcondition that reads no flag (FU-1; FR-032, FR-036, R-06).
add_test(NAME cli_shape
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/cli_shape.sh)

# The repetition aggregates against the printed rows, and the gap that
# drops a sample (T032; US5, FR-025, FR-027, FR-028, FR-035, SC-003).
add_executable(harness_statistics_test source/harness_statistics_test.cpp)
target_link_libraries(harness_statistics_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_statistics_test PRIVATE cxx_std_23)
add_test(NAME harness_statistics_test COMMAND harness_statistics_test)

# The registry, the handle options, and the contract violation of a setter
# after the run starts (T014; FR-001, FR-002, FR-010, FR-013, FR-015, E-02).
add_executable(harness_registry_test source/harness_registry_test.cpp)
target_link_libraries(harness_registry_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_registry_test PRIVATE cxx_std_23)
add_test(NAME harness_registry_test COMMAND harness_registry_test)

# The argument families and the instance expansion (T016; US1, FR-001
# to FR-005, FR-007, FR-008, FR-010, R-12).
add_executable(harness_family_test source/harness_family_test.cpp)
target_link_libraries(harness_family_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_family_test PRIVATE cxx_std_23)
add_test(NAME harness_family_test COMMAND harness_family_test)

# The capture registration macro (T026; US3, FR-013, R-13). One capture
# registers the one instance `fn/captureName`, the captured arguments
# draw no segment of their own, and the values reach the function.
add_executable(harness_capture_macro_test source/harness_capture_macro_test.cpp)
target_link_libraries(harness_capture_macro_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_capture_macro_test PRIVATE cxx_std_23)
add_test(NAME harness_capture_macro_test COMMAND harness_capture_macro_test)

# The template registration macro (T026; US3, FR-014, R-13). The type
# list enters the instance name stringified as written at the macro
# site, including a qualified argument written with a space.
add_executable(harness_template_test source/harness_template_test.cpp)
target_link_libraries(harness_template_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_template_test PRIVATE cxx_std_23)
add_test(NAME harness_template_test COMMAND harness_template_test)

# The instance names of every family shape, the FR-012 duplicate, and
# the suite and case pair of FR-021 (T021; US2, US4, FR-010, FR-011,
# FR-012, FR-021, R-07, R-08, R-12). The suite reads the expansion and
# the result value at the internal seam, so it includes
# "detail/internal.hpp" the way harness_gap_test does.
add_executable(harness_instance_name_test source/harness_instance_name_test.cpp)
target_link_libraries(harness_instance_name_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_instance_name_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_instance_name_test PRIVATE cxx_std_23)
add_test(NAME harness_instance_name_test COMMAND harness_instance_name_test)

# The fixture base, the fixture macros, the pair around each run, the
# one fixture object per run, and the fixture method named DISABLED_x
# (T035; US4, FR-015, FR-016, FR-017, FR-020, R-09, SC-005).
add_executable(harness_fixture_test source/harness_fixture_test.cpp)
target_link_libraries(harness_fixture_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_fixture_test PRIVATE cxx_std_23)
add_test(NAME harness_fixture_test COMMAND harness_fixture_test)

# The setup and teardown callback pair around each run, the untimed
# region, the last attachment winning each slot, and the argument and
# iteration reads of the callback state (T040; US5, FR-018, FR-019,
# R-05, R-10).
add_executable(harness_callback_test source/harness_callback_test.cpp)
target_link_libraries(harness_callback_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_callback_test PRIVATE cxx_std_23)
add_test(NAME harness_callback_test COMMAND harness_callback_test)

# Instance argument access and the no-allocation read inside the timed
# loop (T016; US1, FR-008, FR-009, R-04). The allocation proof replaces
# the global operator new and delete in this translation unit, so the
# thread sanitizer runtime cannot define its own pair alongside them and
# this test stays out of that preset for the same reason as
# harness_capture_test above.
if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "fsanitize=thread")
add_executable(harness_argument_test source/harness_argument_test.cpp)
target_link_libraries(harness_argument_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_argument_test PRIVATE cxx_std_23)
add_test(NAME harness_argument_test COMMAND harness_argument_test)
endif()

# The `DISABLED_` prefix gate at selection: a disabled instance registers,
# never runs, stays out of the filter match and out of list mode, and a
# fixture method named `DISABLED_x` runs because its instance name starts
# with its fixture class (T045; US6, FR-020, FR-021, R-11). The suite
# re-execs its own binary to separate the two streams, so it needs no
# resource lock of its own: the scratch files it writes are named for it.
add_executable(harness_disabled_test source/harness_disabled_test.cpp)
target_link_libraries(harness_disabled_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_disabled_test PRIVATE cxx_std_23)
add_test(NAME harness_disabled_test COMMAND harness_disabled_test)

# The two time columns of the repetition row, the run's own sample
# count, and the primary-time aggregate, over a scripted provider whose
# three machine time leaves each step by a constant per sampling action,
# so every printed figure is that step over the fixed iteration count
# (T008; US1, C-1, FR-001, FR-002, FR-003, FR-013, FR-018, SC-001,
# SC-004).
add_executable(harness_time_columns_test source/harness_time_columns_test.cpp)
target_link_libraries(harness_time_columns_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_time_columns_test PRIVATE cxx_std_23)
add_test(NAME harness_time_columns_test COMMAND harness_time_columns_test)

# The process CPU leaf behind the CPU column (T014; US2, C-2, FR-004,
# FR-002, SC-001). The scripted provider steps each machine time leaf by a
# constant per sampling action, so the switched column names the leaf the
# runner resolved. The missing-leaf script needs a machine object that
# publishes no such leaf, so that scenario runs in a re-exec of this
# binary and the parent reads its wait status.
add_executable(harness_cpu_leaf_test source/harness_cpu_leaf_test.cpp)
target_link_libraries(harness_cpu_leaf_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_cpu_leaf_test PRIVATE cxx_std_23)
add_test(NAME harness_cpu_leaf_test COMMAND harness_cpu_leaf_test)

# Real time as the primary time of a family (T013; US2, C-3, FR-001,
# FR-002, FR-005, SC-001). One scripted provider drives both machine
# time leaves and doubles each run's fold, so the two selections walk
# different lengths and the aggregate names the column it summarizes.
# Each scripted scenario owns one provider script, so it runs in a
# re-exec of this binary and the parent reads its wait status.
add_executable(harness_real_time_test source/harness_real_time_test.cpp)
target_link_libraries(harness_real_time_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_real_time_test PRIVATE cxx_std_23)
add_test(NAME harness_real_time_test COMMAND harness_real_time_test)

# Manual time as the primary time of a family (T017; US3, C-4, FR-006,
# FR-007, FR-008, SC-001). The benchmark hands the harness one nanosecond
# count per iteration, the harness sums those counts, and the sum over the
# run's iteration count fills the real column, so the growth rule grows on
# the accepted sum while the fivefold stop stands exempt. Each scripted
# scenario owns one provider script, so it runs in a re-exec of this binary
# and the parent reads the wait status. The suite reads the calibration
# header at the internal seam the way harness_growth_test does.
add_executable(harness_manual_time_test source/harness_manual_time_test.cpp)
target_link_libraries(harness_manual_time_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_manual_time_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_manual_time_test PRIVATE cxx_std_23)
add_test(NAME harness_manual_time_test COMMAND harness_manual_time_test)

# Pause and resume of a run's timing (T020; US4, C-5, FR-009 to FR-014,
# SC-002, SC-003, SC-004). One scripted provider steps every leaf by a
# constant per sampling action, so each column is the window fold minus the
# paused interval fold to the scripted nanosecond, and an attached metric
# core carries the same subtraction. Each mode owns one provider script, so
# it runs in a re-exec of this binary and the parent reads its wait status.
# The suite is registered with the internal source/harness include directory
# that harness_manual_time_test also carries.
add_executable(harness_pause_test source/harness_pause_test.cpp)
target_link_libraries(harness_pause_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_pause_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_pause_test PRIVATE cxx_std_23)
add_test(NAME harness_pause_test COMMAND harness_pause_test)

# The execution context of feature 020 (T004 to T009; US1, C-1 to C-3): the
# thread of one context runs typed work, and each scenario runs in a re-exec of
# this binary so a contract breach arrives as the signal the contract facility
# gives it. The zero-allocation scenario replaces the global allocation
# operators, which the thread sanitizer runtime also owns, so the file guards
# those hooks at the preprocessor level and one target covers both presets.
add_executable(harness_execution_context_test
               source/harness_execution_context_test.cpp)
target_link_libraries(harness_execution_context_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_execution_context_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_execution_context_test PRIVATE cxx_std_23)
add_test(NAME harness_execution_context_test
         COMMAND harness_execution_context_test)
# The widest scenario drives two dispatchers at once
# (source/harness_execution_context_test.cpp), so two slots cover the pair
# (030 FR-003).
set_tests_properties(harness_execution_context_test PROPERTIES PROCESSORS 2)

# The context barrier of feature 020 (T012; C-4, FR-014, FR-015, D-03, Q-3): a
# round releases every participant, a SPIN participant waits without sleeping
# and a participant on no context waits in BLOCK, each proven by the
# per-thread counters the kernel keeps, and a participant that leaves a round
# early aborts its re-exec, so every mode runs in a re-exec of this binary and
# the parent reads the wait status. The suite carries the internal
# source/harness include directory the pause suite carries.
add_executable(harness_context_barrier_test
               source/harness_context_barrier_test.cpp)
target_link_libraries(harness_context_barrier_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_context_barrier_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_context_barrier_test PRIVATE cxx_std_23)
add_test(NAME harness_context_barrier_test
         COMMAND harness_context_barrier_test)
# A barrier round holds four participants at the barrier: three workers plus
# the thread that opens the round (kRoundParticipants of
# source/harness_context_barrier_test.cpp), so four slots cover one round
# (030 FR-003).
set_tests_properties(harness_context_barrier_test PROPERTIES PROCESSORS 4)

# The benchmark over N contexts of feature 020 (T014 to T017; US2, C-5, FR-016,
# FR-017, FR-020, D-06; SC-06, SC-07): the thread index and count a state
# reads, the name segment of the count, and the merged row of the run with the
# decision seconds of each timing mode divided by that count. Every time figure
# stands on scripted leaves of the fake provider, so each scenario runs in a
# re-exec of this binary and the parent reads the wait status. The suite carries
# the internal source/harness include directory the pause suite carries. The
# suite also pins one context with the shipped placement function, runs a body
# that throws on every context, and counts the fixture pair of one threaded
# attempt.
add_executable(harness_threads_test
               source/harness_threads_test.cpp)
target_link_libraries(harness_threads_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_threads_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_threads_test PRIVATE cxx_std_23)
add_test(NAME harness_threads_test
         COMMAND harness_threads_test)
# The benchmark of the threaded scenarios runs four contexts at once
# (kThreadCount of source/harness_threads_test.cpp), so four slots cover one
# run (030 FR-003).
set_tests_properties(harness_threads_test PROPERTIES PROCESSORS 4)

# The disclosure line and the noise warning of the report (T022 to T024; US4,
# C-7, FR-025, FR-026, FR-028, E-07, SC-12). Each scenario builds the policy
# vector a threaded run leaves on its result and prints the result through the
# report's own printer, so no benchmark runs, no provider registers and no
# thread starts. The suite reads the printer at the internal seam the way
# harness_pause_test reads the calibration header.
add_executable(harness_context_report_test
               source/harness_context_report_test.cpp)
target_link_libraries(harness_context_report_test
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_context_report_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_context_report_test PRIVATE cxx_std_23)
add_test(NAME harness_context_report_test
         COMMAND harness_context_report_test)

# FR-024, T002: the fixed pause allowance leaves the tree, library, tests and
# guide alike. A namespace-scope name that does not exist cannot be probed by
# a `requires` clause, where the failed lookup is a hard error and not a
# substitution failure, so the removal is asserted textually over the three
# trees it could be spelled in (run-fixes contract, quickstart 2). The
# bracket in the pattern keeps the gate from matching its own spelling of the
# name, which would make the check unsatisfiable.
add_test(
    NAME pause_bound_removed
    COMMAND bash -c "! grep -rn 'kMaxPause[P]erRun' ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/test ${CMAKE_SOURCE_DIR}/docs"
)

# FR-001 to FR-008, T010: the user counter store, the flag pass and the counter
# columns. Every scenario scripts the fake provider, so each mode runs in a
# re-exec of this binary and the parent reads the wait status of the child that
# breaches a precondition. The suite carries the internal source/harness
# include directory the pause suite carries.
add_executable(harness_user_counters_test source/harness_user_counters_test.cpp)
target_link_libraries(harness_user_counters_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_user_counters_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_user_counters_test PRIVATE cxx_std_23)
add_test(NAME harness_user_counters_test COMMAND harness_user_counters_test)

# The throughput columns of a run (T016; US2, FR-009, FR-010). One scripted
# provider steps every machine time leaf by a constant per sampling action, so
# each printed rate is the value the body passes divided by that scripted fold
# and by the declared base. The three families divide by the CPU column, the
# real column and the manual sum, so each mode owns one provider script and
# runs in a re-exec of this binary.
add_executable(harness_throughput_test source/harness_throughput_test.cpp)
target_link_libraries(harness_throughput_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_throughput_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_throughput_test PRIVATE cxx_std_23)
add_test(NAME harness_throughput_test COMMAND harness_throughput_test)

# The trailing label field of a row (T018; US4, FR-011, FR-012). One scripted
# provider steps every machine time leaf by a constant per sampling action, so
# the rows are the ones the label field joins or leaves untouched. Each mode
# owns one provider script, so it runs in a re-exec of this binary and the
# parent reads the wait status of the child that breaches the newline bound.
add_executable(harness_label_test source/harness_label_test.cpp)
target_link_libraries(harness_label_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_label_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_label_test PRIVATE cxx_std_23)
add_test(NAME harness_label_test COMMAND harness_label_test)

# Custom aggregate statistics (T021; US5, FR-013 to FR-015, SC-03). The
# scripted CPU leaf gives the three repetitions primary folds of 800, 1600 and
# 2400 ns over four iterations, so the primary list the statistic functions
# receive is 200, 400 and 600 ns per iteration and the printed range and
# percentage have exact expected values. Each mode owns one provider script, so
# it runs in a re-exec of this binary and the parent reads the wait status of
# the child that breaches the reserved built-in name.
add_executable(harness_custom_stats_test source/harness_custom_stats_test.cpp)
target_link_libraries(harness_custom_stats_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_custom_stats_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_custom_stats_test PRIVATE cxx_std_23)
add_test(NAME harness_custom_stats_test COMMAND harness_custom_stats_test)

# A breached child dies at a contract call, so libgcov never runs the exit hook
# that writes its counters, and the lines that child executed stay uncovered. The
# handler in source/detail/gcov_flush.hpp writes them first, but it reaches the
# dump through a weak reference, which leaves the archive member out of the link
# and the address null. Forcing the symbol is what makes the dump reachable, and
# only in a tree built with coverage instrumentation.
if(ENABLE_COVERAGE)
  set(breached_tests
    harness_user_counters_test
    harness_label_test
    harness_custom_stats_test
  )
  foreach(breached IN LISTS breached_tests)
    target_link_options(${breached} PRIVATE -Wl,-u,__gcov_dump)
  endforeach()
endif()

# The aggregate-only modes (T024; US6, FR-016 to FR-018). One scripted provider
# steps both machine time leaves by a constant per sampling action, so a
# three-repetition family holds three measured rows beside its aggregate and a
# one-repetition family holds one row and no aggregate. Each mode owns one
# provider script, so it runs in a re-exec of this binary.
add_executable(harness_aggregates_only_test source/harness_aggregates_only_test.cpp)
target_link_libraries(harness_aggregates_only_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_aggregates_only_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_aggregates_only_test PRIVATE cxx_std_23)
add_test(NAME harness_aggregates_only_test COMMAND harness_aggregates_only_test)

# The complexity fit of a family (T027; US7, FR-019 to FR-023, SC-04). Every
# family of the scenario runs on manual time with one iteration, so the value
# the fit reads is the value the body states, and each planted curve has an
# expected coefficient and RMS printed beside it, no tolerance band: the fit of
# a quadratic family with a counter and a metric, a fixed curve over the same
# data, a user curve, a family that states its own problem size, and the three
# shapes that hold no fit.
add_executable(harness_complexity_test source/harness_complexity_test.cpp)
target_link_libraries(harness_complexity_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_complexity_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_complexity_test PRIVATE cxx_std_23)
add_test(NAME harness_complexity_test COMMAND harness_complexity_test)

# The printed time unit of a run (T023; US5, C-6, FR-015, FR-016, SC-005).
# One scripted provider steps every machine time leaf by a constant per
# sampling action, so each printed figure is that step divided by the fixed
# iteration count and again by the unit divisor, and the aggregate carries the
# scale of the column it summarizes. Each mode owns one provider script, so it
# runs in a re-exec of this binary and the parent reads its wait status.
add_executable(harness_time_unit_test source/harness_time_unit_test.cpp)
target_link_libraries(harness_time_unit_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_time_unit_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_time_unit_test PRIVATE cxx_std_23)
add_test(NAME harness_time_unit_test COMMAND harness_time_unit_test)

# The pass plan, the pass loop and the pass disclosure (T007 registers the
# three skeletons so the build stays green ahead of the first assertion;
# T016 to T021, T022 to T028 and T039 to T046 fill them; FR-001 to FR-024,
# FR-033). The pass plan header is internal to the harness archive, so each
# target carries the source/harness include directory the way
# harness_percentile_test does.
add_executable(harness_pass_plan_test source/harness_pass_plan_test.cpp)
target_link_libraries(harness_pass_plan_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_pass_plan_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_pass_plan_test PRIVATE cxx_std_23)
add_test(NAME harness_pass_plan_test COMMAND harness_pass_plan_test)

add_executable(harness_pass_collect_test source/harness_pass_collect_test.cpp)
target_link_libraries(harness_pass_collect_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_pass_collect_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_pass_collect_test PRIVATE cxx_std_23)
add_test(NAME harness_pass_collect_test COMMAND harness_pass_collect_test)
# The threaded arm of the pass scenarios runs two contexts at once
# (kThreadCount of source/harness_pass_collect_test.cpp), so two slots cover
# it (030 FR-003).
set_tests_properties(harness_pass_collect_test PROPERTIES PROCESSORS 2)

add_executable(harness_pass_report_test source/harness_pass_report_test.cpp)
target_link_libraries(harness_pass_report_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_pass_report_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_pass_report_test PRIVATE cxx_std_23)
add_test(NAME harness_pass_report_test COMMAND harness_pass_report_test)

# The byte-identity scenario of T042 compares the automatic run against the
# bytes captured at the audit point, so its entry names that fixture beside
# the scenario: the provider limit of the capture belongs to the scenario, not
# to the list the shared process walks.
add_test(NAME harness_pass_report_test_identity
         COMMAND harness_pass_report_test byte-identity
                 ${CMAKE_CURRENT_SOURCE_DIR}/source/detail/pass_identity.expected)
# Both entries capture the printed report through the one fixed relative
# name harness_pass_report_out.txt of
# source/harness_pass_report_test.cpp, so neither may read it while the
# other writes it (030 FR-004, P-13).
set_tests_properties(
  harness_pass_report_test harness_pass_report_test_identity
  PROPERTIES RESOURCE_LOCK harness_pass_report_out
)

# ---- Hunter mode core (feature 029) ----

# T007 registers the seven skeletons so the build carries the hunt tree ahead
# of the first assertion; the story tasks named beside each target fill it.
# The hunt plan header is internal to the harness archive, so each target
# carries the source/harness include directory the way
# harness_pass_plan_test does.

# The column shape, the limit rows, the budget split and the seed order
# (T015; US1, FR-002, FR-011, FR-041, AC-1, AC-2, SC-01).
add_executable(harness_hunt_plan_test source/harness_hunt_plan_test.cpp)
target_link_libraries(harness_hunt_plan_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_plan_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_plan_test PRIVATE cxx_std_23)
add_test(NAME harness_hunt_plan_test COMMAND harness_hunt_plan_test)

# The normalization, the median and CV, the noise mark and the rank order
# (T016; US1, FR-030, AC-3, I-5 to I-7).
add_executable(harness_hunt_rank_test source/harness_hunt_rank_test.cpp)
target_link_libraries(harness_hunt_rank_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_rank_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_rank_test PRIVATE cxx_std_23)
add_test(NAME harness_hunt_rank_test COMMAND harness_hunt_rank_test)

# The ranker's invariant trap fixture is NOT a test (it aborts); it is a
# driven executable the trap checker below runs (specs/032 FR-048), the
# shape `counters_trap_fixture.cpp` stands in for the recorder.
add_executable(harness_hunt_rank_trap_fixture
               source/harness_hunt_rank_trap_fixture.cpp)
target_link_libraries(harness_hunt_rank_trap_fixture
                      PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_rank_trap_fixture
                           PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_rank_trap_fixture PRIVATE cxx_std_23)
add_executable(harness_hunt_rank_trap_test
               source/harness_hunt_rank_trap_test.cpp)
target_link_libraries(harness_hunt_rank_trap_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_hunt_rank_trap_test PRIVATE cxx_std_23)
add_test(
  NAME harness_hunt_rank_trap_test
  COMMAND
    harness_hunt_rank_trap_test
    $<TARGET_FILE:harness_hunt_rank_trap_fixture>)

# The collection walk, the skipped kinds and the retry route
# (T017; US1, FR-021, FR-022, AC-3, AC-4, SC-02, SC-04).
add_executable(harness_hunt_collect_test source/harness_hunt_collect_test.cpp)
target_link_libraries(harness_hunt_collect_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_collect_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_collect_test PRIVATE cxx_std_23)
add_test(NAME harness_hunt_collect_test COMMAND harness_hunt_collect_test)
# The threaded arm of the collection scenarios runs two contexts at once
# (kThreadCount of source/harness_hunt_collect_test.cpp), so two slots cover
# it (030 FR-003).
set_tests_properties(harness_hunt_collect_test PROPERTIES PROCESSORS 2)

# One case per refusal row of contracts/hunt-cli.md, in process and forked
# (T018; US1, FR-001, FR-006, FR-081, AC-5, SC-05).
add_executable(harness_hunt_cli_test source/harness_hunt_cli_test.cpp)
target_link_libraries(harness_hunt_cli_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_cli_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_cli_test PRIVATE cxx_std_23)
add_test(NAME harness_hunt_cli_test COMMAND harness_hunt_cli_test)

# The plan line, the ranked table, the skipped list, the estimate and the cut
# line (T032; US2, FR-040 to FR-043, D-19, D-20).
add_executable(harness_hunt_report_test source/harness_hunt_report_test.cpp)
target_link_libraries(harness_hunt_report_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_report_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_report_test PRIVATE cxx_std_23)
add_test(NAME harness_hunt_report_test COMMAND harness_hunt_report_test)

# The hunt objects of the written document against the schema the compiled-in
# path names, and the round trip back (T038; US3, FR-051, AC-6,
# contracts/hunt-json.md). The schema path enters the way
# harness_json_report_test takes it.
add_executable(harness_hunt_json_test source/harness_hunt_json_test.cpp)
target_link_libraries(harness_hunt_json_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_hunt_json_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_hunt_json_test PRIVATE cxx_std_23)
target_compile_definitions(
    harness_hunt_json_test
    PRIVATE SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"
)
add_test(NAME harness_hunt_json_test COMMAND harness_hunt_json_test)

# The byte-identity scenario compares a run with no hunt option against the
# bytes committed under test/data, so its entry names that directory beside
# scenario, the shape harness_pass_report_test_identity gives its fixture
# (T038; FR-052, SC-006).
add_test(NAME harness_hunt_json_test_no_hunt
         COMMAND harness_hunt_json_test no-hunt-bytes
                 ${CMAKE_SOURCE_DIR}/test/data)

# ---- Execution graphs: model (M1) ----

# The graph model, the four outcome words, the start order and the
# slot-to-node map (specs/037-execution-graphs T001; US1; FR-008, FR-013,
# FR-019, FR-020; R-04, R-06). The model is data and pure rules, so no
# scenario runs a benchmark, registers a provider, starts a thread or
# reads a clock (FR-026). The model header is internal to the harness
# archive, so the target carries the source/harness include directory the
# way harness_pass_plan_test does.
add_executable(harness_graph_model_test source/harness_graph_model_test.cpp)
target_link_libraries(harness_graph_model_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_graph_model_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_graph_model_test PRIVATE cxx_std_23)
add_test(NAME harness_graph_model_test COMMAND harness_graph_model_test)

# ---- Main-page documentation gates (feature 018) ----

# The root README.md teaches the harness surface, and every code block it
# quotes is a marked region of a file under example/. One tool scans the
# page against the tree in four passes: each marked block against its
# region, the feature-section list against the sections the page carries,
# the quick-reference rows against the public headers, and the option rows
# against the command-line table. It then checks the list of maintainer
# substance the page must stay free of (specs/018-docs-main-page FR-031 to
# FR-035, FR-038; constitution IV.1). The scan reads no build output, so it
# is registered unconditionally: a tree that configures no examples still
# holds the page, the example sources, and the drift it may have grown.
add_test(
    NAME example_drift
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/docs/example_drift.py
            ${CMAKE_SOURCE_DIR}
)

# The scans are proven live. Each fixture run builds a scratch tree, plants
# one drift of every class the tool claims to catch, and requires the tool
# to name it, so a clean verdict cannot come from a scan that inspects
# nothing (FR-033, SC-008).
add_test(
    NAME example_drift_fixtures
    COMMAND
      python3
      ${CMAKE_CURRENT_SOURCE_DIR}/example-drift-fixture/run_example_drift_fixtures.py
      ${CMAKE_SOURCE_DIR}
)

# The same liveness argument for the report-shape comparator's mask: a
# mask that masks nothing would let a renamed column pass as a changed
# number (FR-012, SC-004).
add_test(
    NAME report_shape_fixtures
    COMMAND
      python3
      ${CMAKE_CURRENT_SOURCE_DIR}/example-drift-fixture/run_report_shape_fixtures.py
      ${CMAKE_SOURCE_DIR}
)

# Every C++ block of the page is a complete top-level declaration, so the
# reader can paste it into the program the quick start builds. This gate
# emits the blocks, wraps each in the canonical include prelude, and
# syntax-compiles the result with the configured compiler. It rejects one
# planted broken snippet first, so a run that compiled nothing cannot pass
# (FR-011, SC-003; contract example-drift R-10).
add_test(
    NAME snippet_compile
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/snippet-compile/run.sh
            ${CMAKE_SOURCE_DIR} ${CMAKE_CXX_COMPILER}
)

if(TARGET first_benchmark)
  # The sample report block on the page is compared with what the quick
  # start binary really prints, by shape: a digit that differs passes, a
  # renamed or missing column fails. Only a tree that builds the examples
  # has a binary to run, so this is the one conditional registration of the
  # group (FR-012, SC-004).
  add_test(
      NAME report_shape
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:first_benchmark>
        --readme ${CMAKE_SOURCE_DIR}/README.md
  )
  set_tests_properties(report_shape PROPERTIES TIMEOUT 120)
endif()

if(TARGET docs_reading_the_report)
  # The user guide page carries a second recorded run: the report of the
  # example that prints a counter column, a label, a custom statistic and the
  # fit. The same shape fold checks it against a real run, so a renamed or
  # dropped column there fails the suite too (T057, FR-006).
  add_test(
      NAME report_shape_guide_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_reading_the_report>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example reading_the_report
  )
  set_tests_properties(report_shape_guide_sample PROPERTIES TIMEOUT 120)
endif()

if(TARGET docs_percentiles)
  # The same page carries the percentile run of the percentile example, so the
  # selector names which recorded run this invocation owns. The fold checks the
  # recorded file against a real run of the example, which is what keeps a
  # quoted report from drifting away from what the harness prints (021 FR-025,
  # SC-011).
  add_test(
      NAME report_shape_percentiles_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_percentiles>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example percentiles
  )
  set_tests_properties(report_shape_percentiles_sample PROPERTIES TIMEOUT 120)
endif()

if(TARGET docs_json_reports)
  # The guide page carries the recorded run of the JSON report example, so the
  # selector names which recorded run this invocation owns. The fold checks the
  # recorded file against a real run of the example (023 FR-038, SC-011).
  add_test(
      NAME report_shape_json_reports_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_json_reports>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example json_reports
  )
  set_tests_properties(report_shape_json_reports_sample PROPERTIES TIMEOUT 120)
endif()
if(TARGET docs_jitter_probe)
  # The guide page carries the recorded run of the jitter probe example, so
  # the selector names which recorded run this invocation owns. The fold
  # checks the recorded file against a real run of the example, which is what
  # keeps a quoted probe block from drifting away from what the harness
  # prints (027 FR-034, FR-035). The fold reads the cpus the host gives the
  # run out of both sides, so a run under an affinity mask still describes a
  # block recorded on a wider host (033 FR-042).
  add_test(
      NAME report_shape_jitter_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_jitter_probe>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example jitter_probe
  )
  set_tests_properties(report_shape_jitter_sample PROPERTIES TIMEOUT 120)
endif()

if(TARGET docs_numa_placement)
  # The guide page carries the recorded node-planned run of the numa
  # example, so the fold checks the recorded file against a real run of
  # the example, which is what keeps a quoted node-planned report from
  # drifting away from what the harness prints (specs/035 FR-022, SC-006).
  add_test(
      NAME report_shape_numa_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_numa_placement>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example numa_placement
  )
  set_tests_properties(report_shape_numa_sample PROPERTIES TIMEOUT 120)
endif()

if(TARGET docs_hunt_rank)
  # The guide page carries the recorded run of the hunter example, so the selector
  # names the golden that owns this invocation's block. The fold checks the
  # recorded run against a real run of the example, which is what fails the suite
  # when a hunt column is renamed or dropped while the page still quotes it
  # (T109, FR-070, PD-19).
  add_test(
      NAME report_shape_hunt_rank_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_hunt_rank>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example hunt_rank
  )
  set_tests_properties(report_shape_hunt_rank_sample PROPERTIES TIMEOUT 120)
endif()

# Each docs example is a complete program whose regions the page quotes, so the
# gate runs every one of them and requires a row for each registered benchmark,
# with no FAILED row and with every skip declared in the source (T035; FR-034,
# FR-035, SC-14, D-12, D-15). Only a tree that builds the examples holds the
# binaries the gate runs, so this registration is conditional the way
# `report_shape` is.
if(TARGET docs_run_loop)
  add_test(
      NAME example_run
      COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/example_run.sh ${CMAKE_BINARY_DIR}
  )
  set_tests_properties(example_run PROPERTIES TIMEOUT 300)
endif()

# The run gate is proven live. Each fixture run builds a scratch tree of fake
# docs_ binaries, plants one outcome of the four the gate tells apart, and
# requires the gate to name it, so a clean verdict cannot come from a scan that
# inspects nothing (FR-035, SC-14). The scratch trees hold their own binaries,
# so this registration is unconditional.
add_test(
    NAME example_run_fixtures
    COMMAND
      bash
      ${CMAKE_CURRENT_SOURCE_DIR}/example-run-fixture/run_example_run_fixtures.sh
      ${CMAKE_SOURCE_DIR}
)

# ---- Execution graphs: file (M2) ----

# The graph file, its schema and the refusal arms of FR-010
# (specs/037-execution-graphs T005; US3; FR-007, FR-009, FR-010; SC-003;
# R-03, D-1). Every arm plants one file under the system temporary
# directory and hands the reader its path, so the file system stands the
# reader's real input and no scenario starts a context (FR-011). The reader
# is internal to the harness archive, so the target carries the
# source/harness include directory the way harness_graph_model_test does.
add_executable(harness_graph_file_test source/harness_graph_file_test.cpp)
target_link_libraries(harness_graph_file_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_graph_file_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_graph_file_test PRIVATE cxx_std_23)
add_test(NAME harness_graph_file_test COMMAND harness_graph_file_test)

# ---- Execution graphs: gate (M4) ----

# The start gate of one threaded attempt: the release rule, the settlement
# words, and the two barrier rounds (specs/037-execution-graphs T010 to T018;
# US1, US2, US4; FR-013 to FR-021, FR-026, FR-027; R-06 to R-09, R-16; D-3,
# D-4, D-10). The gate-level scenarios drive `detail::GraphGate` with one
# thread per slot and no clock, and the run-level scenarios fork a child that
# installs a graph through the test seam, registers one threaded benchmark,
# and runs the entry point, so the parent reads the report the child wrote.
# The gate is internal to the harness archive, so the target carries the
# source/harness include directory the way harness_graph_model_test does.
add_executable(harness_graph_gate_test source/harness_graph_gate_test.cpp)
target_link_libraries(harness_graph_gate_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_graph_gate_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_graph_gate_test PRIVATE cxx_std_23)
add_test(NAME harness_graph_gate_test COMMAND harness_graph_gate_test)

# ---- Execution graphs: record (M5) ----

# The graph record and the console block: the console block beside the
# `contexts=` line, the record's keys, order and read-back, the schema arms,
# the byte identity and the ABI fact (specs/037-execution-graphs T024 to T031;
# US2; FR-004, FR-005, FR-006, FR-022, FR-023, FR-024, FR-028; SC-001, SC-002,
# SC-005, SC-006; R-10, R-11, R-13; D-5, D-6). The console scenarios print
# through `detail::printResult` in this process, the record scenarios write and
# read through `detail::writeReport` and `sg::loadReport`, and the schema arms
# run the standing checker over planted documents, so the schema path enters
# the way harness_hunt_json_test takes it. The record types and the printers
# stand in the harness archive, so the target carries the source/harness
# include directory the way harness_graph_model_test does.
add_executable(harness_graph_report_test source/harness_graph_report_test.cpp)
target_link_libraries(harness_graph_report_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_graph_report_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_graph_report_test PRIVATE cxx_std_23)
target_compile_definitions(
    harness_graph_report_test
    PRIVATE SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"
)
add_test(NAME harness_graph_report_test COMMAND harness_graph_report_test)

# ---- Execution graphs: cli (M3) ----

# The `--graph` row of the option table: the grammar, the bare form, the
# option-file source, the count refusals and the parse-once rule
# (specs/037-execution-graphs T020, T021; US3; FR-001, FR-002, FR-009, FR-011,
# FR-012; SC-003; R-01, R-02, R-05, R-11; contracts/cli-graph.md). Every
# scenario runs the entry point in a forked child with both streams captured to
# files the parent reads, and the graph stands in a planted file under the
# system temporary directory, so the reader reads the file system it reads in
# production and no scenario installs a graph through the seam. The row lookup
# is internal to the harness archive, so the target carries the source/harness
# include directory the way harness_graph_model_test does.
add_executable(harness_cli_graph_test source/harness_cli_graph_test.cpp)
target_link_libraries(harness_cli_graph_test PRIVATE speedgun-ng::harness)
target_include_directories(harness_cli_graph_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)
target_compile_features(harness_cli_graph_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_graph_test COMMAND harness_cli_graph_test)

# ---- Execution graphs: example (M7) ----

# The recorded run of the graph example (specs/037-execution-graphs T035;
# US6; FR-034, IV.1 of the constitution, R-15). The guide page quotes the run
# of the example whose file states a three-node chain over three contexts, so
# the fold checks the recorded block against a real run of the example, which
# is what fails the suite when a graph line is renamed or dropped while the
# page still quotes it. The example writes its own graph file and hands the
# entry point its path, so the fold runs the binary with no option beside it.
if(TARGET docs_execution_graphs)
  add_test(
      NAME report_shape_execution_graphs_sample
      COMMAND
        python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_execution_graphs>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example execution_graphs
  )
  set_tests_properties(report_shape_execution_graphs_sample PROPERTIES TIMEOUT 120)
endif()

# ---- Tier labels and budgets ----

# The entries a fast local run may leave out (030 P-05, FR-025): one row per
# slow entry, with the reason that entry is slow. The block below gives every
# other entry of this directory the quick label, so a new registration lands
# in the tier every path runs until it is listed here. CI passes no label
# filter on any ctest line, so these labels move local speed and never CI
# scope (FR-027).
set(
  SPEEDGUN_SLOW_TESTS
  # the NDEBUG-off -O0 cell configures and builds a tree of its own
  dbc_matrix_off_o0
  # the NDEBUG-off -O2 cell configures and builds a tree of its own
  dbc_matrix_off_o2
  # the NDEBUG-on -O0 cell configures and builds a tree of its own
  dbc_matrix_on_o0
  # the NDEBUG-on -O2 cell configures and builds a tree of its own
  dbc_matrix_on_o2
  # the observe negative-probe cell configures and builds a tree of its own
  dbc_matrix_observe_off_o0
  # compiles the documentation snippets, 27 to 50 s measured
  snippet_compile
  # two windows of 1,000 in-process runs, 52 min on a host granted the PMU
  harness_cli_test
)

# The five matrix rows name entries phase 6 registers; where the matrix is
# unregistered, because the preset leaves SPEEDGUN_DBC_MATRIX off or because
# phase 6 has not landed, the loop finds no entry for those rows.
get_property(speedgun_test_entries DIRECTORY PROPERTY TESTS)
foreach(speedgun_test IN LISTS speedgun_test_entries)
  if(speedgun_test IN_LIST SPEEDGUN_SLOW_TESTS)
    set_tests_properties(${speedgun_test} PROPERTIES LABELS slow)
  else()
    set_tests_properties(${speedgun_test} PROPERTIES LABELS quick)
  endif()
endforeach()

# The time budget (030 P-07, FR-031), from the same read of the directory
# TESTS property: an entry whose registration names no TIMEOUT runs under
# the 1500 s CTest default, so it takes 300 s here and a hang ends inside
# the change that caused it. An entry that names its own value keeps it, so
# the eight master 120 s rows and the 300 s example_run stand where master
# set them. The list under the loop names each of those entries with its
# reason and its measured number (FR-032).
foreach(speedgun_test IN LISTS speedgun_test_entries)
  get_test_property(${speedgun_test} TIMEOUT speedgun_test_timeout)
  if(NOT speedgun_test_timeout)
    set_tests_properties(${speedgun_test} PROPERTIES TIMEOUT 300)
  endif()
endforeach()

# The time budget exceptions (030 FR-032, D-7, SC-008): the entries that
# keep a value other than 300 s, one row per entry. Every value is one
# master names, and FR-031 keeps every lower value master sets. Each row
# names the reason the entry holds its own ceiling and the slowest
# reading of one lane that supports it, from CI run 40239619604 at head
# 49c12ec of branch `030-ci-speedup`, read from the five jobs named
# `test`, `test-rocky`, `coverage`, `sanitize` and `tsan`, read with
# `gh run view --job <job id> --log` piped to `sed -E 's/^[^Z]*Z //'`.
# That run's only red is `harness_jitter_cli_test`, the base red PC-10
# records, so each reading below is a passing duration. No ceiling moves
# here, which is what FR-034 asks of this list. Every entry outside this
# list holds the 300 s above: the slowest such reading is
# dbc_matrix_on_o0 at 138.05 s on the `tsan` lane of the same run.
#
# budget exception harness_calibration_test keeps the 120 s master
#   sets for the run-control calibration, the one entry that drives the
#   real clock: slowest 0.85 s, the `coverage` lane.
# budget exception base_cli_matrix keeps the 120 s master sets for the
#   base command line replayed over the built example binary, one
#   process per matrix row: slowest 26.54 s, the `sanitize` lane.
# budget exception report_shape keeps the 120 s master sets for the
#   shape fold of the quick start report against the root page: slowest
#   1.09 s, the `coverage` lane.
# budget exception report_shape_guide_sample keeps the 120 s master
#   sets for the shape fold of the guide page's recorded run: slowest
#   9.85 s, the `tsan` lane.
# budget exception report_shape_percentiles_sample keeps the 120 s
#   master sets for the shape fold of the percentiles example report:
#   slowest 0.22 s, the `coverage` lane.
# budget exception report_shape_json_reports_sample keeps the 120 s
#   master sets for the shape fold of the json report example: slowest
#   0.18 s, the `coverage` lane.
# budget exception report_shape_jitter_sample keeps the 120 s master
#   sets for the shape fold of the jitter probe example: slowest 1.17 s,
#   the `test` lane.
# budget exception report_shape_hunt_rank_sample keeps the 120 s
#   master sets for the shape fold of the hunt rank example: slowest
#   2.21 s, the `tsan` lane.
# budget exception example_run keeps the 300 s master sets for the
#   entry that runs every docs example through `example_run.sh`, equal
#   to the budget this loop gives: slowest 7.78 s, the `coverage` lane.

# ---- End-of-file commands ----

add_folders(Test)
