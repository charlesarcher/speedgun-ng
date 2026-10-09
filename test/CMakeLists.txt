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
  add_test(
    NAME dbc_semantics_matrix
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/dbc/semantics_matrix.sh ${CMAKE_SOURCE_DIR}
  )

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
# that feeds them runs on a fast-capable developer host. This is the one
# target that includes a private seam header, because a pure function
# behind it is otherwise reachable only from a host that owns the
# hardware.
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
add_executable(harness_capture_test source/harness_capture_test.cpp)
target_link_libraries(harness_capture_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_capture_test PRIVATE cxx_std_23)
add_test(NAME harness_capture_test COMMAND harness_capture_test)

# The run-control calibration on the real clock (T019; US2, FR-010 to
# FR-014, FR-016, SC-014).
add_executable(harness_calibration_test source/harness_calibration_test.cpp)
target_link_libraries(harness_calibration_test PRIVATE speedgun-ng::harness)
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

# The command line over a scripted provider (T020; US2, FR-009, FR-015,
# FR-021, FR-022, FR-034, FR-036, SC-007).
add_executable(harness_cli_test source/harness_cli_test.cpp)
target_link_libraries(harness_cli_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_cli_test PRIVATE cxx_std_23)
add_test(NAME harness_cli_test COMMAND harness_cli_test)

# The catalog listing over a scripted leaf set (T025; US3, FR-036, FR-037,
# SC-009).
add_executable(harness_catalog_test source/harness_catalog_test.cpp)
target_link_libraries(harness_catalog_test PRIVATE speedgun-ng::harness)
target_compile_features(harness_catalog_test PRIVATE cxx_std_23)
add_test(NAME harness_catalog_test COMMAND harness_catalog_test)

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

# ---- End-of-file commands ----

add_folders(Test)
