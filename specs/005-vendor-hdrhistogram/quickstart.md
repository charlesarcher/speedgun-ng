# Quickstart / Validation: Vendor HdrHistogram_c as a Private, Pinned Submodule

**Feature**: 005-vendor-hdrhistogram | **Date**: 2026-09-23

Runnable scenarios that prove the two vendored submodules, the two `add_subdirectory` brackets, the zlib redirect, and the privacy contract end to end. Design: [plan.md](plan.md); bracket contract: [contracts/build-integration.md](contracts/build-integration.md); audit semantics: [contracts/privacy-contract.md](contracts/privacy-contract.md); entities: [data-model.md](data-model.md). Sections 2 through 9 are the privacy audits; 10 through 13 are the bracket guarantees; 14 is the re-pin rehearsal.

## 0. Prerequisites

Linux (the enforced gate) or macOS (developer path). Both vendored trees are native CMake libraries, so no host autoconf/automake/libtool/patch is needed for them (research R-001); the only prerequisites are the usual CMake >= 3.20 and a C/C++23 compiler.

A configured build tree (`cmake --preset=dev` per README, or `cmake --preset=ci-ubuntu` where no `dev` user preset exists) and `cmake --build --preset=dev`.

## 1. Clean clone builds (SC-001)

```sh
git clone --recurse-submodules <speedgun-ng-url>
cd speedgun-ng
cmake --preset=dev && cmake --build --preset=dev -j
ctest --preset=dev --output-on-failure
```

**Expected**: configure processes both vendored trees through `add_subdirectory` (zlib first, then HdrHistogram_c); the build compiles `zlibstatic`, then `hdr_histogram_static`, then the two gate TUs, then links; all tests pass. Every Linux CI preset configures and builds identically.

## 2. Pinned commits (FR-001, FR-001a, US1 scenario 3)

```sh
git submodule status external/hdrhistogram_c
git submodule status external/zlib
```

**Expected**: the HdrHistogram_c line begins with `18c7a324383dded1451d15621cd018b0048057d0` (tag `0.11.10`); the zlib line begins with `da607da739fa6047df13e66a2af6b8bec7c2a498` (tag `v1.3.2`).

## 3. Version tripwires (FR-003, SC-006, US1 scenario 4)

```sh
git -C external/hdrhistogram_c checkout 0.11.9
cmake --build --preset=dev 2>&1 | grep -A3 'static_assert\|error'
git -C external/hdrhistogram_c checkout 18c7a324383dded1451d15621cd018b0048057d0
```

**Expected**: compilation of `source/hdrhistogram/hdrhistogram_gate.cpp` fails with a `static_assert` diagnostic naming the expected HdrHistogram_c 0.11.10 and the mismatching string value. Restore the pin to go green. Repeat against `external/zlib` (checkout `v1.3.1`): compilation of `source/zlib/zlib_gate.cpp` fails naming the expected zlib 1.3.2 via `ZLIB_VERSION`/`ZLIB_VERNUM` (SC-006).

## 4. Pristine submodule worktrees (SC-007, US3 scenario 2)

```sh
cmake --build --preset=dev -j
git -C external/hdrhistogram_c status --porcelain
git -C external/zlib status --porcelain
```

**Expected**: both empty. The `add_subdirectory` builds landed in `${CMAKE_BINARY_DIR}/_hdrhistogram` and `_zlib`; zlib generates its `zconf.h` into its binary directory (research R-010), leaving both submodule trees untouched.

## 5. Static archive proofs (SC-009, SC-010, FR-007, FR-021)

```sh
ctest --preset=dev -R 'hdrhistogram_nm_proof|zlib_nm_proof' --output-on-failure
```

**Expected**: both pass. The HdrHistogram_c test lists `hdr_` vendored objects in `libspeedgun-ng.a` with the gate object's `hdr_alloc` reference resolving into them; the zlib test lists `z_`-prefixed zlib members (from `Z_PREFIX`) with the gate object's `zlibVersion` reference resolving into them (research R-005, R-009).

## 6. Install tree, package files, and the install-rule override (SC-002, SC-003, FR-012)

```sh
cmake --install build --prefix prefix
find prefix/ -iname '*hdr*histogram*'; echo "hdr-find-exit=$?"
find prefix/ -iname '*zlib*' -o -iname '*libz*'; echo "z-find-exit=$?"
grep -riE 'hdr[-_]?histogram' prefix/lib/cmake/speedgun-ng/*.cmake; echo "hdr-grep-exit=$?"
grep -riE 'zlib|libz' prefix/lib/cmake/speedgun-ng/*.cmake; echo "z-grep-exit=$?"
```

**Expected**: all four exit 1 with empty output. `ZLIB_INSTALL OFF` removes zlib's install rules; the HdrHistogram_c install switches plus the scope-limited `install()` override (research R-010) leave nothing in the prefix, including the three unconditional rules (the `hdr_histogram-config.cmake`, the `hdr_histogram.pc`, and the `include/hdr/*.h` headers). This section is the empirical check of the override: a stray `prefix/include/hdr/` or `prefix/.../hdr_histogram.pc` means the override did not hold and R-010's scratch-directory fallback applies.

## 7. Shared-build symbol audit (SC-004, SC-011, FR-017, FR-022)

```sh
cmake -S . -B build-shared -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build-shared -j
cmake --install build-shared --prefix prefix-shared
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -iE 'hdr[-_]?histogram'; echo "hdr-sym-exit=$?"
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -wE '^_?hdr_'; echo "hdr-prefix-exit=$?"
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -iE 'zlib|libz'; echo "z-sym-exit=$?"
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -wE 'inflate|deflate|compress|uncompress'; echo "z-bare-exit=$?"
ldd prefix-shared/lib/libspeedgun-ng.so | grep -iE 'libz|zlib|hdr'; echo "ldd-exit=$?"
```

**Expected**: every grep exits 1 with empty output. The scoped `BUILD_SHARED_LIBS OFF` keeps both vendored builds static, so no `libz`/`hdr_histogram` object is a runtime dependency (research R-007); the hidden-visibility members are absent from `.dynsym` (research R-009); `Z_PREFIX` removes the bare `inflate`/`deflate`/`compress`/`uncompress` names, and hidden visibility removes even the `z_`-prefixed ones; no `hdr_`-prefixed export survives (SC-004, SC-011).

## 8. Downstream consumer test (FR-018, SC-005, US2 scenario 4)

On a machine carrying both dependencies (on Debian-family runners: `sudo apt-get install libhdrhistogram-c-dev zlib1g-dev`):

```sh
cmake --install build --prefix prefix
cmake -S test/consumer -B build-consumer \
  -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release \
  | tee consumer-configure.log
cmake --build build-consumer -v | tee consumer-build.log
./build-consumer/consumer
grep -iE 'hdr[-_]?histogram' consumer-configure.log consumer-build.log; echo "hdr-grep-exit=$?"
grep -iE 'zlib|libz' consumer-configure.log consumer-build.log; echo "z-grep-exit=$?"
```

**Expected**: configure, build, and run all exit 0; both final greps exit 1 with empty output: zero HdrHistogram_c and zero zlib resolutions in the consumer's configure log and link command, on a machine that carries both. `test/consumer/` is the reused specs/003 consumer; it names no vendored dependency.

## 9. Vendored zlib wins over the host (FR-020, SC-010)

```sh
sudo apt-get install -y zlib1g-dev   # a system zlib present in a discoverable prefix
cmake --preset=dev -B build-zcheck 2>&1 | tee zcheck.log
grep -iE 'ZLIB_INCLUDE_DIR|ZLIB_LIBRARY' build-zcheck/CMakeCache.txt
grep -iE 'Found ZLIB.*=/usr' zcheck.log; echo "host-exit=$?"
```

**Expected**: the cache records `ZLIB_INCLUDE_DIR` under `external/zlib` (the vendored source dir) and `ZLIB_LIBRARY` under `_zlib` (the vendored build dir); the host grep exits 1: no `Found ZLIB` line points at a system prefix like `/usr`. HdrHistogram_c's `find_package(ZLIB)` resolves to the vendored copy via the redirect and cache pre-seed (research R-008).

## 10. Forced static under a shared parent (FR-007, FR-021, R-007)

```sh
cmake -S . -B build-shared-check -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build-shared-check -j
find build-shared-check/_zlib -name 'libz.so*' -o -name 'libzlib.so*'; echo "z-so-exit=$?"
find build-shared-check/_hdrhistogram -name '*hdr_histogram.so*'; echo "hdr-so-exit=$?"
```

**Expected**: both `find` commands exit 1 with empty output: no shared vendored object was built; both archives are static regardless of the parent's shared configuration.

## 11. Sanitizer and coverage exclusion (FR-010, FR-009)

```sh
cmake --preset=ci-sanitize && cmake --build build/sanitize -j 2>&1 | tee sanitize.log
grep -iE 'external/(zlib|hdrhistogram_c)' sanitize.log | grep -i 'fsanitize'; echo "grep-exit=$?"
ctest --test-dir build/sanitize --output-on-failure
```

**Expected**: the grep exits 1: the vendored archive compiles carry no sanitizer flags while the enclosing build does (the brackets blank `CMAKE_C_FLAGS_SANITIZE`, research R-011); the sanitize suite stays green; every Linux job applies the identical policy because the identical brackets serve them all.

## 12. Uninitialized submodule guard (FR-002, US1 scenario 6)

```sh
git clone <speedgun-ng-url> && cd speedgun-ng   # no --recurse-submodules
cmake --preset=dev 2>&1 | grep -i submodule
```

**Expected**: configure aborts with a message naming `git submodule update --init external/zlib` or `... external/hdrhistogram_c` (whichever is empty first); the log contains no attempt to locate a system copy.

## 13. System HdrHistogram_c present never selected (FR-004, SC-008 edge case)

```sh
sudo apt-get install -y libhdrhistogram-c-dev   # system copy present in a discoverable prefix
cmake --preset=dev 2>&1 | grep -iE 'find_package\( *hdr[-_]?histogram|Found .*histogram'; echo "grep-exit=$?"
ctest --preset=dev -R hdrhistogram_purity_scan --output-on-failure
```

**Expected**: configure performs no HdrHistogram_c discovery (grep exits 1); the purity scan reports zero discovery calls and zero `hdr[-_]?histogram` under `include/`. The build compiles the submodule copy regardless of the system package.

## 14. Re-pin rehearsal (US3, FR-006)

On a scratch clone, follow the README re-pinning section against any other release tag of either dependency (`0.11.9`, `v1.3.1`; both pins are the newest upstream releases today): check out the tag, commit the submodule pointer, bump that dependency's version assertion in its gate file.

**Expected**: with all three steps the build goes green; skipping the assertion bump leaves the build red with the tripwire diagnostic of section 3.

## Teardown

Scenario artifacts are disposable: `rm -rf build-consumer build-shared build-shared-check build-zcheck prefix prefix-shared consumer-*.log sanitize.log zcheck.log` and, where section 3 or 14 moved a submodule, restore its pinned commit. Nothing outside the build tree is altered by this feature's runs (SC-007).
