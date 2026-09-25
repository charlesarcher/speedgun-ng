# Feature Specification: Vendor yaml-cpp as a Private, Pinned Submodule

**Feature Branch**: `006-vendor-yaml-cpp`

**Created**: 2026-09-25

**Status**: Draft

**Input**: User description: "In the same way that we imported hwloc, simdjson, and hdrhistogram (private, internal library), we need to prepare to use yaml parser as well as a vendor built in library: https://github.com/jbeder/yaml-cpp. The role of this specification is to import this as a built in speedgun dependency." This feature delivers dependency infrastructure: yaml-cpp enters the tree as a private, pinned, statically linked internal dependency of the speedgun-ng library, carrying the same characteristics the hwloc, simdjson, and HdrHistogram_c imports established (specs/003-vendor-hwloc, specs/004-vendor-simdjson, specs/005-vendor-hdrhistogram). It defines no YAML parsing or configuration API built on top of yaml-cpp.

## Clarifications

### Session 2026-09-25

- Q: Should the build fail for every submodule revision other than the pinned commit, or only when the submodule declares a yaml-cpp version other than 0.9.0? (FR-003) → A: Version-string granularity: configuration fails when the version declared in the submodule's build files differs from 0.9.0; revisions that still declare 0.9.0 pass; no git metadata needed at configure time

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The project builds with a pinned, vendored yaml-cpp (Priority: P1)

A developer clones speedgun-ng with submodules and builds it. yaml-cpp compiles from the vendored submodule into a static archive and links into the speedgun-ng library as a private dependency. No path through the build selects a different yaml-cpp, and replacing the pinned sources with a revision that declares a yaml-cpp version other than 0.9.0 fails the build with a readable version diagnostic.

**Why this priority**: The vendored, pinned dependency is the foundation every other characteristic sits on. Without an ingested, version-locked yaml-cpp there is nothing to hide, nothing to prove invisible, and nothing to re-pin.

**Independent Test**: Clone with submodules, build on Linux and macOS, inspect `git submodule status`, grep the build files for system-dependency queries, and swap the submodule tag to observe the version diagnostic fire. Every check runs against a build that satisfies this story alone.

**Acceptance Scenarios**:

1. **Given** a clean clone with submodules initialized, **When** a developer builds on Linux, **Then** the build succeeds, yaml-cpp compiles from the submodule, and the speedgun-ng library links it statically.
2. **Given** the same clone, **When** a developer builds on macOS, **Then** the build succeeds.
3. **Given** `git submodule status`, **When** inspected, **Then** the recorded commit is `56e3bb550c91fd7005566f19c079cb7a503223cf`, the commit named by tag `yaml-cpp-0.9.0`.
4. **Given** the submodule checked out at any revision that declares a yaml-cpp version other than 0.9.0, **When** the build runs, **Then** it fails with a readable diagnostic naming the expected yaml-cpp version 0.9.0.
5. **Given** a machine with a system yaml-cpp installed, **When** the build runs, **Then** the build still compiles the submodule copy and never consults system package discovery.
6. **Given** a clone where `external/yaml-cpp` is empty, **When** configuration runs, **Then** it aborts with a message naming the submodule init command, and it never falls back to a system copy of the dependency.
7. **Given** the built speedgun-ng static archive, **When** audited with `nm`, **Then** yaml-cpp objects are present, pulled in by the internal wrapper unit's reference to a yaml-cpp symbol.

---

### User Story 2 - Consumers cannot observe yaml-cpp (Priority: P1)

A downstream project installs speedgun-ng, configures a trivial consumer against it with `find_package(speedgun-ng)` on a machine with yaml-cpp present, then builds and runs the consumer. Nothing a consumer can observe mentions yaml-cpp: installed headers, package files, link interfaces, and exported symbols are all free of it, and the consumer build resolves zero yaml-cpp.

**Why this priority**: Invisibility is the contract of this feature, the same contract the hwloc, simdjson, and HdrHistogram_c imports carry. Once yaml-cpp leaks into an installed artifact, removing it later becomes a breaking change, and a hidden dependency in a published library is a maintenance trap for every downstream user.

**Independent Test**: Install the library, audit the install tree, the package config files, and the shared-library symbol table, then build and run a consumer on a machine carrying yaml-cpp and audit that the consumer build resolves zero yaml-cpp. These checks run against any build that satisfies User Story 1.

**Acceptance Scenarios**:

1. **Given** an installed speedgun-ng, **When** the install prefix is audited, **Then** it contains zero yaml-cpp headers, archives, shared objects, package files, config files, and pkg-config files.
2. **Given** the installed package files, **When** audited, **Then** `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` contain zero yaml-cpp references, and no link-interface entry of a shared speedgun-ng resolves to yaml-cpp.
3. **Given** a shared-library build, **When** the dynamic symbol table is audited in CI, **Then** zero yaml-cpp symbols are exported and the runtime dependency list of the installed shared library names no yaml-cpp object.
4. **Given** a machine with yaml-cpp present, the vendored submodule in-tree and a system copy optional, **When** a trivial consumer is configured, built, and run against the installed tree, **Then** all three steps exit 0, and the consumer's configure log and link command resolve zero yaml-cpp.

---

### User Story 3 - Re-pinning yaml-cpp is documented and auditable (Priority: P3)

A maintainer updates the pinned yaml-cpp by checking out a new tag, committing the submodule pointer, and bumping the version assertion. One short documentation section states exactly these steps, and the pinned commit recorded in the spec makes the pin auditable.

**Why this priority**: Re-pinning is a rare maintenance operation. Its value is keeping a future update cheap and its history traceable.

**Independent Test**: Follow the documentation section step by step on a scratch clone against a different release tag (newer or older; the pin is the newest upstream release today) and confirm the build stays green; then bump the tag without bumping the version assertion and confirm the build fails.

**Acceptance Scenarios**:

1. **Given** the documentation section, **When** followed step by step, **Then** the pinned revision changes, the pointer commit lands, and the build stays green.
2. **Given** a submodule bump to any revision declaring a yaml-cpp version other than 0.9.0, with the version assertion untouched, **When** the build runs, **Then** it fails with the version diagnostic.

---

### Edge Cases

- Clone without `--recurse-submodules` (an empty `external/yaml-cpp` directory): configuration fails with a message naming the init command, with zero fallback to any system copy.
- A system yaml-cpp present in any prefix: the build never selects it, verified by the FR-018 grep audit showing no `find_package(` or `pkg_check_modules(` call naming it under any spelling of the FR-018 pattern anywhere in the build files.
- Upstream guards every install rule (target, headers, export set, package config files, pkg-config file) behind its `YAML_CPP_INSTALL` option: the ingestion switches it off, and the install-tree audit stays at zero regardless; any install rule that survives the switch is a defect this feature's audit catches.
- Upstream builds its library as shared or static from its own shared-library switch, defaulting to static but overridable from the outside: the ingestion forces the static form, so the build tree contains no shared yaml-cpp object at all.
- Upstream builds its test suite, its `util/` parse tools, and optional contrib integration sources by default in its own main-project context: the ingestion switches all three off, so the built artifact contents never depend on those extras. Contrib sources compile zero external requirement; switching them off dodges no dependency, it declines unused optional integration helpers.
- Upstream applies stricter warning flags to itself only when it is the main project: the speedgun build never makes the vendored tree the main project, and the vendored code stays exempt from the speedgun warning gates in any case.
- Upstream exposes its symbols through an export macro that resolves to nothing on a static, hidden-visibility build and to visible exports on a shared build: the vendored objects are compiled with hidden visibility (or absorbed into the speedgun-ng archive), so a shared speedgun-ng exports zero yaml-cpp symbols even though upstream defaults to exporting.
- A consumer process loads a system yaml-cpp in the same address space as speedgun-ng: hidden visibility plus static absorption removes name collision, and the symbol audit proves the vendored copy stays out of exports.
- Upstream's build uses source globbing and out-of-source generated files (`yaml-cpp.pc`, package config files) only in its own build tree: the ingestion configures and builds out of tree, so a full build leaves the submodule worktree pristine under `git status`.
- Upstream falls back to the C++11 language level only when no standard is set from above: the speedgun build sets its own standard from the top level, and the vendored library must compile under that setting; the plan records the outcome.
- Upstream carries no runtime dependency on any other library: unlike HdrHistogram_c, yaml-cpp needs no companion submodule, and the zero-external-runtime-dependency rule of the constitution is met without further vendoring.
- Sanitizer builds: the vendored archive is excluded from instrumentation, the same policy all three prior imports record, with silent inconsistency between jobs treated as a defect.
- Windows: nothing in this feature blocks a later Windows build. yaml-cpp is a portable CMake project with upstream Windows code paths, so the port needs no autotools bootstrap; this feature drives no Windows-specific work beyond staying unprecluded.
- Licensing: the vendored tree carries its own MIT `LICENSE` file and it stays in-tree and ships with source distributions.

## Requirements *(mandatory)*

### Fixed decisions (settled by the brief and the hwloc/simdjson/HdrHistogram_c precedent)

1. **Submodule, pinned.** yaml-cpp enters as a git submodule at tag `yaml-cpp-0.9.0`, commit `56e3bb550c91fd7005566f19c079cb7a503223cf` (verified 2026-09-25 via `git ls-remote`: a lightweight tag naming the commit directly; it is the newest upstream release). No floating `GIT_TAG`, no branches, no shallow tracking.
2. **Never resolve yaml-cpp from elsewhere.** The only yaml-cpp that exists for this project is the pinned one.
3. **Static, internal linkage.** The archive is consumed by the speedgun-ng library target alone.
4. **Invisible to users.** Every surface a downstream `find_package(speedgun-ng)` consumer can observe stays free of yaml-cpp.
5. **Platform policy.** Linux is the enforced gate. macOS must keep building for developers. Windows stays possible by design with zero priority, runners, or funding.
6. **Vendored dependencies always on.** yaml-cpp joins hwloc, simdjson, HdrHistogram_c, and zlib as an unconditional component of the speedgun-ng library on Linux and macOS. No preset, option, or build path may exclude any of the five.

### Functional Requirements

Pinning and integrity:

- **FR-001**: The repository shall carry the yaml-cpp sources as a git submodule at `external/yaml-cpp`, outside `include/`, `source/`, and `test/`, recording submodule commit `56e3bb550c91fd7005566f19c079cb7a503223cf` (tag `yaml-cpp-0.9.0`).
- **FR-002**: When the build starts with `external/yaml-cpp` empty, the configuration shall abort with a message naming the submodule init command.
- **FR-003**: When the build runs with the submodule checked out at a revision that declares a yaml-cpp version other than 0.9.0, it shall fail with a readable diagnostic naming the expected yaml-cpp version 0.9.0 and the mismatch; a revision that still declares 0.9.0 satisfies the check by design (Clarifications 2026-09-25). The check consults no git metadata, so configuration from a source archive without git history stays supported. The pinned tree exposes no version macro in any header (verified against tag `yaml-cpp-0.9.0`: no version header exists, unlike simdjson and HdrHistogram_c), so the version tripwire is a version-string check; the concrete mechanism (a configure-time check of the version declared in the submodule's own build files, or an equivalent compile-time marker) is recorded in the plan.
- **FR-004**: The build shall never consult a system yaml-cpp: `find_package(yaml-cpp)`, `pkg_check_modules(yaml-cpp)`, and equivalent calls under any FR-018 spelling of the name, and all system fallback paths, are prohibited anywhere in the build files.
- **FR-005**: The vendored license file (`LICENSE`, MIT) shall remain in-tree and ship with source distributions.
- **FR-006**: The documentation shall carry one re-pinning section for yaml-cpp: check out the new tag, commit the submodule pointer, bump the version assertion.

Build integration:

- **FR-007**: The build shall compile yaml-cpp into a static archive and link it into the speedgun-ng library target with private usage requirements only; no PUBLIC dependency edge shall exist. The internal wrapper unit shall reference one yaml-cpp symbol, so the link is proven at object level: `nm` on the built static speedgun-ng archive shows yaml-cpp objects present.
- **FR-008**: When CI builds on Linux, every existing Linux job shall stay green with submodules fetched, and the same build shall succeed on macOS.
- **FR-009**: CI checkout steps shall initialize submodules, and lint, cppcheck, spell-check, and coverage configurations shall exclude the vendored path `external/yaml-cpp`. Vendored code is exempt from warning gates, clang-tidy, cppcheck, and coverage; speedgun-ng code that calls yaml-cpp is held to all of them.
- **FR-010**: The build shall exclude the vendored archive from sanitizer instrumentation, the recorded policy, documented with its rationale and consistent across every Linux job.
- **FR-011**: The build shall consume yaml-cpp through its own CMake via `add_subdirectory` of the submodule with `EXCLUDE_FROM_ALL`, with yaml-cpp's shared-library form, test suite, `util/` tools, contrib sources, and install-to-export option switched off. The build shall contain no shared yaml-cpp object.
- **FR-012**: The ingestion shall suppress every remaining vendored install rule: no vendored tree installs anything into `CMAKE_INSTALL_PREFIX`, and nothing is added to the exported speedgun-ng target set.
- **FR-013**: The build shall keep the submodule directory pristine: it configures and builds yaml-cpp out of tree, and `git status` reports the submodule unmodified after a full build.
- **FR-013a**: The build shall carry no option, preset, or cache variable that excludes yaml-cpp (or hwloc, simdjson, HdrHistogram_c, zlib) from the speedgun-ng library on Linux or macOS: all five link unconditionally (Fixed decision 6).

Non-exposure (the privacy contract):

- **FR-014**: No header under `include/` shall include any yaml-cpp header; its headers shall be includable only from `source/` translation units, preferably through the one internal wrapper unit.
- **FR-015**: The output of `cmake --install` shall contain zero yaml-cpp files: headers, archives, shared objects, CMake package files, config files, and pkg-config files.
- **FR-016**: The installed `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` shall contain zero references to any yaml-cpp target, path, or discovery call; a shared speedgun-ng shall carry no link-interface entry resolving to yaml-cpp.
- **FR-017**: On a shared-library build, the dynamic symbol table shall export zero yaml-cpp symbols, audited in CI. The vendored objects shall compile with hidden visibility (or be absorbed into the speedgun-ng archive) so upstream's export macro cannot publish yaml-cpp symbols.
- **FR-018**: Every leak audit in this feature (FR-004, FR-015, FR-016, FR-017; SC-002 through SC-005, SC-008) shall search with the case-insensitive pattern `yaml[-_]?cpp`, covering the hyphen, underscore, and glued spellings, against file names, install-tree paths, package-config file contents, consumer configure and build logs, and symbol dumps. The shared-build symbol audit shall additionally flag any exported symbol whose mangled name contains the namespace encoding `4YAML` (the demangled `YAML::` namespace). A search for the repository spelling alone under other patterns does not satisfy these audits: upstream names its artifacts `yaml-cpp`, its package `yaml-cpp`, and its namespace `YAML`, and a real leak must trip the check.
- **FR-019**: A downstream consumer test shall configure, build, and run a trivial consumer against the installed speedgun-ng on a machine with yaml-cpp present, and the consumer build shall resolve zero yaml-cpp through speedgun-ng; it is the authoritative proof of the privacy contract.

### Key Entities

- **Vendored yaml-cpp submodule**: the yaml-cpp sources carried in-tree; attributes: vendored path `external/yaml-cpp`, upstream `https://github.com/jbeder/yaml-cpp`, release tag `yaml-cpp-0.9.0`, pinned commit `56e3bb550c91fd7005566f19c079cb7a503223cf`, license `LICENSE` (MIT).
- **Imported static target**: the build-system handle through which the speedgun-ng library links yaml-cpp privately (upstream name `yaml-cpp::yaml-cpp`).
- **Internal wrapper unit**: the one translation unit where yaml-cpp headers enter the build, where the version tripwire's compile-side half (if any) lives, and where the linkage-proving symbol reference sits.
- **Privacy contract**: the set of consumer-observable surfaces that must remain yaml-cpp-free: installed headers, package config files, pkg-config files, target link interfaces, exported symbols, and include paths.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A clean clone with submodule init builds with every existing Linux CI job green, and the same clone builds successfully on macOS.
- **SC-002**: The install-tree audit finds 0 files whose path matches the FR-018 pattern `yaml[-_]?cpp` under the install prefix.
- **SC-003**: The package-config audit finds 0 references matching the FR-018 pattern in the installed `speedgun-ng*.cmake` files.
- **SC-004**: The symbol audit finds 0 exported symbols matching the FR-018 pattern in a shared build, flags 0 exported symbols containing `4YAML`, and the runtime dependency list of the installed shared library names no yaml-cpp object.
- **SC-005**: The downstream consumer test on a machine with yaml-cpp present: configure, build, and run all exit 0, and a grep of the consumer's configure log and link command with the FR-018 pattern finds zero yaml-cpp resolutions.
- **SC-006**: Flipping the vendored submodule to any other release tag fails the build with the version diagnostic, readable in one glance.
- **SC-007**: After a full build, `git status` reports the vendored submodule directory unmodified.
- **SC-008**: The grep audit finds 0 occurrences of a `find_package(` or `pkg_check_modules(` call naming yaml-cpp under any FR-018 spelling (`yaml-cpp`, `yaml_cpp`, `yamlcpp`) in the project's build files, the vendored `external/` tree excluded.
- **SC-009**: `nm` on the built speedgun-ng static archive lists yaml-cpp objects pulled in by the wrapper unit's yaml-cpp symbol reference, and the shared-build audit of SC-004 stays at 0 exported yaml-cpp symbols.

## Assumptions

- **Vendored path**: `external/yaml-cpp`, the root convention for every vendored dependency (`external/hwloc`, `external/simdjson`, `external/hdrhistogram_c`, `external/zlib`), the repository name kept as upstream spells it; the tree never mixes `external/` with `third_party/`.
- **Wrapper unit path**: `source/yaml/yaml_gate.cpp`, following the one-wrapper-unit-per-dependency pattern (`source/simdjson/simdjson_gate.cpp`, `source/hdrhistogram/hdrhistogram_gate.cpp`); the plan records the final spelling.
- **Sanitizer policy**: exclude the vendored archive from sanitizer instrumentation, the identical policy all three prior imports record (specs/003-vendor-hwloc, specs/004-vendor-simdjson, specs/005-vendor-hdrhistogram). The vendored code sits outside our defect surface, and instrumented callers still get out-of-bounds detection on memory the vendored code allocates. Every Linux job applies this policy identically.
- **Ingestion mechanism**: yaml-cpp is a native CMake library with no sub-dependencies. It is consumed through the established scope-isolated `add_subdirectory` bracket that the simdjson import established, with the shared-library form, tests, tools, contrib sources, and install-to-export option switched off. No companion submodule is required (unlike HdrHistogram_c's zlib). No new host build tool is introduced beyond what the existing specs already mandate.
- **Install-rule suppression**: upstream guards all of its install rules behind its install option; switching it off is expected to be sufficient, and the install-tree audit (SC-002) proves it. If any rule escapes the guard, the ingestion neutralizes it inside the bracket; the plan records the concrete mechanism.
- **Version tripwire**: the pinned tree exposes no version macro in its headers, so the simdjson-style compile-time `static_assert` on a version macro is not available verbatim. A configure-time check of the version declared in the submodule's own build files is the expected mechanism; it satisfies FR-003's failure contract (build stops, readable diagnostic, pin and sources in lockstep at version-string granularity, per the 2026-09-25 clarification).
- **Symbol collision**: yaml-cpp carries no symbol-prefix mechanism. Collision avoidance rests on hidden visibility for the vendored objects plus static absorption into the speedgun-ng archive; the plan records the concrete mechanism, the same posture the HdrHistogram_c import took.
- **Constitution dependency clause**: yaml-cpp enters as a build-time, statically linked, internal dependency. The installed speedgun-ng gains zero external runtime dependencies (SC-004 proves it), satisfying the documented-justification requirement for dependencies with this record: the library needs YAML parsing internally for future configuration work, the vendored copy guarantees identical behavior on every user machine, and invisibility keeps the public dependency count at zero.
- **Pinned commit provenance**: `yaml-cpp-0.9.0` verified 2026-09-25 via `git ls-remote` against `jbeder/yaml-cpp`; it is a lightweight tag naming commit `56e3bb550c91fd7005566f19c079cb7a503223cf` directly, and it is the newest release tag upstream. Upstream development on the master line stays untracked.
- **macOS verification**: developer-local. The CI matrix carries no macOS runner today; the `ci-macos` preset exists and must stay usable.
- **Scope boundaries**: excluded work: any public speedgun-ng API exposing YAML nodes, documents, or parser types (yaml-cpp crosses zero API boundaries here); the configuration work that will consume yaml-cpp internally (a separate spec listing this one as prerequisite); FetchContent or registry distribution of yaml-cpp (vendoring or nothing); the capability policy for future vendored packages, decided inside those specs; all Windows build work beyond staying unprecluded.
