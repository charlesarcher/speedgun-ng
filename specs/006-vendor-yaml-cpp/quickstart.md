# Quickstart / Validation: Vendor yaml-cpp as a Private, Pinned Submodule

**Feature**: `006-vendor-yaml-cpp` | [plan.md](../plan.md) | [contracts/](contracts/) | [data-model.md](../data-model.md)

Each section is one runnable validation with a binary verdict (X.4), mapping to the plan's Test Plan rows. Run from the repository root. `prefix/` is a scratch install directory; the shared prefix is `prefix-shared/`.

## 0. Prerequisites

- A clone with submodules: `git clone --recurse-submodules`, or `git submodule update --init`.
- CMake >= 3.20, a C++23 compiler (GCC or Clang on Linux; AppleClang on macOS).
- No new host tool: the yaml-cpp tree builds under plain CMake.
- Linux is the enforced gate; the macOS developer run covers section 1 plus sections 5 and 6 locally.

## 1. Clean clone builds (SC-001)

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

Verdict: all three exit 0; the build log shows yaml-cpp compiling from `external/yaml-cpp` into `_yaml-cpp/`. On macOS: `cmake --preset=ci-macos` configures and builds. CI: every existing Linux job stays green with submodules fetched.

## 2. Pinned commits (FR-001, US1 scenario 3)

```sh
git submodule status external/yaml-cpp
```

Verdict: the line records `56e3bb550c91fd7005566f19c079cb7a503223cf`, the commit the tag `yaml-cpp-0.9.0` names (`git ls-remote https://github.com/jbeder/yaml-cpp refs/tags/yaml-cpp-0.9.0` agrees). The `LICENSE` file (MIT) is present in the submodule root (FR-005).

## 3. Version tripwires (FR-003, SC-006, US1 scenario 4)

```sh
git -C external/yaml-cpp checkout yaml-cpp-0.8.0
cmake --preset=dev
```

Verdict: configure exits non-zero; the diagnostic is readable in one glance, naming the expected yaml-cpp version `0.9.0` and the version found (`0.8.0`). No compilation starts. Restore: `git -C external/yaml-cpp checkout yaml-cpp-0.9.0`, reconfigure, exit 0.

Version-string granularity (Clarifications 2026-09-25): a revision that still declares `0.9.0` (for example a branch point after the release tag) configures clean by design; the check consults no git metadata, so the same configure works from a source archive without git history.

## 4. Pristine submodule worktrees (SC-007)

```sh
cmake --build --preset=dev
git -C external/yaml-cpp status --porcelain
```

Verdict: empty output. All generation (`yaml-cpp-config.cmake`, `yaml-cpp-config-version.cmake`, `yaml-cpp.pc`) lands in `_yaml-cpp/` under the build directory (research R-012).

## 5. Static archive proof (SC-009, FR-007)

```sh
ctest --preset=dev -R yaml_nm_proof
nm -C build/dev/libspeedgun-ng.a | grep -c 'YAML::'
```

Verdict: the ctest proof exits 0, reporting (A) merged members defining `YAML::` symbols, (B) the `yaml_gate` member's undefined `_ZN4YAML4Load` reference, (C) resolution of that reference into a merged member. The manual `nm -C` count is nonzero. The vendored archive is merged: `libspeedgun-ng.a` is self-contained.

## 6. Install tree and package files (SC-002, SC-003, FR-012, FR-015, FR-016)

```sh
cmake --install build/dev --prefix prefix
find prefix/ -iname '*yaml*cpp*'
grep -riE 'yaml[-_]?cpp' prefix/lib/cmake/speedgun-ng/*.cmake
```

Verdict: both commands print nothing. `YAML_CPP_INSTALL OFF` removes every upstream install rule (research R-006); the speedgun-ng export set carries no entry; no pkg-config file for yaml-cpp exists in the prefix. The HdrHistogram_c `install()` override sits in the file as a loud tripwire for any rule that would hypothetically escape the guard.

## 7. Shared-build symbol audit (SC-004, FR-017)

```sh
cmake -S . -B build/shared-audit -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build/shared-audit
cmake --install build/shared-audit --prefix prefix-shared
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -iE 'yaml[-_]?cpp'
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep '4YAML'
ldd prefix-shared/lib/libspeedgun-ng.so | grep -iE 'yaml[-_]?cpp'
```

Verdict: all three greps print nothing (the CI spelling pins the exact commands). The vendored objects compiled hidden with `YAML_CPP_STATIC_DEFINE` (research R-007); the forced-static form means no `libyaml-cpp.so` exists to be named by `ldd` (R-005).

## 8. Downstream consumer test (FR-019, SC-005, US2 scenario 4)

On a machine where yaml-cpp is present (CI premise step installs it into `/usr/local` per the privacy contract section 2; locally, any prefix on `CMAKE_PREFIX_PATH` works):

```sh
cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$PWD/prefix | tee consumer-configure.log
cmake --build build-consumer -v | tee consumer-build.log
./build-consumer/consumer
grep -riE 'yaml[-_]?cpp' consumer-configure.log consumer-build.log
```

Verdict: configure, build, run all exit 0; the final grep prints nothing: the consumer resolves zero yaml-cpp through speedgun-ng. This is the authoritative proof of the privacy contract.

## 9. Forced static under a shared parent (FR-011, R-005)

```sh
find build/shared-audit -iname '*yaml*' -name '*.so*'
find build/shared-audit/_yaml-cpp -iname '*.a' | head
```

Verdict: the first command prints nothing (no shared yaml-cpp object anywhere in the build tree); the second names the static archive. `YAML_BUILD_SHARED_LIBS` defaults to `${BUILD_SHARED_LIBS}` upstream; the bracket forces `OFF` under `CMP0077 NEW`, and the shared-parent build proves it.

## 10. Sanitizer and coverage exclusion (FR-010, FR-009)

```sh
cmake --preset=ci-sanitize >/dev/null
grep -e '-fsanitize' -e '--coverage' \
  build/dev/_yaml-cpp/CMakeFiles/yaml-cpp.dir/flags.make \
  build/sanitize/_yaml-cpp/CMakeFiles/yaml-cpp.dir/flags.make
```

Verdict: exit 1 with no output (the bracket blanks `CMAKE_CXX_FLAGS_SANITIZE` and `CMAKE_CXX_FLAGS_COVERAGE`; research R-008). `ci-sanitize` and the `coverage` preset stay green with `external/` outside every measurement; the wrapper TU contributes zero runtime lines to the coverage trace.

## 11. Uninitialized submodule guard (FR-002, US1 scenario 6)

```sh
mv external/yaml-cpp /tmp/yaml-cpp-hold && mkdir external/yaml-cpp
cmake --preset=dev
mv /tmp/yaml-cpp-hold external/yaml-cpp
```

Verdict: the configure exits non-zero with the message `external/yaml-cpp is uninitialized; run: git submodule update --init external/yaml-cpp`. No fallback to any system copy is attempted.

## 12. System yaml-cpp present never selected (FR-004, SC-008 edge case)

With the `/usr/local` install of section 8 still present:

```sh
rm -rf build/probe && cmake -S . -B build/probe -D CMAKE_BUILD_TYPE=Release
cmake --build build/probe
ctest --preset=dev -R yaml_purity_scan
```

Verdict: the build compiles the submodule copy into `_yaml-cpp/`; the configure log names no system path for yaml-cpp; the purity scan exits 0, reporting zero discovery calls in build files (the grep audit of FR-018, `external/` excluded) and zero references under `include/`.

## 13. Re-pin rehearsal (US3, FR-006)

On a scratch clone, follow the README re-pinning section against a different release tag, for example `yaml-cpp-0.8.0`:

1. `git -C external/yaml-cpp checkout yaml-cpp-0.8.0`
2. Commit the submodule pointer.
3. Bump the expected-version constant in the `import_yaml_cpp` bracket of `CMakeLists.txt` to `0.8.0`.

Verdict: after step 3 the build is green; moving the tag with the constant untouched fails configure at the tripwire (section 3 is that check). The section's three steps match the documented ones exactly.

## Teardown

```sh
rm -rf prefix prefix-shared build-consumer sys-yaml build/probe
```

Restore the submodule (`git -C external/yaml-cpp checkout yaml-cpp-0.9.0`) and the expected-version constant if section 13 bumped them, before any commit.
