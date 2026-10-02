# Feature Specification: Linux as the Supported Platform

**Feature Branch**: `010-linux-only`

**Created**: 2026-10-02

**Status**: Draft

**Input**: User description: "For now, speedgun is linux only. Let's update docs, scripts, ci to leave it linux only." The owner clarified the intent: the design goal is Linux first, nothing in the public API or the code structure may prevent Windows or macOS later, and a future port may need pluggable implementation. For now the project is Linux only.

This feature delivers a governance and documentation change. It changes no code, no public API, and no runtime behaviour. Every per-platform branch in the tree stays exactly where it is, and the only surface that changes is the set of documents and presets that currently claim a support the project cannot demonstrate.

## Clarifications

### Session 2026-10-02

- Q: Does "Linux only" mean Windows and macOS support is withdrawn, or that it is unsupported today and may be added? → A: Linux is the sole supported and enforced platform today. Windows and macOS are out of scope and are added by a future specification. The goal is Linux first.
- Q: May the change delete the per-platform branches to make the policy real? → A: No. Every `if(WIN32)`, `if(APPLE)`, and `if(MSVC)` block in `CMakeLists.txt` and `cmake/` stays byte-identical, including the Windows on-ramp abort in `cmake/ImportAutotoolsSubmodule.cmake` naming upstream `contrib/windows-cmake/`. That message is the port's entry point and is the most valuable line in the file for a future port. Removing a branch forecloses the port; keeping it costs nothing on Linux, because the branch is never taken.
- Q: Should the change introduce a pluggable platform abstraction, since a future port may need one? → A: No. A port that may never land is speculative generality, and Principle X.2 forbids it. The requirement is that nothing forecloses a port, and an untouched per-platform branch already satisfies that. If a future specification needs an abstraction, that specification introduces it against real code.
- Q: The five merged vendor specs each state "Linux is the enforced gate. macOS must keep building for developers. Windows stays possible by design." Rewrite them? → A: No. They are dated, reviewed, merged records. The constitution amendment supersedes them on the platform question by name. Rewriting a merged spec makes it stop being a trustworthy record.
- Q: `CMakePresets.json` carries `ci-macos`, `ci-windows`, `ci-darwin`, `ci-win64`, `flags-appleclang`, `flags-msvc`, and `ci-multi-config`. Keep or remove? → A: Remove. Each one is a gate-shaped surface: a reader sees a CI preset and concludes the platform is supported. No runner has ever executed any of them, and the CI matrix runs Linux only. The amendment records that re-adding a preset is the whole on-ramp.
- Q: What happens to `specs/009-vendor-quill` T038, the open macOS verification task? → A: It retires by reference. The constitution's Open deferrals entry for the macOS gate is what holds T038 open, so closing that entry closes T038 without editing the merged 009 feature.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A developer reads one true statement of what is supported (Priority: P1)

A developer clones speedgun-ng, reads the README, and follows the build and verification instructions. Every command they are told to run works on their Linux machine. Nothing tells them a platform is supported that the project cannot build and test.

**Why this priority**: The current documents send a Linux developer to a preset that does not exist and an install line for a package manager they do not have. Both are false statements about the project, and a reader who trusts them loses time before discovering the truth.

**Independent Test**: Follow `README.md` and `AGENTS.md` end to end on Linux, running every command each names. Every command resolves. No document names macOS or Windows as a supported platform.

**Acceptance Scenarios**:

1. **Given** a fresh Linux clone, **When** a developer follows the README's build, install, and gate instructions, **Then** every command names a preset or script that exists in the tree, and every one succeeds.
2. **Given** the same clone, **When** a developer reads `AGENTS.md`'s build, test, and verify sections, **Then** every preset named is a Linux preset present in `CMakePresets.json`, and the CI matrix line names Linux jobs only.
3. **Given** any document in the tree outside `specs/` and `external/`, **When** a reader searches it for a macOS or Windows support claim, **Then** no document states that either platform is supported today.
4. **Given** a developer on macOS or Windows, **When** they read the README, **Then** they are told plainly that the platform is unsupported and that no build is offered for it.

---

### User Story 2 - The gate set and its governance match the policy (Priority: P1)

The project's constitution states the supported platform set, and the gate set matches that statement. A reader auditing the gate finds one platform named where the gate list is enumerated, and no preset offering a build no runner has ever run.

**Why this priority**: The constitution is the project's highest authority. A gate list naming two platforms the project cannot test is a weaker claim than the project makes elsewhere, and Principle VIII treats a finding as a defect at lint parity. Leaving the gate list alone after deciding the policy would make the governance document the last place still claiming macOS and Windows.

**Independent Test**: Audit the constitution's gate list, the Open deferrals block, and the version lineage table against the declared policy. Then audit `CMakePresets.json` for a preset serving an unsupported platform. Zero of each.

**Acceptance Scenarios**:

1. **Given** the constitution at this feature's head, **When** its hard gate list is read, **Then** it names Linux and no other platform, and its platform enumeration is Linux alone.
2. **Given** the constitution's Open deferrals block, **When** it is read, **Then** it carries no macOS-runner entry, because there is no macOS runner to defer.
3. **Given** the constitution's version lineage table, **When** it is read, **Then** it carries an entry for this amendment with its rationale, and the file's version is bumped with an updated `Last Amended` date.
4. **Given** `CMakePresets.json`, **When** it is searched for a preset naming macOS, Windows, Xcode, Visual Studio, AppleClang, or MSVC, **Then** zero entries are found.
5. **Given** the CI workflow, **When** its jobs are enumerated, **Then** every job runs on a Linux runner, and the count of jobs is unchanged from this feature's base commit.

---

### User Story 3 - A future port stays additive (Priority: P2)

A maintainer who wants macOS or Windows support can add it without touching anything this feature changed. The per-platform branches are still in the tree, and the on-ramp is documented.

**Why this priority**: The owner's stated design goal is Linux first, and Linux first means a port is additive. A rewrite is not what a port costs. This story is what makes the policy safe to adopt: a policy that quietly forecloses a port is a different decision from one that defers it.

**Independent Test**: Diff the tree against this feature's base commit and confirm zero lines changed inside any per-platform branch. Then read the amendment and confirm it names the on-ramp.

**Acceptance Scenarios**:

1. **Given** the tree at this feature's head and its base commit, **When** the two are diffed, **Then** no `if(WIN32)`, `if(APPLE)`, `if(MSVC)`, or `if(UNIX)` block changed, and no line in `cmake/ImportAutotoolsSubmodule.cmake` changed.
2. **Given** the amendment, **When** a maintainer reads its on-ramp statement, **Then** it names re-adding the platform preset as the first step and states that the per-platform blocks are untouched.
3. **Given** the removed presets, **When** the amendment is read, **Then** it lists each removed preset by name, so a future port restores them. Reinventing them gains a future port nothing.

---

### User Story 4 - The merged record stays intact and is superseded by name (Priority: P3)

The five merged vendor specs keep their original platform text, and a reader who finds that text learns from the constitution that it no longer holds.

**Why this priority**: Merged specs are records. A reader auditing what was decided, and when, needs those records to be what was decided at the time. Superseding them silently loses that, and rewriting them makes the record a fabrication.

**Independent Test**: Diff `specs/` against the base commit and confirm zero changes. Then read the amendment's supersession statement and confirm it names every spec it supersedes.

**Acceptance Scenarios**:

1. **Given** the tree at this feature's head and its base commit, **When** `specs/` is diffed, **Then** zero files changed.
2. **Given** the amendment's supersession statement, **When** it is read, **Then** it names every merged spec whose platform decision it supersedes.

---

### Edge Cases

- **A platform conditional that Linux never takes still carries cost**: an untaken branch is dead weight a reader must interpret, and Principle X.2 would ask whether it earns its place. The owner's answer is that the branch is the port's seam and the cost on Linux is zero, so it stays. This feature records that answer so a later reader does not re-open it.
- **`ci-multi-config` exists only for the removed generators**: it sets `CMAKE_CONFIGURATION_TYPES: Release` to shorten Xcode and Visual Studio builds. With those generators gone it is unreachable configuration, and it is removed with them. If a future port reinstates a multi-config generator, the preset returns with it.
- **The macOS deferral and the Windows suspension are historical records too**: amendments 2.8.0 and 2.7.0 stay in the file and in the lineage table. This feature adds a report that supersedes their effect, following the pattern the file already uses for a corrected clause. Removing their rows would erase the record that those gates were once suspended.
- **T038 in `specs/009-vendor-quill` has no macOS evidence and never will**: that task asks for a macOS build result on a platform the project no longer supports. It retires by reference through the Open deferrals entry, so the merged spec is untouched.
- **A developer with a macOS or Windows machine gets no build path at all**: that is the intended outcome. The README says so, so the absence is stated. Leaving a reader to discover it is the outcome this avoids. No unsupported-platform build is offered under a "best effort" label, because an unverified build invites a report the project cannot reproduce.
- **`docs/Doxyfile.in` reads `README.md`, `include/`, and `docs/pages/` only**: a README edit reaches the published documentation. The edit is additive prose, and the docs job builds on Linux, so the change is covered by the existing gate.
- **The spelling gate and the prose gate scan the edited files**: `.codespellrc` already exempts British spellings that appear in closed artifacts, so no exemption is added for the text this feature writes.

## Requirements *(mandatory)*

### Fixed decisions (settled by the brief and the Clarifications of 2026-10-02)

1. **Linux is the supported platform.** Linux is the sole platform the project builds, tests, and gates. macOS and Windows are out of scope and are added by a future specification.
2. **Nothing forecloses a port.** Every per-platform branch in the tree stays. No public API, no data model, and no build-system structure is changed to make the policy real. A future port is additive.
3. **No abstraction for an unlanded port.** Pluggable platform implementation is not introduced. Principle X.2 governs.
4. **The claim is enforced at the gate and in governance.** Prose alone leaves the claim unenforced. The constitution's gate list, its Open deferrals block, and the preset set are all narrowed.
5. **Merged specs are records.** `specs/003`, `004`, `005`, `006`, and `009` keep their platform text and are superseded by name from the constitution.
6. **The on-ramp is documented.** The amendment states what a future port restores, so the change is reversible by reading.

### Functional Requirements

Governance:

- **FR-001**: The constitution's hard gate list shall name Linux as the platform set builds succeed on, and shall enumerate no other platform. Its Windows-suspension sentence, which reinstates the gate when an upstream port lands, shall be replaced: a future specification adds a platform, and landing an upstream port is not what restores one.
- **FR-002**: The constitution's supported-platform definition shall name Linux alone.
- **FR-003**: The constitution's per-feature release-build clause shall name the Linux release preset only, and shall not name a preset for an unsupported platform.
- **FR-004**: The constitution's Open deferrals block shall carry no macOS-runner entry. Closing it is what retires `specs/009-vendor-quill` T038, which asks for a macOS build result.
- **FR-005**: The amendment shall be recorded in the version lineage table with its rationale, shall bump the file's version to 2.11.0, and shall update the `Last Amended` date.
- **FR-006**: The amendment shall follow this file's existing pattern for superseding an earlier report: the earlier reports for 2.7.0 and 2.8.0 stay in the file and in the lineage table, and a new report states that this amendment supersedes their effect.
- **FR-007**: The amendment shall name each merged spec whose platform decision it supersedes, by directory name.
- **FR-008**: The amendment shall state the on-ramp for a future port, naming the presets a port restores and stating that the per-platform blocks are untouched.

Presets and scripts:

- **FR-009**: `CMakePresets.json` shall carry no preset naming macOS, Windows, Xcode, Visual Studio, AppleClang, or MSVC. This covers `ci-macos`, `ci-windows`, `ci-darwin`, `ci-win64`, `flags-appleclang`, `flags-msvc`, and `ci-multi-config`.
- **FR-010**: Removing those presets shall leave every remaining preset valid. No surviving preset shall name a removed preset in its `inherits` array or pull one in through an `include` array, and every surviving preset shall resolve. The check covers all fifteen surviving configure presets and the single build preset, and it covers the six presets the CI jobs use by name.
- **FR-011**: No CI job shall be added, removed, or changed. The CI matrix is already Linux-only, and this feature must not disturb it. The job count is the observable.

Documentation:

- **FR-012**: `README.md` shall state that Linux is the supported platform, and shall tell a macOS or Windows reader that no build is offered.
- **FR-013**: `README.md`'s vendored-autotools bootstrap section shall carry the Debian-family and RPM-family instructions and shall no longer instruct a reader to install the toolchain with a macOS package manager.
- **FR-013a**: The vendored-autotools module's own diagnostics shall state Linux as the supported platform. Its unsupported-platform abort and its Windows-abort shall read "Linux" alone where they read "Linux and macOS", and each shall keep the sentence naming the upstream `contrib/windows-cmake/` on-ramp, because that sentence is the port's entry point. Its missing-host-tools abort shall carry the Debian-family and RPM-family lines and shall drop the macOS line. Its header comment listing the package sets it handles shall drop the macOS package manager.
- **FR-014**: `AGENTS.md` shall name Linux presets only in its build, test, and verify instructions, and its CI matrix line shall name no unsupported platform.
- **FR-015**: No file outside `specs/`, `external/`, and `build/` shall state that macOS or Windows is supported. The audit pattern is case-insensitive over `macos`, `osx`, `apple`, `darwin`, `xcode`, `windows`, `win32`, `msvc`, `appleclang`, `mingw`, `msys`, `homebrew`, and `brew install`, run over `README.md`, `AGENTS.md`, `CMakeLists.txt`, `CMakePresets.json`, `.codespellrc`, `.github/workflows/`, `cmake/`, `tools/`, `test/`, `example/`, `docs/`, `include/`, and `source/`. Every hit is reported with its path and line and classified into one of four buckets: a live claim that changes, a per-platform seam that is preserved, a merged historical record, or third-party tooling. The pattern is wide because the narrower set missed three runtime diagnostics; `darwin`, `win32`, `xcode`, `homebrew`, and `brew install` each appear at a site a narrower pattern would have passed.

Preservation:

- **FR-016**: No per-platform branch shall change. Every `if(WIN32)`, `if(APPLE)`, `if(MSVC)`, and `if(UNIX)` block in `CMakeLists.txt`, `cmake/ImportAutotoolsSubmodule.cmake`, `cmake/variables.cmake`, `cmake/VendoredArchiveMerge.cmake`, `source/counters/clock_provider.cpp`, and the gate units stays byte-identical, and every `_WIN32` preprocessor branch stays. The diff over branch lines shall be empty. Within `cmake/ImportAutotoolsSubmodule.cmake` the change is confined to diagnostic strings and to the one header comment FR-013a names, and the two platform blocks' own comments, at lines 74, 137, 433, and the `if` lines themselves, are preserved because they document the port.
- **FR-016a**: A comment that names a platform version, a platform vendor's product, or a string's origin stays when removing it would make the comment less accurate. The `.codespellrc` comment naming the macOS Big Sur codename is one: it explains why the token `sur` is exempt, and the token's origin is the fact. Removing the platform name from that comment would narrow no support claim, because the claim it explains is about a spelling exemption.
- **FR-017**: No file under `specs/` shall change, apart from this specification's own files created after it. The merged record stays.
- **FR-018**: No C++ source file, header, or test shall change in behaviour, and no public API, no data model, and no runtime behaviour shall change. Comment-only edits inside a preserved branch are permitted under FR-016a. The feature's tracked content footprint is five files: the constitution, `CMakePresets.json`, `README.md`, `AGENTS.md`, and the diagnostic strings of `cmake/ImportAutotoolsSubmodule.cmake`.
- **FR-018a**: A developer's machine-local `CMakeUserPresets.json` may name a deleted preset, because that file is gitignored and outside the commit. The file's presets shall be brought into line with the committed set so `cmake --preset=dev` and `ctest --preset=dev` keep working locally. CMake validates every preset in the file before resolving any one of them, so a dangling `inherits` fails the whole directory and takes the Linux verification loop with it. This is a local action with zero tracked diff and is recorded so a developer who hits the failure knows the file to edit and knows the commit did not break it.
- **FR-019**: No gate, threshold, warning class, analyzer invocation, or dependency shall be weakened, added, or removed. The gate set's content is unchanged; only the platform enumeration inside it narrows, and that change is the amendment itself.

### Key Entities

- **Supported platform set**: the platforms the project builds, tests, and gates. One member after this feature: Linux, on GCC and Clang, on the distribution families the Linux CI jobs cover.
- **Per-platform seam**: a branch in `CMakeLists.txt` or `cmake/` selecting behaviour by operating system or compiler. Untouched by this feature and the asset a future port consumes.
- **Port on-ramp**: the documented steps that restore a platform: re-add its preset, configure, build, and add a runner. Written into the amendment.
- **Gate-shaped preset**: a preset whose name reads as a CI claim about a platform. Six of them plus their multi-config helper claim two platforms no runner has ever executed.
- **Superseded spec**: a merged spec whose platform decision no longer holds. Five of them, left in place and named from the constitution.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Following `README.md` and `AGENTS.md` end to end on Linux runs zero commands that fail to resolve. Every preset and script named by either document exists in the tree.
- **SC-002**: The FR-015 audit over the full token set and the full path list returns only hits classifiable into one of its four buckets. Every hit is listed in the pull request with its path, its line, and its bucket, and the list contains zero hits in the live-claim bucket after the change. The pre-change inventory measured on this feature's base commit is 8 live-claim lines and 45 preserved-seam lines, and the post-change count of live-claim lines is 0.
- **SC-003**: The constitution's gate list, supported-platform definition, per-feature release-build clause, and Open deferrals block each name Linux alone, and the file reads 2.11.0 with an updated `Last Amended` date.
- **SC-004**: `CMakePresets.json` configures and builds successfully on Linux through every preset a developer is told to run, and the seven CI jobs that configure from a preset still pass. The surviving presets those jobs use are `ci-coverage`, `ci-sanitize`, `ci-ubuntu`, `ci-rocky`, `ci-linux-audit`, and `ci-linux-ignore`, across nine invocations.
- **SC-005**: The CI job count is unchanged from this feature's base commit and every job runs on a Linux runner.
- **SC-006**: The diff against the base commit touches exactly five tracked files: the constitution, `CMakePresets.json`, `README.md`, `AGENTS.md`, and `cmake/ImportAutotoolsSubmodule.cmake`. `CMakeUserPresets.json` is gitignored, so FR-018a's local fix contributes nothing to the diff.
- **SC-007**: Every Linux gate passes on this feature's head, including the format, prose, spelling, coverage, and contract-pairing gates.
- **SC-008**: A reader who finds "macOS must keep building for developers" in any of the five merged vendor specs learns from the constitution that the statement no longer holds, without the spec itself having been edited.

## Assumptions

- **The CI matrix stays as it is**: all eleven jobs run on `ubuntu-26.04` and one runs in a Rocky Linux container. No job has ever run on macOS or Windows, so no runner or job is added or removed. Amendment 2.8.0 already records the absent macOS runner.
- **Linux means the distribution families CI already covers**: Ubuntu for the main jobs and Rocky Linux for the container job. A third distribution is out of scope.
- **`docs/Doxyfile.in` needs no change**: it reads `README.md`, `include/`, and `docs/pages/`, so the README edit reaches the published documentation through the existing docs job and nothing else has to be told about it.
- **No package-manager or toolchain change follows from this feature**: the vendored hwloc tree still builds through autotools on Linux, and the Debian-family and RPM-family install instructions in the README already cover both Linux distribution families CI uses.
- **`ImportAutotoolsSubmodule.cmake` is read and left alone**: its Windows abort names the upstream on-ramp, which is exactly the record a future port needs, so the file is a preservation target under FR-016.
- **The closed specs are already committed history**: `specs/003` through `006` and `specs/009` shipped with their platform text, and superseding them from the constitution leaves them accurate as records of their own time.
- **The `docs` job condition is unchanged**: it runs on a push to `master`. A pull request does not trigger it, so this feature's README change is exercised when it lands.

## Scope boundaries

- Excluded: deleting, restructuring, or cleaning up any per-platform branch in `CMakeLists.txt` or `cmake/`.
- Excluded: any public API change, any data model change, and any runtime behaviour change.
- Excluded: introducing a pluggable platform abstraction, an interface layer, or a build-time platform selection feature.
- Excluded: any Windows or macOS build work, runner, or preset.
- Excluded: editing any merged file under `specs/`, including the five merged vendor specs.
- Excluded: adding a third Linux distribution to the CI matrix.
- Excluded: changing the compiler set on Linux. GCC and Clang stay.