# Data Model: Vendor hwloc as a Private, Pinned Submodule

**Feature**: 003-vendor-hwloc | **Date**: 2026-09-20

The feature ships build-time objects: a vendored source tree, a CMake module, a build artifact chain, one wrapper TU, and a contract over observable surfaces. Entities map 1:1 to `spec.md` → Key Entities. The build-state machine (section "State") models the vendor pipeline; the audit surfaces (section "Entities", Privacy contract) carry the privacy invariants.

## Entities

### Vendored submodule

The hwloc sources carried in-tree at the fixed root, input to everything else.

| Field | Value (pinned) | Notes |
|---|---|---|
| `path` | `external/hwloc` | Fixed by clarification 2026-09-20; outside `include/`, `source/`, `test/` (FR-001) |
| `release_tag` | `hwloc-2.14.0` | Lightweight tag naming the commit directly (verified via `git ls-remote`) |
| `pinned_commit` | `b5660dff631a171a96a4b3abf5a170c9cf62d6ef` | The exactness authority (FR-001); floating `GIT_TAG` prohibited |
| `license` | BSD 3-Clause, file `COPYING` | Stays in-tree, ships with source distributions (FR-005) |
| `self_reported_version` | `2.14.0rc2-git` (VERSION file: `major=2 minor=14 release=0 greek=rc2 snapshot=1`) | Verified at the commit 2026-09-20; explains the assertion design (research R-002) |
| `version_macros` | `HWLOC_VERSION_MAJOR=2`, `HWLOC_VERSION_MINOR=14`, `HWLOC_VERSION_RELEASE=0` in generated `include/hwloc/autogen/config.h`; `HWLOC_API_VERSION=0x00020c00` (API series, package-version-blind) | Inputs to the wrapper assertion (R-002) |

**Validation**: configuration aborts when the directory is missing or empty, with the init command named (FR-002); the compile-time assertion rejects any checkout whose `(MAJOR, MINOR, RELEASE)` triple differs from `(2, 14, 0)`, covering 2.14.x patch drift and every other series (FR-003, SC-006); the submodule worktree reports unmodified after builds (SC-007); `git submodule status` matches the pinned commit (FR-001).

### Autotools ingestion module

The reusable capability in `cmake/ImportAutotoolsSubmodule.cmake`. Its full input contract lives in [contracts/ingestion-module.md](contracts/ingestion-module.md); this is the entity view.

| Input (FR-012) | hwloc call-site value |
|---|---|
| vendored submodule path | `external/hwloc` |
| configure invocation | `autogen.sh` then `configure` (git checkout: no pre-generated `configure` at the tag) |
| configure arguments | `--enable-embedded-mode`, `--with-hwloc-symbol-prefix=sg_`, the verified disable list of research R-005 |
| expected output archive | `hwloc/libhwloc_embedded.a` (libtool convenience archive, research R-004) |
| imported target name | `hwloc_vendor` |

**Guarantees** (FR-013 through FR-019, each verified per quickstart section): build ordering for consumers; pristine submodule worktree (copy-based bootstrap, R-007); stamp invalidation keyed on compiler, compiler version, build type, sanitizer state, configure args, via a hash-selected vendor prefix (R-006); offline update steps; private staging prefix under the build tree with zero propagation into `CMAKE_INSTALL_PREFIX` (R-004); configure-time toolchain diagnostics naming apt/dnf/brew packages including `patch` (R-011); per-platform blocks with a guard message pointing at the documented Windows on-ramp (R-014).

**Validation**: the hwloc call site contains the five inputs plus target usage, zero build commands (SC-010); platform-specific code appears only inside per-platform blocks (US3 scenario 5).

### Imported static target

The build-system handle for the private link, produced by the module.

| Property | Value | Notes |
|---|---|---|
| type | `STATIC IMPORTED GLOBAL` | Single internal handle; `GLOBAL` so `test/` and future internal consumers can reach it (R-009) |
| `IMPORTED_LOCATION` | staged `libhwloc_embedded.a` in the private prefix | R-004 |
| `INTERFACE_INCLUDE_DIRECTORIES` | staged `include/` (public headers plus the generated `hwloc/autogen/config.h`) | Gives the wrapper the version macros and the symbol-rename machinery automatically (R-003) |
| `INTERFACE_LINK_LIBRARIES` | `${CMAKE_DL_LIBS}` plus the platform thread library | hwloc core references `dlopen`/`dlsym` probing and pthread primitives; the call-site link test pins the exact set (R-009) |
| usage requirement | `PRIVATE` on `speedgun-ng_speedgun-ng` | Any PUBLIC edge violates FR-007 |
| `BUILD_BYPRODUCTS` | the staged archive | Ninja ordering (FR-015) |

**Validation**: the installed export set names none of these properties (FR-023); the static archive merge (next entity) replaces link-time dependency expression for static configurations (R-010).

### Internal wrapper unit

`source/hwloc/hwloc_gate.cpp`, the single translation unit where hwloc headers enter the build.

| Element | Content | Notes |
|---|---|---|
| include | `#include <hwloc.h>` | The only such line in the project (FR-021, audit-enforced) |
| version gate | `static_assert(HWLOC_VERSION_MAJOR == 2 && HWLOC_VERSION_MINOR == 14 && HWLOC_VERSION_RELEASE == 0, "...expected hwloc 2.14.0...")` | Compile-time; diagnostic names the expected version; rejects 2.14.x patch drift (FR-003, R-002) |
| link proof | `[[maybe_unused]] constinit` function-pointer reference to `hwloc_get_api_version()` | Resolves to the prefixed symbol `sg_hwloc_get_api_version` (R-003); no `reinterpret_cast`, so no P2 exception (plan Constitution Check I); keeps the archive member's necessity real (FR-007, SC-011) |
| runtime footprint | zero runtime lines | The file's content is compile-time assertion plus static initialization; the coverage gates see 0 executable lines (plan Test Plan) |

**Validation**: no header under `include/` includes any hwloc header (FR-021); the wrapper compiles in every Linux preset and on macOS (FR-008); moving the submodule to another revision fails compilation here (SC-006).

### Privacy contract

The consumer-observable surfaces that must carry zero hwloc. Audit commands and pass conditions live in [contracts/privacy-contract.md](contracts/privacy-contract.md); this is the surface inventory.

| Surface | Must contain | Requirements |
|---|---|---|
| installed headers | zero hwloc headers | FR-021/FR-022 |
| installed archives / objects | zero hwloc artifacts as separate files; hwloc objects may live merged inside `libspeedgun-ng.a` | FR-022, R-010 |
| `speedgun-ngConfig.cmake`, `speedgun-ngTargets.cmake` | zero hwloc targets, paths, discovery calls | FR-023 |
| shared-library link interface | no entry resolving to hwloc | FR-023 |
| shared-library dynamic symbols | zero hwloc symbols (prefixed or otherwise) | FR-024, SC-004 |
| runtime dependencies of the installed shared library | no hwloc object | SC-004 |
| downstream consumer configure log and link command | zero hwloc resolutions, on a machine that carries hwloc | FR-026, SC-005 |

**Validation**: every row is a command with a binary verdict, run in CI (plan Test Plan).

## Relationships

- The vendored submodule is the module's only source; the module never reads a system hwloc (FR-004: no discovery call exists anywhere in the build files, audit-enforced).
- The module produces exactly one imported static target; the library links it PRIVATE; the wrapper unit is the link's sole referent.
- The privacy contract binds the install/export chain. The module internals reach the same end by a separate route: `install-rules.cmake`'s export set sees no hwloc, and the static merge keeps the installed artifact self-contained (R-010).
- The downstream consumer test consumes the installed tree exactly as any user would; it is the authoritative proof of the whole set (FR-026).

## State

The vendor pipeline per build-tree prefix (transitions and the invalidation edge are drawn in plan.md → Logical View):

```text
Guarded -> Configured -> Copied -> Bootstrapped -> Built -> Staged -> Merged(static) | (shared: end at Staged)
```

| State | Entry | Invariant |
|---|---|---|
| Guarded | module invoked | vendored tree non-empty; host tools found (FR-002/FR-018) |
| Configured | hash of `{C compiler, compiler version, build type, sanitizer state, configure args}` selects the prefix | a changed hash selects a different prefix: no reuse of any older state (FR-016, SC-008) |
| Copied / Bootstrapped | first build of a fresh prefix | `autoreconf` output lands in the copy; submodule worktree untouched (SC-007) |
| Built | embedded configure + make complete | archive exists at the expected relative path (FR-012); host packages cannot alter contents (FR-020) |
| Staged | copy into the private prefix | `CMAKE_INSTALL_PREFIX` untouched (FR-017) |
| Merged | static configurations only | `libspeedgun-ng.a` contains the vendored members; `nm` proves it (SC-011) |

No transition consults the network, the system package set, or a system hwloc (FR-004/FR-015).
