# Contract: Autotools Ingestion Module

**Feature**: `003-vendor-hwloc` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

`cmake/ImportAutotoolsSubmodule.cmake` ingests an autotools project that lives in a git submodule and produces a first-class imported static-library target. The module owns every build mechanic; a call site supplies inputs and uses the resulting target (FR-011, SC-010). Verified mechanics: research R-001, R-004, R-006, R-007, R-009.

## 1. Interface

```cmake
include(ImportAutotoolsSubmodule)

import_autotools_submodule(
    NAME            hwloc_vendor                    # imported target name
    SUBMODULE_DIR   external/hwloc                  # vendored path (repo-relative)
    BOOTSTRAP       autogen.sh                      # or PREGENERATED (skip bootstrap)
    CONFIGURE_ARGS  --enable-embedded-mode
                    --with-hwloc-symbol-prefix=sg_
                    --disable-libxml2 --disable-opencl --disable-cairo
                    --disable-cuda --disable-levelzero --disable-nvml
                    --disable-rsmi --disable-libudev --disable-pci
                    --disable-plugin-dlopen --disable-plugin-ltdl
    ARCHIVE         hwloc/libhwloc_embedded.a       # expected artifact, relative to
                                                    # the vendor build dir
    MERGE_INTO      speedgun-ng_speedgun-ng         # optional: static-archive merge
)
```

### Input table

| Parameter | Type | Required | Contract |
|---|---|---|---|
| `NAME` | identifier | yes | Name of the imported target produced; must not collide |
| `SUBMODULE_DIR` | path | yes | Must exist and be non-empty at configure time; a missing or empty tree aborts with a message naming `git submodule update --init <dir>` (FR-002) |
| `BOOTSTRAP` | `autogen.sh` \| `PREGENERATED` | yes | `autogen.sh`: runs the script in the copied tree before configure (the git-checkout case at the tag: no pre-generated `configure`). `PREGENERATED`: configure runs directly in the copy, no host autoconf tools demanded beyond `make`/`sh` |
| `CONFIGURE_ARGS` | list | yes | Passed verbatim to `configure`, after the module's fixed inputs (`--prefix`, compiler selection, curated flags) |
| `ARCHIVE` | relative path | yes | The archive the module must find after `make`; absence fails the build naming the path |
| `MERGE_INTO` | target | no | When given and that target is a static library: a post-build step merges the staged archive members into it (research R-010). Ignored for shared configurations. The merge is the module's build command; keeping it out of call sites is what preserves SC-010 |

### Prohibited at call sites (SC-010)

Any `add_custom_command`, `execute_process`, compiler flag manipulation, or archiver invocation in the consuming `CMakeLists.txt`. The call site holds: this call, the `PRIVATE` link, the source-file registration of the wrapper TU.

## 2. Guarantees

Each holds for every invocation and every Linux preset identically.

| Guarantee | Mechanism | Requirement |
|---|---|---|
| Correct build ordering for the consumer | imported target carries an internal dependency edge on the external project; `BUILD_BYPRODUCTS` on the archive for Ninja | FR-013/FR-015 |
| Pristine submodule worktree | bootstrap runs in a build-tree copy; no download/update/patch/git step; `UPDATE_COMMAND ""`, `PATCH_COMMAND ""` | FR-014/FR-015, SC-007 |
| Configure step does not re-run per build | `CONFIGURE_HANDLED_BY_BUILD ON` (CMake 3.20 floor) | FR-015 |
| Input changes force a full vendor rebuild | vendor prefix directory keyed on `hash(CMAKE_C_COMPILER, compiler version, CMAKE_BUILD_TYPE, sanitizer-flag state, CONFIGURE_ARGS)`; any change redirects to a fresh prefix | FR-016, SC-008 |
| Curated child flags | `CC`/`CXX` explicit; child `CFLAGS` = build-type `-O/-g` plus compatible hardening; zero `-fsanitize`, zero parent warning sets, zero `-Werror` | FR-010, R-008 |
| Private prefix only | staging under the build tree; the module defines no install rule; `CMAKE_INSTALL_PREFIX` receives nothing | FR-017, SC-002 |
| Early, named toolchain failure | `find_program` probe of `autoconf`, `automake`, `libtool` (macOS `glibtool` aliases accepted), `patch`, `make`, `sh`; miss aborts at configure listing apt / dnf / brew package sets (`patch` included: `autogen.sh` applies `config/libtool-big-sur-fixup.patch`) | FR-018, R-011 |
| Archive contents independent of host packages | the disable list plus embedded mode; nothing auto-detected remains | FR-020 |
| Platform confinement | shared logic plus per-platform blocks; POSIX implemented; unsupported platforms abort pointing at `contrib/windows-cmake/` as the documented on-ramp | FR-019, R-014 |
| Symbol prefix consistency | prefix flows into hwloc's generated public config header, so every TU including `<hwloc.h>` against the staged headers references renamed symbols automatically | FR-025, R-003 |

## 3. Failure modes

| Condition | Detection | Result |
|---|---|---|
| Submodule uninitialized (missing or empty dir) | configure-time existence check | `FATAL_ERROR` naming `git submodule update --init <SUBMODULE_DIR>`; zero fallback paths (FR-002, FR-004) |
| Host tool missing | configure-time probe | `FATAL_ERROR` naming the tool and the platform package set (FR-018) |
| `ARCHIVE` absent after build | post-build existence check | build fails naming the expected path and the vendor prefix to inspect |
| Wrong vendored revision | downstream compile of the wrapper's `static_assert` | compilation fails with the expected-version diagnostic (FR-003); the module does not second-guess the git state beyond FR-002 |
| Unsupported platform | platform dispatch | `FATAL_ERROR` stating the supported set and the Windows on-ramp reference (R-014) |

## 4. Module header documentation duty

The module file's header comment carries: the input table, the guarantee list above in prose, the sanitizer exclusion policy and its rationale (FR-010), and a pointer to this contract file. The hwloc call-site comment carries the re-pinning pointer to the README section (FR-006).
