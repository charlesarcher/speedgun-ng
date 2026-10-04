# Research: Nanosecond Counter and Simulation-Start Marker

**Feature**: `specs/011-nanosecond-counter-ssc-mark/spec.md`
**Date**: 2026-10-03

Every decision below names the evidence it rests on. Measured figures carry
the machine and the method that produced them. Where the specification and
the platform disagree, the disagreement is recorded with the candidates and
the chosen reading, per Principle X.1.

Eight items need a decision before Phase 1 can close: the clock source, the
read-mode label, the conversion arithmetic, the fast-path mechanism, the
cross-thread monotonicity claim, the marker byte sequence, the marker's
register handling, and the codegen-gate mechanism. Four specification
items conflict with the constitution or with each other and are recorded as
specification findings: the FR-033 build-configuration clause, the FR-007
cross-thread postcondition, the SC-004 and SC-006 measurements, and the
marker's provenance. The maintainer resolved all four on 2026-10-03, in favor
of editing the build files and of the corrected wording each finding
carries.

## Measurement Core

### R-001 The clock source is `CLOCK_MONOTONIC_RAW`, read through libc

**Decision**: the counter reads `clock_gettime(CLOCK_MONOTONIC_RAW, ...)`
through `<ctime>`, the same entry point the existing `machine/monotonic`
counter uses for `CLOCK_MONOTONIC`.

**Rationale**: the kernel documents the rate of this clock directly.
`Documentation/core-api/timekeeping.html` states that `ktime_get_raw()` maps
to `CLOCK_MONOTONIC_RAW` and runs "at the same rate as the hardware
clocksource without (NTP) adjustments for clock drift". The code confirms
the documentation: `tkr_raw.mult` is written once at clocksource install,
and the NTP slew path adjusts `tkr_mono.mult` only, so no offset term
reaches the raw read path. The clock identifier is `4` in
`include/uapi/linux/time.h` and `CLOCK_MONOTONIC_RAW 4` in glibc's
`sysdeps/unix/sysv/linux/bits/time.h`. Reading it through libc keeps the
feature inside the platform vocabulary rule of FR-009, since the identifier
then appears in `source/counters/clock_provider.cpp` alone.

**Alternatives considered**: `CLOCK_BOOTTIME` also runs at the hardware rate
and keeps counting across suspend. FR-003 asks for "nanoseconds since the
platform clock's epoch" and SC-006 compares the new counter against
`machine/monotonic` over a sixty-second interval, a comparison suspend
would invalidate. `CLOCK_MONOTONIC` is already published, which is the
premise of User Story 2.

### R-002 The read-mode label reuses `read_mode::syscall`

**Decision**: the new counter reports `read_mode::syscall`, adding no
enumerator.

**Rationale**: the Clarification of 2026-10-03 answers this question
directly, and FR-002 forbids a new enumerator in either closed vocabulary.
`read_mode` lives in `include/speedgun-ng/counters_core.hpp:93` and carries
exactly `fast_tsc`, `fast_rdpmc`, `syscall`, `push_load`. No enumerator names
the vDSO, and the existing clock leaves already carry `syscall` while glibc
serves them without a system call, so the new leaf is consistent with its
siblings. FR-010 records the fast path in the published table, which is
where a reader looks.

**Alternatives considered**: adding a vDSO enumerator would edit a closed
vocabulary that three other providers read from
(`source/counters/fold.cpp:207`, `source/counters/linux_pmu/provider.cpp:125`,
`source/counters/linux_pmu/group_io.cpp:416`) and would break FR-002.

### R-003 The conversion is integer arithmetic on a 64-bit type

**Decision**: `static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL +
static_cast<std::uint64_t>(stamp.tv_nsec)`, the expression the three
existing clock readers already use.

**Rationale**: FR-004 asks for integer arithmetic with no floating-point
step, and `source/counters/clock_provider.cpp:82` is that arithmetic for
`CLOCK_MONOTONIC` already. The 64-bit product cannot overflow for any
`time_t` the platform produces, since the seconds field stays far below
2^34 for any plausible uptime.

**Alternatives considered**: `std::chrono::nanoseconds` from
`std::chrono::seconds` would multiply two durations and reach the same
value with more template instantiations on a path FR-003 keeps cheap.
`__int128` buys nothing at this width.

### R-004 The fast path is the vDSO, and this feature does not call it directly

**Decision**: the read goes through libc's `clock_gettime` and this feature
adds no direct vDSO symbol call.

**Rationale**: glibc resolves `clock_gettime` to the vDSO symbol
`__vdso_clock_gettime64` at startup and only falls back to
`INTERNAL_SYSCALL_CALL` when the lookup returns null
(`sysdeps/unix/sysv/linux/dl-vdso-setup.h`). The kernel's vDSO keeps a
dedicated data slot for this clock, `vc[CS_RAW]`, read under the vvar
seqlock and converted with `ns = ((cycles - cycles_last) * mult + base) >>
shift` (`lib/vdso/gettimeofday.c`). The Out of Scope section already places
direct calls to the platform's published time symbols outside this feature,
and measured headroom over the current path is one to two nanoseconds.

The fallback is documented, and no mechanism prevents it. Three conditions push the
read onto a real system call: the `vdso=0` boot parameter leaves
`AT_SYSINFO_EHDR` unmapped, so the glibc lookup returns null
(`Documentation/admin-guide/kernel-parameters.txt`, "vdso=0: disable VDSO
mapping"); a clocksource whose `vdso_clock_mode` is
`VDSO_CLOCKMODE_NONE` leaves the raw vvar slot unfilled
(`kernel/time/vsyscall.c`); and on x86 the counter read then returns
`U64_MAX`, which `arch_vdso_cycles_ok` rejects, sending
`__cvdso_clock_gettime_data` to `clock_gettime_fallback`
(`lib/vdso/gettimeofday.c`). FR-005 asserts the counter publishes on every
supported build, which holds under all three conditions; the read-path caveat
belongs in FR-010's table cell, where the specification puts it.

**Alternatives considered**: a `SG_COUNTERS_*` opt-out for a kernel without
the vDSO would add a build option, which FR-032 forbids.

### R-005 Cross-thread monotonicity is conditional, and the documentation says so

**Decision**: FR-007's postcondition is documented with its condition
stated: a sample is greater than or equal to any earlier sample whose
completion happens-before its own start.

**Rationale**: the platform states the per-reader guarantee and nothing
wider. `man 2 clock_getres` states that all `CLOCK_MONOTONIC` variants
guarantee that consecutive calls never go backwards, allowing equal values.
The kernel's clamp is per reader: `timekeeping_cycles_to_ns` returns the
last accumulated nanoseconds unchanged when the delta's mask has its
high bit set, which holds the value for that reader and says nothing about
another processor. The vDSO applies the same rule per conversion period. No
kernel or manual statement promising a cross-processor total order was
found for this clock. Cross-processor comparability on x86 rests on the
synchronized TSC and on `clock->max_cycles`, and a kernel that distrusts its
counter answers with `mark_tsc_unstable` and a clocksource switch.

FR-007 as first written promised an unconditional cross-thread order, and the
clarification session of 2026-10-03 now conditions it on the earlier sample's
completion happening-before the later sample's start. Recording the guarantee
with its condition keeps the contract true, which matters more than matching
the original sentence, because a false contract is the failure mode Principle
II exists to prevent.

**Alternatives considered**: documenting the unconditional claim would make
a benchmark that migrates threads able to record a decreasing sample and
attribute the decreasing sample to the library. The platform is where that
guarantee fails.
Dropping FR-007's cross-thread clause entirely would understate what the
library can promise once a synchronization edge exists.

### R-006 SC-004 needs a stated measurement convention

**Decision**: the per-read cost is measured as one counter read bracketed by
two timestamp-counter reads, with the harness overhead measured under the
identical bracketing and reported beside the figure. Both numbers are
recorded in [quickstart.md](quickstart.md); the gate threshold is judged
against the overhead-corrected figure, and the uncorrected figure is
published beside it.

**Rationale**: SC-004 sets 20 ns at p50 and 25 ns at p99 and says the
measurement uses "the same method used for the existing clock counters'
published figures". The existing published figure is a per-sampling-action
number that includes the library's own machinery:
`docs/pages/counters-overhead.md` records `machine/monotonic` at 40 ns
median and 80 ns maximum per sampling action. SC-004 asks for a per-read
cost, which is a different quantity measured by a different loop.

Measured on the reference platform named in SC-004, an AMD Ryzen 9 9950X3D
running Linux 7.2.4-1-cachyos with `current_clocksource=tsc`, 200,000
samples per row, process pinned to one processor, timestamp-counter rate
calibrated against `CLOCK_MONOTONIC` over 50 ms:

| read | resolution | avg | p50 | p90 | p99 | max |
|------|-----------|-----|-----|-----|-----|-----|
| `CLOCK_MONOTONIC_RAW` | 1 ns | 21.03 | 20.00 | 20.23 | 29.77 | 1300.22 |
| bracketing overhead | n/a | 7.36 | 9.77 | n/a | 10.00 | n/a |

All figures include the bracketing overhead, which is what the bracketing
measures. The uncorrected p50 of 20.00 ns meets SC-004's 20 ns bound and the
uncorrected p99 of 29.77 ns exceeds its 25 ns bound; net of the measured
7.36 ns overhead the p99 falls near 22 ns. A second run on a busier machine
returned p50 30.00 ns and p99 30.00 ns, which is why the convention has to
be stated, leaving the reader of the number nothing to assume. The platform's own
resolution is the kernel-wide timer mode flag, `hrtimer_resolution`, which
`hrtimer_switch_to_hres` sets to `HIGH_RES_NSEC`, literally `1`
(`kernel/time/hrtimer.c`). `clock_getres` reports that flag. It is a
kernel-wide value and no clocksource property enters it, so the 1 ns figure
holds for `tsc` and `kvm-clock` alike. The single-digit-parts-per-million rate difference the specification
expects between this counter and `machine/monotonic` is consistent with the
slew limit of 500 ppm the research surfaced, at an observed rate far below
it.

A published figure for the same read exists: `tycho/clockperf` reports
`monotonic_raw` at 73.34 ns against `monotonic` at 23.40 ns on the same
`tsc` clocksource under Linux 4.5.0. That three-fold gap could not be
derived from the source and is treated as a property of that tool rather
than of x86-64. Its `hpet` rows are the informative ones for R-004:
`monotonic_raw` costs 618.30 ns when the clocksource is memory-mapped,
which is the cost of a read the vDSO cannot serve.

**Alternatives considered**: adopting SC-004's 20/25 ns as the gate against
the uncorrected figure fails on the reference platform for a reason that
belongs to the measuring harness. Adopting the existing per-sampling-action
method would compare a per-read number against a number that includes the
plan machinery, which measures something else.

### R-007 The sixty-second rate comparison is a local confirmation

**Decision**: SC-006's rate agreement over at least sixty seconds is
verified by the command in [quickstart.md](quickstart.md), and the ordinary
suite asserts the properties that are observable in milliseconds.

**Rationale**: Principle VI requires tests to be deterministic and fast
enough to run in every CI job, and the constitution counts a gate weakened
without an amendment as a defect. A sixty-second sleep inside a CTest entry
runs on every job on every push. The specification already uses this split
for User Story 1, where the marker assertion runs in the ordinary suite and
the Intel SDE run stays a local confirmation.

**Alternatives considered**: labeling the sixty-second entry with
`SKIP_RETURN_CODE` would keep it in the suite while removing its verdict from
the run, which is a gate that reports nothing.

## Simulation-Start Marker

### R-008 The marker is eight bytes: `mov` plus a prefixed one-byte no-operation

**Decision**: the emitted sequence is a 32-bit immediate move into the
32-bit view of RBX followed by the three bytes `64 67 90`, giving the
eight-byte pattern `BB imm32(LE) 64 67 90`.

**Rationale**: Intel publishes no byte encoding. Its PinTool Regions
document defines the marker only as "a code sequence consisting of two
instructions, the first has an immediate that identifies this marker" and
calls it "special no-OP instructions built into the binary". The byte
contract therefore comes from two sources that do publish it. Intel C++
Compiler's builtin macro `__SSC_MARK`, mirrored verbatim in Clang's
`clang/lib/Headers/x86gprintrin.h`, emits `mov {%0, %%ebx|ebx, %0}` then
`.byte 0x64, 0x67, 0x90`. Intel's own tracing component matches on the byte
level: the `ALARM_SSC` trace builds `unsigned char ssc_marker[] = {0xbb,
0x00, 0x00, 0x00, 0x00, 0x64, 0x67, 0x90}`, fills bytes 1 through 4 from the
tag with `ssc_marker[1 + j] = (h >> (j * 8)) & 0xff`, and fires when eight
contiguous bytes compare equal.

Verified on this machine before the plan was written, assembling the two
source lines with gcc and reading the object back:

```text
   0:	bb ce fa 00 00       	mov    $0xface,%ebx
   5:	64 67 90             	fs addr32 nop
```

The eight bytes for the tag `0xFACE` are `bb ce fa 00 00 64 67 90`.
`0x64` is the FS segment-override prefix, `0x67` is the address-size
override, and `0x90` is the one-byte no-operation, which is how gcc renders
the second instruction. FR-012 and the Edge Cases entry agree with this
pattern, including their description of the second instruction as a
one-byte no-operation carrying a segment-override prefix and an
address-size-override prefix.

Two sources add bytes that are not part of the marker. Intel's
`intel-ipsec-mb` emits `db 0x64, 0x67, 0x90, 0x90, 0x90`, three no-operation
bytes where the matcher reads one. The matcher's window is eight bytes, so
the trailing bytes sit outside it. The codegen gate asserts the eight-byte
window and ignores any neighboring no-operation, which keeps a padding
change from reading as a marker change and keeps a marker change from
hiding behind padding.

**Alternatives considered**: the IACA-era spelling of the pair, "MOV plus
addr32 fs:NOP", appears in open-source comments and names the same bytes.
The ICC macro's RBX save and restore around the sequence is a portability
measure for its own inline assembly and is handled differently under R-010.

### R-009 The marker name in the request is `__SSC_MARK`; no Intel macro carries the requested name

**Decision**: the feature keeps the public API name `simulation_start` the
specification fixes, and this plan records the provenance correction.

**Rationale**: the request attributes the design to Intel's
`TRACING_SSC_MARK` macro. No Intel source, document, or header defines a
macro by that name. Intel's name is `__SSC_MARK`, an Intel C++ Compiler
builtin that needs no include, confirmed in the LLVM review thread where the
builtin was admitted with the note that ICC includes its intrinsic headers
automatically. The identifier `TRACING_SSC_MARK` appears in exactly one
public project, `pmodels/oshmpi`, whose macro body emits the same bytes
under a local name. Intel's own tracing runtime, the OpenMP runtime's
`kmp_itt.h`, documents the marker's purpose in the words this feature
implements: markers "mark points in instruction traces that represent
spin-loops and are therefore uninteresting when collecting traces for
architecture simulation", emitted as "the instructions necessary to set %ebx
and execute the unlikely no-op".

Two further naming facts belong in the record. Intel publishes no separate
start and stop macros; one macro carries both directions and the tag value
distinguishes them. Intel publishes no option named `-marker` and no
`EMIT_MARK`.

**Alternatives considered**: naming the public macro after the request would
put a third-party identifier into the library's public surface. FR-011 fixes
a free function, which is the shape the request itself preferred.

### R-010 Register preservation is the compiler's clobber obligation

**Decision**: the marker is one extended-assembly statement naming RBX in
its clobber list and no memory clobber. The header documents the resulting
postcondition in prose and declares the doxygen contract empty.

**Rationale**: GCC and Clang save and restore a clobbered callee-saved
register around an extended-assembly statement, so naming RBX in the clobber
list discharges FR-015 without a hand-written save, and every general-purpose
register is bit-identical across the call, which is SC-002. The alternative,
Intel's macro shape, moves the value into another register first and back
afterwards, which duplicates what the compiler already does.

FR-022 forbids a memory clobber and FR-017 asks for the reasoning to be
recorded: an attached tracer observes executed-instruction order directly, so
a memory barrier would constrain the program's own ordering without
observable effect on collection.

The contract annotations are `\pre none` and `\post none`, the escape the
project already documents for a contract with nothing to assert. The
doc-persistence gate requires a precondition and a postcondition on every
in-scope function and accepts `none` as an explicit empty statement. FR-031
covers the remainder: FR-016 is a compile-time assertion on the tag, the
sequence's fidelity is the codegen gate, and register preservation is the
runtime test FR-026 already requires. The tension this creates with
Principle II's rule that every implementation enforce its contracts is
recorded in the plan's Complexity Tracking table, with the reason a
register-comparison check is not added.

**Alternatives considered**: an `SG_ENSURE` comparing RBX before and after
needs a second assembly block to read RBX at all, which adds a second
suppressed statement where FR-021 anticipates one, and it asserts a property
the ABI already guarantees.

### R-011 The codegen gate is a shell script modeled on the one already in the tree

**Decision**: the gate is `test/simulation_mark_shape.sh`, registered with
CTest as one `add_test` block inside the existing Linux-only region of
`test/CMakeLists.txt`. It compiles the marker translation unit itself at a
release optimization level, once per available compiler and once per
contract-enforcement setting, disassembles the object, and asserts the
eight-byte marker window appears exactly once.

**Rationale**: the mechanism exists. `test/counters_tsc_read_shape.sh`
compiles `source/counters/clock_provider.cpp` at `-O2 -std=c++23` for `g++`
and `clang++`, crosses that with `SG_CONTRACTS_SEMANTIC` 0 and 2, requires
the generated export header, disassembles with
`objdump -d --no-show-raw-insn -C`, plants a wrong sequence in a scratch
copy to prove the detector bites, and skips cleanly off x86. FR-023, FR-024,
and FR-025 restate that script's structure. `objdump`, `g++`, and `clang++`
are already implicit dependencies of the suite and of the CI jobs, so
FR-032's ban on new external tools holds with nothing added.

The marker translation unit holds exactly one function, which makes the
in-function count of FR-023 equal to the object-wide count the script
measures. The compiler that targets no marker instruction set is decided by
the compiler, because FR-025 says "a build whose compiler
targets no marker instruction set"; the script asks each compiler for its
target triple and exits successfully with a skip line when none of them
targets x86.

**Alternatives considered**: a second C++-side assertion cannot see its own
machine code. A configure-time or build-target gate would be a new build
option, which FR-032 forbids. Adding the check to
`test/counters_tsc_read_shape.sh` would put a second counter's gate inside
the first counter's gate, which Principle X.3 rejects.

### R-012 Which file registers a new test is a specification conflict

**Decision**: `test/CMakeLists.txt` is treated as the test registry that
FR-023, FR-029, and FR-030 order this feature to extend, and the
build-configuration clause FR-033 is read as covering the cache options,
presets, toolchain files, and vendored-ingestion modules. The maintainer
confirmed on 2026-10-03 that the build files themselves are editable
too, which removes the tension this decision resolves.

**Rationale**: the two clauses collide. FR-033 says no existing build
configuration file changes. FR-023 requires a gate "registered with the
test runner", FR-029 requires the existing vocabulary scan to cover the new
header, and FR-036 requires the coverage gates to measure the new code. A
CTest entry needs a target, and every target in this repository is declared
in `test/CMakeLists.txt` by an explicit `add_executable`, `target_sources`,
and `add_test` block; the only glob in the build covers
`source/counters/*.cpp`, which holds no marker. Under the strict reading of
FR-033, FR-023 is unsatisfiable.

The narrower reading is also the one the repository already practices.
`test/CMakeLists.txt` is the test registry by convention: it carries no
option, no preset, no toolchain setting, and no gate configuration, and the
project's own merged features extend it without amendment. The files FR-033
protects by name in practice are `CMakePresets.json`, the `cmake/` modules,
and the ingestion bracket in `CMakeLists.txt`.

**Alternatives considered**: skipping the CTest registration satisfies
FR-033 and fails FR-023, which makes SC-003 unreachable. The gate ships
unregistered and runs only from a shell, which no CI job would call.

### R-013 The new public header needs the vocabulary scan widened

**Decision**: the new public header is `include/speedgun-ng/simulation.hpp`,
and `test/counters_header_purity.sh` gains it in the scanned set.

**Rationale**: FR-029 says the existing scan is extended to cover the new
public header. The scan globs `--include='counters*.hpp'`
(`test/counters_header_purity.sh:65`), so a header named for what it
declares falls outside the counters subsystem, so today's glob misses it. The
scan looks for `perf_event`, `clock_gettime`, `rdpmc` as a whole word, and
`rdtsc` (`:63`), plus the acronym `PMU` across the five core counter headers
(`:54`). A widening is the change FR-029 asks for and keeps the verdict on
the new header recorded on every run.

The header needs no platform vocabulary to say what it declares. The tag is
a hexadecimal literal, the contract text names no register, and the
prose describes the marker's effect. The register name belongs in the source
file and in this record.

**Alternatives considered**: naming the header `counters_simulation.hpp`
would fit the existing glob and misname the surface, since the declaration is
a free function with no relationship to the counter catalog.

## Specification Findings

Five items need the maintainer's attention. Each is recorded with the
candidates, and this plan carries on with the chosen reading so that
implementation is not blocked.

### SF-001 FR-033 against FR-023, FR-029, FR-030, and FR-036

**Resolved 2026-10-03 by maintainer directive**: the build files are
editable, with nothing in the constitution, the gates, or the project's
conventions prohibiting it. `test/CMakeLists.txt` and the root
`CMakeLists.txt` are in scope for this feature, so the edits T003, T004,
T006, and T009 need proceed. The strict reading of FR-033 is withdrawn, R-012's
narrow reading stands as the record of how the conflict was resolved, and no
task is blocked on this finding.

### SF-002 FR-007 promised an unconditional cross-thread order

**Resolved 2026-10-03**: FR-007 now conditions the cross-thread postcondition
on the earlier sample's completion happening-before the later sample's start,
and it states that the library claims no order between two un-ordered samples
on two threads. User Story 2 carries a fourth acceptance scenario for the
joined-thread comparison, and SC-005 carries the same condition, so the
requirement is testable as written. R-005 carries the evidence.

### SF-003 SC-004 and SC-006 against Principle VI and against the published
figure

**Resolved 2026-10-03**: SC-004 now names its measurement convention, states
that the thresholds judge the overhead-corrected figure, states that the
uncorrected figure is published beside it, and records that the existing
per-sampling-action figures cover a wider quantity and set no threshold here.
SC-006 now states that the sixty-second comparison runs as a local
confirmation recorded in the quickstart and that the ordinary suite asserts
the properties observable in milliseconds. R-006 carries the measured figures
and R-007 carries the placement reasoning.

### SF-004 The request attributes the marker to a macro Intel does not
publish

**Resolved 2026-10-03**: a second clarification session in the specification
records the correct identifier, states that Intel publishes no byte encoding,
and names the two sources the byte contract rests on. The specification's
Input section keeps the user's request verbatim, which is correct as a
quotation, and R-009 carries the provenance with its sources.

### SF-005 The smallest-step clause in FR-008 states a bound the reference
platform cannot satisfy

**Resolved 2026-10-03**: a shipped run reports a resolution of `1` ns against
a smallest observed non-zero step of `70` ns, so a step that stays within the
reported resolution does not occur on the reference platform, where
consecutive samples in a back-to-back sampling loop sit tens of nanoseconds
apart. FR-008, User Story 2 acceptance scenario 3, and guarantee 3 of
`contracts/monotonic-raw-counter.md` now state the granularity bound the test
asserts: no observed step is finer than the resolution the platform reports
for this clock, and a step below the reported figure would mean the counter
reports a granularity it does not have. The assertion and the reasoning for
it are carried at `test/source/counters_clock_raw_test.cpp:179-189`.