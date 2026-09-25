# Feature prompt: vendor hwloc as a private, pinned submodule

Input for `/speckit.specify`. This brief describes dependency
infrastructure only: importing, pinning, building, and hiding hwloc.
It does not define any topology API built on top of it.

## Mission

Import hwloc 2.14.0 into speedgun-ng as a git submodule, build it as
part of the speedgun-ng build, and link it into the speedgun-ng
library as a strictly internal dependency. Consumers of speedgun-ng
must have no knowledge that hwloc exists: no headers, no CMake
package references, no symbols, no installed artifacts.

## Current state

- No `.gitmodules` in the repository; this is the first vendored
  dependency, so the mechanism we choose here sets the precedent.
- The library target is `speedgun-ng_speedgun-ng` (alias
  `speedgun-ng::speedgun-ng`), installed via the `speedgun-ngTargets`
  export set from `cmake/install-rules.cmake`.
- `CXX_VISIBILITY_PRESET hidden` and `VISIBILITY_INLINES_HIDDEN` are
  already set on the library target.
- CI (`.github/workflows/ci.yml`) is Linux-only today: lint,
  coverage, sanitize, test, test-rocky, consumer-release, dbc-gate,
  docs, all on `ubuntu-26.04`. Presets `ci-macos` (Xcode) and
  `ci-windows` (Visual Studio 2022) exist; no workflow runs them.
  Platform policy for this feature: Linux is the enforced gate,
  macOS must keep building for developers, and Windows stays
  possible by design while carrying zero priority, runners, or
  funding.

## Facts (verified 2026-09-15)

- Latest release: **2.14.0**, git tag `hwloc-2.14.0` in
  `open-mpi/hwloc`. Master has moved to 3.0.0 development; we do not
  track master.
- License: BSD 3-clause (file `COPYING`). Compatible with our license
  posture; the vendored license file must be preserved and shipped
  with source distributions.
- hwloc is an autotools project. There is no first-class
  `CMakeLists.txt` at the top of the upstream tree.
- `contrib/windows-cmake/` **is present in the `hwloc-2.14.0` tag**
  (verified, file read at the tag): a current CMake wrapper defining
  a `hwloc` target; honors `BUILD_SHARED_LIBS` (static with OFF),
  defaults plugin support to OFF, performs live feature detection
  with `check_include_file`/`check_symbol_exists`, keeps its own
  tests, and offers `HWLOC_SKIP_TOOLS`, `HWLOC_SKIP_LSTOPO`, and
  `HWLOC_SKIP_INCLUDES`, which covers the tools-off and
  headers-uninstalled posture the privacy contract needs. Its
  README states it derives from vcpkg's pre-autotools hwloc CMake
  port. It is Windows-scoped upstream, and it is the known on-ramp
  if Windows work ever gets funded.
- Building hwloc from a git checkout (as opposed to an official
  tarball) requires running `autogen.sh`, which needs autoconf,
  automake, and libtool on the build host. Official tarballs ship a
  pre-generated `configure` script but are not distributed via
  GitHub. `autogen.sh`/`autoreconf` writes into the source tree it
  is handed; a submodule bootstrapped in place is dirty under
  `git status` afterwards.
- Upstream ships an official embedding story: the "Embedding hwloc
  in Other Software" document describes autotools
  `--enable-embedded-mode`, which builds `libhwloc_embedded.a`, a
  libtool convenience archive with header install, documentation,
  tools, and tests switched off. `HWLOC_SET_SYMBOL_PREFIX` renames
  the exported symbols on request. Open MPI and Charm++ embed hwloc
  exactly this way; upstream calls slurping the embedded archive
  into an enclosing library the canonical pattern.
- vcpkg's current hwloc port drives the autotools build on every
  platform, Windows/MSVC included, via its MSYS-based toolchain
  support (`vcpkg_configure_make AUTOCONFIG`), with
  `ac_cv_prog_cc_c99=` and `--disable-plugin-dlopen` for MSVC, two
  small patches, and a disable list: libxml2, opencl, cairo, cuda,
  libudev, levelzero, nvml, rsmi, pci. Its Windows leg is moot under
  current priorities; the disable list stands as a maintained
  reference for a minimal private hwloc build.
- Prior art consensus on autotools-inside-CMake (CMake discourse,
  scivision, Stack Overflow, ROS2 vendor packages):
  `ExternalProject_Add` driving `configure`/`make`, an imported
  static target, `CONFIGURE_HANDLED_BY_BUILD` (CMake 3.20+) so the
  configure step stops re-running constantly, `BUILD_BYPRODUCTS` so
  Ninja can order the archive, disconnected update steps for
  submodule sources, and a configure-time toolchain availability
  check that fails early with package names. Meson's
  `external_project` module documents the same model: out-of-tree
  builds, with `make` detecting reconfigure itself.
- hwloc is a C library. It defines `HWLOC_API_VERSION`, which we can
  use to assert the exact version at compile time.

## Decisions (fixed; the spec holds these settled)

1. **Submodule, pinned.** hwloc enters the tree as a git submodule at
   tag `hwloc-2.14.0`. The submodule commit is recorded in our tree,
   which pins the exact SHA. No `GIT_TAG` floats, no branches, no
   shallow tracking.
2. **Never resolve hwloc from elsewhere.** No `find_package(hwloc)`,
   no `pkg_check_modules(hwloc)`, no fallback to a system install. If
   a system hwloc is installed, the speedgun-ng build must still use
   the submodule version. The only hwloc that exists for us is the
   pinned one.
3. **Static, internal linkage.** hwloc is built as a static archive
   and consumed only by the speedgun-ng library target. It is not a
   PUBLIC dependency of any speedgun-ng target.
4. **Invisible to users.** Everything a downstream
   `find_package(speedgun-ng)` consumer can observe must be free of
   hwloc: installed headers, package config files, target link
   interfaces, exported symbols, and include paths.

## Requirements

### Pinning and integrity

- Submodule lives at a vendored path (`external/hwloc` proposed;
  planning may rename once, but it must be outside `include/`,
  `source/`, and `test/`).
- Submodule pointer is the commit that `hwloc-2.14.0` names; record
  the SHA in the spec so re-pinning is auditable.
- A compile-time assertion that `HWLOC_API_VERSION` corresponds to
  2.14, placed in whatever internal header wraps hwloc, so a silent
  submodule bump breaks the build.
- Document the update procedure (checkout new tag, commit pointer,
  bump version assert) in one short section of docs.

### Build integration

- The build must work from a clean clone with `--recurse-submodules`
  or an explicit `git submodule update --init`, on Linux (every CI
  job green) and on macOS developer machines. Windows carries one
  requirement: stay unprecluded. Platform specifics live in
  per-platform blocks inside the ingestion module, so a documented
  Windows on-ramp stays additive work. This feature installs no
  MSYS, drives no autotools on Windows, and chases no MSVC issues.
- hwloc's autotools-only build is ingested through the reusable
  capability defined in the next section. The spec must name the
  mechanism per platform and prove it on the CI matrix; no candidate
  may be assumed to work before it is proven.
- Vendor code is not held to constitution lint: the vendored tree is
  exempt from warning gates, clang-tidy, and cppcheck, and excluded
  from coverage instrumentation. Our code that calls it is held to
  all of it.
- CI workflows must be updated so checkouts fetch submodules, and
  so lint/coverage/cppcheck configurations exclude the vendored
  path.

### Autotools ingestion capability (deliverable)

This feature delivers more than one vendored library: it establishes
the sanctioned way to pull an autotools project into the speedgun-ng
CMake system, because the submodule precedent set here will be
reused. The deliverable is a documented CMake module (working name
`cmake/ImportAutotoolsSubmodule.cmake`) plus one thin,
hwloc-specific call site.

The module contract, minimally:

- Inputs: vendored submodule path, configure invocation
  (`autogen.sh` then `configure`, or a pre-generated `configure`),
  configure args for disabling optional features, expected output
  artifact (static archive), and an imported target name.
- Produces a first-class imported static-library target that our
  library target can link against with correct build ordering.
  The module owns the imported-target dependency problem; the call
  site stays thin.
- Keeps the submodule worktree pristine. Since `autoreconf` writes
  into the source tree it is handed, the module bootstraps in a
  copy of the sources under the build tree; the submodule directory
  stays clean under `git status`. Builds are out-of-tree likewise.
- Uses CMake 3.20+ mechanics: `CONFIGURE_HANDLED_BY_BUILD` so the
  configure step does not re-run on every build, `BUILD_BYPRODUCTS`
  so Ninja can order the static archive, and update steps
  disconnected from the network for submodule sources.
- Installs into a private prefix under the build tree. It propagates
  nothing into `CMAKE_INSTALL_PREFIX`, which makes the non-exposure
  contract structural.
- Rebuild correctness: stamp/invalidation keyed on compiler,
  compiler version, build type, sanitizer flags, and configure
  args, so a flag change forces a hwloc rebuild. A stale archive
  never survives silently.
- Exempts the vendored tree from our warning gates, clang-tidy,
  cppcheck, and coverage, while honoring the sanitizer policy the
  spec records (instrument or exclude, deliberately).
- Fails with a clear message when the host lacks the toolchain it
  needs (autoconf, automake, libtool, make/sh on POSIX), including
  the package names to install.

The platform strategy follows from the priorities: the ingestion
module drives hwloc's autotools build on Linux and macOS. Autotools
is upstream's tested path on both. The prior-art alternatives, a
hand-written CMake shim or vcpkg's MSYS-based Windows route, lose
their rationale once Windows drops out of scope. CI runners install
autoconf, automake, and libtool as apt packages; macOS developers
get them from Homebrew, and the module's missing-toolchain error
names them. Wiring a Windows build is out of scope here; the
contrib wrapper in Facts is the on-ramp whenever someone funds it.

Sanitizer policy detail: planning must decide whether hwloc is
compiled with sanitizer instrumentation or excluded in the sanitize
job, and record the choice with its rationale. Either choice is
defensible; silent inconsistency across the Linux jobs is not
acceptable.

### Non-exposure (the privacy contract)

- No hwloc header is included by any header under `include/`
  (speedgun-ng public headers). hwloc may only be included by
  `source/` translation units, preferably through one internal
  wrapper unit.
- `cmake --install` output contains: no hwloc headers, no hwloc
  archives or shared objects, no hwloc CMake package or config
  files.
- The installed `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake`
  contain no reference to any hwloc target, path, or
  `find_package(hwloc)` call. A shared-library speedgun-ng must not
  carry a `INTERFACE_LINK_LIBRARIES` entry that resolves to hwloc.
- On a shared-library build, the dynamic symbol table of
  `libspeedgun-ng.so` contains no hwloc symbols (verify with `nm` /
  `dumpbin` or an equivalent in CI). Hidden visibility is already on;
  the risk is hwloc leaking back through a PUBLIC link interface.
- Evaluate hwloc's symbol prefix (`HWLOC_SET_SYMBOL_PREFIX` under
  autotools, an equivalent compile-time rename under a CMake shim).
  Hidden visibility keeps hwloc out of our exports; a prefix
  additionally removes any collision when a consumer process loads
  speedgun-ng and a system hwloc together, the same reason Charm++
  prefixes its embedded copy.
- A downstream consumer test must exist: configure a trivial consumer
  project against the *installed* speedgun-ng on a machine image
  with no system hwloc, build it, run it. This is the authoritative
  proof of the privacy contract.

## Acceptance criteria

1. Clean clone with submodules initializes and builds; every Linux
   CI job stays green, and the build works on macOS.
2. `git submodule status` pins the exact 2.14.0 commit; no build path
   can select any other hwloc (grep-able proof: no
   `find_package(hwloc`, no pkg-config fallback).
3. Compile-time `HWLOC_API_VERSION` assert present; flipping the
   submodule to any other tag fails the build with a readable
   static_assert message.
4. Install-tree audit: zero hwloc files under the install prefix.
5. Package-config audit: `speedgun-ng*.cmake` files contain no hwloc
   references.
6. Symbol audit on shared build: no `hwloc_*` in exported symbols.
7. Downstream consumer test passes with no system hwloc
   present.
8. Lint, cppcheck, coverage, and sanitizer configurations explicitly
   exclude the vendored path (sanitizer policy per the recorded
   decision).
9. The autotools ingestion module exists as a documented, reusable
   `cmake/` module with the contract above; the hwloc call site uses
   it and contains no bespoke build logic.
10. Platform-specific logic is confined to per-platform blocks in
    the ingestion module, reviewable as structure: a future Windows
    on-ramp plugs in as additive code, requiring no change to
    shared module logic.

## Out of scope

- Any public speedgun-ng API that exposes topology, cpusets, or
  hwloc types. hwloc crosses zero API boundaries in this feature.
- The feature work that will eventually use hwloc internally
  (thread pinning, topology reporting). That is a separate spec and
  should list this one as a prerequisite.
- FetchContent/registry distribution of hwloc; we vendor or we do
  nothing.

## Open questions for clarify/plan

1. Submodule path name (`external/` vs `third_party/`): pick one,
   never mix.
2. Sanitizer instrumentation policy for the vendored C archive in
   the sanitize job.
3. Build subset: adopt `--enable-embedded-mode`, and mirror vcpkg's
   disable list (libxml2, opencl, cairo, cuda, libudev, levelzero,
   nvml, rsmi, pci)?
4. Symbol prefix: adopt it whenever a consumer process can carry a
   system hwloc alongside ours; the autotools path applies it with
   `HWLOC_SET_SYMBOL_PREFIX`.
