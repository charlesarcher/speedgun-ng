# Data Model: Vendor yaml-cpp as a Private, Pinned Submodule

**Feature**: `006-vendor-yaml-cpp` | **Date**: 2026-09-25
**Scope**: build-time entities only. The feature adds zero runtime types: yaml-cpp crosses no API boundary (spec Scope boundaries), so every entity below is a build artifact, a build-system handle, or an audit surface.

## Entities

### Vendored yaml-cpp submodule

The upstream sources carried in-tree at a fixed revision.

| Attribute | Value | Source |
|---|---|---|
| Vendored path | `external/yaml-cpp` | FR-001 (outside `include/`, `source/`, `test/`) |
| Upstream | `https://github.com/jbeder/yaml-cpp` | `.gitmodules` record (FR-001) |
| Release tag | `yaml-cpp-0.9.0` (lightweight tag, names the commit directly) | FR-001; `git ls-remote` 2026-09-25 |
| Pinned commit | `56e3bb550c91fd7005566f19c079cb7a503223cf` | FR-001; newest upstream release |
| License | MIT, `LICENSE` file in the submodule root | FR-005; stays in-tree, ships with source distributions |
| Version declaration | `project(YAML_CPP VERSION 0.9.0 ...)` in its `CMakeLists.txt` (line 11) | the tripwire's read target (FR-003; R-002) |
| Sub-dependencies | none | spec Assumptions; no companion submodule |

Validation rules: the directory is either populated at the pinned commit or configuration aborts (guard, FR-002); the declared version string must equal `0.9.0` or configuration aborts with the diagnostic (FR-003; granularity settled by Clarifications 2026-09-25: version-string, revisions declaring 0.9.0 pass, no git metadata consulted).

### Build-integration bracket (`import_yaml_cpp()`)

The function-scoped ingestion in the root `CMakeLists.txt`. Fixed constants, no knobs (X.2):

| Constant | Value | Purpose | Reference |
|---|---|---|---|
| Submodule guard | `external/yaml-cpp/CMakeLists.txt` must exist | abort naming `git submodule update --init external/yaml-cpp` | FR-002 |
| Tripwire expected | `0.9.0` | version-string equality against the `project()` line | FR-003, R-002 |
| `CMAKE_CXX_FLAGS` | `""` | vendored code exempt from the parent warning set | FR-009 |
| `CMAKE_COMPILE_WARNING_AS_ERROR` | `OFF` | no warnings-as-errors promotion | FR-009 |
| `CMAKE_CXX_FLAGS_SANITIZE` | `""` | sanitizer exclusion | FR-010 |
| `CMAKE_CXX_FLAGS_COVERAGE` | `""` | coverage exclusion | FR-009, VI |
| `CMAKE_CXX_CLANG_TIDY` / `CMAKE_CXX_CPPCHECK` | `""` | static-analysis exclusion | FR-009 |
| `CMAKE_CXX_VISIBILITY_PRESET` | `hidden` | hidden vendored objects | FR-017, R-007 |
| `CMAKE_VISIBILITY_INLINES_HIDDEN` | `ON` | hidden inline/template symbols | FR-017, R-007 |
| `CMAKE_POSITION_INDEPENDENT_CODE` | `ON` | static objects linkable into a shared parent | SC-004 |
| `CMAKE_POLICY_DEFAULT_CMP0077` | `NEW` | upstream `cmake_minimum_required(3.5)` would let `option()` clobber the scoped constants | R-004, R-005 |
| `BUILD_SHARED_LIBS` | `OFF` | belt for any nested `option`/`add_library` default | FR-011 |
| `YAML_BUILD_SHARED_LIBS` | `OFF` | forces the static library type against its `${BUILD_SHARED_LIBS}` default | FR-011, R-005 |
| `YAML_CPP_BUILD_CONTRIB` | `OFF` | no contrib sources | FR-011, R-004 |
| `YAML_CPP_BUILD_TOOLS` | `OFF` | no `util/` parse tools | FR-011, R-004 |
| `YAML_CPP_BUILD_TESTS` | `OFF` | no `test/` subdirectory, no GTest lookup | FR-011, R-004 |
| `YAML_CPP_INSTALL` | `OFF` | removes every upstream install rule | FR-012, R-006 |
| `YAML_CPP_FORMAT_SOURCE` | `OFF` | no `format` target | R-004 |
| Binary dir | `${CMAKE_BINARY_DIR}/_yaml-cpp` | out-of-source build; pristine worktree | FR-013, SC-007, R-012 |

State transitions (configure-time, hard-fail): `guard? -> tripwire? -> constants -> add_subdirectory -> SYSTEM-include promotion`; any failed guard or tripwire ends configure with a `FATAL_ERROR`.

### Linked vendored target

`yaml-cpp` (ALIAS `yaml-cpp::yaml-cpp`), `STATIC` in every configuration (R-005). Its usage requirements: `YAML_CPP_STATIC_DEFINE` as a PUBLIC compile definition (upstream generator expression, verified; R-007), include directories promoted to SYSTEM inside the bracket (R-009), hidden visibility baked in (R-007).

Relationships: exactly one consumer, `speedgun-ng_speedgun-ng`, over one `PRIVATE $<BUILD_INTERFACE:yaml-cpp::yaml-cpp>` edge (FR-007). No PUBLIC edge exists; the name never enters the exported set (FR-016). In a static build the POST_BUILD merge (existing variadic `vendored_archive_merge` call, R-010) folds its members into `libspeedgun-ng.a`.

### Internal wrapper unit

`source/yaml/yaml_gate.cpp`. One TU, zero runtime lines.

| Element | Content | Purpose | Reference |
|---|---|---|---|
| Include | `<yaml-cpp/yaml.h>` (SYSTEM-promoted) | sole point where yaml-cpp headers enter the build | FR-014, R-009 |
| Link proof | `[[gnu::used]] [[maybe_unused]] constinit auto const link_proof = &YAML::Load;` | object-level dependency proof; mangles to `_ZN4YAML4Load...` | FR-007, SC-009, R-003 |

The version tripwire lives at configure time (R-002): the pinned tree exposes no version macro, so this TU carries the compile-side half of nothing. That asymmetry with the simdjson and HdrHistogram_c gates is deliberate and recorded in the README re-pinning section (FR-006).

### Privacy contract

The consumer-observable surfaces that must remain free of yaml-cpp, audited with the FR-018 pattern `yaml[-_]?cpp` (case-insensitive; covers `yaml-cpp`, `yaml_cpp`, `yamlcpp`), plus the `4YAML` namespace marker on symbol dumps:

| Surface | Must contain | Audited by |
|---|---|---|
| Installed headers (`include/` in the prefix) | zero matches | install-tree audit (SC-002) |
| Installed archives/objects/package files by path | zero matches | install-tree audit (SC-002) |
| `speedgun-ngConfig.cmake`, `speedgun-ngTargets.cmake` | zero matches | package-config audit (SC-003) |
| Shared-library `.dynsym` and `NEEDED` entries | zero matches, zero `4YAML` | shared-build audit (SC-004) |
| Consumer configure/build logs | zero matches | downstream consumer test (SC-005) |
| Project build files (discovery calls) | zero `find_package`/`pkg_check_modules` naming the pattern | purity scan (SC-008) |
| `include/` in the source tree | zero references | purity scan (FR-014) |

## Relationships

- One submodule feeds one bracket; the bracket produces one target; one `PRIVATE` link edge plus one merge step carry it into the single library.
- The wrapper is the sole header consumer and the sole link-proof site; the nm proof reads the wrapper's reference and the merged members together (SC-009).
- The privacy contract is a property of the library's installed and shared outputs; the five audits above are its proofs, one command each, plus the downstream consumer test as the authoritative end-to-end run (FR-019).
- Re-pinning is a three-step transaction on the submodule entity: checkout new tag, commit pointer, bump the bracket's expected version; the tripwire enforces the third step (FR-006, US3).

## State

Build-time state machine (mirrors the plan's logical view):

```text
[*] -> Guarded: configure starts
Guarded -> Pinned: submodule populated AND declared version == 0.9.0
Guarded -> [*] (abort): empty dir -> FATAL_ERROR naming init (FR-002)
Guarded -> [*] (abort): declared version != 0.9.0 -> FATAL_ERROR naming 0.9.0 + found (FR-003)
Pinned -> Configured: bracket constants applied, add_subdirectory out of tree
Configured -> Linked: yaml-cpp STATIC generated; gate TU + PRIVATE edge
Linked -> Merged: static config -> ADDLIB into libspeedgun-ng.a (self-contained)
Linked -> Absorbed: shared config -> hidden members absorbed at link, zero exports
```

Invariant holding in every post-`Configured` state: no build path resolves a system yaml-cpp (zero discovery calls exist), `CMAKE_INSTALL_PREFIX` receives nothing from the ingestion, and the submodule worktree stays pristine (SC-007).
