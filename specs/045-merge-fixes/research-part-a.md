# Phase 0 research, Part A: counters (FR-001 to FR-005, FR-013, FR-015)

Base: master `b1dd7670`. Line numbers below name the state at that commit.
The implement phase finds each item by its symbol when a line moves.

Method: every decision below grounds in the named files and lines. The
research names the scripted answer that reaches each repaired line, the
marker each removal takes out, and the test that gains the scenario.

## FR-001: the scripted CPU identity and the ident cache

**Decision.** The `KernelCalls` seam gains one internal field that
returns the `CpuidWords` value. The real build fills that field from
CPUID. `pmuIdentCurrent` drops its static ident cache and reads the
field on each call. The counter-limit register probe stays real, and no
test asserts any value derived from it.

**Rationale.**

- The seam is the options struct of function references at
  `source/counters/linux_pmu/kernel_calls.hpp:55-79`. It carries eight
  fields today, one per kernel call and one per kernel-published path.
  The struct lives in `sg::counters::detail`, an internal namespace, so
  a ninth field moves no public layout and no SOVERSION (spec D-1).
- `CpuidWords` is the six-field struct at
  `source/counters/linux_pmu/pmu_ident.hpp:18-27`: the leaf 0 words
  `EAX`, `EBX`, `EDX`, `ECX`, the leaf 1 answer flag, and the leaf 1
  `EAX`. The spec names it the pair: the two word sets leaf 0 and
  leaf 1 together.
- The CPUID probe stands at `source/counters/linux_pmu/table_parse.cpp:56-75`
  (`readCpuidWords`). It reads leaf 0 and leaf 1 through
  `__get_cpuid_count`, branchless, and a leaf that answers nothing
  leaves its words at zero.
- `pmuIdentCurrent` stands at `table_parse.cpp:113-126`. It computes
  the ident once into a function-local `static const PmuIdent` and
  answers the cached value on every call. The comment at line 115
  states the old reason: the ident never changes. The cache ignores the
  seam, so a test that installs scripted words after the first call
  reads the host ident anyway.
- The consumers are `mergeVendored` at
  `source/counters/linux_pmu/provider.cpp:156`
  (`pmuSelectDirectory(pmuIdentCurrent())`) and the seam test helpers
  at `test/source/counters_linux_pmu_seam_test.cpp:2208` and
  `:3673-3677`. The provider constructor reaches `mergeVendored` at
  `provider.cpp:779`, so every `PmuProvider` construction in
  `counters_pmu_provider_test` reads the real CPUID of the test host.
- The host dependence bites in `kernelAliasWinsScenario`
  (`test/source/counters_pmu_provider_test.cpp:393-442`). Line 420
  asserts `!vendored.empty()`, "the vendored table merges a core row".
  On a host whose CPU matches no row of the pinned mapfile, every
  merged entry carries the kernel's own description text. The scan at
  lines 408-419 then finds no vendored row, and the check fails. Lines 426
  and 437 build on the name the first enumeration found, so the whole
  scenario fails with it. This is the US-2 defect the spec repairs.
- The empty-selection arm of `mergeVendored` sits under the T066
  markers at `provider.cpp:157-163`: the `if (directory.empty())`
  early return at lines 160-162. The comment at lines 148-152 states
  why the arm sat excluded: a CPU no row matches, and CPUID alone
  decides it. A scripted identity whose vendor no mapfile row matches
  drives the arm on every host, so the markers go and the lines count.
  The seam test already proves the selection behavior for such an
  identity at `counters_linux_pmu_seam_test.cpp:2282-2285` with the
  `NoSuchVendorXXXXX` ident.
- The ident sits on cold paths: one call per device inside provider
  construction, plus the seam test helpers. The per-call read replaces
  one cached load with the probe the first call already paid, so the
  cache buys nothing the code needs.
- The real field value follows the file's own pattern. The real set
  at `kernel_calls.cpp:127-136` names one real function per field.
  Each real function carries a Linux arm and an off-Linux stub
  (lines 28-115). The ninth real function holds the probe
  `table_parse.cpp:56-75` carries today.
  `pmuIdentCurrent` then reads `kernelCalls().cpuidWords()` and hands
  the value to `pmuIdentFromWords` (`table_parse.cpp:77-111`), which
  stays a pure function of the words.
- The ident decode itself needs no new test: `pmuIdentDecodeScenario`
  at `counters_linux_pmu_seam_test.cpp:4234-4289` already drives
  `pmuIdentFromWords` over word sets no host has to provide.

**Which tests gain which scenarios.**

- `counters_pmu_provider_test` installs two identities through the
  new field. The first identity matches a pinned mapfile row, so
  `kernelAliasWinsScenario` asserts the merge on every host. The
  second matches no row, so the empty-selection arm at
  `provider.cpp:160-162` carries its own counts, and its markers go.
- `counters_kernel_calls_test` gains the field's round trip: install a
  scripted pair, assert `kernelCalls().cpuidWords()` answers it; clear
  the install, assert the field answers the same value the probe
  function answers directly. The clear-side check asserts equality of
  two routes and asserts no value derived from CPUID, the way the
  watchdog pattern holds (FR-013).
- `counters_linux_pmu_seam_test` installs scripted words at
  `mapfileScenario` (`:2206-2286`) and at the `selectedTable` helper
  (`:3673-3677`), so the host-dependent early returns at
  `:2215-2220` and `:3751-3755` lose their host dependence.

**Alternatives considered.**

- Keep the static cache and add an install hook that clears it:
  rejected. The seam then carries state and a second knob. The cache still
  ignores any install a later scenario stands in. The clear also
  races the one-time magic-static initialization.
- Pass the `CpuidWords` value down as a parameter from
  `mergeVendored` to the selection: rejected. `pmuIdentCurrent` is
  also a seam-test entry point at
  `counters_linux_pmu_seam_test.cpp:2208` and `:3676`. The signature
  change ripples through both. The function-reference field is the
  pattern the seam's other eight members already follow.
- Script the identity through a wider provider-level seam: rejected.
  The spec names the internal `KernelCalls` seam, the field needs no
  new type, and a provider-level seam would move a public surface.

## FR-002: the scripted kernel refusal and the one real call

**Decision.** The seam-driven tests script every kernel refusal
through `KernelCalls`. One real `perf_event_open` call stays: the
malformed-attribute refusal check at
`test/source/counters_kernel_calls_test.cpp:133-140`, because it
asserts a result every host gives.

**Rationale.**

- A host with `perf_event_paranoid = -1` refuses no event the caller can
  open. Any check that expects a real open to fail reads a host
  artifact, which is the root cause the spec names under US-2.
- The refusal machinery already stands in the scripted sets. The
  provider test tunes the open ordinal and the errno through
  `gRefuseAt` and `gErrno`
  (`test/source/counters_pmu_provider_test.cpp:110-111`, consumed at
  `:134-137`). The window test carries the same pair plus the ioctl
  ordinal (`test/source/counters_pmu_window_test.cpp:69-71`, consumed
  at `:87-90` and `:111-114`).
- The refusal arms that decide the published state sit in the errno
  switch of `pmuProbe` at `provider.cpp:726-733`: `EINVAL`,
  `EOPNOTSUPP`, and `ENOENT` answer `NOT_ENCODABLE`, every other
  errno answers `PERMISSION_BLOCKED`. The provider test's fixture
  sets `gErrno` to `EACCES` alone (`:111`, `:168`), so the three
  encoding errnos carry no scripted answer at base. The fix stands a
  scenario per errno class: `EINVAL`, `EOPNOTSUPP`, and `ENOENT`
  each assert the `NOT_ENCODABLE` state, and `EACCES` asserts
  `PERMISSION_BLOCKED` beside them.
- The kept real call reads as follows. The check at
  `counters_kernel_calls_test.cpp:136-140` opens with a
  `perf_event_attr` whose size field is zero. The kernel checks the
  size before any capability or permission question, so it refuses
  with an errno on every host, at every `perf_event_paranoid` level.
  The assertion names only the refusal and a set errno, no value the
  host shapes. FR-002 carves this one exception over FR-013.
- The other real calls in that test run over a scratch file the test
  owns (`:142-168`): a real `read`, `mmap`, `munmap`, `ioctl`, and
  `close` of a regular file. No `perf_event_open` answer takes part,
  so FR-013 leaves them alone. The two path checks at `:126-131`
  compare the real set against constant strings, so they carry no
  host fact either.
- `readParanoid` at `provider.cpp:87-95` already routes through the
  seam (`paranoidPath`), and the provider test writes the file the
  scenario tunes (`writeParanoid`, `:212-223`; scenario at
  `:269-295`). No work rides there.

**Which tests gain which scenarios.**

- `counters_pmu_provider_test` gains the errno matrix over
  `pmuProbe`'s switch, driven through the existing `gErrno` fixture.
- `counters_kernel_calls_test` keeps its real-call block as the spec
  carves it and gains no real refusal scenario.

**Alternatives considered.**

- Delete the malformed-attribute real call and script its refusal:
  rejected. The spec names the check as the one that stays, and the
  check is the only direct evidence the seam's errno convention
  matches the kernel's own convention.
- Keep real refused opens for the `NOT_ENCODABLE` arms: rejected. A
  refused open with a chosen errno needs a host the kernel refuses in
  that exact way, which no CI host guarantees.

## FR-003: the marker repairs, line by line

**Decision.** The repair touches four sites. Each site names the
marker that goes, the marker that stays, and the scripted answer or
the kept-list reason that governs it.

### Site 1: `include/speedgun-ng/counters_provider.hpp:111`

- The defaulted copy constructor of `ObjectSink` loses its
  `LCOV_EXCL_LINE` (FR-003, first bullet). Commit `65dea585`, a PR #71
  commit, added it. gcc 16 attaches an unexecuted block to the line
  record. `gcov -b` shows the block on the provider test's unit
  while the CI compiler reports a count.
- Reachability: no scripted answer reaches a defaulted special
  member. The repair stands on the host-record grounds alone, and the
  fix head's coverage run on the acceptance host verifies the line
  reads covered after the removal. The commit body states the gcc 16
  record and its removal reason, the way `65dea585` stated the
  addition.
- The default constructor's marker at `:106` and its comment block at
  `:100-105` stay: the T066 record there predates PR #71 and the
  measurement behind it stands.

### Site 2: the two switch lines `group_io.cpp:306` and `:432`

- Both lines carry `LCOV_EXCL_LINE` today. The repair swaps each for
  `LCOV_EXCL_BR_LINE` on the same line. The swap is a narrowing
  replacement, never an addition (FR-015); the ledger records it.
- Commit `387ca19f`, a PR #71 commit, widened the group window's
  marker from `LCOV_EXCL_BR_LINE` to `LCOV_EXCL_LINE` and set the
  fast window's the same way. Both windows' tests drive all four
  enumerators: the window test's scenarios hand each switch a leaf
  set with member, time, and disclosure slots
  (`test/source/counters_pmu_window_test.cpp:291-293` and `:536-540`).
  The dark thing the marker names is the compiler's implicit no-case
  arc, a block for a value the closed enumeration cannot hold
  (`group_io.cpp:304-305` and `:430-431`).
- Under `LCOV_EXCL_BR_LINE`, the line itself counts for line
  coverage. Only the branch data stays excluded. The no-case arc then
  stays out of the branch shortfall, and the line joins the
  denominator.
- The swap stands only when both lines stay line-covered at the fix head.
`readPoints` reaches each switch whenever a window with slots reads a point, and
both window tests guarantee that. A line can sit dark on one acceptance host.
The work then covers it through a scripted seam answer or keeps the
`LCOV_EXCL_LINE` in the kept list with its reason (FR-003's own condition).
- Reachability: the four enumerators are reachable by scripted
  answers today. The no-case arc stays unreachable by any input. The
  BR marker is the exclusion that names exactly that arc.

### Site 3: the three `gapped = true` lines at `group_io.cpp:288`, `:402`, `:426`

- Line 288, the group short read. No marker sits on it. The scripted
  answer is `groupWindowGappedScenario`
  (`test/source/counters_pmu_window_test.cpp:277-329`). The scripted
  `readGroup` answers `-1` with an errno at `:285-289` and a two-word
  short count at `:310-311`. The scenario asserts the gap disclosure
  at `:303-304` and `:322-323`. The line counts today and keeps
  counting. The repair here is a verification entry in the ledger,
  no edit.
- Line 402, the refused fast read. It sits under the T140
  `LCOV_EXCL_BR` block at `group_io.cpp:395-404`. A scripted answer
  reaches it today. `fastWindowReadScenario`
  (`counters_pmu_window_test.cpp:508-559`) registers a member page
  whose `cap_user_rdpmc` is clear (`:530-533`). The scripted
  `mapPage` hands the page to the window (`:95-99`). The read
  answers `NOT_ALLOWED` without executing the instruction
  (`fast_read.cpp:343-355`). The scenario asserts the gap beside the
  zero count (`:548-553`). The marker excludes a line a scripted
  answer reaches, so the whole block goes: the `LCOV_EXCL_BR_START`
  at `:395`, the BR lines at `:400` and `:403`, and the
  `LCOV_EXCL_BR_STOP` at `:404`. Line 402 joins the denominator and
  carries its count.
- Line 426, the unstable-pair arm. It carries `LCOV_EXCL_LINE`
  inside the T140 block at `:409-428`. No scripted answer reaches
  it. `fastContextTimePair` reads the page sequence at
  `fast_read.cpp:379` and compares it against the same page at
  `:408`. The arm needs the kernel to rewrite the page between the
  two reads. A fixture cannot force that rewrite; a racing
  writer thread in a test is a data race the sanitize and tsan
  presets report. The block stays, and the kept list carries it with
  that reason (FR-003's third bullet, D-7).
- The retry arm of the member read keeps its block too, for the same
  reason. The T140 block at `group_io.cpp:376-390` with the line
  marker at `:388` names the arm where the sequence moves between
  the two reads inside `fastContextRead` (`fast_read.cpp:322-359`).
  No fixture answer forces a page rewrite mid-call. The kept list
  carries the block with its reason.

### Site 4: the empty-selection arm `provider.cpp:157-163`

- After FR-001, the provider test scripts an identity no pinned
  mapfile row matches. The arm at lines 160-162 then
  executes, and the markers go. They are the `LCOV_EXCL_BR_START`
  at `:157`, the BR line at `:160`, the two LINE markers at `:161`
  and `:162`, and the `LCOV_EXCL_BR_STOP` at `:163`.
- Reachability: fully scripted, through the new seam field. The
  selection behavior for such an identity is already asserted at
  `counters_linux_pmu_seam_test.cpp:2282-2285`.

**Markers Part A keeps in the counters tree.**

- `provider.cpp:483-517`: the default arm and the fall-through of
  `entryReadSelectionFor`'s switch over `Availability` (T056). The
  enumeration is closed, the arm answers only a value past the last
  enumerator, and the object's disassembly shows the dispatch as a
  range check. Manufacturing an out-of-range value in a test buys a
  dead assertion. Kept with its recorded reason.
- `table_parse.cpp:712-714`: the epilogue block of `toEcma`'s
  by-value return. Kept, unchanged.
- `table_parse.cpp:807-817`: the arm where the embedded registry
  holds no `arch/x86` directory (T056). The table generator embeds
  the directory unconditionally and fails the build when the
  pattern matches nothing. Kept, unchanged.
- `fast_read.cpp:335-356` and `:408-410`: the instruction arm and
  the pair-stability arm of the mapped-page read (T140). The
  instruction needs the processor to serve the counter, the pair
  arm needs a page rewrite. Kept, unchanged.

## FR-004: how coverage stays green

**Decision.** Every counters line that sits under no kept marker
carries its count at the fix head on thrawn, on corsairone, and in
CI. A kept marker removes its lines from both the numerator and the
denominator of the coverage reading. The scenarios mapped above
supply the execution each repaired line needs.

**Rationale.**

- lcov removes a line an `LCOV_EXCL` marker names from the tracefile
  before the gate reads it, so a kept marker shrinks the denominator
  with its lines. This is the mechanism the clarification under
  FR-004 records.
- The PR #71 coverage job read 16051 of 16059 lines, with
  `group_io.cpp` at 2 dark lines and `provider.cpp` at 6, and a
  branch shortfall of 25 (FR-004). The fix head re-measures the dark
  lines by file and drives each one with the map above.
- The 6 dark `provider.cpp` lines at base sit where the scripted
  answers above reach. Four of them are the errno switch's
  `NOT_ENCODABLE` arm at `provider.cpp:727-730`; no scenario drives
  `gErrno` with an encoding errno at base. The head's run names the
  last two. The errno matrix under FR-002 covers the four, the
  head's coverage run confirms the last two, and the map of FR-003
  decides each.
- The 2 dark `group_io.cpp` lines sit at the switch lines the swap
  covers. The window tests cover the lines once the marker narrows
  to branch data. The FR-003 fallback stands ready when a host's
  compiler attributes the no-case block to the line itself.
- Removing the T140 block at `group_io.cpp:395-404` adds the
  not-taken side of `if (verdict != FastReadVerdict::OK)` to the
  branch shortfall. The OK side needs the processor to serve the
  instruction over a mapped counter; no fixture page stands in for
  it (`fast_read.cpp:346-354`). One short branch position enters
  the whole-tree count, which FR-011 holds at 21 or less. The
  implement phase reports the number in the PR body.
- The swap at `:306` and `:432` moves the no-case arcs out of
  neither count. Both arcs were branch-excluded before the swap and
  stay branch-excluded after it, so the shortfall holds.
- Removing the `provider.cpp:157-163` and `:395-404` markers and the
  `provider.hpp:111` marker removes no denominator relief the tree
  still needs. Each removed marker's lines gain real counts through
  the scenarios named above.

## FR-005: the kept-marker ledger

**Decision.** The PR body carries the marker count by file and the kept list,
one reason per kept marker. `git grep -c LCOV_EXCL -- source include` reads 84
at the Part A head, down from 94.

**Rationale.**

- The base count is 94. By file: `clock_provider.cpp` 17,
  `system.cpp` 12, `plan.cpp` 13, `group_io.cpp` 15,
  `provider.cpp` 9, `fold.cpp` 5, `fast_read.cpp` 7, `dbc.hpp` 4,
  `table_parse.cpp` 4, `counters_provider.hpp` 3,
  `detail/core.hpp` 2, `marker.cpp` 2, `encode.cpp` 1.
- Part A removes 10 marker lines: one at `counters_provider.hpp:111`,
  four in the T140 ok-verdict block at `group_io.cpp:395-404`, and
  five in the empty-selection block at `provider.cpp:157-163`. The
  two swaps at `group_io.cpp:306` and `:432` change no count. The
  Part A head reads 84 before Part B's work.
- Each marker a scripted answer now reaches is removed, per FR-005.
  The five markers of the empty-selection block and the four of the
  ok-verdict block sit on lines the new scenarios drive.
- The six markers G-21 keeps stay. Four sit in
  `include/speedgun-ng/dbc.hpp` on the abort path, two
  `LCOV_EXCL_START`/`LCOV_EXCL_STOP` pairs at `:106-216` and
  `:238-394`. Two sit in `source/simulation/marker.cpp` on the
  comment-only `#else` branch (`:40-43`).
- The kept list at the Part A head carries one reason per marker:
  - `dbc.hpp` `:106-216` and `:238-394`: the abort path of the
    contract checker; no supported build executes an abort and
    returns.
  - `marker.cpp` `:40-43`: the comment-only `#else` branch of a
    supported build set.
  - `group_io.cpp` `:306` and `:432` (as BR): the no-case arc of a
    switch over a closed enumeration; every enumerator carries a
    scripted count.
  - `group_io.cpp` `:376-390` and `:409-428`: the page-rewrite arms
    the protocol names; no fixture answer forces a kernel page
    rewrite inside one call.
    - `provider.cpp` `:483-517`: the past-the-enum arm of a closed
    switch; the disassembly names a range check.
  - `counters_provider.hpp` `:100-106`: the defaulted default
    constructor emits no code (the T066 record, older than PR #71);
    only the line 111 marker of the pair goes.
  - `table_parse.cpp` `:712-714`: the by-value return epilogue.
  - `table_parse.cpp` `:807-817`: the registry arm the generator
    guarantees against.
  - `fast_read.cpp` `:335-356` and `:408-410`: the instruction arm
    and the pair-stability arm.
  - Every other file's markers stand outside Part A's scope and
    carry their recorded T066 reasons.
- A marker no ruling covers stays in the kept list as "waits for
  ruling"; the work invents no ruling (D-7).

## FR-013: what the seam-driven tests give up

**Decision.** The seam-driven counters tests use no sleep, no clock
read, no real CPU identity, no sysfs, no procfs, and no real
`perf_event_open` answer. The seam-driven set names the `kernel_calls`,
`pmu_provider`, `pmu_window` and `linux_pmu_seam` tests and every other
counters test the fix touches. `counters_pmu_test` stays the host-aware
integration check with its skip-with-reason design. The watchdog
file read inside a provider enumerate stays real. The suite passes
under `taskset -c 0,1`.

**Rationale.**

- `counters_pmu_test` keeps its design: the test entry carries
  `SKIP_RETURN_CODE 2` (`test/CMakeLists.txt:461-468`), so a host
  that refuses the syscall skips with a reason.
- The watchdog read is `readWatchdogValue` at
  `source/counters/linux_pmu/counter_limits.cpp:270-278`, reached
  through `hostWatchdogValue` (`provider.cpp:111-115`) at
  construction (`provider.cpp:757`). No test asserts its value, so
  it stays real, the pattern the spec names the watchdog pattern.
  The counter-limit register probe
  (`counter_limits.cpp:133-205`, cached at `provider.cpp:101-106`)
  follows the same pattern: the probe stays real and no test
  asserts a value derived from it.
- The real CPU identity sites the fix scripts away. Direct
  `pmuIdentCurrent` calls sit at
  `counters_linux_pmu_seam_test.cpp:2208` and `:3676`. The
  `mergeVendored` calls at `:3702` and `:3783` read the ident
  inside `mergeVendored`. The whole provider test reads it through
  `provider.cpp:779`.
- The procfs sites in the seam test are the descriptor and mapping
  counters at `counters_linux_pmu_seam_test.cpp:208-238`
  (`/proc/self/fd` and `/proc/self/maps`), consumed by
  `fixtureScenario` at `:262-263` and by the lifecycle scenarios.
  The fix reworks them to fixture state or moves the lifecycle
  evidence to `counters_pmu_test`, which FR-013 exempts.
- The clock-read site is `fastContextTimePair`'s `__rdtsc` at
  `fast_read.cpp:399`, reached by `multiplexWindowScenario`
  (`counters_linux_pmu_seam_test.cpp:821-835`) over a page whose
  enabled and running counts differ. The recipe assertions over a
  fixture cycle count already stand at `:867-883` through the pure
  `fastTimePair` seam. The scenario keeps those and drops the
  differing-count call through the real context. The call can also
  stay, over a page whose counts agree; the guard at
  `fast_read.cpp:397-399` withholds the read there.
- The real-open sites in the seam test: `fastBranchScenario`'s
  availability probe at `:3196-3199` (a real `perf_event_open`
  through the real set) and `devicePageVerdictScenario`'s granted
  opens at `:4225-4227`. The fix scripts both through
  `installKernelCalls`, the way `scriptedKernelScenario`
  (`:4300-4435`) already stands a descriptor and a page in.
  `contextOpenRefusalScenario` (`:2160`) opens with a PMU type no
  kernel publishes and asserts a refusal every host gives. The
  implement phase holds it to the FR-002 rule, which names a result
  every host gives as the only real-call allowance. The scenario
  scripts the refusal where the rule names the asserted result
  host-shaped.
- The two-cpu bound holds: the touched scenarios install scripted
  answers and map no real resource beyond the fixture files the
  provider and window tests already write under
  `temp_directory_path`.

## FR-015: what the ban covers

**Decision.** The Part A work adds no net-new exclusion marker, no
`--omit-lines` pattern, no `--ignore-errors` setting, and no
`--remove` setting for coverage. The accepted exceptions (R-1, R-2,
the W-1 omit patterns, branch allowance 21) stay as they are.

**Rationale.**

- The two `LCOV_EXCL_LINE` to `LCOV_EXCL_BR_LINE` swaps at
  `group_io.cpp:306` and `:432` replace a wider exclusion with a
  narrower one on the same lines. No marker line is added, no
  exception class is created, and the ledger records the swap. The
  clarification under FR-015 places the swap outside the ban.
- Every other Part A edit to markers is a removal: the five lines of
  `provider.cpp:157-163`, the four of `group_io.cpp:395-404`, and
  the one of `counters_provider.hpp:111`.
- The coverage gate configuration sits out of scope (`cmake/`, the
  coverage tooling), so no gate setting changes.

## Fact table

| Fact | Anchor at `b1dd7670` |
|---|---|
| Seam struct, eight fields | `kernel_calls.hpp:55-79` |
| Real set and install | `kernel_calls.cpp:127-150` |
| `CpuidWords` struct | `pmu_ident.hpp:18-27` |
| CPUID probe | `table_parse.cpp:56-75` |
| Ident decode, pure | `table_parse.cpp:77-111` |
| Ident static cache | `table_parse.cpp:113-126` |
| `mergeVendored` ident call | `provider.cpp:156` |
| Empty-selection markers | `provider.cpp:157-163` |
| Provider merges vendored rows | `provider.cpp:779` |
| Host-dependent provider check | `counters_pmu_provider_test.cpp:420` |
| `gErrno` fixture, one value | `counters_pmu_provider_test.cpp:111` |
| errno switch of `pmuProbe` | `provider.cpp:726-733` |
| Malformed-attribute real call | `counters_kernel_calls_test.cpp:133-140` |
| Copy-constructor marker | `counters_provider.hpp:111`, commit `65dea585` |
| Switch markers | `group_io.cpp:306`, `:432`, commit `387ca19f` |
| Group short read driver | `counters_pmu_window_test.cpp:277-329` |
| Ok-verdict block, `gapped` line 402 | `group_io.cpp:395-404` |
| Refused-page driver | `counters_pmu_window_test.cpp:508-559` |
| Unstable-pair block, `gapped` line 426 | `group_io.cpp:409-428` |
| Retry block | `group_io.cpp:376-390` |
| Watchdog read | `counter_limits.cpp:270-278` |
| Counter-limit probe | `counter_limits.cpp:133-205` |
| `pmu_test` skip design | `test/CMakeLists.txt:461-468` |
| Marker count at base | 94, by file in FR-005 |
| PR #71 coverage reading | 16051 of 16059, shortfall 25 |
