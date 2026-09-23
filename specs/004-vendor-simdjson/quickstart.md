# Quickstart / Validation: Vendor simdjson as a Private, Pinned Submodule

**Feature**: 004-vendor-simdjson | **Date**: 2026-09-23

Runnable scenarios that prove the vendoring, the `add_subdirectory` bracket, and the privacy contract end to end. Design: [plan.md](plan.md); bracket contract: [contracts/build-integration.md](contracts/build-integration.md); audit semantics: [contracts/privacy-contract.md](contracts/privacy-contract.md); entities: [data-model.md](data-model.md). Sections 2 through 8 are the privacy audits; 9 through 12 are the bracket guarantees; 13 is the re-pin rehearsal.

## 0. Prerequisites

Linux (the enforced gate) or macOS (developer path). Unlike the hwloc import, simdjson is a native CMake library and needs no host autoconf/automake/libtool/patch (research R-001); the only prerequisites are the usual CMake >= 3.20 and a C++23 compiler.

A configured build tree (`cmake --preset=dev` per README, or `cmake --preset=ci-ubuntu` where no `dev` user preset exists) and `cmake --build --preset=dev`.

## 1. Clean clone builds (SC-001)

```sh
git clone --recurse-submodules <speedgun-ng-url>
cd speedgun-ng
cmake --preset=dev && cmake --build --preset=dev -j
ctest --preset=dev --output-on-failure
```

**Expected**: configure processes the vendored `external/simdjson` through `add_subdirectory`; the build compiles the simdjson library (visible in the log), then `simdjson_gate.cpp`, then links; all tests pass. Every Linux CI preset configures and builds identically.

## 2. Pinned commit (FR-001, US1 scenario 3)

```sh
git submodule status external/simdjson
```

**Expected**: the line begins with the pinned SHA `f5de14f09256982933af2849beb43778bd421ca7` (tag `v4.6.11`).

## 3. Version tripwire (FR-003, SC-006, US1 scenario 4)

```sh
git -C external/simdjson checkout v4.6.4
cmake --build --preset=dev 2>&1 | grep -A3 'static_assert\|error'
git -C external/simdjson checkout f5de14f09256982933af2849beb43778bd421ca7
```

**Expected**: compilation of `source/simdjson/simdjson_gate.cpp` fails with a `static_assert` diagnostic naming the expected simdjson 4.6.11 and the mismatching enum values (`SIMDJSON_VERSION_REVISION = 4`, say); restoring the pin restores a green build. The same fire pattern applies to a 4.6.x patch revision: the revision component breaks the equality (research R-002). A 3.x or 4.7 revision fails on the major/minor components.

## 4. Pristine submodule worktree (SC-007, US3 scenario 2)

```sh
cmake --build --preset=dev -j
git -C external/simdjson status --porcelain
```

**Expected**: empty output; the `add_subdirectory` build landed in `${CMAKE_BINARY_DIR}/_simdjson`, leaving the submodule tree untouched.

## 5. Static archive proof and purity (SC-008, SC-009, FR-004, FR-013)

```sh
ctest --preset=dev -R 'simdjson_nm_proof|simdjson_purity_scan' --output-on-failure
```

**Expected**: both tests pass. The nm test lists vendored simdjson objects in `libspeedgun-ng.a` with the gate object's `simdjson::get_active_implementation` reference resolving into them (no prefix: mangled names contain `simdjson`); the purity scan reports zero `find_package(simdjson` / `pkg_check_modules(simdjson` occurrences and zero simdjson includes under `include/`.

## 6. Install tree and package files (SC-002, SC-003)

```sh
cmake --install build --prefix prefix
find prefix/ -iname '*simdjson*'
grep -i simdjson prefix/lib/cmake/speedgun-ng/*.cmake
```

**Expected**: both commands print nothing (`SIMDJSON_INSTALL OFF` plus `add_subdirectory` exporting no simdjson target).

## 7. Shared-build symbol audit (SC-004)

```sh
cmake -S . -B build-shared -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build-shared -j
cmake --install build-shared --prefix prefix-shared
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -i simdjson; echo "nm-grep-exit=$?"
ldd prefix-shared/lib/libspeedgun-ng.so | grep -i simdjson; echo "ldd-grep-exit=$?"
find prefix-shared/ -iname '*simdjson*'; echo "find-exit=$?"
```

**Expected**: all three exit 1 with empty output. The scoped `BUILD_SHARED_LIBS OFF` on the vendored build (research R-005) keeps simdjson a static archive, so no `libsimdjson` object exists to name as a runtime dependency, and the hidden-visibility members (research R-007) are absent from `.dynsym`. `SIMDJSON_INSTALL OFF` holds even in this shared configuration (research R-004), so the install tree stays clean.

## 8. Downstream consumer test (FR-017, SC-005, US2 scenario 4)

On a machine carrying simdjson (on Debian-family runners: `sudo apt-get install libsimdjson-dev`):

```sh
cmake --install build --prefix prefix
cmake -S test/consumer -B build-consumer \
  -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release \
  | tee consumer-configure.log
cmake --build build-consumer -v | tee consumer-build.log
./build-consumer/consumer
grep -i simdjson consumer-configure.log consumer-build.log; echo "grep-exit=$?"
```

**Expected**: configure, build, and run all exit 0; the final grep exits 1 with empty output: zero simdjson resolutions in the consumer's configure log and link command. `test/consumer/` is the reused specs/003 consumer; it never names simdjson.

## 9. Forced static under a shared parent (FR-007, R-005)

```sh
cmake -S . -B build-shared-check -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build-shared-check -j
find build-shared-check/_simdjson -name 'libsimdjson.so*'; echo "so-exit=$?"
```

**Expected**: the `find` exits 1 with empty output: no shared simdjson object was built; the vendored archive is `libsimdjson.a` regardless of the parent's shared configuration.

## 10. Sanitizer and coverage exclusion (FR-010, FR-009)

```sh
cmake --preset=ci-sanitize && cmake --build build/sanitize -j 2>&1 | tee sanitize.log
grep -i 'simdjson' sanitize.log | grep -i 'fsanitize'; echo "grep-exit=$?"
ctest --test-dir build/sanitize --output-on-failure
```

**Expected**: the grep exits 1: the vendored archive compiles carry no sanitizer flags while the enclosing build does (the bracket blanks `CMAKE_CXX_FLAGS_SANITIZE`, research R-006); the sanitize suite stays green; every Linux job applies the identical policy because the identical bracket serves them all.

## 11. Uninitialized submodule guard (FR-002, US1 scenario 6)

```sh
git clone <speedgun-ng-url> && cd speedgun-ng   # no --recurse-submodules
cmake --preset=dev 2>&1 | grep -i submodule
```

**Expected**: configure aborts with a message naming `git submodule update --init external/simdjson`; the log contains no attempt to locate a system simdjson.

## 12. System simdjson present never selected (FR-004, SC-008 edge case)

```sh
sudo apt-get install -y libsimdjson-dev   # system copy present in a discoverable prefix
cmake --preset=dev 2>&1 | grep -iE 'find_package\( *simdjson|Found simdjson'; echo "grep-exit=$?"
ctest --preset=dev -R simdjson_purity_scan --output-on-failure
```

**Expected**: configure performs no simdjson discovery (grep exits 1); the purity scan reports zero discovery calls. The build compiles the submodule copy regardless of the system package.

## 13. Re-pin rehearsal (US3, FR-006)

On a scratch clone, follow the README re-pinning section against any newer tag: check out the tag, commit the submodule pointer, bump the version assertion in `source/simdjson/simdjson_gate.cpp`.

**Expected**: with all three steps the build goes green; skipping the assertion bump leaves the build red with the tripwire diagnostic of section 3.

## Teardown

Scenario artifacts are disposable: `rm -rf build-consumer build-shared build-shared-check prefix prefix-shared consumer-*.log sanitize.log` and, where section 3 or 13 moved it, `git -C external/simdjson checkout f5de14f09256982933af2849beb43778bd421ca7`. Nothing outside the build tree is altered by this feature's runs (SC-007).
