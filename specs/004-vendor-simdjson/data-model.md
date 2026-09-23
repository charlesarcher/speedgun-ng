# Data Model: Vendor simdjson as a Private, Pinned Submodule

**Feature**: 004-vendor-simdjson | **Date**: 2026-09-23

The feature ships build-time objects: a vendored source tree, a CMake `add_subdirectory` bracket, a build artifact chain, one wrapper TU, and a contract over observable surfaces. Entities map 1:1 to `spec.md` → Key Entities. The build-state model (section "State") models the ingestion configuration; the audit surfaces (section "Entities", Privacy contract) carry the privacy invariants.

## Entities

### Vendored submodule

The simdjson sources carried in-tree at the fixed root, input to everything else.

| Field | Value (pinned) | Notes |
|---|---|---|
| `path` | `external/simdjson` | Fixed by the spec assumption: the root convention for every vendored dependency, matching `external/hwloc`; outside `include/`, `source/`, `test/` (FR-001) |
| `release_tag` | `v4.6.11` | Lightweight tag naming the commit directly (verified via `git ls-remote` 2026-09-23) |
| `pinned_commit` | `e153ffadd9ae29b00c90bedc76f65d25a993d2b5` | The exactness authority (FR-001); floating `GIT_TAG`, branches, shallow tracking prohibited |
| `licenses` | MIT and Apache-2.0 | Stay in-tree, ship with source distributions (FR-005) |
| `self_reported_version` | `4.6.11` (macro `SIMDJSON_VERSION` = `"4.6.11"`) | Verified at the tag 2026-09-23; a clean dotted triple, no greek/snapshot suffix (contrast hwloc's `2.14.0rc2-git`), so both the enum triple and the string equality pass on the pin (research R-002) |
| `version_identifiers` | `simdjson::SIMDJSON_VERSION_MAJOR=4`, `MINOR=6`, `REVISION=11` (enum constants in `include/simdjson/simdjson_version.h`) | Inputs to the wrapper assertion; `#if` reads only macros and cannot see them, `static_assert` can (R-002) |
| `symbol_prefix` | none | No upstream prefix or namespace-rename mechanism; privacy rests on hidden visibility plus static absorption (R-007) |

**Validation**: configuration aborts when the directory is missing or empty, with the init command named (FR-002); the compile-time assertion rejects any checkout whose `(MAJOR, MINOR, REVISION)` triple differs from `(4, 6, 11)`, covering 4.6.x patch drift and every other series (FR-003, SC-006); the submodule worktree reports unmodified after builds (SC-007); `git submodule status` matches the pinned commit (FR-001).

### Build-integration bracket

The scoped construct in `CMakeLists.txt` that owns every vendored-build setting. Its full input/guarantee contract lives in [contracts/build-integration.md](contracts/build-integration.md); this is the entity view.

| Constant (fixed) | Value | Reason |
|---|---|---|
| vendored path | `external/simdjson` | FR-001 |
| `SIMDJSON_INSTALL` | `OFF` | default tracks `BUILD_SHARED_LIBS`; explicit OFF closes the shared-audit leak (FR-014, R-004) |
| `SIMDJSON_DEVELOPER_MODE` | unset | never declared for a subproject; library-only (FR-011, R-004) |
| `BUILD_SHARED_LIBS` (scoped) | `OFF` | forces simdjson static in every configuration, incl. the shared speedgun-ng build (FR-007, FR-016, R-005) |
| `CMAKE_CXX_FLAGS` (scoped) | blanked | drops parent strict warnings, `-Werror`, hardening from the vendored compile (FR-009, R-006) |
| `CMAKE_CXX_FLAGS_SANITIZE` / `_COVERAGE` (scoped) | blanked | no sanitizer, no `--coverage` on vendored code (FR-010, R-006) |
| `CMAKE_CXX_CLANG_TIDY` / `_CPPCHECK` (scoped) | blanked | no static analysis of vendored code (FR-009, R-006) |
| `CMAKE_CXX_VISIBILITY_PRESET` (scoped) | `hidden` | stamps `STV_HIDDEN` into vendored members (FR-016, R-007) |
| `EXCLUDE_FROM_ALL` | set | keeps the vendored target out of the default `all` target (FR-011) |
| include promotion | `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` | vendored header compiled as SYSTEM for the wrapper (FR-009, FR-013, R-008) |

**Guarantees** (FR-009 through FR-016, verified per quickstart): offline (no network step); pristine submodule worktree (out-of-source build dir, SC-007); no install into `CMAKE_INSTALL_PREFIX` (FR-014); no sanitizer/coverage on vendored objects (FR-010); tidy/cppcheck exempt for vendored code while speedgun-ng code stays held (FR-009); hidden-visibility members (FR-016); early named guard on an uninitialized tree (FR-002).

**Validation**: the call site holds the bracket invocation, the `PRIVATE` link, and the wrapper registration; it names no `find_package(simdjson)` and no bespoke build command (FR-004, FR-007, proven by the purity scan and `dependency_scan`).

### Linked vendored target

The build-system handle for the private link, produced by `add_subdirectory`.

| Property | Value | Notes |
|---|---|---|
| target | `simdjson` (alias `simdjson::simdjson`) | `add_library(simdjson ...)` at `CMakeLists.txt:206` (R-001, R-003) |
| type | `STATIC` (forced by scoped `BUILD_SHARED_LIBS OFF`) | one static archive in every configuration (R-005) |
| member visibility | `STV_HIDDEN` (scoped `CXX_VISIBILITY_PRESET hidden`) | not promoted to a shared `.dynsym` (FR-016, R-007) |
| compile flags | none of the parent's strict/sanitizer/coverage flags | FR-009, FR-010, R-006 |
| interface include | promoted to `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` | wrapper compiles `<simdjson.h>` via `-isystem` (FR-013, R-008) |
| usage requirement on speedgun-ng | `PRIVATE`, wrapped in `$<BUILD_INTERFACE:...>` | no PUBLIC edge; the name never enters the exported set (FR-007, FR-015) |

**Validation**: the installed export set names none of these properties (FR-015); the static merge (next entity) replaces the link-time dependency expression for static configurations (R-009); `SIMDJSON_INSTALL OFF` means simdjson defines no install rule of its own (FR-014).

### Internal wrapper unit

`source/simdjson/simdjson_gate.cpp`, the single translation unit where simdjson headers enter the build.

| Element | Content | Notes |
|---|---|---|
| include | `#include <simdjson.h>` (via the SYSTEM interface include) | The only such line in the project (FR-013, audit-enforced); SYSTEM keeps header-originated diagnostics off while the wrapper's own lines stay gated (R-008) |
| version gate | `static_assert(simdjson::SIMDJSON_VERSION_MAJOR == 4 && ... MINOR == 6 && ... REVISION == 11, "...expected simdjson 4.6.11...")` | Compile-time; diagnostic names the expected version and the enum values; rejects 4.6.x patch drift (FR-003, R-002) |
| link proof | `[[maybe_unused]] [[gnu::used]] constinit` function-pointer reference to `simdjson::get_active_implementation` | Real out-of-line symbol; no runtime version accessor exists (R-003); keeps the archive member's necessity real (FR-007, SC-009) |
| runtime footprint | zero runtime lines | compile-time assertion plus static initialization; coverage gates see 0 executable lines (plan Test Plan) |

**Validation**: no header under `include/` includes any simdjson header (FR-013); the wrapper compiles in every Linux preset and on macOS (FR-008); moving the submodule to another revision fails compilation here (SC-006).

### Privacy contract

The consumer-observable surfaces that must carry zero simdjson. Audit commands and pass conditions live in [contracts/privacy-contract.md](contracts/privacy-contract.md); this is the surface inventory.

| Surface | Must contain | Requirements |
|---|---|---|
| installed headers | zero simdjson headers | FR-013/FR-014 |
| installed archives / objects | zero simdjson artifacts as separate files; simdjson objects may live merged inside `libspeedgun-ng.a` | FR-014, R-009 |
| `speedgun-ngConfig.cmake`, `speedgun-ngTargets.cmake` | zero simdjson targets, paths, discovery calls | FR-015 |
| shared-library link interface | no entry resolving to simdjson; no `libsimdjson` runtime dependency | FR-015, SC-004 |
| shared-library dynamic symbols | zero simdjson symbols (hidden members not promoted) | FR-016, SC-004 |
| downstream consumer configure log and link command | zero simdjson resolutions, on a machine that carries simdjson | FR-017, SC-005 |

**Validation**: every row is a command with a binary verdict, run in CI (plan Test Plan).

## Relationships

- The vendored submodule is the bracket's only source; the bracket never reads a system simdjson (FR-004: no discovery call exists anywhere in the build files, audit-enforced).
- `add_subdirectory` produces exactly one vendored library target; the library links it PRIVATE through `BUILD_INTERFACE`; the wrapper unit is the link's sole referent.
- The privacy contract binds the install/export chain. The bracket reaches the same end by a separate route: `install-rules.cmake`'s export set sees no simdjson, `SIMDJSON_INSTALL OFF` defines no install rule, and the static merge keeps the installed artifact self-contained (R-009).
- The downstream consumer test consumes the installed tree exactly as any user would, on a machine that carries simdjson; it is the authoritative proof of the whole set (FR-017).

## State

The ingestion configuration per build (transitions in plan.md → Logical View):

```text
Guarded -> Configured -> Compiling -> Linked -> Merged(static) | Absorbed(shared)
```

| State | Entry | Invariant |
|---|---|---|
| Guarded | bracket invoked | vendored tree non-empty, else FATAL_ERROR naming init (FR-002) |
| Configured | scoped constants applied; `add_subdirectory` processed | simdjson is static, hidden, uninstrumented, install-off (FR-009/FR-010/FR-014/FR-016) |
| Compiling | simdjson targets generated in-tree | compiled with `-fvisibility=hidden`, no `-fsanitize`, no `--coverage`, no `-Werror` (R-006/R-007) |
| Linked | gate TU compiles, PRIVATE link established | `static_assert` holds only at `(4,6,11)` (FR-003); submodule worktree untouched (SC-007) |
| Merged | static configurations only | `libspeedgun-ng.a` contains vendored members; `nm` proves it (SC-009) |
| Absorbed | shared configurations | vendored hidden members absorbed; `.dynsym` exports zero simdjson, `ldd` names no simdjson object (SC-004) |

No transition consults the network, the system package set, or a system simdjson (FR-004).
