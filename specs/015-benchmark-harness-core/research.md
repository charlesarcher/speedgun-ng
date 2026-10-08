# Research: Benchmark Harness Core

Phase 0 output for `specs/015-benchmark-harness-core/spec.md`. Every
Technical Context entry resolved before design; no NEEDS CLARIFICATION
remains. Each record states the decision, the rationale, and the
alternatives considered.

## R-01: the harness build target

**Decision**: a new static archive target `speedgun-ng_harness` with
the alias `speedgun-ng::harness`, inside the `speedgun-ng` project,
installed in the same export set as `speedgun-ng_speedgun-ng`. A suite
links `speedgun-ng::harness`, which links the counters archive.

**Rationale**: FR-049 places the harness in the `speedgun-ng` library
with its public interface under `include/speedgun-ng/`, and the
Library-first entry of Additional Constraints states the same. The
separate archive keeps the counters surface free of harness terms,
which FR-049 also binds, and keeps `getopt_long` in the implementation
of one target (FR-050). D-1 already fixes the static-archive shape, and
the split adds one target name, which sits outside the C++ identifier
set (FR-033).

**Alternatives considered**: adding the harness sources to
`speedgun-ng_speedgun-ng`, rejected because every counters consumer
would carry `speedgunMain` and the runner, and the header-purity and
vocabulary audits would widen for no benefit; a separate project or
package, rejected because D-1 names one link line and the archive
already carries the counters surface.

## R-02: the two-point capture mechanism

**Decision**: one `hardStop` recorder per benchmark, minted with
`Plan::recorder(2 × (96 + 96 + R))` before the first run, where R is
the repetition count. Each run calls `sample()` at the timed-loop
entry and at its exit; the fold for run k reads points `2k` and
`2k+1` through `Expression::fold(recorder.view(), i, j)`.

**Rationale**: D-2 mints one recorder per benchmark and FR-019 fixes
its capacity from the FR-016 run bound before the first run; the
recorder spelling is the one those clauses use. The capacity formula
is the one D-3 derives. `RecorderHandle::sample()` is the documented
zero-allocation, zero-lock action (PC-2), and the fold layer runs all
metric math after the run (FR-018, 007 FR-021).

**Alternatives considered**: a `Scope` per run, rejected because a
scope allocates its two point columns at construction and releases
them at destruction, which contradicts the one-recorder rule of D-2
and puts arena work between runs; a ring recorder, rejected because a
hardStop recorder with the exact capacity reports its bound on
overflow (FR-025 of 007), which turns a sizing bug into a loud
failure.

## R-03: the fake clock substitution (FR-042)

**Decision**: `speedgunMain` registers `ClockProvider` and treats a
refusal naming the machine object as a pass. A test executable
registers its `FakeProvider`, carrying scripted `machine/monotonic`
and `machine/thread_cpu` leaves, before it calls `speedgunMain`; the
real provider is then refused and the scripted leaves drive every
harness decision.

**Rationale**: `FakeProvider::addCounter` auto-creates the machine
object with the machine kind and description
(`source/counters/fake_provider.cpp:145-149`), while
`FakeProvider::addObject` refuses to redeclare the machine root
(`:105-106`), so the fake can own the clock leaves and the real
provider cannot. `System::registerProvider` refuses a duplicate
canonical path and leaves the tree unchanged (007 FR-008), so the
real registration is a clean no-op in that case. FR-042 holds on the
existing surface, and no counters change is needed (FR-041 stays a
route with no route taken).

**Alternatives considered**: a command-line switch that skips the real
clock provider, rejected as configurability serving one caller (X.2); a
counters change that lets a provider replace machine leaves, rejected
because it widens a contract no user asked for.

## R-04: the dimension of an address-attached leaf (FR-021)

**Decision**: split the address at its last `/` into the object path
and the leaf name; read the entry's catalog unit; map it through
`dimensionOf` (`include/speedgun-ng/counters_core.hpp:196`); resolve
with `Object::counter<D>` for the mapped exponent pair. The closed
mapping sends `seconds` and `nanoseconds` to time^1 events^0, and
`bytes`, `ops`, and `none` to time^0 events^1. A leaf whose unit maps
to a pair the harness does not instantiate is a recoverable resolution
failure at the 007 FR-046 tier.

**Rationale**: the catalog already carries the dimension facts, and
`Object::counter<D>` already refuses a dimension mismatch with a
message naming both (007 FR-005). The harness instantiates the two
pairs the closed mapping yields, so no second resolution path enters.

**Alternatives considered**: attaching every address as an
events^1 count, rejected because it breaks on a leaf with a time unit,
which the machine object publishes; a user-supplied dimension flag,
rejected as configurability the catalog already answers.

## R-05: statistics definitions (FR-027, SC-003)

**Decision**: over the repetitions that produced a measured value, the
aggregates are mean, median, sample standard deviation (the `n − 1`
denominator), coefficient of variation (standard deviation divided by
mean), min, and max. A repetition whose run carries a gap contributes
no sample (FR-025).

**Rationale**: the upstream rule read at the D-3 revision uses the
sample form, and the SC-003 fixture is computed from the same
definitions, so the fixture pins the choice. Principle VII names the
min, median, and max form, and the spec adds the mean, standard
deviation, and coefficient of variation beside it.

**Alternatives considered**: the population form with the `n`
denominator, rejected because the upstream fixture values would not
match and the spec pins the upstream reading.

## R-06: SIGINT mechanics (FR-032)

**Decision**: `speedgunMain` installs a `SIGINT` handler that stores
`true` into one `std::atomic<bool>`. A `static_assert` on
`std::atomic<bool>::is_always_lock_free` keeps the store signal-safe.
The runner reads the flag after each warm-up, calibration, and
measured run. The timed loop reads no flag. On a set flag, the runner
marks the current benchmark skipped with the interrupt reason and
starts no later run. The process exits nonzero. The plan and recorder
are RAII objects of the runner frame. The release path of the
exception rule (FR-032, PC-6) discharges them.

**Rationale**: A flag test inside the loop adds a load and a branch to
every iteration of every benchmark. The overhead floor measures the
sampling action alone. The reported time per iteration would carry
that cost uncorrected. The trade is latency: the interrupt waits for
the current run to complete.

**Alternatives considered**: a flag test in the `State` iterator
before each iteration, rejected for the per-iteration cost above;
`setitimer` with a periodic tick, rejected as a second mechanism for
one flag; `signalfd`, rejected because it adds a descriptor to poll
inside a loop the harness does not own.

## R-07: the time-source gate mechanism (FR-040, SC-013)

**Decision**: `test/time_source_gate.sh` scans every C++ source and
header the D-6 scope names: `source/`, `include/`, `example/`,
`test/`, and `tools/`, with the counters library excluded. The
counters library is `source/counters/`, the
`include/speedgun-ng/counters*.hpp` headers, and the `counters_` tests
and gate scripts under `test/`. The script carries one allowance keyed
by path and term: `tools/dbc/overhead.cpp` with the `<chrono>` include
and `std::chrono::steady_clock`. Any other term in that file is a hit.
The script applies the constitutional banned list: the `std::chrono`
clocks, `std::clock`, `std::time`,
`timespec_get`, `clock_gettime`, `clock_getres`, `gettimeofday`,
`time`, `times`, `getrusage`, the `rdtsc` and `rdtscp` instructions
and their intrinsics, and the headers that declare them. A hit prints
`file:line` and exits 1. The script is registered in CTest, so the
`test` job's ctest step runs it as a hard gate, and the quickstart
records the planted-failure and removal runs SC-013 asks for.

**Rationale**: the pattern scan matches the constitutional list
directly, follows the shape of the existing gate scripts
(`test/counters_header_purity.sh` and siblings), and needs no new
tooling. CTest registration is the mechanism Principle VIII already
runs in every job that tests. The 006 overhead measurement times the
contract cost against an independent, well-known reference clock on
purpose. D-6 names it as the one exception. A port onto the counters
library would remove that independence.

**Alternatives considered**: a port of `tools/dbc/overhead.cpp` onto
the counters library, rejected by Charles; a scope that leaves
`tools/` outside the rule, rejected because it opens every future tool
to the banned list; a clang-tidy custom check, rejected
because the project carries no custom analyzer module and the pin is
governance; an include-graph scan, rejected because the list names
spelled sources and instructions, which a pattern scan reaches
directly.

## R-08: the context-line sources (FR-035)

**Decision**: the context lines print the library version and the
build type from build-written macros, and print the host and cpu
fields only where the counters catalog publishes them. A field the
library does not publish stays out of the line, which is the clause
FR-035 states.

**Rationale**: FR-038 binds every measured value in a context line to
the counters library, and the amendment of D-6 bans the direct routes
(`uname`, `sysconf`, `/proc` reads). The catalog at the audit point
publishes no host-name or cpu-model string, so the first release
prints the two build facts and the catalog facts it holds. If a later
need names a model string, FR-041 routes it into a counters change.

**Alternatives considered**: `gethostname` and a `/proc/cpuinfo` read,
rejected because they are time-adjacent platform reads the amendment
route forbids for reported values; omitting the context lines,
rejected because FR-035 mandates them.

## R-09: the registration mechanism (FR-001)

**Decision**: a function-local static registry in
`source/harness/registry.cpp`. `SG_BENCHMARK(fn)` defines a namespace
scope object whose constructor calls the registry before `main`;
`registerBenchmark(callable, name)` reaches the same registry from any
code that runs before the run. The registry keeps insertion order, and
a duplicate name reports at the 007 FR-046 tier, keeps the first
entry, and runs the rest (FR-002).

**Rationale**: FR-001 names both forms, and a static registry is the
only shape a pre-`main` macro can use. The project already relies on
static initialization order in the counters registration tests
(`test/source/counters_registration_order_test.cpp`), so the pattern
is established.

**Alternatives considered**: link-section registration, rejected as a
platform extension with no benefit over a static object; requiring
runtime registration only, rejected because FR-001 mandates the macro.

## R-10: the macro spelling

**Decision**: the registration macro is `SG_BENCHMARK`.

**Rationale**: N-3 requires `UPPER_SNAKE_CASE` under the `SG_` prefix
for a project macro. Google Benchmark's `BENCHMARK` keeps no prefix,
and the harness copies no old spelling (FR-047).

**Alternatives considered**: `BENCHMARK`, rejected because it violates
N-3 and would need a V.2 entry no language or platform lookup
requires.

## R-11: the plan compile set (FR-017, FR-038)

**Decision**: one plan per benchmark compiles the `machine/monotonic`
counter, the `machine/thread_cpu` counter, and every metric
expression of the benchmark through `sg::counters::compile`. The
availability of each metric leaf is read before the run (FR-023), and
the first `sampleOverheadNs*` call on the plan runs the documented
calibration.

**Rationale**: every decision the runner makes reads a fold (FR-018),
so every leaf a fold needs must sit in the plan compiled in the
untimed region (007 FR-050). One compile covers the clock pair and the
metrics, which keeps the timed loop free of any other counter work
(FR-017).

**Alternatives considered**: a second plan for the clocks, rejected
because two recorders per benchmark contradicts D-2 and doubles the
sampling actions per window.

## R-12: the growth-rule arithmetic (FR-010, FR-016)

**Decision**: the runner implements the D-3 rule in integer
nanoseconds: the factor is `1.4 × minTime / max(decisionTime, 1 ns)`,
the factor is 10 when `decisionTime ≤ 0.1 × minTime`, the next count
is `max(round(N × factor), N + 1)`, capped at 10^12 iterations. The
qualify test is the five-condition list of FR-008. The 96-run bound
of D-3 sizes the recorder.

**Rationale**: D-3 pins the upstream text at the recorded revision,
and SC-002 walks the sequence step by step against scripted clocks.
Integer nanoseconds keep the scripted comparisons exact.

**Alternatives considered**: floating-point seconds, rejected because
the scripted exactness of SC-002 would depend on rounding; a
simplified doubling rule, rejected because FR-010 states the rule in
full.

## R-13: the barrier form and its P2 (FR-029, FR-055)

**Decision**: `doNotOptimize` is one empty extended-assembly statement
per overload, in the D-4 form: the value names an input and output
operand with a register-or-memory constraint and a memory clobber.
`clobberMemory` is `std::atomic_signal_fence(std::memory_order_acq_rel)`.
The P2 justification under Principle I: the barrier's contract is a
property of the generated code, and no ISO C++ construct can force the
compiler to keep a value it can prove dead. The upstream barrier the
spec reads uses the same statement, and the 011 plan set the precedent
of recording an `__asm__` P2 in the plan's Constitution Check. The
alternative, a volatile round-trip, defeats itself at `-O2` on the
patterns benchmarks write, and the codegen gate `barrier_shape.sh`
proves the property the statement carries.

**Rationale**: FR-055 names this statement as the one compiler
extension the spec adds and requires the plan to record its P2.

**Alternatives considered**: a volatile local plus an opaque function
call, rejected because the compiler may still constant-fold across the
call at link time and the guarantee is weaker than the statement;
omitting the barrier, rejected because FR-029 mandates it and US6
measures pure computation.

## R-14: the build-type field (FR-035)

**Decision**: the harness target carries an `SG_BUILD_TYPE` compile
definition holding `CMAKE_BUILD_TYPE`, written by CMake from the
preset. The name follows N-3, and V.2 and `.clang-tidy` stay
unchanged.

**Rationale**: the build type is a build fact, the counters library
publishes none, and FR-035 wants the field. A build-written macro is
the smallest mechanism, and the report reads it as a literal. N-3
admits an `SG_` name, and the build-written family of V.2 covers
generator-derived names alone.

**Alternatives considered**: a runtime query of the library build,
rejected because no such query exists; printing the harness's own
`NDEBUG` state, rejected because it reports one
translation unit's macros, which leaves the configured build type
unreported; a `SPEEDGUN_`-prefixed build-type name, rejected because it
needs a V.2 entry and a `.clang-tidy` change.

## Resolved unknowns

| Technical Context entry | Resolved by |
| --- | --- |
| Primary dependencies | R-01, R-03: the counters library and the 007 fake provider, no new dependency |
| Testing | R-03, R-05, R-07: fake-provider suites, fixture aggregates, script gates |
| Target platform | R-04, R-08: catalog facts carry platform differences; no harness branch on the clock set |
| Performance goals | R-02, R-12: two sampling actions, fixed capacity, 96-run bound |
| Constraints | R-06, R-13: signal flag mechanics, the recorded P2 |
| Scale/Scope | R-01: one target, two headers, six sources |
