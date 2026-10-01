# Phase 0 Research: Vendor quill as a Private, Pinned Submodule

**Feature**: `009-vendor-quill` | **Date**: 2026-09-30 | **Spec**: [spec.md](spec.md)

All findings verified against the pinned upstream tree at `v13.0.0`
(`eb802a37c7d585840324886a3d8648c9c2159952`), against this repository's
four existing ingestion brackets, and against measurements taken by
compiling against the pinned headers on the host toolchain.

---

### R-001: Ingestion mechanism is a scope-isolated `add_subdirectory` bracket

**Decision**: Consume the submodule through
`add_subdirectory(external/quill "${CMAKE_BINARY_DIR}/_quill" EXCLUDE_FROM_ALL)`
inside a `function(import_quill)` bracket in the root `CMakeLists.txt`,
placed after the yaml-cpp bracket, then link
`quill::quill` into `speedgun-ng_speedgun-ng` as
`PRIVATE $<BUILD_INTERFACE:quill::quill>`.

**Rationale**: quill is a native CMake project, so the simdjson, zlib, and
yaml-cpp path applies and the hwloc autotools module does not. quill defines
no compiled target, so `EXCLUDE_FROM_ALL` and the out-of-tree binary
directory are belt-and-braces. They carry no load, and they cost
nothing and keep the bracket uniform with its four neighbors. The ordering
after yaml-cpp preserves the global `install()` override defined inside
`import_hdrhistogram()`, which stands guard over every later bracket. The
`BUILD_INTERFACE` genex keeps the name `quill::quill` out of the exported
package files.

**Alternatives considered**: `FetchContent` rejected: the other five are
committed gitlinks and the constitution prefers in-tree sources with
recorded provenance. A new `cmake/ImportQuillSubmodule.cmake` rejected:
nothing about quill needs a module, and a module for an interface-only
dependency would be indirection with no payload.

---

### R-002: Version tripwire is a compile-time check on quill's version constants

**Decision**: Pin the version in `source/quill/quill_gate.cpp` with
`static_assert` on `quill::VersionMajor`, `quill::VersionMinor`, and
`quill::VersionPatch`, and add a second assertion on the combined
`quill::Version` value. Assert the versioned inline namespace as well.

**Rationale**: The pinned tree publishes its version as
`inline constexpr uint32_t` in `include/quill/Backend.h`. No preprocessor
macro exists, so `#if` is impossible. Its own build parses the same
constants at configure time and publishes the parsed value with
`PARENT_SCOPE` only, so the value never reaches our scope and reading it
after `add_subdirectory` fails. A compile-time check is the only mechanism
that works, and it matches the four prior imports that carry a
compile-time tripwire. The inline namespace independently encodes the major
version, so a second assertion costs one line and catches an upstream edit
that bumps one place and not the other. No git metadata is consulted, so a
configure from a source archive without history still works.

**Alternatives considered**: Configure-time regex over
`include/quill/Backend.h`, the yaml-cpp technique, rejected: it duplicates a
constant upstream already computes, and a benign upstream reformat of that
header breaks the check while the compile-time assertion keeps working.
Major-version-only rejected: the four prior compile-time tripwires all pin
the exact version.

---

### R-003: Every quill option is pinned at its default inside the bracket

**Decision**: Define all twenty-two upstream options as bracket-scope
normal variables at their upstream defaults before consuming the subtree,
and set `CMAKE_POLICY_DEFAULT_CMP0077 NEW` so those declarations become
inert. Applies the Clarifications (2026-09-30).

**Rationale**: Consuming the subtree injects `QUILL_*` entries into this
project's cache. Nothing stops a developer passing
`-DQUILL_BUILD_TESTS=ON` or `-DQUILL_BUILD_EXAMPLES=ON` and pulling quill's
test suite, example programs, documentation generation, or sanitizers into
this build. Those are exactly the switches FR-011 and FR-012 require off.
Under CMP0077 NEW an option declaration cannot clear a normal variable that
is already defined, so the defaults hold and upstream declares nothing. This
is the same lever the yaml-cpp bracket already pulls for its install,
shared-library, and test switches. Applying it to the whole option set
in place of a hand-picked few means a future upstream option defaults to
off with no change here.

**Alternatives considered**: `FORCE` into the cache rejected: it locks the
values but leaves twenty-two foreign read-only entries visible in this
project's configuration, which is the surface FR-013a forbids. A denylist of
the consequential options rejected: it needs updating on every upstream
release and silently admits the next unknown switch.

---

### R-004: The global build-type mutation is neutralized

**Decision**: Assert in the bracket that the configured build type is
unchanged after the subtree is consumed, and fail configure with a readable
diagnostic naming both values if it moved.

**Rationale**: The pinned tree runs
`set(CMAKE_BUILD_TYPE "Release" CACHE STRING ... FORCE)` when no build type
is set. `CMAKE_BUILD_TYPE` is a global cache variable, so this is not
directory-scoped: a configuration with no build type silently becomes an
optimized build. Our presets always set one, which makes the line inert
today, and inert-until-remembered is how it reaches someone later. SC-007
makes the unchanged value an observable outcome.

**Alternatives considered**: Setting `CMAKE_BUILD_TYPE` in the bracket
rejected: the bracket is function-scoped and the mutation is a cache write
that outlives it, so re-setting inside the bracket hides the symptom rather
than reporting it. Reordering our own preset rejected: the presets are the
authority and should stay outside a dependency's bracket.

---

### R-005: Both install gates are switched off, and the quill packaging artifacts are audited

**Decision**: Set `QUILL_ENABLE_INSTALL OFF` and `QUILL_BUILD_EXAMPLES OFF`
in the bracket. Audit the build tree for the quill-named artifacts that
install path would create, and audit the install tree for any path matching
the FR-020 pattern.

**Rationale**: Upstream has two independent install paths. The main block is
gated on `QUILL_MASTER_PROJECT OR QUILL_ENABLE_INSTALL` and produces a
pkg-config file, package config files, the full vendored header tree, an
export set named `quill-targets`, and four cache variables. Its last
statement is `include(CPack)`, which writes a packaging configuration into
the build tree and adds global packaging targets. This project includes
CPack deliberately through its own install rules, so a packaging
configuration file in the build tree is not evidence either way; the
evidence is the quill-named artifacts that block alone would create.
Separately, the example
programs carry their own `install(TARGETS ...)` calls reached only through
the example switch, and the main block does not check that switch even
though the option's own documentation claims the examples are installed.
Switching one leaves the other live. `EXCLUDE_FROM_ALL` does not suppress
installs.

**Alternatives considered**: Trusting the option defaults rejected: the
upstream defaults are already off, but a user or a future preset could flip
them, and FR-011 requires the ingestion to hold. The moment does not
hold. Auditing only for surviving install rules rejected: "no install rule
survived" and "no quill package configuration was generated" are different
claims, and only the second is what a consumer can observe.

---

### R-006: Diagnostic suppression is a system include marking; bracket flag clearing does not work

**Decision**: After the subtree is consumed, read the interface target's
include directory property and re-set it as a system include directory,
exactly as all four existing brackets do. Do not clear warning, sanitizer,
or analyzer variables in this bracket, because nothing there compiles.
Applies the Clarifications (2026-09-30), FR-010a, FR-010b.

**Rationale**: The four compiled brackets achieve vendored-warning
exclusion by clearing the flag variables inside their own bracket, which
works because each dependency compiles targets inside its bracket. quill
compiles nothing, so clearing variables there affects no compilation at
all. The single unit that includes quill's headers is compiled in this
project's scope under this project's strict flags. The system include
marking is the only mechanism that reaches those headers, and it is the
mechanism the four brackets already use as their second step. The project's
own unit stays fully gated, which is the property the constitution cares
about.

**Alternatives considered**: Narrow per-diagnostic suppressions in the one
unit rejected: it requires maintaining a list against upstream churn, and
the known false-positive overflow diagnostic from one supported compiler
would need pinning to a compiler version. Per-unit system marking rejected:
the promoted property covers every future unit at no extra cost. Leaving
warnings visible rejected: this project's gate would then report findings
from code it does not own, at lint parity, blocking its own merges.

---

### R-007: The link edge is build-only and there is no archive merge

**Decision**: Link `PRIVATE $<BUILD_INTERFACE:quill::quill>>` and add
`quill::quill` to no merge list. Record the absence of a merge as a
deliberate deviation.

**Rationale**: quill's build defines `quill` as an `INTERFACE` library with
`quill::quill` as an alias. It produces no object files, no archive, and no
shared object. The four compiled imports route through
`cmake/VendoredArchiveMerge.cmake`; there is nothing here to route.

**Alternatives considered**: Omitting the target link and adding a bare
include path rejected: it loses the promoted system include marking of R-006
and the one warning relaxation upstream attaches for Clang-family compilers,
and it duplicates what the target already carries. Forcing an archive
rejected: nothing compiles to put in one.

---

### R-008: The dependency proof is a runnable check; an archive scan has nothing to scan

**Decision**: Prove the dependency with a test executable that links the
speedgun-ng library and quill, drives a real counter to a real value, emits
it through quill, and asserts the value reached the log. Name it distinctly
from the `*_nm_proof` family. Applies the Clarifications (2026-09-30), FR-008,
FR-008a.

**Rationale**: Measured on the pinned tree. A unit that includes a quill
header and takes the address of one quill function emits that function and
its entire inline call graph as weak definitions in the same unit, and emits
**zero** undefined symbols, at both `-O0` and `-O2`. The three-way archive
proof the four compiled imports use requires an undefined reference resolved
by a different member, so it cannot pass here under any formulation. The
same measurement gives the cost of forcing one. With debug information off, in
the release configuration the referencing unit's object is 563,104 bytes
carrying 194 quill symbols, against 3,744 bytes and 2 symbols for a unit that
reads only the version constants, a factor of about 150. Unoptimized the two
are 2,670,304 and 24,648 bytes. For a benchmarking library, permanently adding
half a megabyte of never-called code to the shipped archive to satisfy a proof
is the wrong trade. SC-009a caps the archive growth in the release
configuration so the trade stays visible, and names that configuration
because the same unit is about 319 KB in a debug build, almost entirely debug
records, so one unstated number would be untestable.

**Alternatives considered**: Keep a symbol reference in the shipped archive
rejected on the measurement above. Archive scan of any shape rejected: it has
nothing to scan. Version tripwire alone rejected as sufficient: it proves
the headers are reachable and version-locked, and says nothing about whether
the counter library and quill interoperate at link and run time, which is
what a future logging facility depends on.

---

### R-009: The wrapper unit carries the tripwire and nothing else

**Decision**: `source/quill/quill_gate.cpp` includes one quill header and
holds the version assertions. No symbol reference, no call into quill, no
quill macro pre-set anywhere in the tree.

**Rationale**: Follows from R-002 and R-008. The unit's job is to make the
include path and the pin load-bearing. Measured, a unit limited to the
version constants emits 3,744 bytes at `-O3 -DNDEBUG`, so the shipped archive barely
moves.
Upstream offers two mutually exclusive switches whose combination is a
hard preprocessor error, and one switch requiring a project-supplied
definition in exactly one translation unit whose violation is a genuine
one-definition-rule break. Leaving all of them alone and asserting their
absence keeps both traps closed.

**Alternatives considered**: Reusing the pattern the other five wrapper
units use, which pairs the tripwire with a link-proof reference, rejected:
the reference half is the part that cannot work and the part that costs the
weight.

---

### R-010: quill's symbol visibility is a declared exception, and the audits read the installed shared object

**Decision**: Treat upstream's default-visibility singletons as a declared
exception to symbol hiding. Run every symbol and runtime-dependency audit
against the installed shared object, never against the internal archive's
member list.

**Rationale**: Upstream's export attribute resolves to
`__attribute__((visibility("default")))` on GCC and Clang regardless of the
host's visibility settings, so its singletons cannot be hidden by any
setting we apply. Upstream reports those singletons colliding when its
headers reach more than one shared object in a process; that report does not
apply here, because this feature builds one internal library into one binary
where the linker merges one instance. Auditing the installed shared object
keeps the exception from becoming a blind spot: a singleton that escapes
into `.dynsym` still trips the audit, which is the leak that would
matter. Auditing the internal archive instead would have to whitelist the
singletons and would then miss exactly that escape.

**Alternatives considered**: Compiling the headers with a macro that
suppresses the export attribute rejected: upstream documents the
static-build branch as the no-attribute case, and forcing it would mean
defining a DLL-mode macro in a static build. Auditing the archive with a
whitelist rejected, for the escape reason above.

---

### R-011: The threading dependency reaches the installed artifact, and the example audit is unaffected

**Decision**: Require the installed library's runtime dependency list to name
no quill-owned object, and record any platform threading entry as a platform
property. Add no change to the standalone example's link-manifest audit;
verify it. Widening it is rejected.

**Rationale**: Upstream's build requires the platform threads package
unconditionally and propagates it through the interface target, so a static
library consuming that target carries a link-only edge to it. On the
supported Linux distribution the threading routines are part of the base C
library, so the entry adds nothing to the installed object's runtime
dependency list. The example audit permits only the platform C and C++
runtime and is unaffected either way, because the example compiles public
headers only and those never include a quill header. Verified by reading the
audit and the example's link line. Nothing here rests on an assumption.

**Alternatives considered**: Widening the example audit's allowlist rejected
preemptively: it would weaken a gate to accommodate a dependency that has not
been shown to need it. Dropping the propagation with a link option rejected:
it would fight the interface target for no proven benefit.

---

### R-012: Test names are distinct from the archive-proof family

**Decision**: Register `quill_purity_scan` in the same shape as the other
five, and register the runnable check under its own name, never
`*_nm_proof`.

**Rationale**: Five existing CTest entries are named `*_nm_proof` and mean a
three-way archive scan. Reusing that name for a runnable check would make a
passing test name assert something false about what was proven, and a reader
auditing the privacy contract would look for an archive scan that does not
exist.

**Alternatives considered**: Reusing the `_nm_proof` suffix for symmetry
rejected on the naming claim above. Omitting the purity scan rejected: the
discovery-call and public-header greps are the same shape as the other five
and catch the same two leak classes.

---

### R-013: The coverage capture needs one narrow flag, found by verification

**Decision**: Add `--ignore-errors mismatch` to the capture command in
`cmake/coverage.cmake`, and verify that the coverage trace contains no path
under any vendored tree. Corrected during implementation: this decision was
originally "add no change", which verification overturned.

**Rationale**: `cmake/coverage.cmake` captures with `--no-external` and then
extracts with an allowlist admitting only
`${PROJECT_SOURCE_DIR}/include/speedgun-ng/*` and
`${PROJECT_SOURCE_DIR}/source/*`. A vendored header at
`external/quill/include/quill/...` matches neither pattern under either
absolute or relative filename matching, so it is dropped by the extract step
regardless of the capture flags. This matters more for quill than for the
four compiled imports, whose vendored lines sit in separate objects: quill's
inline code is emitted into this project's own instrumented unit, so the
drop has to come from the extract allowlist. Excluding a
target. The four existing wrapper units have the same shape, one
declaration and no executable line, and the gate passes on them today.

**Verification outcome**: the assumption behind the original decision was
half right and the half that mattered was wrong. The extract allowlist does
drop every vendored path, so the gate's reported numbers were never polluted,
and the final trace holds zero vendored paths. The capture, however, does not
skip vendored headers: `--no-external` does not exclude them, because
`external/` sits inside this project's source directory. Every vendored tree
already lands in the raw trace today, simdjson, hwloc, HdrHistogram_c, zlib,
and yaml-cpp all appear in it, so this is pre-existing behavior and not
something quill introduced. quill is the first vendored tree whose headers
trip lcov's exception-tag consistency check, and that aborts the capture,
which takes the whole coverage target down with it. Measured: 48 quill
records in the raw trace and one hard error at
`external/quill/include/quill/core/LoggerManager.h:149`.

**Alternatives considered**: Clearing the coverage flag variable in the
bracket rejected, for the reason R-006 gives, and it would not help in any
case because the contamination reaches the trace through this project's own
instrumented unit. A vendored target cannot carry it. Excluding the path
at capture time rejected: lcov's `--exclude` cannot be combined with
`--capture` in the installed version, and a removal pass after the fact does
not help because the capture never completes. Suppressing the whole error
class project-wide is what the chosen flag does, and it is bounded: the flag
names one lcov error, gcov's own diagnostics stay live, and the gate's
metrics are unaffected because the extract step removes every vendored path
before a number is read. Measured after the change: 100.0% lines, 100.0%
branches, and zero vendored paths in the final trace.

---

### R-014: The downstream consumer premise is built from the pinned tree

**Decision**: In the downstream consumer job, establish the premise that a
system quill exists by configuring and installing the pinned tree out of
tree into a system prefix, then audit the consumer's configure and build
logs for the FR-020 pattern.

**Rationale**: Upstream publishes no system development package on the
supported distribution, so the premise cannot be met by a package install.
The yaml-cpp import established the same technique for the same reason. The
premise is about the machine; the build still consumes the submodule, and
FR-005 forbids the alternative.

**Alternatives considered**: Skipping the premise rejected: the consumer test
proves nothing about a dependency absent from the machine. Building the
pinned tree as the system copy rejected as circular; the premise must be an
independently installed copy, which is what makes the audit meaningful.

---

### R-015: The dependency-classification gate needs its own branch

**Decision**: Add a `vendored-private` branch for quill to
`tools/dbc/dependency_scan.sh` naming this feature's requirements and its
audit identifiers.

**Rationale**: That script holds one branch per vendored dependency and exits
1 on any classification of `library-runtime`. A new private link edge with no
branch classifies as a runtime dependency and fails the gate. The yaml-cpp
import needed the same one-line-per-dependency addition and shipped it as
its own commit.

**Alternatives considered**: Generalizing the script's classifier rejected:
it works today and the change is one branch. A redesign is not warranted.
