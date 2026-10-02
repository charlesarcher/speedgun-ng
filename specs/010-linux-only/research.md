# Phase 0 Research: Linux as the Supported Platform

**Feature**: `010-linux-only` | **Spec**: [spec.md](spec.md) | **Date**: 2026-10-02

Six decisions, each measured against this repository at the base commit
`fbdfc6f`. Three of them corrected the specification,
which is recorded here because the plan is built on the corrected text.

## R-001: The seven deleted presets form a closed inheritance cluster

**Decision**: Delete `flags-appleclang`, `flags-msvc`, `ci-darwin`,
`ci-win64`, `ci-macos`, `ci-windows`, and `ci-multi-config` from
`CMakePresets.json`, leaving 15 configure presets, 1 build preset, and
1 test preset.

**Rationale**: Measured by reading every `inherits` array in the file. The
file is preset schema version 2, so it carries no `include` array; the
`include` field arrived in version 4. All six `inherits` edges that name a
deleted preset originate inside a deleted preset:

| Deleted preset | Inherits | Deleted among them |
|---|---|---|
| `ci-darwin` | `flags-appleclang`, `ci-std` | `flags-appleclang` |
| `ci-win64` | `flags-msvc`, `ci-std` | `flags-msvc` |
| `ci-macos` | `ci-build`, `ci-darwin`, `dev-mode`, `ci-multi-config` | `ci-darwin`, `ci-multi-config` |
| `ci-windows` | `ci-build`, `ci-win64`, `dev-mode`, `ci-multi-config` | `ci-win64`, `ci-multi-config` |

No surviving preset names a deleted one. The deletion set is closed under
`inherits`, so no survivor is left with a dangling parent and the removal
cannot break a Linux preset. `ci-std` survives on its own merit: its
remaining referrer is `ci-linux`, which keeps it.

**Alternatives considered**: Keeping the presets and marking them
deprecated. Rejected: a preset named `ci-macos` reads as a CI claim about
macOS to every reader who lists presets, and no runner has ever executed
one. Leaving them while the constitution says Linux is the only platform
leaves the two surfaces contradicting each other.

## R-002: A machine-local preset file breaks the Linux verification loop

**Decision**: Record FR-018a, a local fix to `CMakeUserPresets.json`, and
keep it out of the tracked diff.

**Rationale**: `CMakeUserPresets.json` is gitignored at `.gitignore:10` and
`AGENTS.md` states it is machine-local and never committed. This machine's
copy carries `dev-darwin`, inheriting `ci-darwin`, and `dev-win64`,
inheriting `ci-win64`. CMake validates every preset in the resolved set
before it resolves any single preset, so a dangling `inherits` fails the
whole directory. Measured directly: with the seven committed presets
deleted and this machine's user file untouched, the configure step fails
with `Invalid configure preset: "dev-darwin": Could not find inherited
preset "ci-darwin"`, and `cmake --preset=dev` stops working. After removing
those two presets from the user file, `dev`, `ci-ubuntu`, and `ci-coverage`
all configure at exit 0.

The surviving local chain is clean: `dev` inherits `dev-linux`, which
inherits `dev-common` and `ci-linux`; `dev-coverage` inherits `dev-mode` and
`coverage-linux`. Nothing else in that file names a deleted preset.

**Alternatives considered**: Editing the committed `CMakePresets.json` to
keep `ci-darwin` and `ci-win64` alive for local files that inherit them.
Rejected: those two presets exist only to serve `ci-macos` and
`ci-windows`, so keeping them leaves the gate-shaped surface the change
exists to remove. The alternative of documenting the breakage and leaving
it was rejected because the fix is two lines in a file that is not
committed.

## R-003: Three runtime diagnostics assert macOS support outside the declared file scope

**Decision**: Correct the specification. FR-013a now covers the vendored
autotools module's diagnostics, the tracked footprint is five files, and
FR-016 now separates diagnostic strings from platform branches.

**Rationale**: A full-repository sweep for the platform tokens found eight
live support-claim lines outside `specs/` and `external/`, and seven of
them are in `cmake/ImportAutotoolsSubmodule.cmake`, which FR-016 had
declared untouchable. Three are runtime `FATAL_ERROR` diagnostics a
developer can hit:

| Line | Text | Why it is a claim |
|---|---|---|
| 140 to 142 | "this module supports Linux and macOS only." | names macOS as supported, in the Windows abort |
| 204 to 205 | "macOS: brew install autoconf automake libtool (BSD patch ships with macOS)." | tells a developer to install a toolchain for a platform now unsupported |
| 567 to 568 | "Supported set: Linux and macOS." | names the supported set, in the catch-all abort |

The eighth is `.codespellrc:12`, a comment explaining that the token `sur`
is exempt because it is the macOS Big Sur codename.

The first seven are changeable without touching a single platform branch.
The strings are separate string literals from the branches that select
platform behaviour, and the surrounding `if(WIN32)`, `elseif(UNIX OR
APPLE)`, and `if(NOT APPLE AND IAS_MERGE_INTO)` lines stay.

**Alternatives considered**: Leaving the module alone on the reading that
FR-016 outranks FR-015. Rejected: FR-015 exists so no file claims an
unsupported platform, and a configure-time abort is the most consequential
place to make that false claim, because it is the text a developer reads
when the build has already failed. The alternative of leaving the claim and
recording a deferral was rejected because the fix is three string literals.

## R-004: The narrow audit pattern missed every site that mattered

**Decision**: Widen FR-015's token set from four tokens to thirteen and its
path list from six paths to fourteen.

**Rationale**: The first token set was `macos`, `windows`, `msvc`,
`appleclang`. Measured against the sweep, it catches none of the three
diagnostics in R-003, because two of them spell the platform as `macOS`
inside a longer sentence that the pattern would match on `macos` yet an
implementer auditing by hand would read as a Homebrew instruction, and the
third names `Supported set: Linux and macOS`. More decisively, the narrow
set misses five tokens that appear at live sites: `darwin` at the
`ci-darwin` generator, `win32` at four `_WIN32` branches in
`source/counters/clock_provider.cpp`, `xcode` at `ci-darwin`'s generator,
and `homebrew` and `brew install` in the autotools module. An audit that
passes while `brew install autoconf automake libtool` still ships is worse
than no audit, because it reports clean.

The four-bucket classification exists for the same reason. The sweep
returns 116 hits under `specs/` alone, all of them dated merged records,
and 45 seam lines that must not move. A count-based criterion would either
be drowned by the records or satisfied by deleting seams. SC-002 therefore
requires every hit to be listed with its bucket and requires the
live-claim bucket to reach zero.

**Alternatives considered**: Auditing only the four edited files. Rejected:
it cannot see a claim in a fifth file, which is exactly the failure this
research found. Auditing with the narrow token set. Rejected as above.

## R-005: One sweep hit is a false positive that must survive

**Decision**: FR-016a keeps `.codespellrc:12` unchanged, and keeps the
comments at `ImportAutotoolsSubmodule.cmake:74`, `:137`, and `:433`.

**Rationale**: The `.codespellrc` comment explains why the spelling gate
exempts the token `sur`. The token's origin is the fact the comment
records, and the comment is about a spelling exemption, with no
supported target in it. Stripping the platform name from it narrows no claim and
makes the comment harder to follow, which Principle IV weighs against
content-free comment removal.

The three module comments are the port's own record. Line 74 documents the
upstream `contrib/windows-cmake/` on-ramp, line 137 labels the Windows
block as unsupported, which remains true, and line 433 explains why an
archiver flag is ELF-only. The owner's instruction is that nothing
forecloses a port, and these lines are what a future port reads first.

**Alternatives considered**: Removing the platform name from the
`.codespellrc` comment for sweep symmetry. Rejected on the Principle IV
ground above, and because SC-002 already classifies the hit, so symmetry
buys nothing.

## R-006: The amendment follows the file's existing supersession pattern

**Decision**: Amendment 2.11.0, MAJOR. A new Sync Impact Report comment
sits at the top of the constitution, the 2.10.0 report becomes a Prior
report beside the existing ones, and the lineage table gains a row.

**Rationale**: Governance classifies MAJOR as an incompatible principle
removal or redefinition, MINOR as a new principle or materially expanded
guidance, and PATCH as wording. Narrowing VIII's gate list from three
platforms to one removes an obligation from a NON-NEGOTIABLE principle.
That is a removal, so MAJOR at 2.11.0.

The file already has the pattern this needs. The 2.9.1 report corrects a
defect in the 2.9.0 clause while leaving the 2.9.0 report in place, and the
2.10.0 report is a Sync Impact Report at the top in an HTML comment. So the
2.7.0 Windows suspension and the 2.8.0 macOS deferral stay in the file and
in the lineage table, their rows stay, and the new report states that
2.11.0 supersedes their effect. Deleting their rows would erase the record
that those gates were once suspended and why.

Four sites change text inside the constitution, listed in the plan's
Project Structure. The Open deferrals macOS entry closes, which is what
retires `specs/009-vendor-quill` T038 by reference and leaves that merged
spec untouched.

**Alternatives considered**: MINOR, on the reading that a platform
enumeration narrowing redefines no principle. Rejected because VIII's list
is where the obligation lives, and removing two of its three platforms
removes an obligation. The owner chose MAJOR.

## R-007: The CI matrix needs no change and the job count is the guard

**Decision**: Touch no workflow file. Record the job count as FR-011's
observable.

**Rationale**: Measured, all eleven jobs run on `ubuntu-26.04`, with one of
them, `test-rocky`, running in a `rockylinux:10` container. A token sweep
of `.github/` returns zero hits for any platform token, so no workflow
claims a platform it does not run. Amendment 2.8.0 already records that the
matrix carries no macOS runner. The second platform in the matrix is a
second Linux distribution, and a second operating system is a different
thing, so narrowing the
supported set to Linux leaves the matrix intact.

Seven of the eleven jobs configure through a preset, across nine
invocations, naming six presets: `ci-coverage`, `ci-sanitize`, `ci-ubuntu`,
`ci-rocky`, `ci-linux-audit`, and `ci-linux-ignore`. All six survive R-001's
deletion. The four jobs that configure without a preset are `lint`,
`prose-lint`, `docs`, and `downstream-consumer`'s library build.

**Alternatives considered**: Adding a Linux-only assertion to the workflow,
a step that fails if a non-Linux runner appears. Rejected: the matrix
already is Linux-only, so the assertion guards against a future edit that
belongs to whatever specification makes that edit, and a gate added for a
condition that cannot occur is the speculation Principle X.2 forbids.

## R-008: No contract surface, no data model, no runtime effect

**Decision**: One contract document, recording the amendment's required
statements and the audit's four buckets. No data model.

**Rationale**: The feature exposes no interface, so the contract step
would normally be skipped outright. It earns one document here because the
constitution text becomes governance: the required statements are the
contract, and a future amendment that drops one of them should be able to
cite what was required. The data-model step has nothing to model. The five
entities the spec names are documents and a preset set, with no fields, no
validation, and no state transitions; recording them as a table in the
contract document keeps them where a reader looks. An artifact whose every
section says "not applicable" teaches a reader nothing.

**Alternatives considered**: Skipping `contracts/` entirely. Rejected on
the same ground the spec's own checklist took for its non-technical
stakeholders: a placeholder artifact teaches a reader nothing, while one
document with the required statements in it is checkable.