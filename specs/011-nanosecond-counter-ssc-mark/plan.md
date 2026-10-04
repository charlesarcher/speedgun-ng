# Implementation Plan: Nanosecond Counter and Simulation-Start Marker

**Branch**: `011-nanosecond-counter-ssc-mark` | **Date**: 2026-10-03 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/011-nanosecond-counter-ssc-mark/spec.md`

**Artifacts**: [research.md](research.md) · [data-model.md](data-model.md) · [contracts/simulation-start.md](contracts/simulation-start.md) · [contracts/monotonic-raw-counter.md](contracts/monotonic-raw-counter.md) · [quickstart.md](quickstart.md)

## Summary

Two additions to the library's measurement surface, and a gate that makes the
second one verifiable.

The counter is a leaf on the machine root of the existing counter catalog,
published at `machine/monotonic_raw`. It reads `CLOCK_MONOTONIC_RAW`
through libc's `clock_gettime`, which glibc serves from the vDSO, and
converts the seconds-and-nanoseconds pair with the same integer expression
the three existing clock readers already use (research.md R-001, R-003). It
adds no provider, no registration macro, no dispatch table, and no
enumerator: the leaf is one entry in `clock_provider::enumerate`'s existing
list, the unit is the existing `nanoseconds` token, and the read mode is the
existing `syscall` label every clock counter already carries (R-002).

The marker is a free function, `simulation_start`, in a new public header.
Its body is one extended-assembly statement emitting the eight-byte Intel
SDE SSC marker, `BB imm32(LE) 64 67 90`, with RBX named in the clobber list
and no memory clobber (R-008, R-010). The sequence's fidelity is invisible at
run time, so a gate reads the compiled object instead: a shell script
modeled on the one the TSC counter already ships compiles the marker
translation unit at `-O2` for each available compiler crossed with each
contract-enforcement setting, disassembles it, and asserts the marker window
appears exactly once, after planting a wrong sequence to prove the detector
bites (R-011).

Two lane shapes, one per story, and no shared foundation. The counter lane
touches five files, the marker lane seven, and two files sit outside both.
Every task writes one file, or one file plus the registration that names it.
The whole graph is nine tasks. Five dependency edges join the eight build
tasks, and one closing task fans in from four of them, as listed in the Task
Decomposition section below.

## Technical Context

**Language/Version**: C++23 (`CMAKE_CXX_EXTENSIONS=OFF`). The marker
statement is spelled `__asm__`, since `asm` is not a keyword under a strict
`-std=c++23` and the project builds with extensions off.

**Primary Dependencies**: no dependency change. libc through `<ctime>` for
the clock read, the existing DBC facility for contracts, and binutils
`objdump` inside the gate script, which the suite already invokes
(`test/counters_tsc_read_shape.sh`, `tools/dbc/asm_smoke.sh`).

**Storage**: none; no schema, no persistent state, no runtime configuration.

**Testing**: CTest, extended by two test executables and one gate script.
No test framework is added; the hand-rolled `check()`/`fail()` convention in
`test/source/` continues.

**Target Platform**: Linux on x86-64. The marker emits its sequence under
`#if defined(__x86_64__) || defined(__i386__)`, matching the guard the TSC
counter's test already uses, so a future port compiles and returns.

**Project Type**: C++ library with a CMake build, adding public API.

**Performance Goals**: at or below 20 ns per read at p50 and 25 ns at p99
over 200,000 consecutive reads on the reference platform, measured under
the convention research.md R-006 fixes. Measured on the AMD Ryzen 9
9950X3D running Linux 7.2.4-1-cachyos: p50 20.00 ns, p99 29.77 ns
uncorrected, 7.36 ns of that being the bracketing harness.

**Constraints**: no new runtime dependency, build option, preset, or
external tool (FR-032); no change to any preprocessor branch preserved for a
future port, and no change to any build configuration file read as the cache
options, presets, toolchain inputs, and vendored-ingestion modules
(FR-033, research.md R-012), with the maintainer's 2026-10-03 directive that the build files themselves are editable; no provider, registration macro, dispatch
table, or counter base class (FR-034); every changed line traces to a
requirement (FR-035); 100 percent line, branch, and contract coverage on the
added code (FR-036).

**Scale/Scope**: fourteen files touched, five of them new. Two new public
declarations, one new catalog leaf, one new gate, one documentation row.

## Constitution Check

*GATE: evaluated before Phase 0 research and re-evaluated after Phase 1
design.*

### Pre-research evaluation

| Principle | Gate | Verdict |
|-----------|------|---------|
| I | Standard-first | PASS. C++23, no compiler extension beyond the `__asm__` spelling that strict ISO mode requires, no new library. The marker's register preservation rides on the compiler's clobber obligation. Hand-written assembly adds a second block and a second suppressed statement, which the pinned Core Guidelines baseline does not ask for. The analyzer suppression FR-021 anticipates is written with its reason on the same line, as X.2 requires. |
| II | Design By Contract | PASS with one recorded deviation, in Complexity Tracking below. The new declaration carries doxygen `\pre none` and `\post none`, the project's documented form for a contract with nothing to assert, and the doc-persistence gate accepts it. The deviation is that no `SG_ENSURE` accompanies the declaration, with the reason recorded there and in research.md R-010. |
| III | R-DCUT | PASS. spec, plan, research, data model, contracts, quickstart, then tasks. TDD is in use for the counter lane and recorded as such; the marker lane is implementation-first for the reason in the Task Decomposition section. |
| IV | Documentation | PASS. The public header carries descriptive text plus the tag's value, the tracer option that consumes it, and the byte order it expects (FR-014, FR-019). Two non-obvious facts are recorded next to the code, so a reader meets them there: the emitted sequence's lack of architectural effect, and the absence of a memory barrier with the reasoning attached. No content-free comment is added. |
| V | Style and formatting | PASS. Every new file is inside the formatter's glob (`cmake/lint.cmake`), so `format-check` covers them with no configuration change. No formatting-only change is bundled with content. |
| VI | Test-Backed Code and Coverage | PASS with one recorded split, research.md R-007. Tests arrive with the code they cover. The sixty-second rate comparison of SC-006 leaves the suite, because VI requires tests fast enough for every CI job; it becomes a command in quickstart.md, the split the specification already uses for the Intel SDE confirmation. Coverage needs no configuration change: `cmake/coverage.cmake` already extracts `include/speedgun-ng/*` and `source/*`, so added files are measured on the next run. |
| VII | Performance Discipline | PASS. The counter is a designated critical path, so it gets a cost distribution reported per the constitution's min, median, and tail shape and a published row in the same table format as its siblings. The baseline-infrastructure deferral recorded in the constitution's Open deferrals is untouched, and this feature adds no second platform threshold file. |
| VIII | CI Quality Gates | PASS. Every gate runs already: build and test on the developer and CI presets, sanitizers, clang-tidy and cppcheck through the compile launchers, `format-check`, `spell-check`, `prose-lint`, the coverage target, and the `dbc-gate` target. No gate is added, removed, or weakened, so no amendment is required. |
| IX | Spec-driven development | PASS. The change touches public API, so the workflow runs. One release-configuration build with `cmake --preset=ci-ubuntu` then `cmake --build build` closes the feature, per IX's per-feature clause. |
| X | Anti-slop discipline | PASS. The counter is a leaf in an existing list. The marker is one statement. The gate copies a script that already exists, and introduces no mechanism. No abstraction, extension hook, or configuration value serves a single caller. Assumptions are recorded in research.md with the candidates each one displaced. |
| XI | Prose standards | PASS, verified mechanically by `prose-lint` over this feature's artifacts. The em-dash code point, the ASCII `--` and `---` in prose, the contrastive framings, the voucher list, and the filler and marketing lists are all absent from the four artifacts. |

**Gate weakening requested**: no. The gate set is unchanged.

### Post-design re-evaluation

| Principle | Change after design | Verdict |
|-----------|--------------------|---------|
| I | The marker body was settled during design at one extended-assembly statement with a register clobber, after Intel's own macro shape was read. Intel's shape moves the value into a second register and back; the clobber list discharges the same obligation with less code and no second block. | PASS, research.md R-010 |
| II | The contract annotations were settled at `\pre none` and `\post none`, and the properties that carry content are discharged by a compile-time assertion, a codegen gate, and a runtime test instead. The Verification Matrix records which gate proves each one. | PASS with the deviation in Complexity Tracking |
| VI | The coverage wiring needed no change, and the empty non-x86 arm of the marker needs an exclusion marker in the spelling the project already uses at `source/counters/clock_provider.cpp`. | PASS |
| X | The design phase found the task shape wrong on first pass. Splitting the marker header from its source and splitting the two marker test registrations into two tasks would have added two edges on one file. Merging each pair back removed both edges at the cost of a wider task, which is the trade the Task Decomposition section makes explicit. | PASS, correction recorded in the Task Decomposition rules |
| XI | The gate-coupling sentence in the header prose was rewritten during the self-check, because the first wording used a contrastive framing. | PASS |

No violation requires an amendment. One deviation from a NON-NEGOTIABLE
principle is recorded in Complexity Tracking.

## Project Structure

### Documentation (this feature)

```text
specs/011-nanosecond-counter-ssc-mark/
├── spec.md                              # /speckit.specify output
├── plan.md                              # This file (/speckit.plan output)
├── research.md                          # Phase 0 output: R-001..R-013, SF-001..SF-005
├── data-model.md                        # Phase 1 output: entities and the catalog placement
├── quickstart.md                        # Phase 1 output: validation commands and outcomes
├── contracts/
│   ├── simulation-start.md              # Phase 1 output: the marker byte and API contract
│   └── monotonic-raw-counter.md         # Phase 1 output: the catalog leaf contract
├── checklists/
│   └── requirements.md                  # /speckit.specify output
└── tasks.md                             # Phase 2 output (/speckit.tasks; not created here)
```

### Source Code (repository root)

Files this feature adds:

```text
include/speedgun-ng/simulation.hpp   # public: simulation_start(), the tag constant
source/simulation/marker.cpp         # the marker statement and its compile-time guards
test/source/counters_clock_raw_test.cpp  # catalog resolution, unit, monotonicity, cost
test/source/simulation_test.cpp      # register preservation, repeated calls
test/simulation_mark_shape.sh        # codegen gate: compile, disassemble, count, probe
```

Files this feature edits:

```text
source/counters/clock_provider.cpp       # address table, parse case, read case, seed, reader
include/speedgun-ng/counters_clock.hpp   # fifth seeded leaf, two sample-ordering guarantees
test/source/counters_overhead.cpp        # measure the new counter's sampling cost
test/counters_header_purity.sh           # widen the scanned set to the new header
test/CMakeLists.txt                      # two test targets and one gate registration
CMakeLists.txt                           # one target_sources line for the marker unit
docs/pages/counters-overhead.md          # one row per build table, one raw trial line
test/consumer/main.cpp                   # simulation header, one marker call
example/counters_standalone_example.cpp  # one call at a region boundary
```

Deliberately untouched: every preprocessor branch the project preserves for
a future port; `read_mode` and `unit` in `include/speedgun-ng/counters_core.hpp`,
which are closed vocabularies FR-002 forbids extending; the counters
registration, selection, and read-dispatch machinery; the existing
`machine/monotonic` counter and its reader; `CMakePresets.json` and
`CMakeUserPresets.json`; and every file under `specs/001` through `specs/010`.

## Design

### Logical view: sampling the new counter

```text
caller                system              clock_provider        libc          vDSO
  |  object("machine")    |                    |                  |            |
  |---------------------->|                    |                  |            |
  |  counter("monotonic_raw")                  |                  |            |
  |---------------------->| resolve_leaf_core  |                  |            |
  |                       |------------------->|                  |            |
  |  counter handle       |                    |                  |            |
  |<----------------------|                    |                  |            |
  |  read()                |                    |                  |            |
  |------------------------------------------------> monotonic_raw_ns()
  |                                                     | clock_gettime(CLOCK_MONOTONIC_RAW)
  |                                                     |----------------->|
  |                                                     |                  | seqlock read
  |                                                     |                  | rdtsc_ordered
  |                                                     |                  | mult/shift
  |                                                     |<-----------------|
  |                       |                    | tv_sec * 1e9 + tv_nsec (integer)
  |  cumulative ns        |                    |                  |            |
  |<------------------------------------------------|
```

The only new state is the leaf record the existing catalog machinery already
stores. `machine/monotonic_raw` is a sibling of `machine/monotonic`,
`machine/thread_cpu`, `machine/process_cpu`, and `machine/tsc`, and it is the
fifth leaf on a root that `push_provider::enumerate` also seeds, so the name
must stay clear of the names a user declares through
`push_provider::add_counter`.

### Logical view: the marker call

```text
caller                     simulation_start()              tracer
  |  simulation_start()       |                              |
  |-------------------------->|                              |
  |                           | read RBX into a local        |
  |                           | __asm__ volatile(            |
  |                           |   "movl $0xFACE, %%ebx"      |
  |                           |   ".byte 0x64, 0x67, 0x90"    |
  |                           |   : : : "ebx" )              |
  |                           | restore RBX from the local  |
  |                           | return                      |
  |<--------------------------|                              |
  |                           |                              |
  |                           | bytes BB CE FA 00 00 64 67 90 reach the
  |                           | decoder as one matched window
```

The function has no arguments, no return value, no allocation, no I/O, and
no state. Every general-purpose register is bit-identical across the call,
because the compiler saves and restores a clobbered callee-saved register.

### Physical view: change per file

| File | Change | Requirements |
|------|--------|--------------|
| `source/counters/clock_provider.cpp` | one address string in the address table, one case in the address parser, one case in the read switch, one reader function, one entry in the seed list | FR-001, FR-003, FR-004, FR-005, FR-034 |
| `include/speedgun-ng/counters_clock.hpp` | the fifth seeded leaf named in the class documentation, the two sample-ordering guarantees | FR-001, FR-006, FR-007 |
| `include/speedgun-ng/simulation.hpp` | exported free function, exported tag constant, doxygen with `\pre none` and `\post none`, the tag's value with the tracer option and byte order, the marker's three documented properties | FR-011, FR-013, FR-014, FR-019, FR-020, FR-029, FR-030 |
| `source/simulation/marker.cpp` | architecture guard, one `static_assert` on the tag's width, one extended-assembly statement with the register clobber and the analyzer suppression's written reason, the reasoning for the absent memory clobber | FR-012, FR-015, FR-016, FR-017, FR-018, FR-021, FR-022 |
| `CMakeLists.txt` | one `target_sources` line naming the marker unit | FR-028, FR-032 |
| `test/source/counters_clock_raw_test.cpp` | resolution by canonical address, unit token, read-mode label, non-decreasing samples on one thread, the platform resolution bound, a cross-thread pass under a join, a cost distribution over ten million reads | FR-001, FR-002, FR-006, FR-007, FR-008, FR-036 |
| `test/source/counters_overhead.cpp` | the new counter measured by the harness that produces the published figures | FR-008, FR-010 |
| `test/source/simulation_test.cpp` | a known value with the upper 32 bits set into RBX, the call, a bit-identical assertion; repeated calls terminating | FR-026, FR-027, FR-036 |
| `test/simulation_mark_shape.sh` | the compiler by contract-semantic matrix, the disassembly, the exactly-once assertion, the planted-wrong-sequence probe, the skip path | FR-023, FR-024, FR-025 |
| `test/CMakeLists.txt` | two `add_executable` blocks and three `add_test` calls | FR-023, FR-029, FR-036 |
| `test/counters_header_purity.sh` | the new header added to the scanned set, and a scan proving the widened set bites | FR-009, FR-029 |
| `test/consumer/main.cpp` | the new public header compiled by the downstream consumer, the marker call | FR-028, SC-009 |
| `docs/pages/counters-overhead.md` | one row per build table in the existing column format, one raw trial line | FR-010 |
| `example/counters_standalone_example.cpp` | one call at the region boundary, outside the timed window | User Story 1 independent test |

### Verification Matrix

FR-031 requires the plan to record which gate proves each documented
contract. Every row names the gate that proves it, and rows whose gate is a
reviewer carry that word so the gap stays visible.

| Contract | Gate that proves it |
|----------|--------------------|
| FR-001 catalog leaf at `machine/monotonic_raw` | `counters_clock_raw_test`, resolution by canonical address |
| FR-002 unit token and read-mode label, no new enumerator | `counters_clock_raw_test`, plus `counters_core.hpp` left unedited |
| FR-003 read served without a system call | `counters_clock_raw_test` observes the read; the vDSO route is research.md R-004, and FR-010 publishes the read-path cell |
| FR-004 integer conversion, no floating point | source inspection under the pinned warning set, which rejects an implicit narrowing, plus the rate agreement in quickstart.md |
| FR-005 published on every supported build | `counters_clock_raw_test` resolves the leaf with no architecture guard |
| FR-006 sample never decreases against the previous same-thread sample | `counters_clock_raw_test`, one thread |
| FR-007 sample never decreases against an earlier sample on another thread | `counters_clock_raw_test`, threads joined before comparison, per research.md R-005 |
| FR-008 measurable without an oracle | `counters_clock_raw_test`, monotonicity, resolution bound, and the cost distribution; the clause matching the distribution to the published figure is reviewer-checked against the page's published row, using the figures `counters_clock_raw_test` and `counters_overhead` print, because `docs/pages/counters-overhead.md` states that its numbers are no CI gate and Principle VII records per-platform baseline infrastructure as an open deferral |
| FR-009 platform vocabulary out of public headers | `counters_header_purity`, widened set |
| FR-010 overhead row and read-path cell | `counters_overhead` prints the figures; the page states it is not a CI gate, so the row is reviewer-checked against the printed run |
| FR-011 signature: no arguments, no return, `noexcept` | the compiler, from `simulation_test`'s call site and the header |
| FR-012 the two-instruction marker sequence | `simulation_mark_shape`, the eight-byte window exactly once |
| FR-013 tag `0xFACE` exposed as a named constant | `simulation_test` asserts the constant's value; `simulation_mark_shape` asserts the encoded bytes |
| FR-014 tag value published with the tracer option and byte order | reviewer, plus `prose-lint` and `spell-check` on the header |
| FR-015 full 64-bit register preserved | `simulation_test`, the upper-32-bits-set case; the compiler's clobber obligation is the mechanism |
| FR-016 tag within the unsigned 32-bit range | the `static_assert` in the marker unit, checked at compile time in every build |
| FR-017 no memory-ordering constraint, reasoning recorded | reviewer; no gate inspects a clobber list, and the plan records that |
| FR-018 empty body where no marker instruction exists | no supported build exercises it; the arm carries an exclusion marker in the project's existing spelling |
| FR-019 marker's documented properties | reviewer, plus `prose-lint` and `spell-check`; the doc gate reads `\pre` and `\post` only |
| FR-020 no caller-invoked macro, no runtime tag | reviewer; the header declares a function and a constant, and the shape gate compiles the marker unit |
| FR-021 suppression justified on the same line | reviewer, against the analyzer report from the `test` job's build log |
| FR-022 no memory clobber, reasoning recorded | reviewer; the codegen gate's byte window would survive a barrier, so no gate covers this |
| FR-023 gate compiles, disassembles, counts once | `simulation_mark_shape` is its own verdict |
| FR-024 the gate fails on a wrong sequence | `simulation_mark_shape`'s planted-sequence probe |
| FR-025 skip and success with no marker instruction set | `simulation_mark_shape`'s compiler-target skip path |
| FR-026 register bit-identical across the call | `simulation_test` |
| FR-027 repeated calls terminate | `simulation_test` |
| FR-028 no tracer library, no tracer tool | the link manifest audit in the `test` job, plus no tracer token anywhere in the tree |
| FR-029 scan covers the new header | `counters_header_purity`, plus a widened-set probe |
| FR-030 contracts documented and paired | `cmake --build build/dev -t dbc-gate` |
| FR-031 compile-time assertion where nothing is observable | the `static_assert`, recorded in this table |
| FR-032 no dependency, option, preset, or external tool | diff review; the gate's tools are already invoked by the suite |
| FR-033 no preserved branch and no build configuration change | diff review, with the reading recorded in research.md R-012 |
| FR-034 no provider, macro, dispatch table, or base class | diff review |
| FR-035 every changed line traces to a requirement | the Physical view table above, one requirement column per file |
| FR-036 line, branch, and contract coverage at 100 percent | `cmake --build build/coverage -t coverage` |

### Test Plan

| Level | Test | Requirement | Command |
|-------|------|-------------|---------|
| unit | catalog resolution, unit token, read-mode label | FR-001, FR-002 | `ctest --preset=dev -R counters_clock_raw` |
| unit | samples never decrease, one thread | FR-006 | same |
| unit | samples never decrease across a join | FR-007 | same |
| unit | no observed step is finer than the resolution the platform reports | FR-008 | same |
| unit | cost distribution over ten million reads | FR-008 | same, and the printed distribution |
| unit | register bit-identical with the upper 32 bits set | FR-026 | `ctest --preset=dev -R simulation` |
| unit | repeated calls terminate | FR-027 | same |
| codegen | eight-byte marker window exactly once, per compiler and per contract setting | FR-012, FR-023 | `ctest --preset=dev -R simulation_mark_shape` |
| codegen | planted wrong sequence fails the gate | FR-024 | same, probe branch |
| codegen | compiler targeting no marker set skips and exits zero | FR-025 | same, skip branch |
| vocabulary | the new public header carries no platform term | FR-009, FR-029 | `ctest --preset=dev -R counters_header_purity` |
| contract | every documented contract paired with its enforcement | FR-030 | `cmake --build build/dev -t dbc-gate` |
| documentation | the new counter's overhead row exists in the existing format | FR-010 | `ctest --preset=dev -R counters_overhead -V` |
| coverage | 100 percent line and branch on the added code | FR-036 | `cmake --build build/coverage -t coverage` |
| local | rate agreement within 20 ppm over sixty seconds | SC-006 | quickstart.md, sixty-second command |
| local | trace starts at the marked region under Intel SDE | SC-001 | quickstart.md, Intel SDE command |

## Task Decomposition

The tasks artifact is generated from this section. Its shape decides how much
of the graph runs at once, so the rules are stated here, leaving the
generator nothing to infer.

### Rules the generator applies

1. One task writes one file. The single exception is a file plus the
   registration that names it, which merge into one task because the
   registration cannot be verified without the file.
2. Two tasks marked parallel never edit the same file.
3. Every task names its verify command, and the command's expected result is
   binary.
4. Every task carries the requirement identifiers it discharges.
5. There is no foundational phase. The graph starts at width three, and no
   task blocks a lane it does not belong to.
6. Tests precede their implementation inside a lane, so the first
   verification of a test task is a failure with a named assertion.
7. The order of task identifiers is a topological order of the edge list.

### Work units

Counter lane, TDD, three tasks:

| Task | File | Verify |
|------|------|--------|
| W1 | `test/source/counters_clock_raw_test.cpp`, registered in `test/CMakeLists.txt` | `ctest --preset=dev -R counters_clock_raw` fails, naming the unresolved address |
| W2 | `source/counters/clock_provider.cpp` | `ctest --preset=dev -R counters_clock_raw` passes; the whole suite stays green |
| W3 | `test/source/counters_overhead.cpp` and `docs/pages/counters-overhead.md` | `ctest --preset=dev -R counters_overhead -V` prints the new row, and the published table matches the printed figures |

Marker lane, implementation-first, four tasks:

| Task | File | Verify |
|------|------|--------|
| M1 | `include/speedgun-ng/simulation.hpp` | `cmake --build build/dev -t dbc-gate` reports zero gaps; `format-check` passes |
| M2 | `source/simulation/marker.cpp`, registered in `CMakeLists.txt` | `cmake --build --preset=dev` compiles and links the unit; the analyzer build reports the suppression with its reason |
| M3 | `test/source/simulation_test.cpp`, `test/simulation_mark_shape.sh`, both registered in `test/CMakeLists.txt` | `ctest --preset=dev -R simulation` and `ctest --preset=dev -R simulation_mark_shape` both pass |
| M4 | `example/counters_standalone_example.cpp` | `ctest --preset=dev -R counters_standalone_example` passes |

One task outside both lanes:

| Task | File | Verify |
|------|------|--------|
| X1 | `test/counters_header_purity.sh` | `ctest --preset=dev -R counters_header_purity` passes, and a temporary token planted in the new header makes it fail |

Closing task:

| Task | File | Verify |
|------|------|--------|
| X2 | none; the release-configuration build and the full gate sweep | `cmake --preset=ci-ubuntu` then `cmake --build build`, `ctest --preset=dev`, the `dbc-gate` and `coverage` targets, and `cmake -P cmake/prose-lint.cmake` |

### Edges

```text
W1 -> W2 -> W3
M1 -> M2 -> M3
         M2 -> M4
X1 (no edge)
W3, M3, M4, X1 -> X2
```

Five edges over the eight build tasks, plus the closing task's fan-in from
W3, M3, M4, and X1. The three tasks with no incoming edge, W1, M1, and X1,
run first and touch four different files, so all three start together.

### Why the marker lane is implementation-first

W1's test compiles and links against the existing catalog, so writing it
first produces a red test, which is TDD. M3's runtime test cannot: a test
translation unit that declares a call to an undefined function fails at link
time, and that link error takes the whole suite's build down with it, so no
other test runs at the intermediate commit. The constitution's Pull Request
Quality section requires every commit to compile and pass its tests so that
history stays bisectable. The marker lane therefore implements first, and the
gate script in M3 still observes red before green, because it compiles the
marker unit itself and finds no marker window until the statement exists.

### Two merges, and what they cost

M3 carries two new files and one registration edit. The alternative splits
the runtime test and the gate script into two tasks, and both edits land in
`test/CMakeLists.txt`, so the split adds an edge between two tasks that touch
one file and can never run in parallel. The merge is recorded here because it
is the one place the plan chooses a wider task over a shallower graph.

X1 stays independent of M1 even though the widened scan is most meaningful
once the new header exists. Widening the scanned set is a change to a test
script, verifiable on its own with a temporary violation, so the edge it would
add buys nothing.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Principle II's rule that every implementation enforce its documented contracts at run time, for `simulation_start` only | The function takes no argument and returns nothing, so its precondition is empty and its only postcondition is that the call leaves every general-purpose register bit-identical. Asserting that needs a second assembly block to read RBX at all, which adds a second suppressed statement where FR-021 anticipates one, and it asserts a property the ABI already guarantees: a register named in an extended-assembly clobber list is saved and restored by the compiler. The specification anticipated this case, and FR-031 directs the plan to record which gate proves each remaining property. | An `SG_ENSURE` comparing RBX across the statement. It adds assembly the marker does not need, a suppression the specification did not budget for, and coverage of a path the compiler guarantees. The property is already asserted by the runtime test FR-026 requires, with the upper 32 bits set, which is the case a naive implementation fails. |