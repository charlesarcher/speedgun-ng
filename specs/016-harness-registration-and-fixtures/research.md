# Research: Harness Registration and Fixtures

Phase 0 output for `specs/016-harness-registration-and-fixtures/spec.md`. Every Technical Context entry
resolves before design. Each record states the decision, the rationale, and the alternatives considered. The
reference read is `google/benchmark` `main` at `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08), the
same revision H1 read (015 `spec.md:1111`). Google Benchmark is a reference read, nothing more: no vendored
copy enters this feature and the harness links nothing from it (`spec.md:191`, `spec.md:705`). A citation
naming a Google Benchmark file cites the reference-read table of the spec.

## R-01: the family record

**Decision**: `RegistryEntry` (`source/harness/detail/internal.hpp:31-42`) gains `args`, `argNames`,
`rangeMultiplier`, `setup`, `teardown`, and a fixture factory. It stays the one record per registration.

**Rationale**: `BenchmarkHandle` holds one `RegistryEntry *` (`benchmark.hpp:542`), so a family call mutates
that record and returns the handle for chaining. The registry keeps insertion order (`registry.cpp:13-17`),
and the setter guard `!runStarted` (`registry.cpp:55`, `:66`, `:76`, `:86`, `:97`) orders every family call
before the run starts (`runner.cpp:141`).

**Alternatives considered**: a second record type beside the entry, rejected because the one-pointer shape
(`benchmark.hpp:542`) and the duplicate scan (`registry.cpp:22-29`) would each need a second lookup path;
family calls stored in the handle alone, rejected because expansion reads the registry (`internal.hpp:45`).

## R-02: the instance record

**Decision**: a new internal record `Instance` carries the instance name, the suite name, the case name, the
argument span, and a family pointer. `Runner::run` takes an instance; `Instance` and `RegistryEntry` stay
inside `source/harness/detail/internal.hpp`.

**Rationale**: the Key Entities name the instance as what the filter selects and what the runner runs.
`Runner::run` takes `RegistryEntry &` today (`internal.hpp:92`), so one signature change moves the run path
onto the expanded set, and the public surface gains no type beyond `sg::Fixture` (FR-022).

**Alternatives considered**: replacing the registry entries with instances, rejected because the duplicate
scan (`registry.cpp:22-29`) and the family-size warning need the family records intact; a family record plus
an index at run time, rejected because the runner would re-derive name and arguments at every run.

## R-03: expansion at the start of `speedgunMain` (FR-003)

**Decision**: `expandRegistry()` runs at the start of `speedgunMain` (`cli.cpp:142`), before the filter
selection and the first run. The family-size warning and the duplicate-instance check live there.

**Rationale**: the CLI order is providers, parse, selection, list mode, run loop (`cli.cpp:151-165`,
`:185-220`, `:227-248`, `:250-255`, `:278-299`). The filter matches instance names (FR-011), so expansion
precedes the regex search at `:243-247`. FR-003 keeps expansion work out of `sampleRun`
(`runner.cpp:317-366`).

**Alternatives considered**: expansion at registration time, rejected because family calls chain after the
registration call and the last call defines the set; expansion inside the run loop, rejected by FR-003 and
by the list-mode exit (`cli.cpp:254`), which prints instance names with no runner present.

## R-04: the state argument view (FR-008)

**Decision**: `State` gains `std::span<const std::int64_t> m_arguments`.
`range(std::size_t) -> std::int64_t` carries `SG_REQUIRE(index < size)`, and `rangeCount() -> std::size_t`
reports the count. The instance owns the storage.

**Rationale**: a read is one span index and allocates nothing (SC-002). `State` builds per run
(`runner.cpp:324`); its data members sit at `benchmark.hpp:400-408`, and the span adds two words to that
record. The layout change is the fact FR-026 prices in R-16.

**Alternatives considered**: a `std::vector` member in `State`, rejected because it allocates at every
`sampleRun` construction; a raw pointer plus a count, rejected because `std::span` is the standard view and
Principle I prefers the standard route.

## R-05: the callback-state rule (FR-018)

**Decision**: the enforced precondition set for a callback state is `begin()`, `skipWithError`, and
`skipWithMessage`. `end()` stays legal in that state. The state stays a `State &`; no new public view type
enters the surface.

**Rationale**: `State::end()` is `static auto end() noexcept -> Cursor` at `benchmark.hpp:277`, so it cannot
see the state it was called on, and no precondition can guard it. It returns `Cursor {0}` and touches no
state, so a callback call is harmless. The enforced methods read the state: `begin()` opens the sampling
window (`benchmark.hpp:262`), and the skip methods set `m_skipRequested`, `m_outcome`, and `m_reason`, then
call `takeExitSample()` (`:293-296`, `:314-317`). A callback state opens no window, so `SG_REQUIRE` enforces
each of those three violations. FR-018 names four violations; the plan records the narrowing to three in
Complexity Tracking.

**Alternatives considered**: making `end()` non-static, rejected because it replaces an H1 signature, which
FR-022 forbids, and it breaks every H1 loop that already compiles; a separate callback view type, rejected
by FR-018, which keeps the state a `State &`; leaving all four unenforced, rejected by Principle II, which
requires source enforcement for every documented precondition.

## R-06: the loop-shape exposure (FR-009)

**Decision**: the timed loop keeps the H1 statement list. Contract checks for the new surface sit outside
the loop, and no new check may emit a backward jump shorter than the timed loop.

**Rationale**: `test/loop_shape.sh` reads generated machine code through `objdump`
(`test/loop_shape.sh:50-51`). `loop_body` selects the backward jump with the shortest span in the whole
function (`:96-113`), and `analyze` fails on an out-of-line `call` inside that body (`:190`) or on a body
longer than the `referenceLoop` count-down (`:210`). `begin()` already carries one `SG_ENSURE`
(`benchmark.hpp:263-264`), runs once per run, and sits outside the loop body. The gate
(`test/CMakeLists.txt:548-549`) must stay green (SC-008).

**Alternatives considered**: a per-iteration check inside the cursor, rejected because the length test
(`:210`) fails it and H1 FR-004 and FR-005 forbid per-iteration work; a check inside the loop body, rejected
by the same two fail conditions.

## R-07: the result value and its two fields (FR-021, FR-026)

**Decision**: the result value of FR-021 is `sg::BenchmarkResult` (`benchmark.hpp:116`). The fields `suite`
and `caseName` go beside `name`, `outcome`, `rows`, `reason`, and `metricLabels` (`:118-125`). `ResultRow`
(`benchmark.hpp:101`) carries neither.

**Rationale**: `Runner::run` builds one `BenchmarkResult` per instance (`runner.cpp:143-144`) and returns it
(`:482`), so that value is the carrier the caller reads. `ResultRow` is one repetition or aggregate row
(`RowKind`, `benchmark.hpp:49-53`), `printResult` dispatches on `row.kind` (`report.cpp:57`), and a row
repeats per repetition while the pair belongs to the instance. The spelling `caseName` answers a language
fact: `case` is a C++ reserved word and cannot name an identifier. N-12 gives a public aggregate member the
camelBack, prefix-free shape (`constitution.md:573`, `.clang-tidy:86`), the shape of the five existing
fields.

**Alternatives considered**: the pair on `ResultRow`, rejected because rows repeat per repetition and the
pair belongs to the instance; a new public result type, rejected because `BenchmarkResult` is already the
exposed value and FR-022 extends the surface; the spelling `case`, rejected as a keyword.

## R-08: suite and case derivation (FR-021)

**Decision**: the suite is the family name up to its first `/`, and the fixture class name for a fixture
instance. The case is the instance name with the leading suite and its `/` removed. An instance name equal
to its suite keeps that name as its case.

**Rationale**: Q-1 of the clarify session confirmed the rule and FR-021 carries it. The instance name starts
with the family name (FR-010), and the fixture macros name instances `Class/Method` at the cited revision
(`registration.h:121-130`), so the split is a string operation at expansion. Pair uniqueness follows from
the unique instance name (FR-012).

**Alternatives considered**: the whole family name as the suite, rejected because `fib/bits` would group
each argument tuple as its own suite; a registration call stating the suite, rejected because FR-021 derives
the pair and adds no registration surface.

## R-09: the fixture lifetime (FR-017)

**Decision**: the factory builds one fixture object per run inside `sampleRun` (`runner.cpp:317-366`),
before `setUp`, and destroys it after `tearDown`. Warm-up, calibration, and measured runs each get a fresh
object.

**Rationale**: `sampleRun` is the single invocation site of the three phases: warm-up (`:374-380`),
calibration (`:410-416`), measured repetitions (`:435`). A run-scoped object puts the pair around every run
(SC-005), and the catch blocks (`:327-335`) destroy the local on an exception. FR-017 states the every-run
rule.

**Alternatives considered**: one fixture per benchmark built before the warm-up gate (`:373`), rejected
because a shared object carries state across runs and FR-017 scopes the pair to each run; a destruction
guard over the early exits of `Runner::run`, rejected because the run-scoped local needs no guard.

## R-10: the order inside one run (FR-017, FR-018)

**Decision**: the order is the setup callback, the fixture `setUp`, the callable, the fixture `tearDown`,
the teardown callback. One callback per slot, and the last attachment wins.

**Rationale**: the insertion points sit in the untimed region: after the `State` construction
(`runner.cpp:324`) and after `topUpWindow()` (`:336`), with the window open only between `begin()`
(`benchmark.hpp:262`) and the close (`:175-178`). The cited revision runs the fixture pair around the method
inside `Run`, and `Run` inside the callbacks (`benchmark_runner.cc:440-442`, `:513-515`), so the callback
pair wraps the fixture pair.

**Alternatives considered**: the fixture pair outside the callback pair, rejected because it inverts the
nesting the cited revision ships; one callback pair per benchmark, rejected by FR-018, which states once
around each run.

## R-11: the `DISABLED_` gate (FR-020)

**Decision**: the prefix test runs on the expanded instance name at selection time in `cli.cpp`. The filter
skips such an instance and list mode omits it. A fixture method named `DISABLED_x` keeps its class prefix
and runs.

**Rationale**: selection (`cli.cpp:227-248`) and list mode (`:250-255`) are the two read points of the H1
CLI, so one name test covers both (FR-011, FR-020). The cited revision tests the prefix on the full instance
name (`benchmark_register.cc:181`, `kDisabledPrefix` `:67`), and a fixture instance name starts with the
class (`registration.h:121-130`), which is why a method named `DISABLED_x` runs.

**Alternatives considered**: refusing a `DISABLED_` name at registration, rejected because FR-020 requires
registration and the cited revision registers the family; a stored flag on the instance record, rejected
because the name test at selection is one string check and the flag adds a field with one reader.

## R-12: the report order (FR-021)

**Decision**: after expansion the instance list groups stably by suite in first-appearance order, and the
runner walks that order. The console row keeps the H1 single name column.

**Rationale**: FR-021 fixes suite order at first registered instance and row order at registration order.
The run loop (`cli.cpp:278-299`) walks the selected list in order and prints each result through
`printResult` (`report.cpp:42-111`), so a stable grouping of the instance list is the whole mechanism. The
name column prints `result.name` (`report.cpp:60`, `:91`), and Q-6 keeps that shape.

**Alternatives considered**: an alphabetical suite order, rejected because FR-021 names first-registration
order; two console columns for suite and case, rejected by Q-6 and FR-021, which keep the H1 row shape and
put the pair on the result value (R-07).

## R-13: the macro names and the generated identifiers (FR-013, FR-014, FR-016)

**Decision**: `SG_BENCHMARK_CAPTURE` names the instance `fn/captureName`. `SG_BENCHMARK_TEMPLATE` names it
`fn<` plus the stringified type list plus `>`. The seven fixture macros follow the cited revision with the
class name as the suite. Generated identifiers follow the `SG_BENCHMARK` shape `SgBenchmarkRegistrar_##fn`
(`benchmark.hpp:582-593`), carry no leading underscore, and carry no `__`.

**Rationale**: the name forms are the cited revision's (`registration.h:69-75`, `:101-107`, `:121-130`,
`:154-163`). N-3 requires the `SG_` prefix (`constitution.md:549`, `.clang-tidy:66`). clang-tidy reads
macro-body tokens without resolving the identifiers inside them (015 `spec.md:196-201`), so generated names
stay a review item, and a leading or double underscore is language-reserved.

**Alternatives considered**: the Google Benchmark spellings, rejected because the missing `SG_` prefix is a
`MacroDefinitionPrefix` finding; a leading-underscore registrar name, rejected as language-reserved.

## R-14: the family-size warning (FR-007)

**Decision**: a family that expands to more than 100 instances draws one warning naming the bound
`kMaxFamilySize = 100`. The instances stay and the run continues.

**Rationale**: the cited revision carries the same bound and the same reaction (`benchmark_register.cc:65`,
warning `:163-165`). The count is known when expansion completes, so the check sits in `expandRegistry()`
(R-03); the spec's Assumptions move the check from the revision's filter-resolution point to expansion.

**Alternatives considered**: dropping the instances above the bound, rejected by FR-007, which keeps them; a
silent pass, rejected because FR-007 mandates the warning line.

## R-15: the free argument-list builders (FR-002, FR-006)

**Decision**: `createRange` and `createDenseRange` in namespace `sg` return `std::vector<std::int64_t>`.
`SG_REQUIRE` preconditions reject a multiplier below 2, a low bound above a high bound, and a step below 1.

**Rationale**: the cited revision ships the same two builders (`benchmark_register.cc:544`, `:550`), and
FR-002 requires a family call to accept the list they build. Q-2 settled the violation style: the values
come from code, so a `SG_REQUIRE` precondition matches every other contract check in the library. The step
check goes beyond the revision, and FR-006 records why: under the ignore semantic the check emits no code,
and a zero step then never ends.

**Alternatives considered**: the revision's abort, rejected by Q-2; an empty list for a bad input, rejected
because it hides a code bug until a later run reports an empty family.

## R-16: the version and `SOVERSION` effect (FR-026)

**Decision**: the project moves 0.6.0 to 0.7.0. The `speedgun-ng_harness` archive `SOVERSION` moves 2 to 3.
The counters archive stays at 2, and the constitution stays 2.18.0 with no amendment.

**Rationale**: `State` gains argument storage (R-04) and `BenchmarkResult` gains two fields (R-07), so both
exported layouts change. The hand-kept rule (`CMakeLists.txt:41-47`) ties a layout change to the `SOVERSION`
number, and the harness archive block (`:211-218`) carries the H1 comment that kept the number at 2 because
H1 changed no layout. FR-026 records the new fact.

**Alternatives considered**: keeping `SOVERSION` 2, rejected because a layout change then passes silently
between releases; a major version step, rejected because FR-022 replaces no existing signature.

## R-17: the naming discipline (FR-023)

**Decision**: every new name passes V.1 rules N-1 to N-12 with no `.clang-tidy` exemption.
`kDefaultRangeMultiplier` and `kMaxFamilySize` take the `k` prefix on the shape of `kDefaultMinTimeNs`
(`runner.cpp:22-24`).

**Rationale**: `WarningsAsErrors` names `readability-identifier-naming` alone (`.clang-tidy:21`), so a
naming finding is a build error. The family calls, `createRange`, `createDenseRange`, `rangeCount`, `setUp`,
`tearDown`, `setup`, and `teardown` take camelBack under N-2 (`constitution.md:546`); the nine macros take
`SG_` under N-3 (`constitution.md:549`); the two constants take `k` plus CamelCase under N-8
(`constitution.md:559`). `MethodIgnoredRegexp` (`.clang-tidy:152`) exempts `size` alone, which is why the
count method spells `rangeCount` (spec Assumptions).

**Alternatives considered**: `range_size`, rejected because the exemption reaches `size` alone and a
compound is a finding; `case` for the case field, rejected as a keyword (R-07); a `.clang-tidy` exemption,
rejected by V.1, which admits no local override (`constitution.md:578-581`).

## Resolved unknowns

| Technical Context entry | Resolved by |
| --- | --- |
| Language/Version | R-04, R-15: `std::span`, `std::vector`, `std::function`; no extension |
| Primary dependencies | R-01, R-03: the H1 registry, runner, and CLI; no new dependency |
| Storage | R-02, R-04: process memory, the instance owns the arguments |
| Testing | R-05, R-06, R-09: fake-provider suites, compile-fail cases, the loop-shape gate |
| Target platform | R-06: the gate scripts run on Linux with GCC and Clang |
| Performance goals | R-04, R-06: one span index per read, the H1 loop body |
| Project type | R-01, R-13: one archive gains one source, one header gains the macros |
| Constraints | R-03, R-14: expansion before selection, the 100-instance warning |
| Scale | R-02, R-16: one new source, eight test executables, version 0.7.0 |


