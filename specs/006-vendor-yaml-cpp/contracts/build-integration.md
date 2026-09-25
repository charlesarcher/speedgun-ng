# Contract: yaml-cpp Build Integration

**Feature**: `006-vendor-yaml-cpp` | **Date**: 2026-09-25
**Implements**: FR-001 through FR-013a; research R-001 through R-010.
**Companion**: [privacy-contract.md](privacy-contract.md) for the non-exposure surfaces.

## 1. Interface

The contract surface is CMake: one bracket function, one link edge, one merge-call extension in the root `CMakeLists.txt`. No runtime interface exists.

```cmake
# CMakeLists.txt, beside the existing hwloc/simdjson/zlib/HdrHistogram_c blocks.

function(import_yaml_cpp)
  # Guard (FR-002): abort configure with the init command, no fallback path.
  # Tripwire (FR-003, R-002): read the submodule's own version declaration,
  # no git metadata; mismatch aborts naming expected 0.9.0 and found.
  #   file(READ external/yaml-cpp/CMakeLists.txt ...)
  #   regex: project\( *YAML_CPP[^\)]*VERSION ([0-9.]+)
  # Constants (FR-009/FR-010/FR-011/FR-012/FR-017): the table below.
  # add_subdirectory(external/yaml-cpp "${CMAKE_BINARY_DIR}/_yaml-cpp" EXCLUDE_FROM_ALL)
  # SYSTEM include promotion of the yaml-cpp include dirs (R-009).
endfunction()

import_yaml_cpp()

target_sources(speedgun-ng_speedgun-ng PRIVATE source/yaml/yaml_gate.cpp)
target_link_libraries(
    speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:yaml-cpp::yaml-cpp>
)

# Existing call extended with the third vendored archive (R-010):
vendored_archive_merge(
    speedgun-ng_speedgun-ng hdr_histogram_static zlibstatic yaml-cpp
)
```

### Constant table (fixed constants; no knob is exposed)

| Name | Value | Forced because |
|---|---|---|
| tripwire expected version | `0.9.0` | FR-003, Clarifications 2026-09-25, R-002 |
| `CMAKE_CXX_FLAGS`, `CMAKE_COMPILE_WARNING_AS_ERROR`, `CMAKE_CXX_FLAGS_SANITIZE`, `CMAKE_CXX_FLAGS_COVERAGE`, `CMAKE_CXX_CLANG_TIDY`, `CMAKE_CXX_CPPCHECK` | cleared / `OFF` | FR-009, FR-010; CMake children inherit parent settings (R-008) |
| `CMAKE_CXX_VISIBILITY_PRESET` | `hidden` | FR-017 (R-007) |
| `CMAKE_VISIBILITY_INLINES_HIDDEN` | `ON` | FR-017: template-heavy API, keep weak instantiations hidden (R-007) |
| `CMAKE_POSITION_INDEPENDENT_CODE` | `ON` | shared-parent absorption (SC-004) |
| `CMAKE_POLICY_DEFAULT_CMP0077` | `NEW` | upstream `cmake_minimum_required(VERSION 3.5...3.30)`: `option()` would clobber the scoped constants (R-004) |
| `BUILD_SHARED_LIBS` | `OFF` | FR-011 |
| `YAML_BUILD_SHARED_LIBS` | `OFF` | upstream default is `${BUILD_SHARED_LIBS}`: a shared parent would build yaml-cpp shared (R-005) |
| `YAML_CPP_BUILD_CONTRIB` / `YAML_CPP_BUILD_TOOLS` / `YAML_CPP_BUILD_TESTS` | `OFF` | FR-011 (R-004); the `test/` subdirectory is the only GTest lookup site and is never added |
| `YAML_CPP_INSTALL` | `OFF` | FR-012: every upstream install rule and the `uninstall` target are guarded by it (R-006) |
| `YAML_CPP_FORMAT_SOURCE` | `OFF` | no `format` target from a subproject (R-004) |
| `add_subdirectory` binary dir | `${CMAKE_BINARY_DIR}/_yaml-cpp` | FR-013, SC-007: out-of-source generation (R-012) |

`YAML_USE_SYSTEM_GTEST` and `YAML_ENABLE_PIC` keep their upstream defaults (`OFF`, `ON`); the first gates nothing while tests are off, the second matches the PIC constant.

### Prohibited at the call site (FR-004, FR-007, FR-013a)

- `find_package(yaml-cpp ...)`, `pkg_check_modules(... yaml-cpp ...)`, and every other system-discovery spelling of the FR-018 pattern, anywhere in the build files.
- A `PUBLIC` or `INTERFACE` link edge to `yaml-cpp`.
- Any option, preset, or cache variable that excludes yaml-cpp (or hwloc, simdjson, HdrHistogram_c, zlib) from the library on Linux or macOS.
- In-tree generation into `external/yaml-cpp`: the bracket passes the explicit binary directory (SC-007).
- `--exclude-libs` link flags for yaml-cpp: hidden compilation plus the static-form export-macro resolution cover the export surface (R-007).

## 2. Guarantees

| ID | Guarantee | Proven by |
|---|---|---|
| G1 | Configure aborts on an empty submodule with the init command in the message | quickstart section 11 |
| G2 | Configure aborts on a submodule declaring a version other than `0.9.0`; the diagnostic names expected and found; a revision declaring `0.9.0` passes | quickstart section 3 (FR-003, SC-006) |
| G3 | The vendored target is `STATIC` in every configuration, including a shared-parent build; the build tree holds no shared yaml-cpp object | quickstart section 9 (FR-011) |
| G4 | The vendored compile carries no parent warning flags, no sanitizer, no coverage, no clang-tidy, no cppcheck | compile-command inspection, quickstart section 10 (FR-009, FR-010) |
| G5 | Vendored sources compile under the preset's `CMAKE_CXX_STANDARD` (23); upstream's C++11 fallback never fires | CI build jobs record the outcome (R-008) |
| G6 | `nm` on the built static `libspeedgun-ng.a` lists members defining `4YAML` symbols, and the gate object's `_ZN4YAML4Load` reference resolves in-archive | `ctest -R yaml_nm_proof` (FR-007, SC-009) |
| G7 | `cmake --install` emits zero yaml-cpp files and adds nothing to the speedgun-ng export set | quickstart section 6 (FR-012, FR-015, SC-002) |
| G8 | `git status` reports `external/yaml-cpp` unmodified after a full build | quickstart section 4 (FR-013, SC-007) |
| G9 | The `LICENSE` (MIT) file remains in the submodule tree | FR-005; the submodule content is the pinned commit |

## 3. Failure modes

| Symptom | Cause | Remedy |
|---|---|---|
| `external/yaml-cpp is uninitialized; run: git submodule update --init external/yaml-cpp` | clone without `--recurse-submodules` | run the named command (FR-002) |
| Tripwire diagnostic: expected `0.9.0`, found `x.y.z` | submodule checked out at another release | re-pin per README or `git -C external/yaml-cpp checkout yaml-cpp-0.9.0` (FR-006) |
| Scoped options ignored (shared yaml-cpp appears) | `CMP0077` default missing, `option()` clobbered the constants | restore `set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)` in the bracket (R-004) |
| A `yaml-cpp` install rule trips the HdrHistogram_c `install()` override `FATAL_ERROR` | an upstream rule escaped `YAML_CPP_INSTALL` | loud abort by design (R-006 defense in depth); the escape is a defect in the constant set, fix the constant |
| Vendored compile error under a new toolchain at C++23 | upstream source vs newer standard | recorded escape: `set(CMAKE_CXX_STANDARD 11)` inside the bracket scope (R-008); file with the spec |
| Merge step missing the yaml-cpp archive | merge call not extended | extend the `vendored_archive_merge` argument list (R-010) |

## 4. Module documentation duty

The bracket carries adjacent comments naming: the guard and FR-002; the tripwire, its version-string granularity, and the 2026-09-25 clarification (R-002); the inheritance asymmetry note pointing at the simdjson bracket comment; the `CMP0077` trap note mirroring the HdrHistogram_c bracket; the privacy statement that the link is PRIVATE plus `BUILD_INTERFACE` only; the re-pinning pointer to the README section (FR-006). The wrapper carries the FR-014 sole-includer note and the link-proof explanation, mirroring `source/hdrhistogram/hdrhistogram_gate.cpp`.
