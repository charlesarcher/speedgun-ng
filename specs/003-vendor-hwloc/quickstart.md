# Quickstart / Validation: Vendor hwloc as a Private, Pinned Submodule

**Feature**: 003-vendor-hwloc | **Date**: 2026-09-20

Runnable scenarios that prove the vendoring, the ingestion module, and the privacy contract end to end. Design: [plan.md](plan.md); module interface: [contracts/ingestion-module.md](contracts/ingestion-module.md); audit semantics: [contracts/privacy-contract.md](contracts/privacy-contract.md); entities: [data-model.md](data-model.md). Sections 2 through 8 are the CI audits; 9 through 12 are the module guarantees; 13 is the re-pin rehearsal.

## 0. Prerequisites

Linux (the enforced gate) with the host tools, macOS for the developer path:

```sh
# Debian-family
sudo apt-get install autoconf automake libtool patch
# RPM-family
sudo dnf install autoconf automake libtool patch
# macOS (plus BSD patch ships with the OS)
brew install autoconf automake libtool
```

A configured build tree (`cmake --preset=dev` per README, or `cmake --preset=ci-ubuntu` where no `dev` user preset exists) and `cmake --build --preset=dev`.

## 1. Clean clone builds (SC-001)

```sh
git clone --recurse-submodules <speedgun-ng-url>
cd speedgun-ng
cmake --preset=dev && cmake --build --preset=dev -j
ctest --preset=dev --output-on-failure
```

**Expected**: configure reports the vendored ingestion; the build compiles the copied tree (autogen, configure, make visible in the log on the cold build only); all tests pass. Every Linux CI preset configures and builds identically.

## 2. Pinned commit (FR-001, US1 scenario 3)

```sh
git submodule status external/hwloc
```

**Expected**: the line begins with the pinned SHA `b5660dff631a171a96a4b3abf5a170c9cf62d6ef`.

## 3. Version tripwire (FR-003, SC-006, US1 scenario 4)

```sh
git -C external/hwloc checkout hwloc-2.13.0
cmake --build --preset=dev 2>&1 | grep -A2 'static_assert\|error'
git -C external/hwloc checkout b5660dff631a171a96a4b3abf5a170c9cf62d6ef
```

**Expected**: compilation of `source/hwloc/hwloc_gate.cpp` fails with a diagnostic naming the expected hwloc 2.14.0; restoring the pin restores a green build. The same fire pattern applies to a 2.14.x patch revision: the release component breaks the equality (research R-002).

## 4. Pristine submodule worktree (SC-007, US3 scenario 2)

```sh
cmake --build --preset=dev -j
git -C external/hwloc status --porcelain
```

**Expected**: empty output; all generation happened in the build-tree copy.

## 5. Static archive proof and purity (SC-009, SC-011, FR-004, FR-021)

```sh
ctest --preset=dev -R 'hwloc_nm_proof|hwloc_purity_scan' --output-on-failure
```

**Expected**: both tests pass. The nm test lists vendored members in `libspeedgun-ng.a` with the gate object's `sg_hwloc_get_api_version` reference resolving into them; the purity scan reports zero `find_package(hwloc` / `pkg_check_modules(hwloc` occurrences and zero hwloc includes under `include/`.

## 6. Install tree and package files (SC-002, SC-003)

```sh
cmake --install build --prefix prefix
find prefix/ -iname '*hwloc*'
grep -i hwloc prefix/lib/cmake/speedgun-ng/*.cmake
```

**Expected**: both commands print nothing.

## 7. Shared-build symbol audit (SC-004)

```sh
cmake -S . -B build-shared -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build-shared -j
cmake --install build-shared --prefix prefix-shared
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -i hwloc; echo "nm-grep-exit=$?"
ldd prefix-shared/lib/libspeedgun-ng.so | grep -i hwloc; echo "ldd-grep-exit=$?"
```

**Expected**: both greps exit 1 with empty output: the installed shared library carries no dynamic hwloc symbol and no hwloc runtime dependency. The audits run against the installed library, the surface named by privacy-contract A5 and US2 scenario 3.

## 8. Downstream consumer test (FR-026, SC-005, US2 scenario 4)

On a machine carrying hwloc (verify the discovery package exists; on Debian-family runners: `sudo apt-get install libhwloc-dev`):

```sh
cmake --install build --prefix prefix
cmake -S test/consumer -B build-consumer \
  -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release \
  | tee consumer-configure.log
cmake --build build-consumer -v | tee consumer-build.log
./build-consumer/consumer
grep -i hwloc consumer-configure.log consumer-build.log; echo "grep-exit=$?"
```

**Expected**: configure, build, and run all exit 0; the final grep exits 1 with empty output: zero hwloc resolutions in the consumer's configure log and link command.

## 9. Stamp invalidation (SC-008, US3 scenario 3)

```sh
cmake --preset=dev -D CMAKE_BUILD_TYPE=Debug
cmake --build --preset=dev -j 2>&1 | grep -c 'hwloc'
```

**Expected**: the vendor chain (copy, autogen, configure, make) re-runs visibly: the build-type change moves the hash, the fresh prefix forces the full sequence (research R-006). The same holds for a compiler change, a sanitizer-flag change, or an added configure argument.

## 10. Missing-toolchain diagnostic (FR-018, US3 scenario 4)

```sh
env PATH="/usr/bin:/bin" cmake -S . -B build-notools \
  -D speedgun-ng_DEVELOPER_MODE=ON -D CMAKE_BUILD_TYPE=Release 2>&1 | tail
```

(Trim `autoconf`, `automake`, `libtool`, or `patch` from `PATH` however the host allows; on macOS also `glibtool`.)

**Expected**: configuration fails at configure time, the message names the missing tool and the install packages (`autoconf automake libtool patch`, with `brew`/`apt`/`dnf` names surfaced per platform). No partial state is produced.

## 11. Uninitialized submodule guard (FR-002, US1 scenario 6)

```sh
git clone <speedgun-ng-url> && cd speedgun-ng   # no --recurse-submodules
cmake --preset=dev 2>&1 | grep -i submodule
```

**Expected**: configure aborts with a message naming `git submodule update --init external/hwloc`; the log contains no attempt to locate a system hwloc.

## 12. Sanitizer policy (FR-010)

```sh
cmake --preset=ci-sanitize && cmake --build build/sanitize -j 2>&1 | tee sanitize.log
grep -i 'fsanitize' sanitize.log | grep -i hwloc; echo "grep-exit=$?"
ctest --test-dir build/sanitize --output-on-failure
```

**Expected**: the grep exits 1: the vendored archive compiles carry no sanitizer flags while the enclosing build does (policy and rationale in the module header); the sanitize suite stays green; every Linux job applies the identical policy because the identical call site serves them all.

## 13. Re-pin rehearsal (US4, FR-006)

On a scratch clone, follow the README re-pinning section against any newer tag: check out the tag, commit the submodule pointer, bump the version assertion in `source/hwloc/hwloc_gate.cpp`.

**Expected**: with all three steps the build goes green; skipping the assertion bump leaves the build red with the tripwire diagnostic of section 3.

## Teardown

Scenario artifacts are disposable: `rm -rf build-consumer build-shared build-notools prefix prefix-shared consumer-*.log sanitize.log` and, where scenario 3 or 13 moved it, `git -C external/hwloc checkout b5660dff631a171a96a4b3abf5a170c9cf62d6ef`. Nothing outside the build tree is altered by this feature's runs (SC-007).
