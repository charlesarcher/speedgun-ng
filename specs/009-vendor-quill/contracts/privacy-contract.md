# Contract: quill Non-Exposure

**Feature**: `009-vendor-quill` | **Date**: 2026-09-30
**Implements**: FR-005, FR-016 through FR-020; research R-008, R-010, R-011, R-014.
**Companion**: [build-integration.md](build-integration.md) for the ingestion mechanism.

## 1. Boundary

Quill is a private internal dependency. Every surface a downstream
`find_package(speedgun-ng)` consumer can observe must be free of it. This
contract is unchanged in shape from the four prior vendor imports; what
differs is that quill produces no archive, so there is nothing to merge and
nothing to scan inside the shipped archive.

Audit identifiers restart at A1 in this feature, as they do in
specs/004, specs/005, and specs/006. The A-numbers are per-spec references,
not a global sequence.

## 2. Detection pattern

Every audit uses the case-insensitive literal `quill`, per FR-020. Upstream
names its artifacts, its include directory, its namespace, and its bundled
formatter namespace by that one spelling, so a real leak in any of them trips
the check and no variant spellings are needed.

The symbol audits add two mangled-namespace markers, because a namespace can
appear in a symbol table without appearing in a path or a file name:

| Marker | Encodes |
|--------|---------|
| `5quill` | quill's namespace |
| `8fmtquill` | the bundled formatter's namespace, which upstream namespaces separately so it cannot collide with a host's own formatter |

## 3. Audits

| ID | Surface | Command | Expected | Requirements |
|----|---------|---------|----------|--------------|
| A1 | Install tree | `find prefix/ -iname '*quill*'` | empty output | FR-016, SC-002 |
| A2 | Package files | `grep -riE 'quill' prefix/lib/cmake/speedgun-ng/*.cmake` | empty output (covers `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake`) | FR-017, SC-003 |
| A3 | Shared link interface | `grep -riE 'quill' prefix/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake` on a shared build | empty output; no `INTERFACE_LINK_LIBRARIES` entry resolves to quill | FR-017 |
| A4 | Dynamic symbols | `nm -D --defined-only libspeedgun-ng.so`, filtered on `5quill` and on `8fmtquill`, each result required to be a member of the FR-018 allowlist | a subset of the allowlist, currently both entries, with an empty set also passing; zero `8fmtquill` | FR-018, FR-020, SC-004 |
| A5 | Runtime dependencies | `ldd prefix-shared/lib/libspeedgun-ng.so \| grep -iE 'quill'` | empty output | FR-016, SC-004, R-011 |
| A6 | Dependency proof | the runnable check: links the library and quill, emits a real counter value through quill, asserts the value reached the log | exits 0 with the counter value present in the log | FR-008, FR-008a, SC-009, R-008 |
| A7 | Build files | `grep -rniE 'find_package\( *quill\|pkg_check_modules\([^)]*quill' --include='*.cmake' --include='CMakeLists.txt' .` with `external/`, `build/`, `prefix*`, `.git/`, `.specify/`, and `specs/` excluded | zero hits | FR-005, SC-008 |
| A8 | Public headers | `grep -riE 'quill' include/` | zero hits | FR-014 |

A7 excludes `specs/` in addition to the five directories the five existing
scans exclude, because this feature's own specification and research files
name the pattern throughout and would otherwise trip the scan.

## 4. Deviations from the four prior imports, and why

| Prior shape | quill | Reason |
|-------------|-------|--------|
| A6 is a three-way archive scan: vendored members present, gate reference undefined, reference resolved in another member | A6 is a runnable check | Measured: every quill symbol is emitted as a weak definition in the same unit that declares it, and zero undefined symbols exist at any optimization level, so the three-way shape cannot pass (R-008). Forcing one costs 563,104 bytes at `-O3 -DNDEBUG` against 3,744 bytes for the version constants alone. |
| A6 scans the shipped archive's member list | A4 and A5 read the installed shared object only | quill's export attribute is default visibility on GCC and Clang regardless of this project's settings, so its singletons cannot be hidden. Auditing the archive would require whitelisting them and would then miss a singleton that escapes into `.dynsym`, which is the leak that would matter (R-010). |
| The bracket clears warning and sanitizer variables to exclude the vendored tree | The bracket clears nothing; the interface include is promoted to a system include | quill compiles no target, so clearing variables in its bracket affects no compilation (R-006). |
| The consumer premise is met by a system package | The premise is met by installing the pinned tree into a system prefix | Upstream publishes no system development package on the supported distribution (R-014). |

## 5. Declared exceptions

### 5.1 Symbol visibility

quill's singletons carry default symbol visibility and cannot be hidden.
That is upstream's design for static consumers and it is correct here: this
feature builds one internal library into one binary, where the linker merges
a single instance. The exception is bounded three ways:

1. It covers upstream's singletons only, never this project's own symbols.
2. A6 never touches the internal archive, so the exception opens no path.
3. A4 still fails if any quill symbol reaches the installed shared object's
   dynamic symbol table, which is the observable leak.

## 6. Threading dependency

Upstream's build requires the platform threads package unconditionally and
propagates it through the interface target, so the installed library gains a
link-only edge to it. A5 requires that no quill-owned object appears in the
runtime dependency list. Any platform threading entry the edge introduces is
a property of the platform C library, recorded as such and not a leak
(R-011).

The standalone example's link-manifest audit is left unwidened. The example
compiles public headers only, and no public header includes a quill header,
so the example never reaches the dependency. The audit is verified rather
than relaxed in advance.

## 7. Verdict

All eight audits green is the privacy contract. A1 through A5, A7, and A8
are static or build-time checks. A6 is the authoritative end-to-end proof
that the two libraries interoperate, and it is the only audit that runs the
code. A red audit is a defect at lint parity: it blocks merge the way a
clang-tidy finding does.

### 5.2 Two exported thread helpers

A shared build of this project exports exactly two quill symbols:

```text
W _ZN5quill3v136detail13get_thread_idEv
W _ZN5quill3v136detail15get_thread_nameB5cxx11Ev
```

Both are weak, stateless inline helpers. Neither carries a singleton, a
queue, or any other shared state, so nothing observable about this library's
behavior can be influenced through them.

The pinned tree marks both `QUILL_ATTRIBUTE_USED`, which forces emission
whether or not anything calls them, and `QUILL_EXPORT`, which resolves to
default visibility on GCC and Clang regardless of this project's settings.
They arrive in any translation unit that includes `quill/Backend.h`, because
that header includes `quill/backend/SignalHandler.h`, which includes
`quill/backend/ThreadUtilities.h` unconditionally. `Backend.h` is the only
header publishing the version constants the FR-002 tripwire asserts on.

Two ways to remove them were considered and rejected:

| Option | Why rejected |
|--------|--------------|
| Edit the vendored tree | FR-001 forbids modifying the pinned sources; the pin's value is that the bytes are upstream's |
| Add a project-wide linker version script | Changes export policy for every platform, including the Windows gate amendment 2.7.0 already suspends, and interacts with the generated export header |

The exception is bounded: every exported quill symbol must appear in the
allowlist, so a third symbol, a different symbol type, or any
bundled-formatter symbol fails it. The CI job also runs a dedicated
`8fmtquill` marker step. That step cannot fail independently, because the
allowlist already rejects any bundled-formatter symbol, and it is retained
as defense in depth at no cost. The audit does not require both
allowlisted symbols to be present, because an upstream release that emits
strictly less is the safe direction and must not read as a leak. Verified
against a stubbed symbol table: two allowlisted symbols pass, one passes,
none passes, a third symbol fails, a type change fails, and a bundled
formatter symbol fails.
