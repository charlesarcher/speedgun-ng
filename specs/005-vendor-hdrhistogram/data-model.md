# Data Model: Vendor HdrHistogram_c as a Private, Pinned Submodule

**Feature**: 005-vendor-hdrhistogram | **Date**: 2026-09-23

The feature ships build-time objects: two vendored source trees, two `add_subdirectory` scoped brackets, a zlib redirect, a build artifact chain, two wrapper TUs, and a contract over observable surfaces. Entities map 1:1 to `spec.md` → Key Entities. The build-state model (section "State") models the ingestion configuration; the audit surfaces (Privacy contract) carry the privacy invariants.

## Entities

### Vendored HdrHistogram_c submodule

The HdrHistogram_c sources carried in-tree at the fixed root, input to everything else.

| Field | Value (pinned) | Notes |
|---|---|---|
| `path` | `external/hdrhistogram_c` | Fixed by the spec assumption: the root convention for every vendored dependency, matching `external/hwloc`/`external/simdjson`; outside `include/`, `source/`, `test/` (FR-001) |
| `release_tag` | `0.11.10` | Lightweight tag naming the commit directly (verified via `git ls-remote` 2026-09-23) |
| `pinned_commit` | `18c7a324383dded1451d15621cd018b0048057d0` | The exactness authority (FR-001); floating `GIT_TAG`, branches, shallow tracking prohibited |
| `licenses` | MIT (`LICENSE.txt`, `COPYING.txt`) | Stay in-tree, ship with source distributions (FR-005) |
| `self_reported_version` | `0.11.10` (macro `HDR_HISTOGRAM_VERSION` = `"0.11.10"`) | Verified at the tag; the only version identifier, a single string macro (research R-002) |
| `version_identifiers` | none numeric; only the string `HDR_HISTOGRAM_VERSION` | `include/hdr/hdr_histogram_version.h:10`; no MAJOR/MINOR/PATCH constants, so the assertion compares the string (R-002) |
| `symbol_prefix` | `hdr_` (public C symbols) | No upstream rename mechanism; privacy rests on hidden visibility plus static absorption (R-009) |

**Validation**: configuration aborts when the directory is missing or empty, naming the init command (FR-002); the compile-time string assertion rejects any checkout whose `HDR_HISTOGRAM_VERSION` differs from `"0.11.10"` (FR-003, SC-006); the submodule worktree reports unmodified after builds (SC-007); `git submodule status` matches the pinned commit (FR-001).

### Vendored zlib submodule

The zlib sources carried in-tree as the logging component's zlib provider (Fixed decision 6).

| Field | Value (pinned) | Notes |
|---|---|---|
| `path` | `external/zlib` | Root convention, outside `include/`/`source/`/`test/` (FR-001a) |
| `upstream` | `https://github.com/madler/zlib` | |
| `release_tag` | `v1.3.2` | Annotated tag peeling to the pinned commit (verified via `git ls-remote` 2026-09-23) |
| `pinned_commit` | `da607da739fa6047df13e66a2af6b8bec7c2a498` | The exactness authority (FR-001a) |
| `licenses` | zlib license (`LICENSE`, BSD-compatible) | Stay in-tree, ship with source distributions (FR-005) |
| `self_reported_version` | `1.3.2` (macro `ZLIB_VERSION` = `"1.3.2"`, `ZLIB_VERNUM` = `0x1320`) | Verified at the tag (research R-003) |
| `version_identifiers` | `ZLIB_VER_MAJOR=1`, `MINOR=3`, `REVISION=2`, `SUBREVISION=0`; **no `ZLIB_VER_PATCH`** | The patch level is `REVISION`; the assertion compares `ZLIB_VERSION` and `ZLIB_VERNUM` (R-003) |
| `symbol_rename` | `Z_PREFIX` (option `ZLIB_PREFIX ON`) | Renames public symbols to `z_`-prefixed (`inflate` → `z_inflate`, ...); FR-022 control (R-009) |

**Validation**: the same guard, worktree-pristine, and pinned-commit checks as the HdrHistogram_c submodule (FR-002, FR-001a, SC-007); the version assertion rejects any other zlib revision (FR-003, SC-006); `zconf.h` is generated into the binary directory, never the source tree (SC-007, research R-010).

### Build-integration brackets

The two scoped constructs in `CMakeLists.txt` (plus the zlib redirect between them) that own every vendored-build setting. Full contract: [contracts/build-integration.md](contracts/build-integration.md); this is the entity view.

| Constant (fixed) | Value | Reason |
|---|---|---|
| vendored paths | `external/hdrhistogram_c`, `external/zlib` | FR-001, FR-001a |
| HdrHistogram_c options | `BUILD_SHARED OFF`, `BUILD_STATIC ON`, `INSTALL_SHARED OFF`, `INSTALL_STATIC OFF`, `BUILD_PROGRAMS OFF`, `HDR_LOG_REQUIRED` default `ON` | static library-only; logging kept (FR-011, Fixed decision 6; R-004) |
| zlib options | `ZLIB_BUILD_SHARED OFF`, `ZLIB_BUILD_STATIC ON`, `ZLIB_INSTALL OFF`, `ZLIB_BUILD_TESTING OFF`, `ZLIB_PREFIX ON` | static, no install, renamed symbols (FR-011, FR-021, FR-022; R-004) |
| `BUILD_SHARED_LIBS` (scoped, both) | `OFF` | forces each vendored build static in every configuration (FR-007, R-007) |
| `CMAKE_C_FLAGS`/`CXX` (scoped, both) | blanked | drops parent strict warnings, `-Werror`, hardening (FR-009, R-011) |
| `CMAKE_C_FLAGS_SANITIZE`/`_COVERAGE` and `CXX` (scoped) | blanked | no sanitizer, no `--coverage` on vendored code (FR-010, R-011) |
| `CMAKE_C_CLANG_TIDY`/`_CPPCHECK` and `CXX` (scoped) | blanked | no static analysis of vendored code (FR-009, R-011) |
| `CMAKE_C_VISIBILITY_PRESET` (scoped, both) | `hidden` | stamps `STV_HIDDEN` into vendored members (FR-017, FR-022, R-009) |
| `CMAKE_POSITION_INDEPENDENT_CODE` (scoped, both) | `ON` | vendored members absorb into a shared parent (R-006) |
| `EXCLUDE_FROM_ALL` (both) | set | vendored targets excluded from default `all` (FR-011) |
| zlib install neutralization | `ZLIB_INSTALL OFF` | zlib installs nothing (R-010) |
| HdrHistogram_c install neutralization | scope-limited `install()` override | neutralizes the 3 unconditional rules (R-010) |
| zlib redirect | parent-scope `ZLIB::ZLIB` `INTERFACE IMPORTED GLOBAL` → `zlibstatic`; `ZLIB_INCLUDE_DIR`/`ZLIB_LIBRARY` cache pre-seed | HdrHistogram_c's `find_package(ZLIB)` resolves vendored (FR-020, R-008) |
| include promotion | `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` | gate TUs compile vendored headers via `-isystem` (FR-014, R-012) |

**Guarantees** (FR-009 through FR-022, verified per quickstart): offline (no network step); pristine submodule worktrees (out-of-source build dirs, SC-007); no install into `CMAKE_INSTALL_PREFIX` (FR-012); no sanitizer/coverage on vendored objects (FR-010); tidy/cppcheck exempt for vendored code while speedgun-ng code stays held (FR-009); hidden-visibility members plus zlib renaming (FR-017, FR-022); early named guard on an uninitialized tree (FR-002).

**Validation**: the call site holds the two bracket invocations, the zlib redirect, the `PRIVATE` links, and the wrapper registrations; it names no `find_package(hdr_histogram)` and no host zlib resolution (FR-004, FR-020, proven by the purity scans and `dependency_scan`).

### Linked vendored targets

The build-system handles for the private links, produced by the two `add_subdirectory` calls.

| Property | Value | Notes |
|---|---|---|
| HdrHistogram_c target | `hdr_histogram_static` (alias `hdr_histogram::hdr_histogram_static`) | `src/CMakeLists.txt:75` (R-001) |
| zlib target | `zlibstatic` (alias `ZLIB::ZLIBSTATIC`) | `CMakeLists.txt:204` (R-001) |
| zlib redirect handle | `ZLIB::ZLIB` (parent-scope `INTERFACE IMPORTED GLOBAL` → `zlibstatic`) | the sibling-visible name HdrHistogram_c's `find_package(ZLIB)` binds to (R-008) |
| type | both `STATIC` (forced by scoped `BUILD_SHARED_LIBS OFF` plus the per-tree build switches) | one static archive each in every configuration (R-007) |
| member visibility | `STV_HIDDEN` (scoped `C_VISIBILITY_PRESET hidden`); zlib additionally `z_`-prefixed via `Z_PREFIX` | not promoted to a shared `.dynsym` (FR-017, FR-022, R-009) |
| compile flags | none of the parent's strict/sanitizer/coverage flags | FR-009, FR-010, R-011 |
| interface include | promoted to `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` | gate TUs compile vendored headers via `-isystem`; zlib reaches generated `zconf.h` through the target (FR-014, R-012) |
| usage requirement on speedgun-ng | `PRIVATE`, wrapped in `$<BUILD_INTERFACE:...>` | no PUBLIC edge; names never enter the exported set (FR-007, FR-016) |

**Validation**: the installed export set names none of these properties (FR-016); the static merge (next entity) replaces the link-time dependency expression for static configurations (R-006); both install switches mean the vendored trees define no surviving install rule (FR-012).

### Internal wrapper units

`source/hdrhistogram/hdrhistogram_gate.cpp` and `source/zlib/zlib_gate.cpp`, the single translation units where each vendored library's headers enter the build (FR-014).

| Element | Content | Notes |
|---|---|---|
| HdrHistogram_c include | `#include <hdr/hdr_histogram.h>` (via the SYSTEM interface include) | The only such line (FR-014, audit-enforced); SYSTEM keeps header diagnostics off while the gate's own lines stay gated (R-012) |
| HdrHistogram_c version gate | `static_assert(std::string_view{HDR_HISTOGRAM_VERSION} == "0.11.10", "...")` | Compile-time; diagnostic names the expected version; the only identifier available is the string (FR-003, R-002) |
| HdrHistogram_c link proof | `[[maybe_unused]] [[gnu::used]] constinit` reference to `&hdr_alloc` | Real out-of-line `hdr_` symbol; keeps the merged member's necessity real (FR-007, SC-009, R-005) |
| zlib include | `#include <zlib.h>` (via the SYSTEM interface include; reaches generated `<zconf.h>`) | The only such line (FR-014) |
| zlib version gate | `static_assert(std::string_view{ZLIB_VERSION} == "1.3.2" && ZLIB_VERNUM == 0x1320u, "...")` | Compile-time; no `ZLIB_VER_PATCH` exists, the patch level is `REVISION` (FR-003, R-003) |
| zlib link proof | `[[maybe_unused]] [[gnu::used]] constinit` reference to `&zlibVersion` | Under `Z_PREFIX` the header renames it to `z_zlibVersion`, matching the archive symbol (FR-021, SC-010, R-005) |
| runtime footprint | zero runtime lines each | compile-time assertions plus static initialization; coverage gates see 0 executable lines (plan Test Plan) |

**Validation**: no header under `include/` includes any HdrHistogram_c or zlib header (FR-014); the gate TUs compile in every Linux preset and on macOS (FR-008); moving either submodule to another revision fails compilation there (SC-006).

### Privacy contract

The consumer-observable surfaces that must carry zero HdrHistogram_c and zero zlib/libz. Audit commands and pass conditions live in [contracts/privacy-contract.md](contracts/privacy-contract.md); this is the surface inventory. Audit patterns per FR-019: `hdr[-_]?histogram` for HdrHistogram_c, `zlib`/`libz` for zlib, plus the `hdr_` symbol-prefix flag and the `inflate`/`deflate`/`compress`/`uncompress` names on a shared build.

| Surface | Must contain | Requirements |
|---|---|---|
| installed headers | zero HdrHistogram_c and zero zlib headers | FR-014, FR-015 |
| installed archives / objects | zero as separate files; the vendored objects may live merged inside `libspeedgun-ng.a` | FR-015, R-006 |
| `speedgun-ngConfig.cmake`, `speedgun-ngTargets.cmake` | zero references, targets, paths, discovery calls | FR-016, SC-003 |
| shared-library link interface | no entry resolving to either dependency; no `libz` runtime dependency | FR-016, SC-004, SC-010 |
| shared-library dynamic symbols | zero `hdr[-_]?histogram`, zero `hdr_`-prefixed, zero `zlib`/`libz`, zero bare `inflate`/`deflate`/`compress`/`uncompress` | FR-017, FR-022, SC-004, SC-011 |
| downstream consumer configure log and link command | zero resolutions, on a machine carrying both system HdrHistogram_c and zlib | FR-018, SC-005 |

**Validation**: every row is a command with a binary verdict, run in CI (plan Test Plan).

## Relationships

- The two vendored submodules are the brackets' only sources; the build never reads a system HdrHistogram_c (FR-004, no discovery call exists, audit-enforced) and resolves zlib only to the vendored copy (FR-020, redirect plus cache pre-seed).
- The zlib bracket and its redirect run before the HdrHistogram_c bracket, so `zlibstatic` and the `ZLIB::ZLIB` handle exist when HdrHistogram_c's `find_package(ZLIB)` executes (FR-020, R-008).
- Each `add_subdirectory` produces exactly one vendored library target; the library links each PRIVATE through `BUILD_INTERFACE`; each wrapper unit is that link's sole referent.
- The privacy contract binds the install/export chain. The brackets reach the same end by separate routes: `install-rules.cmake`'s export set sees neither dependency, the install switches plus the `install()` override leave nothing to install (R-010), and the static merge keeps the installed artifact self-contained (R-006).
- The downstream consumer test consumes the installed tree exactly as any user would, on a machine carrying both dependencies; it is the authoritative proof of the whole set (FR-018).

## State

The ingestion configuration per build (transitions in plan.md → Logical View):

```text
Guarded -> ZlibConfigured -> ZlibRedirected -> HdrConfigured -> Compiling -> Linked -> Merged(static) | Absorbed(shared)
```

| State | Entry | Invariant |
|---|---|---|
| Guarded | brackets invoked | vendored trees non-empty, else FATAL_ERROR naming init (FR-002) |
| ZlibConfigured | zlib bracket applied | zlib is static, hidden, `Z_PREFIX`-renamed, uninstrumented, install-off (FR-009/FR-010/FR-022) |
| ZlibRedirected | redirect published | parent-scope `ZLIB::ZLIB` → `zlibstatic`; `ZLIB_INCLUDE_DIR`/`ZLIB_LIBRARY`/`ZLIB_FOUND` pre-seeded (FR-020, R-008) |
| HdrConfigured | HdrHistogram_c bracket applied | static library-only, logging on against the redirect, install neutralized (FR-011/FR-012) |
| Compiling | vendored targets generated in-tree | compiled with `-fvisibility=hidden`, no `-fsanitize`, no `--coverage`, no `-Werror` (R-009/R-011) |
| Linked | gate TUs compile, PRIVATE links established | both `static_assert`s hold only at the pins (FR-003); submodule worktrees untouched (SC-007) |
| Merged | static configurations only | `libspeedgun-ng.a` contains both vendored member sets; `nm` proves each reference resolves (SC-009, SC-010) |
| Absorbed | shared configurations | both hidden/prefixed member sets absorbed; `.dynsym` exports zero of either, `ldd` names no `libz` object (SC-004, SC-011) |

No transition consults the network, the system package set, a system HdrHistogram_c, or a host zlib (FR-004, FR-020).
