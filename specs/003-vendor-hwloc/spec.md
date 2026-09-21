# Feature Specification: Vendor hwloc as a Private, Pinned Submodule

**Feature Branch**: `003-vendor-hwloc`

**Created**: 2026-09-19

**Status**: Draft

**Input**: User description: "read hwloc.md as your prompt ulw". The feature brief is `hwloc.md` at the repository root: import hwloc 2.14.0 as a git submodule, build it as part of the speedgun-ng build, and link it into the speedgun-ng library as a strictly internal dependency. This feature delivers dependency infrastructure. It defines no topology API built on top of hwloc.

## Clarifications

### Session 2026-09-20

- Q: Since this feature ships no hwloc-calling functionality, what is the binary-observable proof that hwloc is linked into the speedgun-ng library? → A: The internal wrapper unit calls one hwloc function (`hwloc_get_api_version()`); `nm` on the built speedgun-ng static archive shows hwloc objects pulled in.
- Q: In the `ci-sanitize` job, should the vendored hwloc C archive be excluded from sanitizer instrumentation or compiled with it? → A: Exclude the vendored archive from sanitizer instrumentation; every Linux job applies the policy identically.
- Q: Should the build apply hwloc's symbol prefix (`HWLOC_SET_SYMBOL_PREFIX`) to the embedded copy? → A: Adopt the prefix.
- Q: What mechanism gives the consumer test its "machine image with no system hwloc"? → A: No hwloc-free environment exists: hwloc is always present because the project vendors it as a submodule, and system copies are common. The consumer test runs on a machine with hwloc present, and the consumer build resolves zero hwloc through speedgun-ng. The term `smoke test` is banned from project prose (constitution amendment 2.6.0); the canonical name is downstream consumer test.
- Q: Which vendored-path root does the hwloc submodule get? → A: `external/hwloc`, the root convention for every vendored dependency.
- Q: Should the compile-time version assertion accept only exactly 2.14.0, or any release in the 2.14 series? → A: Exactly 2.14.0; any other revision, including a 2.14.x patch release, fails the assertion.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The project builds with a pinned, vendored hwloc (Priority: P1)

A developer clones speedgun-ng with submodules and builds it. hwloc 2.14.0 compiles from the vendored submodule into a static archive and links into the speedgun-ng library as a private dependency. No path through the build can select a different hwloc, and replacing the pinned sources with any other revision breaks the build at compile time with a readable version assertion.

**Why this priority**: The vendored, pinned dependency is the foundation everything else in this feature sits on. Without an ingested, version-locked hwloc there is nothing to hide, nothing to reuse, and nothing to re-pin.

**Independent Test**: Clone with submodules, build on Linux and macOS, inspect `git submodule status`, grep the build files for system-dependency queries, and swap the submodule tag to observe the version assertion fire. Every check runs against a build that satisfies this story alone.

**Acceptance Scenarios**:

1. **Given** a clean clone with submodules initialized, **When** a developer builds on Linux, **Then** the build succeeds, hwloc compiles from the submodule, and the speedgun-ng library links it statically.
2. **Given** the same clone, **When** a developer builds on macOS, **Then** the build succeeds.
3. **Given** `git submodule status`, **When** inspected, **Then** the recorded commit is `b5660dff631a171a96a4b3abf5a170c9cf62d6ef`, the commit named by tag `hwloc-2.14.0`.
4. **Given** the submodule checked out at any other revision, **When** the build runs, **Then** compilation fails with a readable version-assertion diagnostic naming expected hwloc 2.14.0.
5. **Given** a machine with a system hwloc installed, **When** the build runs, **Then** the build still compiles the submodule copy and never consults system package discovery.
6. **Given** a clone where the submodule directory is empty, **When** configuration runs, **Then** it aborts with a message naming the submodule init command, and it never falls back to a system hwloc.
7. **Given** the built speedgun-ng static archive, **When** audited with `nm`, **Then** hwloc objects are present, pulled in by the internal wrapper unit's `hwloc_get_api_version()` reference.

---

### User Story 2 - Consumers cannot observe hwloc (Priority: P1)

A downstream project installs speedgun-ng, configures a trivial consumer against it with `find_package(speedgun-ng)` on a machine with hwloc present, then builds and runs the consumer. Nothing a consumer can observe mentions hwloc: installed headers, package files, link interfaces, and exported symbols are all free of it, and the consumer build resolves zero hwloc.

**Why this priority**: Invisibility is the contract of this feature. Once hwloc leaks into an installed artifact, removing it later becomes a breaking change, and a hidden dependency in a published library is a maintenance trap for every downstream user.

**Independent Test**: Install the library, audit the install tree, the package config files, and the shared-library symbol table, then build and run a consumer on a machine carrying hwloc and audit that the consumer build resolves zero hwloc. These checks run against any build that satisfies User Story 1.

**Acceptance Scenarios**:

1. **Given** an installed speedgun-ng, **When** the install prefix is audited, **Then** it contains zero hwloc headers, archives, shared objects, package files, or config files.
2. **Given** the installed package files, **When** audited, **Then** `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` contain zero hwloc references, and no link-interface entry of a shared speedgun-ng resolves to hwloc.
3. **Given** a shared-library build, **When** the dynamic symbol table is audited in CI, **Then** zero hwloc symbols are exported and the runtime dependency list of the installed shared library names no hwloc object.
4. **Given** a machine with hwloc present, the vendored submodule in-tree and a system copy optional, **When** a trivial consumer is configured, built, and run against the installed tree, **Then** all three steps exit 0, and the consumer's configure log and link command resolve zero hwloc.

---

### User Story 3 - The next autotools dependency reuses the ingestion capability (Priority: P2)

A maintainer vendoring a second autotools project hands a documented CMake module the vendored path, the configure invocation and arguments, the expected archive, and an imported target name. The module produces a ready-to-link imported static target with correct build ordering. The submodule directory stays pristine under `git status` after builds; a compiler, build-type, sanitizer, or configure-argument change forces a rebuild; a missing host toolchain fails at configure time naming the packages to install.

**Why this priority**: This feature establishes the sanctioned precedent for vendoring, and the brief delivers the reusable module as a first-class deliverable. hwloc alone still ships if generality is proven later, so this ranks below ingestion and the privacy contract.

**Independent Test**: Review the module against its input and guarantee contract, verify the hwloc call site passes inputs and uses the resulting target and contains no build commands of its own, and verify platform-specific code is confined to per-platform blocks. The structure review plus the build behavior from User Story 1 covers it.

**Acceptance Scenarios**:

1. **Given** the module contract, **When** the hwloc call site is reviewed, **Then** it supplies inputs and links the imported target, and it contains zero bespoke build commands.
2. **Given** a full build, **When** `git status` runs inside the submodule directory, **Then** it reports the directory unmodified.
3. **Given** a changed compiler version, build type, sanitizer flags, or configure arguments, **When** the project is rebuilt, **Then** hwloc rebuilds as well, visibly in the build log, and a stale archive never survives.
4. **Given** a host without autoconf, automake, libtool, or make, **When** configuration runs, **Then** it fails early with a message naming the packages to install, with Homebrew names surfaced for macOS.
5. **Given** the module source, **When** reviewed, **Then** platform-specific logic lives in per-platform blocks, so a future Windows port plugs in as additive code requiring no change to shared module logic.

---

### User Story 4 - Re-pinning hwloc is documented and auditable (Priority: P3)

A maintainer updates the pinned hwloc by checking out a new tag, committing the submodule pointer, and bumping the version assertion. One short documentation section states exactly these steps, and the pinned commit recorded in the spec makes each pin auditable.

**Why this priority**: Re-pinning is a rare maintenance operation. Its value is keeping a future update cheap and its history traceable.

**Independent Test**: Follow the documentation section step by step on a scratch clone against a newer tag and confirm the build stays green; then bump the tag without the version assertion and confirm the build fails.

**Acceptance Scenarios**:

1. **Given** the documentation section, **When** followed step by step, **Then** the pinned revision changes, the pointer commit lands, and the build stays green.
2. **Given** a submodule bump to any revision other than the pinned 2.14.0, with the version assertion untouched, **When** the build runs, **Then** it fails with the version-assertion diagnostic.

---

### Edge Cases

- Clone without `--recurse-submodules` (empty submodule directory): configuration fails with a message naming the init command, with zero fallback to a system hwloc.
- A system hwloc present in any prefix: the build never selects it, verified by a grep audit showing no `find_package(hwloc` and no `pkg_check_modules(hwloc` anywhere in the build files.
- Autotools generation writes into the source tree it is handed: the ingestion bootstraps in a copy under the build tree, so the submodule worktree stays clean under `git status`.
- Ninja generator ordering: the static archive is declared a build byproduct so the link step orders correctly.
- A configure step rerunning on every build wastes time: the ingestion re-runs configure only when its inputs change, per `CONFIGURE_HANDLED_BY_BUILD` semantics (CMake 3.20+).
- Host packages varying the archive (libxml2, plugins, and the other optional features): all optional features are disabled, so the built archive contents never depend on what the host happens to carry.
- A consumer process loads a system hwloc in the same address space as speedgun-ng: the applied symbol prefix removes name collision, and the symbol audit proves hidden visibility keeps hwloc out of exports.
- Sanitizer builds: the vendored C archive is excluded from instrumentation across every Linux job, with silent inconsistency between jobs treated as a defect.
- Windows: nothing in this feature blocks a later Windows port. Platform specifics stay confined to per-platform blocks; the upstream `contrib/windows-cmake/` wrapper (present at the `hwloc-2.14.0` tag) stands as the documented on-ramp. This feature installs no MSYS, drives no autotools on Windows, and chases no MSVC issues.
- Licensing: the vendored tree carries its own BSD 3-Clause license (file `COPYING`); it stays in-tree and ships with source distributions.

## Requirements *(mandatory)*

### Fixed decisions (settled by the brief)

1. **Submodule, pinned.** hwloc enters as a git submodule at tag `hwloc-2.14.0`, commit `b5660dff631a171a96a4b3abf5a170c9cf62d6ef` (verified 2026-09-19 via `git ls-remote`: a lightweight tag naming the commit directly). No floating `GIT_TAG`, no branches, no shallow tracking.
2. **Never resolve hwloc from elsewhere.** The only hwloc that exists for this project is the pinned one.
3. **Static, internal linkage.** The archive is consumed by the speedgun-ng library target alone.
4. **Invisible to users.** Every surface a downstream `find_package(speedgun-ng)` consumer can observe stays free of hwloc.
5. **Platform policy.** Linux is the enforced gate. macOS must keep building for developers. Windows stays possible by design with zero priority, runners, or funding.

### Functional Requirements

Pinning and integrity:

- **FR-001**: The repository shall carry the hwloc sources as a git submodule at `external/hwloc`, outside `include/`, `source/`, and `test/`, recording submodule commit `b5660dff631a171a96a4b3abf5a170c9cf62d6ef` (tag `hwloc-2.14.0`).
- **FR-002**: When the build starts with the vendored directory empty, the configuration shall abort with a message naming the submodule init command.
- **FR-003**: When the internal wrapper translation unit compiles, a compile-time assertion shall verify the vendored hwloc reports exactly version 2.14.0 (no other revision passes, including a 2.14.x patch), and the diagnostic shall identify the expected version and the mismatch.
- **FR-004**: The build shall never consult a system hwloc: `find_package(hwloc)`, `pkg_check_modules(hwloc)`, and system fallback paths are prohibited anywhere in the build files.
- **FR-005**: The vendored hwloc `COPYING` license shall remain in-tree and ship with source distributions.
- **FR-006**: The documentation shall carry a re-pinning section: check out the new tag, commit the submodule pointer, bump the version assertion.

Build integration:

- **FR-007**: The build shall compile hwloc into a static archive and link it into the speedgun-ng library target with private usage requirements only; no PUBLIC dependency edge shall exist. The internal wrapper unit shall reference `hwloc_get_api_version()`, so the link is proven at object level: `nm` on the built static speedgun-ng archive shows hwloc objects present.
- **FR-008**: When CI builds on Linux, every existing Linux job shall stay green with submodules fetched, and the same build shall succeed on macOS.
- **FR-009**: CI checkout steps shall initialize submodules, and lint, cppcheck, and coverage configurations shall exclude the vendored path. Vendored code is exempt from warning gates, clang-tidy, cppcheck, and coverage; speedgun-ng code that calls hwloc is held to all of them.
- **FR-010**: The build shall exclude the vendored archive from sanitizer instrumentation, the recorded policy, documented with its rationale and consistent across every Linux job.

Autotools ingestion capability:

- **FR-011**: The feature shall deliver a documented, reusable CMake module (final name `cmake/ImportAutotoolsSubmodule.cmake`) that ingests an autotools submodule; the hwloc call site shall supply inputs only, and contain no bespoke build logic.
- **FR-012**: The module inputs shall cover: vendored submodule path, configure invocation (`autogen.sh` then `configure`, or a pre-generated `configure`), configure arguments disabling optional features, expected output archive, and imported target name.
- **FR-013**: The module shall produce a first-class imported static-library target with correct build ordering for its consumer; the module owns the imported-target dependency problem.
- **FR-014**: The module shall bootstrap autotools generation in a copy of the sources under the build tree, keeping the submodule directory pristine under `git status`.
- **FR-015**: The module shall build out of tree, re-run configure only when its inputs change (`CONFIGURE_HANDLED_BY_BUILD`, CMake 3.20+), declare the archive with `BUILD_BYPRODUCTS` for Ninja ordering, and use update steps disconnected from the network.
- **FR-016**: The module shall invalidate its stamps on compiler, compiler version, build type, sanitizer flags, and configure-argument changes, forcing a rebuild; a stale archive shall never survive silently.
- **FR-017**: The module shall install hwloc into a private prefix under the build tree and propagate nothing into `CMAKE_INSTALL_PREFIX`.
- **FR-018**: When a required host tool is missing (autoconf, automake, libtool, make), configuration shall fail early with a message naming the packages to install.
- **FR-019**: Platform-specific logic shall live in per-platform blocks inside the module, so a future Windows port plugs in as additive code requiring no change to shared module logic.
- **FR-020**: The build shall disable hwloc optional features (at minimum: dlopen plugins, tools, documentation, tests, libxml2, opencl, cairo, cuda, libudev, levelzero, nvml, rsmi, pci), so archive contents never vary with host packages.

Non-exposure (the privacy contract):

- **FR-021**: No header under `include/` shall include any hwloc header; hwloc headers shall be includable only from `source/` translation units, preferably through one internal wrapper unit.
- **FR-022**: The output of `cmake --install` shall contain zero hwloc files: headers, archives, shared objects, CMake package files, and config files.
- **FR-023**: The installed `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` shall contain zero references to any hwloc target, path, or discovery call; a shared speedgun-ng shall carry no link-interface entry resolving to hwloc.
- **FR-024**: On a shared-library build, the dynamic symbol table shall export zero hwloc symbols, audited in CI.
- **FR-025**: The build shall apply hwloc's symbol prefix through the `HWLOC_SET_SYMBOL_PREFIX` mechanism, with its rationale recorded: the prefix removes symbol collision when a consumer process links speedgun-ng and a system hwloc together.
- **FR-026**: A downstream consumer test shall configure, build, and run a trivial consumer against the installed speedgun-ng on a machine with hwloc present, and the consumer build shall resolve zero hwloc through speedgun-ng; it is the authoritative proof of the privacy contract.

### Key Entities

- **Vendored submodule**: the hwloc sources carried in-tree; attributes: vendored path `external/hwloc`, release tag `hwloc-2.14.0`, pinned commit `b5660dff631a171a96a4b3abf5a170c9cf62d6ef`, license `COPYING` (BSD 3-Clause).
- **Autotools ingestion module**: the reusable build capability; inputs: vendored path, configure invocation, configure arguments, expected archive, imported target name; guarantees: build ordering, pristine submodule worktree, stamp invalidation, private install prefix, toolchain diagnostics, per-platform structure.
- **Imported static target**: the build-system handle through which the speedgun-ng library links hwloc privately.
- **Internal wrapper unit**: the single translation unit where hwloc headers enter the build, where the version assertion lives, and where the linkage-proving `hwloc_get_api_version()` reference sits.
- **Privacy contract**: the set of consumer-observable surfaces that must remain hwloc-free: installed headers, package config files, target link interfaces, exported symbols, and include paths.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A clean clone with submodule init builds with every existing Linux CI job green, and the same clone builds successfully on macOS.
- **SC-002**: The install-tree audit finds 0 files matching hwloc under the install prefix.
- **SC-003**: The package-config audit finds 0 hwloc references in the installed `speedgun-ng*.cmake` files.
- **SC-004**: The symbol audit finds 0 hwloc symbols in the exported dynamic symbols of a shared build, and its runtime dependency list names no hwloc object.
- **SC-005**: The downstream consumer test on a machine with hwloc present: configure, build, and run all exit 0, and a grep of the consumer's configure log and link command finds zero hwloc resolutions.
- **SC-006**: Flipping the submodule to any other release tag fails the build at compile time with a version-assertion message readable in one glance.
- **SC-007**: After a full build, `git status` reports the submodule directory unmodified.
- **SC-008**: A compiler, build-type, sanitizer, or configure-argument change forces a visible hwloc rebuild in the build log.
- **SC-009**: The grep audit finds 0 occurrences of `find_package(hwloc` and `pkg_check_modules(hwloc` in the project's build files, the vendored `external/` tree excluded.
- **SC-010**: The hwloc call site of the ingestion module contains 0 build commands: inputs and imported-target usage only.
- **SC-011**: `nm` on the built speedgun-ng static archive lists hwloc objects pulled in by the wrapper unit's `hwloc_get_api_version()` reference, and the shared-build audit of SC-004 stays at 0 exported hwloc symbols.

## Assumptions

- **Vendored path**: settled at clarification 2026-09-20: `external/hwloc`, the root convention for every vendored dependency; the tree never mixes `external/` with `third_party/`.
- **Sanitizer policy**: settled at clarification 2026-09-20: exclude the vendored C archive from sanitizer instrumentation. The vendored C code sits outside our defect surface, and instrumented callers still get out-of-bounds detection on hwloc-allocated memory. Every Linux job applies this policy identically.
- **Build subset**: adopt the upstream `--enable-embedded-mode` path (the convenience-archive embedding story: header install, documentation, tools, and tests switched off) plus the optional-feature disable list in FR-020. This keeps host packages from varying the artifact and keeps the archive minimal.
- **Symbol prefix**: settled at clarification 2026-09-20: adopt it (the `HWLOC_SET_SYMBOL_PREFIX` mechanism under autotools). A benchmarking framework can live in the same address space as MPI runtimes and other hwloc consumers, and Charm++ prefixes its embedded copy for exactly that collision risk.
- **Host toolchain**: Linux CI runners install autoconf, automake, and libtool as apt packages; macOS developers get them from Homebrew, and the missing-toolchain diagnostic names them.
- **Constitution dependency clause**: hwloc enters as a build-time, statically linked, internal dependency. The installed speedgun-ng gains zero external runtime dependencies (SC-004 proves it), satisfying the documented-justification requirement for dependencies with this record: the library needs hardware-topology discovery internally for future pinning work, the vendored copy guarantees identical behavior on every user machine, and invisibility keeps the public dependency count at zero.
- **Pinned commit provenance**: `hwloc-2.14.0` verified 2026-09-19 via `git ls-remote` against `open-mpi/hwloc`; it is a lightweight tag naming commit `b5660dff631a171a96a4b3abf5a170c9cf62d6ef`. Master has moved to 3.0.0 development and stays untracked.
- **macOS verification**: developer-local. The CI matrix carries no macOS runner today; the `ci-macos` preset exists and must stay usable.
- **Scope boundaries**: excluded work: any public speedgun-ng API exposing topology, cpusets, or hwloc types (hwloc crosses zero API boundaries here); the thread-pinning and topology-reporting work that will consume hwloc internally (a separate spec listing this one as prerequisite); FetchContent or registry distribution of hwloc (vendoring or nothing); all Windows build work beyond staying unprecluded.
