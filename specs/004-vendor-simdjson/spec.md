# Feature Specification: Vendor simdjson as a Private, Pinned Submodule

**Feature Branch**: `004-vendor-simdjson`

**Created**: 2026-09-23

**Status**: Draft

**Input**: User description: "similarly how we imported hwloc as an external repository, we need a fast json parser: https://github.com/simdjson/simdjson will be our json parser of choice. Let's get this integrated as an internal library with similar characteristics to hwloc's import." This feature delivers dependency infrastructure: simdjson enters the tree as a private, pinned, statically linked internal dependency of the speedgun-ng library, carrying the same characteristics the hwloc import established (specs/003-vendor-hwloc). It defines no JSON API built on top of simdjson.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The project builds with a pinned, vendored simdjson (Priority: P1)

A developer clones speedgun-ng with submodules and builds it. simdjson compiles from the vendored submodule into a static archive and links into the speedgun-ng library as a private dependency. No path through the build selects a different simdjson, and replacing the pinned sources with any other revision breaks the build at compile time with a readable version assertion.

**Why this priority**: The vendored, pinned dependency is the foundation every other characteristic sits on. Without an ingested, version-locked simdjson there is nothing to hide, nothing to prove invisible, and nothing to re-pin.

**Independent Test**: Clone with submodules, build on Linux and macOS, inspect `git submodule status`, grep the build files for system-dependency queries, and swap the submodule tag to observe the version assertion fire. Every check runs against a build that satisfies this story alone.

**Acceptance Scenarios**:

1. **Given** a clean clone with submodules initialized, **When** a developer builds on Linux, **Then** the build succeeds, simdjson compiles from the submodule, and the speedgun-ng library links it statically.
2. **Given** the same clone, **When** a developer builds on macOS, **Then** the build succeeds.
3. **Given** `git submodule status`, **When** inspected, **Then** the recorded commit is `e153ffadd9ae29b00c90bedc76f65d25a993d2b5`, the commit named by tag `v4.6.11`.
4. **Given** the submodule checked out at any other revision, **When** the build runs, **Then** compilation fails with a readable version-assertion diagnostic naming the expected simdjson version.
5. **Given** a machine with a system simdjson installed, **When** the build runs, **Then** the build still compiles the submodule copy and never consults system package discovery.
6. **Given** a clone where the submodule directory is empty, **When** configuration runs, **Then** it aborts with a message naming the submodule init command, and it never falls back to a system simdjson.
7. **Given** the built speedgun-ng static archive, **When** audited with `nm`, **Then** simdjson objects are present, pulled in by the internal wrapper unit's reference to a simdjson symbol.

---

### User Story 2 - Consumers cannot observe simdjson (Priority: P1)

A downstream project installs speedgun-ng, configures a trivial consumer against it with `find_package(speedgun-ng)` on a machine with simdjson present, then builds and runs the consumer. Nothing a consumer can observe mentions simdjson: installed headers, package files, link interfaces, and exported symbols are all free of it, and the consumer build resolves zero simdjson.

**Why this priority**: Invisibility is the contract of this feature, the same contract the hwloc import carries. Once simdjson leaks into an installed artifact, removing it later becomes a breaking change, and a hidden dependency in a published library is a maintenance trap for every downstream user.

**Independent Test**: Install the library, audit the install tree, the package config files, and the shared-library symbol table, then build and run a consumer on a machine carrying simdjson and audit that the consumer build resolves zero simdjson. These checks run against any build that satisfies User Story 1.

**Acceptance Scenarios**:

1. **Given** an installed speedgun-ng, **When** the install prefix is audited, **Then** it contains zero simdjson headers, archives, shared objects, package files, or config files.
2. **Given** the installed package files, **When** audited, **Then** `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` contain zero simdjson references, and no link-interface entry of a shared speedgun-ng resolves to simdjson.
3. **Given** a shared-library build, **When** the dynamic symbol table is audited in CI, **Then** zero simdjson symbols are exported and the runtime dependency list of the installed shared library names no simdjson object.
4. **Given** a machine with simdjson present, the vendored submodule in-tree and a system copy optional, **When** a trivial consumer is configured, built, and run against the installed tree, **Then** all three steps exit 0, and the consumer's configure log and link command resolve zero simdjson.

---

### User Story 3 - Re-pinning simdjson is documented and auditable (Priority: P3)

A maintainer updates the pinned simdjson by checking out a new tag, committing the submodule pointer, and bumping the version assertion. One short documentation section states exactly these steps, and the pinned commit recorded in the spec makes each pin auditable.

**Why this priority**: Re-pinning is a rare maintenance operation. Its value is keeping a future update cheap and its history traceable.

**Independent Test**: Follow the documentation section step by step on a scratch clone against a newer tag and confirm the build stays green; then bump the tag without the version assertion and confirm the build fails.

**Acceptance Scenarios**:

1. **Given** the documentation section, **When** followed step by step, **Then** the pinned revision changes, the pointer commit lands, and the build stays green.
2. **Given** a submodule bump to any revision other than the pinned one, with the version assertion untouched, **When** the build runs, **Then** it fails with the version-assertion diagnostic.

---

### Edge Cases

- Clone without `--recurse-submodules` (empty submodule directory): configuration fails with a message naming the init command, with zero fallback to a system simdjson.
- A system simdjson present in any prefix: the build never selects it, verified by a grep audit showing no `find_package(simdjson` and no `pkg_check_modules(simdjson` anywhere in the build files.
- simdjson ships its own CMake package and install rules: the ingestion consumes it through `add_subdirectory` with simdjson's own install switched off, so simdjson installs nothing into `CMAKE_INSTALL_PREFIX` and adds nothing to the exported speedgun-ng target set.
- simdjson does not hide its symbols by default upstream: the vendored objects are compiled with hidden visibility (or absorbed into the speedgun-ng archive), so a shared speedgun-ng exports zero simdjson symbols.
- A consumer process loads a system simdjson in the same address space as speedgun-ng: hidden visibility plus static absorption removes name collision, and the symbol audit proves the vendored copy stays out of exports.
- simdjson's build options vary the artifact (tests, benchmarks, fuzzers, single-header, examples): developer mode is switched off so the built archive contents never depend on those extras.
- Sanitizer builds: the vendored archive is excluded from instrumentation across every Linux job, the same policy the hwloc import records, with silent inconsistency between jobs treated as a defect.
- Windows: nothing in this feature blocks a later Windows build. simdjson is a CMake project with upstream MSVC support, so the port needs no autotools bootstrap; this feature drives no Windows-specific work beyond staying unprecluded.
- Licensing: the vendored tree carries its own MIT and Apache-2.0 license files; they stay in-tree and ship with source distributions.

## Requirements *(mandatory)*

### Fixed decisions (settled by the brief and the hwloc precedent)

1. **Submodule, pinned.** simdjson enters as a git submodule at tag `v4.6.11`, commit `e153ffadd9ae29b00c90bedc76f65d25a993d2b5` (verified 2026-09-23 via `git ls-remote`: a lightweight tag naming the commit directly). No floating `GIT_TAG`, no branches, no shallow tracking.
2. **Never resolve simdjson from elsewhere.** The only simdjson that exists for this project is the pinned one.
3. **Static, internal linkage.** The archive is consumed by the speedgun-ng library target alone.
4. **Invisible to users.** Every surface a downstream `find_package(speedgun-ng)` consumer can observe stays free of simdjson.
5. **Platform policy.** Linux is the enforced gate. macOS must keep building for developers. Windows stays possible by design with zero priority, runners, or funding.

### Functional Requirements

Pinning and integrity:

- **FR-001**: The repository shall carry the simdjson sources as a git submodule at `external/simdjson`, outside `include/`, `source/`, and `test/`, recording submodule commit `e153ffadd9ae29b00c90bedc76f65d25a993d2b5` (tag `v4.6.11`).
- **FR-002**: When the build starts with the vendored directory empty, the configuration shall abort with a message naming the submodule init command.
- **FR-003**: When the internal wrapper translation unit compiles, a compile-time assertion shall verify the vendored simdjson reports exactly the pinned version 4.6.11 (no other revision passes, including a 4.6.x patch), and the diagnostic shall identify the expected version and the mismatch.
- **FR-004**: The build shall never consult a system simdjson: `find_package(simdjson)`, `pkg_check_modules(simdjson)`, and system fallback paths are prohibited anywhere in the build files.
- **FR-005**: The vendored simdjson license files (MIT and Apache-2.0) shall remain in-tree and ship with source distributions.
- **FR-006**: The documentation shall carry a re-pinning section: check out the new tag, commit the submodule pointer, bump the version assertion.

Build integration:

- **FR-007**: The build shall compile simdjson into a static archive and link it into the speedgun-ng library target with private usage requirements only; no PUBLIC dependency edge shall exist. The internal wrapper unit shall reference one simdjson symbol, so the link is proven at object level: `nm` on the built static speedgun-ng archive shows simdjson objects present.
- **FR-008**: When CI builds on Linux, every existing Linux job shall stay green with submodules fetched, and the same build shall succeed on macOS.
- **FR-009**: CI checkout steps shall initialize submodules, and lint, cppcheck, and coverage configurations shall exclude the vendored path. Vendored code is exempt from warning gates, clang-tidy, cppcheck, and coverage; speedgun-ng code that calls simdjson is held to all of them.
- **FR-010**: The build shall exclude the vendored archive from sanitizer instrumentation, the recorded policy, documented with its rationale and consistent across every Linux job.
- **FR-011**: The build shall consume simdjson through its own CMake via `add_subdirectory` of the submodule with `EXCLUDE_FROM_ALL`, with simdjson's developer mode and install switched off, so the vendored tree builds only the library archive and installs nothing.
- **FR-012**: The build shall keep the submodule directory pristine: it configures and builds simdjson out of tree, and `git status` reports the submodule unmodified after a full build.

Non-exposure (the privacy contract):

- **FR-013**: No header under `include/` shall include any simdjson header; simdjson headers shall be includable only from `source/` translation units, preferably through one internal wrapper unit.
- **FR-014**: The output of `cmake --install` shall contain zero simdjson files: headers, archives, shared objects, CMake package files, and config files.
- **FR-015**: The installed `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` shall contain zero references to any simdjson target, path, or discovery call; a shared speedgun-ng shall carry no link-interface entry resolving to simdjson.
- **FR-016**: On a shared-library build, the dynamic symbol table shall export zero simdjson symbols, audited in CI. The vendored objects shall compile with hidden visibility (or be absorbed into the speedgun-ng archive) so upstream default symbol export cannot leak simdjson.
- **FR-017**: A downstream consumer test shall configure, build, and run a trivial consumer against the installed speedgun-ng on a machine with simdjson present, and the consumer build shall resolve zero simdjson through speedgun-ng; it is the authoritative proof of the privacy contract.

### Key Entities

- **Vendored submodule**: the simdjson sources carried in-tree; attributes: vendored path `external/simdjson`, release tag `v4.6.11`, pinned commit `e153ffadd9ae29b00c90bedc76f65d25a993d2b5`, licenses MIT and Apache-2.0.
- **Imported static target**: the build-system handle through which the speedgun-ng library links simdjson privately.
- **Internal wrapper unit**: the single translation unit where simdjson headers enter the build, where the version assertion lives, and where the linkage-proving simdjson symbol reference sits.
- **Privacy contract**: the set of consumer-observable surfaces that must remain simdjson-free: installed headers, package config files, target link interfaces, exported symbols, and include paths.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A clean clone with submodule init builds with every existing Linux CI job green, and the same clone builds successfully on macOS.
- **SC-002**: The install-tree audit finds 0 files matching simdjson under the install prefix.
- **SC-003**: The package-config audit finds 0 simdjson references in the installed `speedgun-ng*.cmake` files.
- **SC-004**: The symbol audit finds 0 simdjson symbols in the exported dynamic symbols of a shared build, and its runtime dependency list names no simdjson object.
- **SC-005**: The downstream consumer test on a machine with simdjson present: configure, build, and run all exit 0, and a grep of the consumer's configure log and link command finds zero simdjson resolutions.
- **SC-006**: Flipping the submodule to any other release tag fails the build at compile time with a version-assertion message readable in one glance.
- **SC-007**: After a full build, `git status` reports the submodule directory unmodified.
- **SC-008**: The grep audit finds 0 occurrences of `find_package(simdjson` and `pkg_check_modules(simdjson` in the project's build files, the vendored `external/` tree excluded.
- **SC-009**: `nm` on the built speedgun-ng static archive lists simdjson objects pulled in by the wrapper unit's simdjson symbol reference, and the shared-build audit of SC-004 stays at 0 exported simdjson symbols.

## Assumptions

- **Vendored path**: `external/simdjson`, the root convention for every vendored dependency, matching `external/hwloc`; the tree never mixes `external/` with `third_party/`.
- **Sanitizer policy**: exclude the vendored archive from sanitizer instrumentation, the identical policy the hwloc import records (specs/003-vendor-hwloc). The vendored C++ sits outside our defect surface, and instrumented callers still get out-of-bounds detection on simdjson-allocated memory. Every Linux job applies this policy identically.
- **Ingestion mechanism**: simdjson is a native CMake library. It is consumed through `add_subdirectory` with `EXCLUDE_FROM_ALL` plus simdjson's own `SIMDJSON_INSTALL=OFF` and `SIMDJSON_DEVELOPER_MODE=OFF`, so the `ImportAutotoolsSubmodule` module (written for the autotools hwloc tree) stays unused here. No autotools bootstrap, no host autoconf/automake/libtool requirement is introduced.
- **Symbol collision**: simdjson carries no upstream symbol-prefix mechanism equivalent to `HWLOC_SET_SYMBOL_PREFIX`. Collision avoidance rests on hidden visibility for the vendored objects plus static absorption into the speedgun-ng archive; the plan records the concrete mechanism.
- **Constitution dependency clause**: simdjson enters as a build-time, statically linked, internal dependency. The installed speedgun-ng gains zero external runtime dependencies (SC-004 proves it), satisfying the documented-justification requirement for dependencies with this record: the library needs JSON parsing internally for future serialization work, the vendored copy guarantees identical behavior on every user machine, and invisibility keeps the public dependency count at zero.
- **Pinned commit provenance**: `v4.6.11` verified 2026-09-23 via `git ls-remote` against `simdjson/simdjson`; it is a lightweight tag naming commit `e153ffadd9ae29b00c90bedc76f65d25a993d2b5`. Master has moved on and stays untracked.
- **macOS verification**: developer-local. The CI matrix carries no macOS runner today; the `ci-macos` preset exists and must stay usable.
- **Scope boundaries**: excluded work: any public speedgun-ng API exposing JSON values, parsers, or simdjson types (simdjson crosses zero API boundaries here); the serialization work that will consume simdjson internally (a separate spec listing this one as prerequisite); FetchContent or registry distribution of simdjson (vendoring or nothing); all Windows build work beyond staying unprecluded.
