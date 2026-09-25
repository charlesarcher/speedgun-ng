# Feature Specification: Vendor HdrHistogram_c as a Private, Pinned Submodule

**Feature Branch**: `005-vendor-hdrhistogram`

**Created**: 2026-09-23

**Status**: Draft

**Input**: User description: "In the same way we imported hwloc and simdjson, I want to import another utility library we'll need (same private characteristics): https://github.com/hdrhistogram/HdrHistogram_c. This will be bundled as part of our speedgun distribution, but not exposed externally, an internal library. We'll use it to analyze data." This feature delivers dependency infrastructure: HdrHistogram_c enters the tree as a private, pinned, statically linked internal dependency of the speedgun-ng library, carrying the same characteristics the hwloc and simdjson imports established (specs/003-vendor-hwloc, specs/004-vendor-simdjson). It defines no histogram or analysis API built on top of HdrHistogram_c.

## Clarifications

### Session 2026-09-23

- Q: What pattern do the leak audits (SC-002 through SC-005, SC-008) match against installed files, package-config contents, exported symbols, and consumer logs? → A: Match the case-insensitive pattern `hdr[-_]?histogram` across file names, install-tree paths, package-config file contents, consumer configure and build logs, and symbol dumps, plus an exported-symbol check for the `hdr_` prefix on a shared build. Upstream spells its artifacts `hdr_histogram`, so a repo-name-only search would pass vacuously over a real leak.
- Q: Should the ingestion disable HdrHistogram_c's logging component and dodge zlib, or keep it enabled and take zlib from the host? → A: Logging stays enabled: disabling a dependency's capability to dodge its requirement is prohibited for this and every vendored dependency. The zlib requirement is satisfied by a second vendored, pinned submodule: `https://github.com/madler/zlib`, tag `v1.3.2`, commit `da607da739fa6047df13e66a2af6b8bec7c2a498` (verified 2026-09-23 via `git ls-remote`; annotated tag peels to that commit). The build resolves zlib exclusively from that submodule, and the zero-external-runtime-dependency rule of the constitution stays intact.
- Q: How far does the "never disabled" directive reach into the shipped hwloc and simdjson ingestions? → A: At the dependency level, unconditionally: hwloc, simdjson, and HdrHistogram_c are always on. No preset, option, or build path may exclude any of the three from a speedgun-ng build for Linux or macOS. The ingestion-internal choices for the shipped three stay as landed: hwloc's embedded mode and optional-backend disable list (specs/003-vendor-hwloc), simdjson's static and no-install selections (specs/004-vendor-simdjson), and HdrHistogram_c's shared-library, test, and install-to-export switches per FR-011. The capability policy for future vendored packages is decided in those future specs, deferred here.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The project builds with a pinned, vendored HdrHistogram_c (Priority: P1)

A developer clones speedgun-ng with submodules and builds it. HdrHistogram_c compiles from the vendored submodule into a static archive and links into the speedgun-ng library as a private dependency. No path through the build selects a different HdrHistogram_c, and replacing the pinned sources with any other revision breaks the build at compile time with a readable version assertion.

**Why this priority**: The vendored, pinned dependency is the foundation every other characteristic sits on. Without an ingested, version-locked HdrHistogram_c there is nothing to hide, nothing to prove invisible, and nothing to re-pin.

**Independent Test**: Clone with submodules, build on Linux and macOS, inspect `git submodule status`, grep the build files for system-dependency queries, and swap the submodule tag to observe the version assertion fire. Every check runs against a build that satisfies this story alone.

**Acceptance Scenarios**:

1. **Given** a clean clone with submodules initialized, **When** a developer builds on Linux, **Then** the build succeeds, HdrHistogram_c compiles from the submodule, and the speedgun-ng library links it statically.
2. **Given** the same clone, **When** a developer builds on macOS, **Then** the build succeeds.
3. **Given** `git submodule status`, **When** inspected, **Then** the recorded commit is `18c7a324383dded1451d15621cd018b0048057d0`, the commit named by tag `0.11.10`.
4. **Given** the submodule checked out at any other revision, **When** the build runs, **Then** compilation fails with a readable version-assertion diagnostic naming the expected HdrHistogram_c version.
5. **Given** a machine with a system HdrHistogram_c installed, **When** the build runs, **Then** the build still compiles the submodule copy and never consults system package discovery.
6. **Given** a clone where either submodule directory is empty, **When** configuration runs, **Then** it aborts with a message naming the submodule init command, and it never falls back to a system copy of that dependency.
7. **Given** the built speedgun-ng static archive, **When** audited with `nm`, **Then** HdrHistogram_c objects are present, pulled in by the internal wrapper unit's reference to an HdrHistogram_c symbol.
8. **Given** a machine with a system zlib installed, **When** the build runs, **Then** HdrHistogram_c's zlib search resolves to the vendored pinned zlib, and no host zlib header, library, or package entry is consulted.

---

### User Story 2 - Consumers cannot observe HdrHistogram_c (Priority: P1)

A downstream project installs speedgun-ng, configures a trivial consumer against it with `find_package(speedgun-ng)` on a machine with HdrHistogram_c present, then builds and runs the consumer. Nothing a consumer can observe mentions HdrHistogram_c: installed headers, package files, link interfaces, and exported symbols are all free of it, and the consumer build resolves zero HdrHistogram_c.

**Why this priority**: Invisibility is the contract of this feature, the same contract the hwloc and simdjson imports carry. Once HdrHistogram_c leaks into an installed artifact, removing it later becomes a breaking change, and a hidden dependency in a published library is a maintenance trap for every downstream user.

**Independent Test**: Install the library, audit the install tree, the package config files, and the shared-library symbol table, then build and run a consumer on a machine carrying HdrHistogram_c and audit that the consumer build resolves zero HdrHistogram_c. These checks run against any build that satisfies User Story 1.

**Acceptance Scenarios**:

1. **Given** an installed speedgun-ng, **When** the install prefix is audited, **Then** it contains zero HdrHistogram_c headers, archives, shared objects, package files, config files, and pkg-config files.
2. **Given** the installed package files, **When** audited, **Then** `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` contain zero HdrHistogram_c references, and no link-interface entry of a shared speedgun-ng resolves to HdrHistogram_c.
3. **Given** a shared-library build, **When** the dynamic symbol table is audited in CI, **Then** zero HdrHistogram_c symbols are exported and the runtime dependency list of the installed shared library names no HdrHistogram_c object.
4. **Given** a machine with HdrHistogram_c present, the vendored submodule in-tree and a system copy optional, **When** a trivial consumer is configured, built, and run against the installed tree, **Then** all three steps exit 0, and the consumer's configure log and link command resolve zero HdrHistogram_c.

---

### User Story 3 - Re-pinning HdrHistogram_c is documented and auditable (Priority: P3)

A maintainer updates a pinned vendored dependency (HdrHistogram_c or zlib) by checking out a new tag, committing the submodule pointer, and bumping that dependency's version assertion. One short documentation section per dependency states exactly these steps, and the pinned commit recorded in the spec makes each pin auditable.

**Why this priority**: Re-pinning is a rare maintenance operation. Its value is keeping a future update cheap and its history traceable.

**Independent Test**: Follow the documentation section step by step on a scratch clone against a different release tag (newer or older; both pins are the newest upstream releases today) and confirm the build stays green; then bump the tag without the version assertion and confirm the build fails.

**Acceptance Scenarios**:

1. **Given** the documentation section, **When** followed step by step, **Then** the pinned revision changes, the pointer commit lands, and the build stays green.
2. **Given** a submodule bump to any revision other than the pinned one, with the version assertion untouched, **When** the build runs, **Then** it fails with the version-assertion diagnostic.

---

### Edge Cases

- Clone without `--recurse-submodules` (an empty submodule directory, either `external/hdrhistogram_c` or `external/zlib`): configuration fails with a message naming the init command, with zero fallback to any system copy.
- A system HdrHistogram_c present in any prefix: the build never selects it, verified by the FR-019 grep audit showing no `find_package(` or `pkg_check_modules(` call naming it under any spelling of the FR-019 pattern anywhere in the build files.
- HdrHistogram_c's own CMake installs package config files and a pkg-config file unconditionally, and installs its public headers unconditionally, independent of its install-target options: the ingestion must neutralize every vendored install rule so nothing lands in `CMAKE_INSTALL_PREFIX`; the install-tree audit stays at zero.
- HdrHistogram_c's logging component is required by default, calls `find_package(ZLIB)` unconditionally, and fails configuration without zlib. The logging component stays enabled by maintainer directive (Clarifications 2026-09-23): the ingestion satisfies the requirement with the vendored, pinned zlib, so the build consults no host zlib, links no system libz, and the built archive never varies with host packages.
- A system zlib installed in any prefix: the ingestion resolves HdrHistogram_c's zlib search to the vendored copy only; the host copy is never selected, linked, or named in any installed artifact.
- The shared library target is built by default: the ingestion switches it off, so the build tree contains no shared HdrHistogram_c object at all; only the static archive is consumed.
- Tests, examples, and their test-driver wiring are built by default: the ingestion switches them off, so the built artifact contents never depend on those extras.
- Upstream runs its own thread-library discovery, and its own zlib discovery call: the thread module is CMake's built-in, and the zlib search resolves to the vendored copy inside the ingestion scope (FR-020). Neither consults a host package that varies the artifact; the dependency grep audit targets HdrHistogram_c discovery and host zlib resolution.
- HdrHistogram_c does not hide its symbols by default upstream and carries no symbol-prefix mechanism: the vendored objects are compiled with hidden visibility (or absorbed into the speedgun-ng archive), so a shared speedgun-ng exports zero HdrHistogram_c symbols.
- A consumer process loads a system HdrHistogram_c in the same address space as speedgun-ng: hidden visibility plus static absorption removes name collision, and the symbol audit proves the vendored copy stays out of exports.
- The version header is generated by configure-time file processing: the build produces it in the build tree, so a full build leaves the submodule worktree pristine under `git status`.
- Sanitizer builds: the vendored archives (HdrHistogram_c and zlib) are excluded from instrumentation across every Linux job, the same policy both prior imports record, with silent inconsistency between jobs treated as a defect.
- Windows: nothing in this feature blocks a later Windows build. HdrHistogram_c is a CMake project with upstream Windows code paths, and the vendored zlib ships its own CMakeLists.txt, so the port needs no autotools bootstrap; this feature drives no Windows-specific work beyond staying unprecluded.
- Licensing: the vendored trees carry their own license files and they stay in-tree and ship with source distributions: HdrHistogram_c's MIT `LICENSE.txt` and `COPYING.txt`, and zlib's `LICENSE` (the zlib license, BSD-compatible).

## Requirements *(mandatory)*

### Fixed decisions (settled by the brief, the hwloc/simdjson precedent, and the 2026-09-23 clarification session)

1. **Submodule, pinned.** HdrHistogram_c enters as a git submodule at tag `0.11.10`, commit `18c7a324383dded1451d15621cd018b0048057d0` (verified 2026-09-23 via `git ls-remote`: a lightweight tag naming the commit directly). No floating `GIT_TAG`, no branches, no shallow tracking.
2. **Never resolve HdrHistogram_c from elsewhere.** The only HdrHistogram_c that exists for this project is the pinned one.
3. **Static, internal linkage.** The archive is consumed by the speedgun-ng library target alone.
4. **Invisible to users.** Every surface a downstream `find_package(speedgun-ng)` consumer can observe stays free of HdrHistogram_c.
5. **Platform policy.** Linux is the enforced gate. macOS must keep building for developers. Windows stays possible by design with zero priority, runners, or funding.
6. **zlib vendored, logging enabled.** HdrHistogram_c's logging component stays enabled. Its zlib requirement is satisfied by a second vendored, pinned submodule (madler/zlib `v1.3.2`), and the build resolves zlib exclusively from that submodule. Disabling a dependency capability to dodge its requirement is prohibited (Clarifications 2026-09-23).
7. **Vendored dependencies always on.** hwloc, simdjson, and HdrHistogram_c are unconditional components of the speedgun-ng library on Linux and macOS. No preset, cache option, or build path may exclude any of the three (Clarifications 2026-09-23).

### Functional Requirements

Pinning and integrity:

- **FR-001**: The repository shall carry the HdrHistogram_c sources as a git submodule at `external/hdrhistogram_c`, outside `include/`, `source/`, and `test/`, recording submodule commit `18c7a324383dded1451d15621cd018b0048057d0` (tag `0.11.10`).
- **FR-001a**: The repository shall carry the zlib sources as a git submodule at `external/zlib`, outside `include/`, `source/`, and `test/`, recording submodule commit `da607da739fa6047df13e66a2af6b8bec7c2a498` (annotated tag `v1.3.2` on `https://github.com/madler/zlib`), the pinned provider of the logging component's zlib requirement (Fixed decision 6, FR-020).
- **FR-002**: When the build starts with either vendored directory empty (HdrHistogram_c or zlib), the configuration shall abort with a message naming the submodule init command.
- **FR-003**: When the internal wrapper translation unit compiles, a compile-time assertion shall verify the vendored HdrHistogram_c reports exactly the pinned version 0.11.10 through its version macro (no other revision passes), and the diagnostic shall identify the expected version and the mismatch. A second compile-time assertion in the zlib wrapper unit shall verify the vendored zlib reports exactly 1.3.2 through the `ZLIB_VERSION` macro, with the same diagnostic rule.
- **FR-004**: The build shall never consult a system HdrHistogram_c: `find_package(hdr_histogram)`, `pkg_check_modules(hdr_histogram)`, and equivalent calls under any FR-019 spelling of the name, and all system fallback paths, are prohibited anywhere in the build files.
- **FR-005**: The vendored license files shall remain in-tree and ship with source distributions: HdrHistogram_c's `LICENSE.txt` and `COPYING.txt`, and zlib's `LICENSE`.
- **FR-006**: The documentation shall carry one re-pinning section per vendored dependency (HdrHistogram_c and zlib): check out the new tag, commit the submodule pointer, bump that dependency's version assertion.

Build integration:

- **FR-007**: The build shall compile HdrHistogram_c into a static archive and link it into the speedgun-ng library target with private usage requirements only; no PUBLIC dependency edge shall exist. The internal wrapper unit shall reference one HdrHistogram_c symbol, so the link is proven at object level: `nm` on the built static speedgun-ng archive shows HdrHistogram_c objects present.
- **FR-008**: When CI builds on Linux, every existing Linux job shall stay green with submodules fetched, and the same build shall succeed on macOS.
- **FR-009**: CI checkout steps shall initialize submodules, and lint, cppcheck, spell-check, and coverage configurations shall exclude the vendored paths (`external/hdrhistogram_c`, `external/zlib`). Vendored code is exempt from warning gates, clang-tidy, cppcheck, and coverage; speedgun-ng code that calls HdrHistogram_c or zlib is held to all of them.
- **FR-010**: The build shall exclude the vendored archives (HdrHistogram_c and zlib) from sanitizer instrumentation, the recorded policy, documented with its rationale and consistent across every Linux job.
- **FR-011**: The build shall consume HdrHistogram_c through its own CMake via `add_subdirectory` of the submodule with `EXCLUDE_FROM_ALL`, with HdrHistogram_c's shared-library build, test and example build, and install-to-export options switched off. The logging component stays enabled: its zlib requirement resolves to the vendored pinned zlib (FR-020), so the vendored tree builds the static library archive including the logging translation units, and consults no host zlib.
- **FR-012**: The ingestion shall suppress every remaining vendored install rule, covering HdrHistogram_c's header, package-config, and pkg-config rules that upstream applies unconditionally and the install rules the vendored zlib build carries: no vendored tree installs anything into `CMAKE_INSTALL_PREFIX`, and nothing is added to the exported speedgun-ng target set.
- **FR-013**: The build shall keep both submodule directories pristine: it configures and builds HdrHistogram_c and zlib out of tree, and `git status` reports both submodules unmodified after a full build.
- **FR-013a**: The build shall carry no option, preset, or cache variable that excludes hwloc, simdjson, HdrHistogram_c, or the companion zlib from the speedgun-ng library on Linux or macOS: all four link unconditionally (Fixed decision 7, Clarifications 2026-09-23).

Non-exposure (the privacy contract):

- **FR-014**: No header under `include/` shall include any HdrHistogram_c or zlib header; their headers shall be includable only from `source/` translation units, preferably through the one internal wrapper unit.
- **FR-015**: The output of `cmake --install` shall contain zero HdrHistogram_c files: headers, archives, shared objects, CMake package files, config files, and pkg-config files.
- **FR-016**: The installed `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake` shall contain zero references to any HdrHistogram_c target, path, or discovery call; a shared speedgun-ng shall carry no link-interface entry resolving to HdrHistogram_c.
- **FR-017**: On a shared-library build, the dynamic symbol table shall export zero HdrHistogram_c symbols, audited in CI. The vendored objects shall compile with hidden visibility (or be absorbed into the speedgun-ng archive) so upstream default symbol export cannot leak HdrHistogram_c.
- **FR-018**: A downstream consumer test shall configure, build, and run a trivial consumer against the installed speedgun-ng on a machine with HdrHistogram_c present, and the consumer build shall resolve zero HdrHistogram_c through speedgun-ng; it is the authoritative proof of the privacy contract.
- **FR-019**: Every leak audit in this feature (FR-004, FR-015, FR-016, FR-017, FR-018; SC-002 through SC-005, SC-008) shall search with the case-insensitive pattern `hdr[-_]?histogram`, covering the underscore, hyphen, and glued spellings, against file names, install-tree paths, package-config file contents, consumer configure and build logs, and symbol dumps. The zlib surfaces (FR-020, FR-021, FR-022; SC-004, SC-010) shall be searched with the case-insensitive patterns `zlib` and `libz`. The shared-build symbol audit shall additionally flag any exported symbol whose name starts with `hdr_`. A search for the repository spelling alone (`hdrhistogram`) does not satisfy these audits: upstream names its artifacts `hdr_histogram`, and a real leak must trip the check.

Vendored zlib (the logging component's requirement):

- **FR-020**: The build shall resolve no zlib from the host. HdrHistogram_c's own CMake calls `find_package(ZLIB)` unconditionally; within the ingestion scope the zlib search shall resolve to the vendored pinned zlib (the concrete hint mechanism, such as a `ZLIB_ROOT` or the `ZLIB_INCLUDE_DIR`/`ZLIB_LIBRARY` cache variables, is recorded in the plan), a system zlib present in any prefix shall be selected zero times, linked zero times, and named in zero installed artifacts, and the ingestion shall build the vendored zlib ahead of HdrHistogram_c's configuration.
- **FR-021**: The vendored zlib shall link into the speedgun-ng library privately and statically, and its objects shall be absorbed into the speedgun-ng archive the same way HdrHistogram_c's are: the static archive resolves its zlib references in-archive, and the shared build's runtime dependency list names no libz object (SC-004).
- **FR-022**: The vendored zlib objects shall compile with hidden visibility, so no `inflate`, `deflate`, `compress`, or `uncompress` symbol appears in a shared build's exported symbol table. zlib's `Z_PREFIX` compile macro is available to the plan as the symbol-renaming control equivalent to hwloc's `sg_` prefix.

### Key Entities

- **Vendored HdrHistogram_c submodule**: the HdrHistogram_c sources carried in-tree; attributes: vendored path `external/hdrhistogram_c`, release tag `0.11.10`, pinned commit `18c7a324383dded1451d15621cd018b0048057d0`, license `LICENSE.txt` (MIT) with `COPYING.txt`.
- **Vendored zlib submodule**: the zlib sources carried in-tree as the logging component's zlib provider (Fixed decision 6); attributes: vendored path `external/zlib`, upstream `https://github.com/madler/zlib`, release tag `v1.3.2`, pinned commit `da607da739fa6047df13e66a2af6b8bec7c2a498`, license file `LICENSE` (the zlib license).
- **Imported static target**: the build-system handle through which the speedgun-ng library links HdrHistogram_c privately (upstream name `hdr_histogram_static`).
- **Internal wrapper units**: one translation unit per vendored dependency where its headers enter the build, where its version assertion lives, and where its linkage-proving symbol reference sits.
- **Privacy contract**: the set of consumer-observable surfaces that must remain HdrHistogram_c-free: installed headers, package config files, pkg-config files, target link interfaces, exported symbols, and include paths.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A clean clone with submodule init builds with every existing Linux CI job green, and the same clone builds successfully on macOS.
- **SC-002**: The install-tree audit finds 0 files whose path matches the FR-019 pattern `hdr[-_]?histogram` under the install prefix, and 0 files matching the zlib patterns `zlib` or `libz`.
- **SC-003**: The package-config audit finds 0 references matching the FR-019 pattern in the installed `speedgun-ng*.cmake` files, and 0 references matching `zlib` or `libz`.
- **SC-004**: The symbol audit finds 0 exported symbols matching the FR-019 pattern in a shared build, flags 0 exported symbols with the `hdr_` prefix, and its runtime dependency list names no HdrHistogram_c object and no libz object.
- **SC-005**: The downstream consumer test on a machine with HdrHistogram_c present: configure, build, and run all exit 0, and a grep of the consumer's configure log and link command with the FR-019 pattern finds zero HdrHistogram_c resolutions.
- **SC-006**: Flipping either vendored submodule to any other release tag fails the build at compile time with that dependency's version-assertion message, readable in one glance.
- **SC-007**: After a full build, `git status` reports both vendored submodule directories unmodified.
- **SC-008**: The grep audit finds 0 occurrences of a `find_package(` or `pkg_check_modules(` call naming HdrHistogram_c under any FR-019 spelling of the name (`hdr_histogram`, `hdr-histogram`, `hdrhistogram`) in the project's build files, the vendored `external/` tree excluded.
- **SC-009**: `nm` on the built speedgun-ng static archive lists HdrHistogram_c objects pulled in by the wrapper unit's HdrHistogram_c symbol reference, and the shared-build audit of SC-004 stays at 0 exported HdrHistogram_c symbols.
- **SC-010**: On a machine with a system zlib installed, the configured build records the vendored path for the zlib headers and library it resolves; the host copy resolves zero times; `ldd` on the installed shared library names no libz object; the static archive resolves its zlib references in-archive.
- **SC-011**: The shared-build symbol audit finds 0 exported symbols matching `zlib` or `libz`, including `inflate`, `deflate`, `compress`, and `uncompress`.

## Assumptions

- **Vendored path**: `external/hdrhistogram_c`, the root convention for every vendored dependency (`external/hwloc`, `external/simdjson`), the repository name in lower case; the tree never mixes `external/` with `third_party/`.
- **Sanitizer policy**: exclude the vendored archives (HdrHistogram_c and zlib) from sanitizer instrumentation, the identical policy both prior imports record (specs/003-vendor-hwloc, specs/004-vendor-simdjson). The vendored C sits outside our defect surface, and instrumented callers still get out-of-bounds detection on memory the vendored code allocates. Every Linux job applies this policy identically.
- **Ingestion mechanism**: HdrHistogram_c is a native CMake library. It is consumed through the established scope-isolated `add_subdirectory` bracket that the simdjson import established, with the shared-library, programs, and install-to-export options switched off and the logging component left enabled. The companion zlib at `external/zlib` enters through the same convention: upstream `v1.3.2` ships both a checked-in `configure` script and an in-tree `CMakeLists.txt`, so either the `add_subdirectory` bracket or the `ImportAutotoolsSubmodule` module (written for the autotools hwloc tree) fits; the plan records the choice. No new host build tool is introduced beyond what specs/003-vendor-hwloc already mandates.
- **Install-rule suppression**: upstream's own options gate only the install of its library targets; its header, package-config, and pkg-config install rules are unconditional. The ingestion must additionally neutralize those rules inside its scope, and the same for the vendored zlib build's own install rules; the plan records the concrete mechanism.
- **zlib**: the logging component stays enabled (Fixed decision 6, Clarifications 2026-09-23), and its zlib requirement is met by the vendored pinned submodule at `external/zlib`: the build resolves zlib exclusively from that submodule. With that in place the shared library's runtime dependency list gains no libz entry, and the built archive never varies with host packages.
- **Symbol collision**: HdrHistogram_c carries no upstream symbol-prefix mechanism equivalent to `HWLOC_SET_SYMBOL_PREFIX`. Collision avoidance rests on hidden visibility for the vendored objects plus static absorption into the speedgun-ng archive; the plan records the concrete mechanism.
- **Version gate**: the vendored version header defines the version as a string macro with the clean release value `0.11.10`, and the vendored `zlib.h` defines `ZLIB_VERSION` as the string macro `1.3.2`; the assertions compare each exactly, with no release-candidate ambiguity of the kind the hwloc pin recorded.
- **Constitution dependency clause**: HdrHistogram_c enters as a build-time, statically linked, internal dependency, and the vendored zlib enters the same way as its logging provider. The installed speedgun-ng gains zero external runtime dependencies (SC-004 and SC-010 prove it), satisfying the documented-justification requirement for dependencies with this record: the library needs histogram analysis internally for future data-analysis work, the vendored copies guarantee identical behavior on every user machine, and invisibility keeps the public dependency count at zero.
- **Pinned commit provenance**: `0.11.10` verified 2026-09-23 via `git ls-remote` against `hdrhistogram/HdrHistogram_c`; it is a lightweight tag naming commit `18c7a324383dded1451d15621cd018b0048057d0` directly. Upstream branches have moved on and stay untracked. `v1.3.2` verified the same day via `git ls-remote` against `madler/zlib`; it is an annotated tag peeling to commit `da607da739fa6047df13e66a2af6b8bec7c2a498`. Upstream development on the zlib master line stays untracked.
- **macOS verification**: developer-local. The CI matrix carries no macOS runner today; the `ci-macos` preset exists and must stay usable.
- **Scope boundaries**: excluded work: any public speedgun-ng API exposing histograms, percentiles, or HdrHistogram_c types (HdrHistogram_c crosses zero API boundaries here); the data-analysis work that will consume HdrHistogram_c internally (a separate spec listing this one as prerequisite); FetchContent or registry distribution of HdrHistogram_c (vendoring or nothing); the capability policy for future vendored packages, decided inside those specs (Clarifications 2026-09-23); all Windows build work beyond staying unprecluded.
