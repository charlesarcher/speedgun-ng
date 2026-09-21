# Research: Vendor hwloc as a Private, Pinned Submodule

**Feature**: 003-vendor-hwloc | **Date**: 2026-09-20
**Status**: All NEEDS CLARIFICATION resolved (0 remaining).

Every fact below was verified on 2026-09-20 against commit `b5660dff631a171a96a4b3abf5a170c9cf62d6ef` (the commit tag `hwloc-2.14.0` names, re-confirmed via `git ls-remote` this session), against the upstream files at that commit, and against the speedgun-ng tree as it stands. Each decision names the requirements it settles.

## Phase 0 Findings

### R-001: Ingestion mechanism = submodule + copy-and-bootstrap ExternalProject inside a reusable module

- **Decision**: hwloc enters as a git submodule at `external/hwloc` (settled path, clarification 2026-09-20). The build ingests it through `cmake/ImportAutotoolsSubmodule.cmake`, a `ExternalProject_Add`-based module: it copies the submodule sources into the build tree, runs `autogen.sh` in the copy, configures and builds out of tree, and hands back an imported static target. The hwloc call site supplies inputs (FR-011/FR-012).
- **Rationale**: `autogen.sh` at the tag runs `autoreconf -ivf` plus an in-tree `patch` of the generated `configure` (verified in `autogen.sh`), so bootstrapping inside the submodule dirties it; SC-007 requires a pristine worktree, so the copy is mandatory (FR-014). A git checkout carries no pre-generated `configure`, so `autogen.sh` always runs (FR-018 names the host tools it needs).
- **Alternatives considered**:
  - *FetchContent / registry distribution*: rejected by the spec (vendoring or nothing, Scope boundaries).
  - *In-place bootstrap of the submodule*: rejected: `autoreconf` writes generated files into the tree it is handed; SC-007 fails.
  - *Upstream autotools-parent embedding* (`HWLOC_SET_SYMBOL_PREFIX` + `HWLOC_SETUP_CORE` m4 inclusion, the Open MPI shape verified at `tests/hwloc/embedded/configure.ac`): rejected; it presupposes an autotools parent build; speedgun-ng is CMake. The m4 route also couples our build to hwloc's automake internals.
  - *`contrib/windows-cmake/` wrapper on POSIX*: rejected: it is Windows-scoped upstream, performs live host feature detection (`check_include_file`/`check_symbol_exists`), and host-dependent detection violates the artifact-determinism requirement (FR-020 edge case).

### R-002: Version assertion = `(MAJOR, MINOR, RELEASE) == (2, 14, 0)` on the generated public header

- **Decision**: The wrapper TU asserts `static_assert(HWLOC_VERSION_MAJOR == 2 && HWLOC_VERSION_MINOR == 14 && HWLOC_VERSION_RELEASE == 0, "...expected hwloc 2.14.0...")`. The macros come from `include/hwloc/autogen/config.h` (generated from `include/hwloc/autogen/config.h.in`, which carries `#undef HWLOC_VERSION_MAJOR/MINOR/RELEASE/GREEK`), included transitively by `#include <hwloc.h>` (verified `hwloc.h:59`). `GREEK` is unchecked.
- **Rationale**: FR-003 and its clarification demand exactly 2.14.0 with a readable diagnostic. The triple rejects 2.13.x and older, 2.15+, 3.x, and any 2.14.x patch release (`RELEASE == 0` fails at 2.14.1). Two verified facts constrain the design. The pinned tree's `VERSION` file reads `major=2 minor=14 release=0 greek=rc2 snapshot=1`, so the self-reported version string is `2.14.0rc2-git` (the release commit kept the rc2 greek plus snapshot marker); a string equality assertion would reject the exact commit the tag names. `HWLOC_API_VERSION` at the tag is `0x00020c00` (`hwloc.h:118`): an API/ABI level distinct from the library version by the header's own comment (`hwloc.h:114-116`); it cannot express "2.14.0". The submodule SHA pin is the exactness authority (FR-001); the macro assertion is the drift tripwire for a hand-moved worktree, and the triple is the strongest reading that passes on the pin (SC-006).
- **Alternatives considered**:
  - *Assert `HWLOC_API_VERSION == 0x00020c00`*: rejected. The macro tracks the API/ABI level (the header comment at `hwloc.h:114-116` distinguishes it from the library version); it cannot pin a package release.
  - *`static_assert(std::string_view(HWLOC_VERSION) == "2.14.0")`*: rejected: fails against the pinned commit itself (`2.14.0rc2-git`); an assertion that rejects the spec's own pin is broken by construction.
  - *Assert the triple plus `GREEK == ""`*: rejected: the pin's greek is `rc2`; same failure mode.
  - *Runtime `hwloc_get_api_version()` comparison*: rejected as the assertion: FR-003 says compile-time. The runtime call keeps its separate job as the linkage proof (R-010).

### R-003: Symbol prefix = `sg_`, adopted through the autoconf input the prefix machinery reads

- **Decision**: Configure with `--with-hwloc-symbol-prefix=sg_`. Exported hwloc symbols become `sg_hwloc_<name>` (example: `sg_hwloc_get_api_version`).
- **Rationale**: FR-025 adopts the prefix; the motivation recorded in the spec is collision removal when a consumer process carries a system hwloc alongside ours (the Charm++ precedent). The mechanism verified at the tag: `config/hwloc.m4:145-162` reads `with_hwloc_symbol_prefix` (autoconf sets it from `--with-hwloc-symbol-prefix=<str>`), `AC_DEFINE`s `HWLOC_SYM_PREFIX`/`HWLOC_SYM_PREFIX_CAPS`, and enables `HWLOC_SYM_TRANSFORM` whenever the value differs from the default `hwloc_`. `include/hwloc/rename.h:23-33` maps every public symbol through `HWLOC_NAME(name)` = `<prefix>hwloc_<name>`, and `rename.h` is pulled by `hwloc.h`. Consistency is structural: any TU including `<hwloc.h>` against the staged headers sees the renamed references automatically, the wrapper unit included (FR-025, R-009).
- **Alternatives considered**:
  - *Keep the default `hwloc_` prefix*: rejected: the clarification adopts a custom prefix; the default leaves the collision surface open.
  - *Call the `HWLOC_SET_SYMBOL_PREFIX` m4 macro from our configure*: rejected: that macro exists for an autotools embedder's `configure.ac` (R-001); CMake has no parent autoconf to host it. The configure-flag input reaches the identical variable.

### R-004: Build subset = `--enable-embedded-mode`, consumed as the convenience archive; the private prefix is a staged copy under the build tree

- **Decision**: Configure with `--enable-embedded-mode`. The module stages the result under `<build>/<vendor>/hwloc-prefix/` (`include/` plus `lib/libhwloc_embedded.a`) as the private prefix (FR-017). Nothing ever installs into `CMAKE_INSTALL_PREFIX`.
- **Rationale**: Verified at the tag: with embedded mode the top `Makefile.am` builds `SUBDIRS = include hwloc` (the standalone branches into `utils tests contrib doc ...` are standalone-only, so tools, tests, and documentation are off structurally), and `hwloc/Makefile.am:20` builds `noinst_LTLIBRARIES = libhwloc_embedded.la`: the libtool convenience archive, upstream's canonical embedding artifact (hwloc.md Facts; `HWLOC_LIBS` is forced empty in embedded mode, `hwloc.m4:1849-1861`). Upstream installs nothing for embedded mode, so the module's staging copy realizes FR-017's private prefix with one generator-independent mechanism and gives the wrapper a single, stable include path.
- **Alternatives considered**:
  - *Standalone build with a long `--disable-*` list*: rejected: standalone still builds the shared library, tools, docs, and tests unless each is switched, more surface than needed, and it drifts from the upstream embedding story the spec adopts.
  - *Consume the archive straight out of the external project build tree (no staging)*: considered; rejected in favor of staging: FR-017 names a private prefix, and a staged tree keeps the imported target's properties stable and auditable (the SC-002 install-tree audit reads one prefix root and finds speedgun-ng's own files in it).

### R-005: Optional-feature disable list (exact flag spellings verified at the tag)

- **Decision**: `--disable-libxml2 --disable-opencl --disable-cairo --disable-cuda --disable-levelzero --disable-nvml --disable-rsmi --disable-libudev --disable-pci --disable-plugin-dlopen --disable-plugin-ltdl`.
- **Rationale**: FR-020 names this minimum set; the flag spellings were extracted from the configure help text in `config/hwloc.m4` at the tag (`cairo`, `levelzero`, `libudev`, `plugin-dlopen`, `plugin-ltdl` all appear there). With every optional feature off, the archive contents do not vary with packages the host happens to carry (FR-020 edge case), and `gl` (the `--enable-gl` help line) is reachable in standalone `lstopo` only, embedded mode builds no tools, so no flag is needed.
- **Alternatives considered**: *`--without-*` spellings*: rejected: the tree reads `enable_*` variables (autoconf magic), the verified set is `--disable-*`. *Pass nothing and rely on defaults*: rejected: hwloc auto-detects host packages (`libxml2`, `udev`, ...); the archive would then vary per machine, exactly the edge case FR-020 forbids.

### R-006: Re-run discipline = `CONFIGURE_HANDLED_BY_BUILD` plus hash-keyed vendor prefixes for stamp invalidation

- **Decision**: The module sets `CONFIGURE_HANDLED_BY_BUILD ON` (CMake 3.20+, the project's floor) and keys the external project's `PREFIX` directory on a hash of: `CMAKE_C_COMPILER`, the compiler's version output, `CMAKE_BUILD_TYPE`, the sanitizer-flag state in effect, and the full configure-argument list. Any change to those inputs redirects the build to a fresh prefix: full copy, bootstrap, configure, and build, from scratch.
- **Rationale**: FR-015 and FR-016. `CONFIGURE_HANDLED_BY_BUILD ON` stops the configure step from re-running on every `make`/`ninja` invocation (the spec edge case) by letting the build system order the configure step only when its stamps demand. ExternalProject stamp files do not track compiler identity, build type, or flags by default; a hash-keyed prefix is the one mechanism where stale state cannot survive: the old prefix is never the active one (SC-008: the rebuild is visible in the log because the whole vendor target runs).
- **Alternatives considered**:
  - *Stamp-file surgery (delete configure/build stamps on input change)*: rejected: fragile across Make/Ninja/Xcode and easy to miss one step; a missed stamp is the silent-stale-archive defect FR-016 forbids.
  - *Always-run configure*: rejected: the spec names this as the waste to avoid.
  - *Rely on CMake re-generating the external project script when `ExternalProject_Add` arguments change*: rejected: regenerated step scripts do not remove existing stamps; that is precisely the trap FR-016 calls out.

### R-007: Pristine source = copy-based download, every network step disabled

- **Decision**: No download, update, patch, or git step touches the network. The configure chain begins with `${CMAKE_COMMAND} -E copy_directory external/hwloc <prefix>/src` (executed as the first configure command, inside the copied area's step), then `autogen.sh` in the copy, then `configure` in a separate build directory. `GIT_REPOSITORY` and friends stay unset; `UPDATE_COMMAND ""` and `PATCH_COMMAND ""`.
- **Rationale**: FR-014/FR-015. The submodule is the only source; a build-time network step would contradict "the only hwloc that exists is the pinned one" and break offline builds. With `CONFIGURE_HANDLED_BY_BUILD` (R-006) the copy re-runs exactly when the vendor step runs, and every such run wants a fresh copy anyway.
- **Alternatives considered**: *symlink farm*: rejected: `autoreconf` writes through and around it, and generator support is inconsistent. *`file(COPY)` at configure time*: rejected: re-copies on every CMake re-configure even when hwloc is untouched, and it runs at configure time, outside build-system ordering.

### R-008: Sanitizer policy = vendored archive excluded from instrumentation, enforced structurally, documented in the module header

- **Decision**: The vendored C archive builds without sanitizer flags on every Linux job. The module passes a curated `CFLAGS` to `configure` (`-O`/`-g` from the build type plus hardening where compatible; no `-fsanitize`, no parent warning set, no `-Werror`), and passes `CC=${CMAKE_C_COMPILER}` explicitly.
- **Rationale**: Settled by clarification 2026-09-20 (SC / FR-010): hwloc's C code sits outside the project's defect surface, and instrumented callers still get out-of-bounds detection on hwloc-allocated memory. The exclusion is structural: `ExternalProject` children inherit no parent `CMAKE_*_FLAGS`; the child compiler flags are whatever the module hands `configure`. The same module and call site serve every Linux preset, so the consistency the spec demands ("silent inconsistency between jobs treated as a defect") holds by construction; the rationale text lives in the module header (FR-010 "documented with its rationale"). The repo's sanitizer injection point (`CMAKE_CXX_FLAGS_SANITIZE`, `CMakePresets.json:130`) touches C++ flags, a second reason the C archive cannot be instrumented accidentally.
- **Alternatives considered**: *Instrumented vendored archive*: rejected by the clarification. *Per-job flag overrides*: rejected: per-job divergence is the named defect.

### R-009: Imported target = `STATIC IMPORTED GLOBAL`, ordering and system libraries carried by the module

- **Decision**: The module produces `add_library(<name> STATIC IMPORTED GLOBAL)` with `IMPORTED_LOCATION` on the staged archive, `INTERFACE_INCLUDE_DIRECTORIES` on the staged `include/`, `INTERFACE_LINK_LIBRARIES` set to `${CMAKE_DL_LIBS}` plus the platform thread library, `BUILD_BYPRODUCTS` on the archive, and an explicit internal dependency edge so any consumer link orders after the external project. The hwloc call site links it with `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE hwloc_vendor)`: a PUBLIC edge is prohibited (FR-007).
- **Rationale**: FR-013: the module owns the imported-target dependency problem; the call site stays thin (SC-010). `GLOBAL` makes the handle usable from `test/` and future internal consumers without re-export. Ninja cannot infer the archive from the configure/make command lines, hence `BUILD_BYPRODUCTS` (FR-015, spec edge case). `dl`/thread linkage: hwloc core references `dlopen`/`dlsym` (feature probing, independent of the disabled plugin loaders) and pthread primitives on Linux; the call-site link test (R-010 wrapper compiling and linking) is the verification seam that pins the exact set at implement time.
- **Alternatives considered**: *raw file path in `target_link_libraries`*: rejected: loses include directories and ordering to the call site, violating FR-013/SC-010. *`INTERFACE` library wrapper*: rejected: one more indirection for a single consumer, constitution X.2.

### R-010: Static builds merge the convenience archive into `libspeedgun-ng.a`; shared builds link it PRIVATE

- **Decision**: When `BUILD_SHARED_LIBS=OFF` (the repo default, `variables.cmake`), a post-build step merges the vendored archive members into the speedgun-ng static archive through the archiver (`ar -M` with an `ADDLIB` script on GNU ar, `llvm-ar -M` on current macOS toolchains). When `BUILD_SHARED_LIBS=ON`, the library links the imported target PRIVATE; hidden visibility (`CMakeLists.txt:38-39`, already set) keeps every hwloc symbol out of the dynamic table. The wrapper unit `source/hwloc/hwloc_gate.cpp` is the single TU that includes `<hwloc.h>`: it holds the version assertion (R-002) and takes the address of `hwloc_get_api_version()` (the prefixed symbol) in a `constinit` pointer, the reference that keeps the link real.
- **Rationale**: Two requirements meet here. FR-007 and SC-011 demand `nm` on the built static archive to show hwloc objects pulled in by the wrapper's reference; static archives never absorb members of other archives at creation, so the merge step is what makes the members present, and it is exactly the upstream "slurping the embedded archive into the enclosing library" pattern hwloc.md Facts records for Open MPI. The same merge answers a consumer-facing correctness fact the privacy contract forces: a static library's PRIVATE link dependencies do not survive installation, so a downstream link of a thin `libspeedgun-ng.a` would find unresolved `sg_hwloc_*` the moment anything pulled the gate object. A merged archive is self-contained, exports nothing (SC-004 audits the shared build), and keeps hwloc out of every package file (FR-023). The wrapper placement matches the house convention for internal implementation (`source/dbc/` precedent, R-015).
- **Alternatives considered**: *Thin static archive plus a hidden `INTERFACE_LINK_LIBRARIES` carry-over*: rejected: it leaks hwloc paths into the installed `speedgun-ngTargets.cmake` (FR-023 violation) and pushes a dependency onto consumers in disguise. *`$<TARGET_OBJECTS>`*: impossible: an imported static target exposes no object list. *Skip the merge for static builds*: rejected: it fails SC-011 and breaks the future internal consumer the moment it references the gate.

### R-011: Host-toolchain check names the packages, `patch` included

- **Decision**: At configure time the module `find_program`s `autoconf`, `automake`, `libtool` (accepting `glibtoolize`/`glibtool` aliases on macOS, the Homebrew names), `patch`, `make`, and a POSIX `sh`; any miss aborts configuration with a message naming the packages to install: `apt-get install autoconf automake libtool patch` for Debian-family Linux, `dnf install autoconf automake libtool patch` for RPM-family, `brew install autoconf automake libtool` for macOS (BSD `patch` ships with macOS).
- **Rationale**: FR-018 plus the spec edge case for the missing-toolchain diagnostic. `patch` joins the list from evidence: `autogen.sh` at the tag applies `config/libtool-big-sur-fixup.patch` with `patch` during bootstrap, so a host without it fails mid-build; failing early at configure with a package name is the module's contract.
- **Alternatives considered**: *let `autogen.sh` fail naturally*: rejected: an `autoreconf: command not found` in a build log is exactly the diagnostic quality FR-018 upgrades; *probe by running `autoreconf --version`*: rejected: `find_program` plus a readable message is the CMake idiom and matches the repo's hard-fail style (`coverage.cmake:7-13`).

### R-012: CI wiring = submodule checkouts, toolchain packages, four audits, one new consumer job

- **Decision**:
  1. Every `actions/checkout@v4` step in `.github/workflows/ci.yml` gains `submodules: true` (FR-009; verified: no checkout carries the flag today).
  2. Ubuntu jobs add `autoconf automake libtool patch` to their `apt-get install` lists; the `test-rocky` container step adds `dnf -y install autoconf automake libtool patch` (`make` is already installed there).
  3. The `test` job gains post-install audit steps: install-tree audit (`find prefix/ -iname '*hwloc*'` must be empty, SC-002), package-config audit (`grep -i hwloc prefix/lib/cmake/speedgun-ng/*.cmake` must be empty, SC-003), and the grep purity audit via the registered ctest script (SC-009).
  4. A new `shared-audit` job builds `-DBUILD_SHARED_LIBS=ON`, installs to a prefix, and asserts `nm -D --defined-only <prefix>/lib/libspeedgun-ng.so | grep -i hwloc` empty plus `ldd` on the installed library naming no hwloc object (SC-004; privacy-contract A5 names the installed shared library); the static nm proof (SC-011, hwloc objects present in the archive) rides the `test` job's build tree.
  5. A new `downstream-consumer` job installs the library to a prefix, installs system hwloc (`apt-get install libhwloc-dev`) so the machine carries one, then configures, builds, and runs `test/consumer/` (a `find_package(speedgun-ng)` project) with `CMAKE_PREFIX_PATH` pointed at the install; it asserts exit 0 at each step and greps the consumer's configure log and link command for zero hwloc resolutions (FR-026, SC-005, the authoritative privacy proof).
- **Rationale**: FR-008/FR-009/FR-024/FR-026 with the spec's audit list; the existing `test` job already installs (`cmake --install build --prefix prefix`), so the tree audits land where the install already happens. The existing `consumer-release` job is DBC cache asserts, a find_package consumer does not exist yet, hence the new job (verified `.github/workflows/ci.yml:146-236`).
- **Alternatives considered**: *fold everything into `test`*: rejected: the shared-library build and the system-hwloc consumer environment change the job's build matrix shape; separate jobs keep each audit's failure diagnosis readable. *Run the consumer test without installing system hwloc*: rejected: FR-026 names a machine with hwloc present; `libhwloc-dev` on the runner supplies it.

### R-013: Static-analysis, coverage, format, and spell gates exclude `external/`

- **Decision**: `external/` joins `.codespellrc`'s `skip` list (the only config edit needed). No other gate configuration changes.
- **Rationale**: FR-009's exemption list is already structural for three of the four gates, verified in-tree: clang-tidy and cppcheck run through `CMAKE_CXX_CLANG_TIDY`/`CMAKE_CXX_CPPCHECK` (`CMakePresets.json:33-45`), which apply to CMake targets; the external project's children are autotools processes, invisible to both. The coverage trace extracts only `include/speedgun-ng/*` and `source/*` (`cmake/coverage.cmake:50-51`), a whitelist that `external/` never enters; the vendored archive also never sees `--coverage` (R-008 reasoning). `cmake/lint.cmake` formats a whitelist of `source/ include/ test/ example/` globs (lines 10-16). Codespell runs repo-wide (`cmake/spell.cmake`), so it is the one gate needing the explicit skip. Speedgun-ng code that calls hwloc (the wrapper) lives in `source/` and is held to every gate (FR-009, second half).
- **Alternatives considered**: *per-tool `--exclude` flags*: rejected: the existing gates are whitelist- or target-scoped; adding exclusion machinery would be dead configuration.

### R-014: Platform structure = shared logic plus one POSIX block; Windows stays additive

- **Decision**: The module keeps platform-specific code in per-platform blocks (FR-019). This feature implements the POSIX block only. On an unsupported platform at configure time the module fails with a message pointing at the documented on-ramp: upstream `contrib/windows-cmake/` at the `hwloc-2.14.0` tag (presence verified in hwloc.md Facts).
- **Rationale**: The spec's platform policy: Linux enforced, macOS developer-must-build, Windows unprecluded at zero cost. A guard message plus confined platform blocks keep a future Windows port additive (the spec's Windows edge case); no MSYS, no autotools-on-Windows, no MSVC work happens here.
- **Alternatives considered**: *write a Windows block now*: rejected: explicitly out of scope; speculative generality under X.2. *no platform structure at all*: rejected: FR-019 makes the block structure itself a requirement, reviewable as shape.

### R-015: macOS build path is developer-local, pinned facts included

- **Decision**: The module passes `CC=`/`CXX=` explicitly from `CMAKE_C_COMPILER`, documents `brew install autoconf automake libtool` in the missing-toolchain diagnostic, and relies on `autoreconf` finding Homebrew's `glibtoolize` (keg-only paths need the standard `brew` PATH prepend; the quickstart states it). The `ci-macos` preset (Xcode generator, Release-only) must configure and build; verification is developer-local per the spec's macOS assumption.
- **Rationale**: The spec assumes macOS verification off CI (no macOS runner exists; `ci-macos` stays usable). Verified facts that shape the path: `autogen.sh` itself detects and patches the Apple `libtool.m4` Big Sur bug; `hwloc.m4:145+` runs identically under AppleClang; `llvm-ar` (the macOS archiver) supports the `ar -M` merge used by R-010.
- **Alternatives considered**: *require macOS CI*: rejected: the matrix carries no runner (brief, Current state), and adding runners is out of scope.

---

**Open items**: none. Every Technical Context unknown resolves through R-001 through R-015; the two spec-level tensions (the rc2 VERSION string against FR-003's "exactly 2.14.0"; the static-archive self-containment against FR-023's zero-leakage) are settled here with the evidence in place (R-002, R-010).
