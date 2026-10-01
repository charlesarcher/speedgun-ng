# Data Model: quill as a Private, Pinned Submodule

**Feature**: `009-vendor-quill` | **Date**: 2026-09-30
**Spec**: [spec.md](spec.md) | **Research**: [research.md](research.md)

This feature holds no runtime data of its own. It has no schema, no
persistent state, and no public type. The entities below are build-time and
source-tree artifacts, recorded so their attributes stay auditable.

---

## Entity: Vendored quill submodule

The quill sources carried in-tree.

| Attribute | Value | Source |
|-----------|-------|--------|
| Vendored path | `external/quill` | FR-001 |
| Upstream | `https://github.com/odygrd/quill` | FR-001 |
| Release tag | `v13.0.0` | FR-001 |
| Pinned commit | `eb802a37c7d585840324886a3d8648c9c2159952` | FR-001 |
| License | `LICENSE` in the submodule root, MIT | FR-006 |
| Upstream submodules | none | FR-001, R-001 |
| Compiled artifact | none; the build defines an interface-only target | FR-008, R-001 |
| Minimum language level | C++17; this project sets 23 | FR-002, R-002 |
| Interface target name | `quill::quill`, an alias over `quill` | FR-008, R-001 |
| Version publication | compiled constants and a versioned inline namespace; no preprocessor macro; parsed value not published to the consuming scope | FR-002, R-002 |
| Declared build-system minimum | below the version that introduced the switch-declaration policy the ingestion pins | FR-013, R-003 |
| Options declared | 22, all pinned at their defaults by the ingestion | FR-013a, R-003 |

**Validation**: the commit is the pinned one, per `git submodule status`.
**Identity**: the commit hash. **Lifecycle**: replaced only through the
README re-pinning steps, which also bump the version constants.

---

## Entity: Bundled formatter

The formatting library quill carries inside its own tree.

| Attribute | Value | Source |
|-----------|-------|--------|
| Location | inside the pinned submodule; no separate vendoring | R-001 |
| Version | 12.0.2 | R-001 |
| Namespace | namespaced separately from the common formatter namespace, so a host with its own formatter sees no collision | FR-020, R-010 |
| Detection marker | `8fmtquill` | FR-020 |

**Validation**: no companion submodule and no separate package requirement
exists, so the zero-external-runtime-dependency rule holds without further
vendoring. **Identity**: travels with the submodule commit.

---

## Entity: Imported interface target

The build-system handle through which the library consumes quill privately.

| Attribute | Value | Source |
|-----------|-------|--------|
| Target name | `quill::quill` | FR-008, R-001 |
| Linkage | `PRIVATE`, wrapped in the build-only genex | FR-008, R-007 |
| Object files | none | FR-008, R-007 |
| Archive merge | none; nothing to merge | FR-008, R-007 |
| Propagated compiler options | exactly one, the Clang-family warning relaxation | FR-009 |
| Propagated compile definitions | none at upstream defaults | FR-009 |
| Propagated link dependency | the platform threads package | FR-016, R-011 |
| Include marking | promoted to a system include directory | FR-010a, R-006 |
| Presence in exported package files | none | FR-017 |

**Validation**: no propagated option or definition beyond the one named
above, and no target name in any installed package file (audits A2, A3).

---

## Entity: Internal wrapper unit

The one translation unit where quill's headers enter the build.

| Attribute | Value | Source |
|-----------|-------|--------|
| Path | `source/quill/quill_gate.cpp` | FR-014 |
| Quill headers included | one | FR-014 |
| Contents | the version assertions | FR-002, FR-003, R-009 |
| Symbol references to quill | none | FR-008, SC-009a, R-008 |
| Pre-set quill macros | none | FR-010 |
| Executable lines | none | FR-010, R-009 |
| Object size | 3,744 bytes at `-O3 -DNDEBUG`, measured | SC-009a, R-008 |

**Validation**: no header under `include/` includes a quill header (audit
A8), so this unit is the only point of entry. **State**: none. **Identity**:
none; it is a compile-time artifact only.

---

## Entity: Runnable dependency check

The end-to-end proof, outside the shipped archive.

| Attribute | Value | Source |
|-----------|-------|--------|
| Links | the speedgun-ng library and quill | FR-008 |
| Drives | a real counter from the speedgun-ng counter interface | FR-008a |
| Emits | that counter value through quill's logging entry point | FR-008a |
| Asserts | the value appears in what reached the log | FR-008a |
| Test name | distinct from the `*_nm_proof` family | R-012 |
| In the shipped archive | no | FR-008 |
| In the installed artifact | no | FR-008 |
| In the exported target set | no | FR-008 |

**Validation**: builds, links, runs, exits 0, counter value present in the
log (SC-009). **State**: none beyond the process it runs in. It starts a
quill backend, which is why Fixed decision 8 exempts it.

---

## Entity: Privacy contract

The consumer-observable surfaces that must stay quill-free.

| Surface | Audit | Requirement |
|---------|-------|-------------|
| Install tree | A1 | FR-016 |
| Package config files | A2 | FR-017 |
| Shared link interface | A3 | FR-017 |
| Dynamic symbol table | A4 | FR-018 |
| Runtime dependency list | A5 | FR-016 |
| Dependency proof, end to end | A6 | FR-008, FR-008a |
| Build-file discovery calls | A7 | FR-005 |
| Public headers | A8 | FR-014 |

**Detection pattern**: the case-insensitive literal `quill`, plus the
mangled-namespace markers `5quill` and `8fmtquill` for symbol surfaces
(FR-020).

---

## Relationships

```text
Vendored quill submodule
  contains  Bundled formatter
  consumed through  Imported interface target
  reached only by  Internal wrapper unit
  proven by  Runnable dependency check
  constrained by  Privacy contract
```

The submodule is the single source. Nothing else in the tree may resolve a
quill from anywhere else (FR-005), and no consumer may observe it through
any surface (FR-016 through FR-019).
