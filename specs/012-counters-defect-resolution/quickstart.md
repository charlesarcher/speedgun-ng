# Quickstart: validating counters defect resolution

This is the run guide for the feature. It names the command, the
expected outcome, and the success criterion each step proves. It holds
no implementation code; the entities and contracts live in
[data-model.md](data-model.md) and [contracts/](contracts/), and the
decisions live in [research.md](research.md).

## Prerequisites

- Linux, on Ubuntu for the main jobs or Rocky Linux for the container
  job. macOS and Windows are unsupported.
- The vendored submodules checked out, including `external/pmu-events`.
- `perf_event_paranoid` at 2 or higher. Every test runs unprivileged.
- A pinned processor for the two gated sampling-cost measurements in
  step 8. A shared host moves further than the 5 percent tolerance
  FR-008 sets.
- For the coverage step: `lcov`, `genhtml`, and the distribution's perl
  `GD` module. Configuration fails when lcov or genhtml is absent.

## Baseline: observe the defects before the corrections

TDD mode is in force (FR-033). Each covering test is written first and
observed failing at the pre-fix head.

```sh
git checkout 6aafd2d
cmake --preset=dev && cmake --build --preset=dev
ctest --test-dir build/dev -R counters --output-on-failure
```

**Expected**: the new tests fail. Every one of I-01 through I-10 has a
failing test at this head (FR-032). A test that passes here is not
covering its defect and is rewritten before any correction lands.

## Step 1: build and test the corrected tree

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev --output-on-failure
```

**Expected**: exit 0, no failures. Proves SC-011 in part.

## Step 2: release-configuration build, once per feature

The dev preset builds unoptimized, and an unoptimized build cannot
report a finding an optimizer's analysis produces (Principle IX).

```sh
cmake --preset=ci-ubuntu
cmake --build build
```

**Expected**: exit 0.

## Step 3: fast-path decode, folds, and the disclosure column

```sh
ctest --test-dir build/dev -R counters_linux_pmu_seam_test --output-on-failure
```

**Expected**: the page fixtures report three results. A point that
crosses the published counter width folds to the true delta. A refused
read takes the fallback and discloses through the managed column. The
ratio over a multiplexed window matches the time recipe to within one
tick. A group read that returns fewer bytes than the header marks the
action, and no fold across it reports a delta above the counts the
fixture drove. Proves SC-001 and SC-002. Field and column rules are in
[contracts/fast-read-fold.md](contracts/fast-read-fold.md).

## Step 4: catalog and plan concurrency under the thread sanitizer

```sh
cmake --preset=ci-tsan
cmake --build build/tsan
ctest --test-dir build/tsan --output-on-failure
```

**Expected**: exit 0, and the run reports no race while several threads
resolve counters and compile plans concurrently after open. Proves
SC-003. The job that runs this on every change is the `tsan` job in
`.github/workflows/ci.yml`.

The thread sanitizer does not share a build with the address
sanitizer. The existing preset keeps address and undefined behavior
alone:

```sh
cmake --preset=ci-sanitize
cmake --build build/sanitize
ctest --test-dir build/sanitize --output-on-failure
```

**Expected**: exit 0, no sanitizer report.

## Step 5: fast-window resource lifetime

```sh
ctest --test-dir build/dev -R counters_resource_lifetime --output-on-failure
ctest --test-dir build/dev -R counters_resource_lifetime_host --output-on-failure -V
```

**Expected**: the first test runs on every job and passes. It opens and
destroys a fast-mode plan 10,000 times, plus one partial open where a
later member fails, and the process descriptor count and mapping count
return to their starting values, counted through `/proc/self/fd` and
`/proc/self/maps`. The second test measures real event descriptors and
mappings on a host whose kernel grants the event. On a host that refuses
the event it prints the reason and skips with exit code 2, which CTest
reports as skipped. Proves SC-004.

## Step 6: Intel encodable-row counts and device placement

```sh
ctest --test-dir build/dev -R counters_pmu_tables --output-on-failure
```

**Expected**: the test pins the encodable-row count for `skylake`,
`icelake`, `alderlake`, and `sapphirerapids`, measured over the pinned
tree against the named synthetic sysfs format list the fixture supplies,
and the counts for `amdzen4` and `amdzen5` do not fall. The figures
this implementation measured are recorded in
[data-model.md](data-model.md), under the measured encodable-row counts
table, beside the counts the reference host's own formats yield.

The synthetic hybrid and uncore device fixtures report every row on the
device its scope names, and no uncore row under the core device. Proves
SC-005 and SC-006.

The Intel confirmation on Intel hardware is deferred and recorded in
[plan.md](plan.md). No step here depends on it.

## Step 7: availability, scope refusal, and target kinds

```sh
ctest --test-dir build/dev -R counters_pmu_test -R availability --output-on-failure
```

**Expected**: a device-scoped entry publishes a state that separates a
scope refusal from an encoding refusal, a cpu-target plan over that
entry compiles where the kernel grants it, and the supported target
kinds arrive as a fixed-size bitmask that allocates no memory. Proves
SC-007. The shape is in
[contracts/availability.md](contracts/availability.md).

## Step 8: the two gated sampling-cost figures

Run this once at the pre-fix head and once after the corrections, under
one run method, and record both figures.

```sh
# at the pre-fix head
git checkout 6aafd2d
cmake --preset=ci-ubuntu && cmake --build build
./build/test/counters_overhead

# after the corrections
git checkout 012-counters-defect-resolution
cmake --preset=ci-ubuntu && cmake --build build
./build/test/counters_overhead
```

**Run method**: release preset, pinned processor, `counters_overhead`'s
own protocol of 64 repeats of 1000 sampling actions, and the median of
each distribution. The same protocol covers both measurements.

**Expected**: the post-fix median of each gated plan stays within 5
percent of its pre-fix median. Two plans are gated. The first is the
core PMU group over `cpu/instructions` and `cpu/cpu-cycles`, with one
read per leader per action. The second is the clock-leaf plan over
`machine/monotonic` beside it.

**Record both figures here, beside this step**:

| Plan | Pre-fix median at `6aafd2d` | Post-fix median | Run method |
| --- | --- | --- | --- |
| core PMU group | 70.0 ns (min 70.0, max 80.0) | 80.0 ns (min 70.0, max 90.0) | release preset, pinned processor, 64 x 1000 actions, median, both trees measured in one session |
| clock leaf `machine/monotonic` | 40.0 ns (min 40.0, max 50.0) | 40.0 ns (min 40.0, max 60.0) | same |

**The bound holds for one gated plan and fails for the other.** The
clock leaf is unchanged at 40.0 ns, inside the 5 percent bound. The core
PMU group rose from 70.0 ns to 80.0 ns, which is 14.3 percent and outside
the bound. FR-008 is therefore **not** met for the core PMU group plan,
and the finding is recorded as measured: the disclosure
column FR-007 adds is one managed-column write per sampling action, and
the group plan performs two member reads per action, so the added store
lands on the plan that was already the more expensive of the two. T057
re-measures this figure, and the regression needs its own correction
before the feature merges.

**Why both figures were measured in one session.** T001 first measured
the pre-fix head in a separate worktree hours before T019, and recorded
60.0 ns and 30.0 ns. Re-measuring the pre-fix head in the same session as
the post-fix build gives 70.0 ns and 40.0 ns: the host runs about 10 ns
per sampling action slower now than it did then, on *both* plans,
including the clock-leaf plan, whose code path shares nothing with the
PMU decode or the disclosure column. An hours-old baseline is therefore
not comparable to a fresh one, and the drift-free pair above is the pair
the bound is judged on. Both medians quantize to a 10 ns tick, so the
14.3 percent figure carries one tick of resolution.

The figures come from `./build/test/counters_overhead` in the release
preset `cmake --preset=ci-ubuntu`, on an AMD Ryzen 9 9950X3D, reading the
lines `pmu group, fast_rdpmc` and `clock, syscall (vDSO)`. The pre-fix
tree is a separate worktree at `6aafd2d` built with the same preset.

The measurement covers the disclosure column FR-007 adds, because the
disclosure is written on every sampling action of both plans.

### The cpu-target pinning precondition under `ignore`

FR-045's precondition is a semantic-gated `SG_REQUIRE`, so a build
configured `ignore` must emit no check for it, exactly as it emits none
for any other gated site. The `consumer-release` job proves that over the
release archive, and the same proof was run locally over an
`ci-linux-ignore` tree:

```sh
cmake --preset=ci-linux-ignore -B build/v-rel-ign -D CMAKE_BUILD_TYPE=Release
cmake --build build/v-rel-ign
nm -C build/v-rel-ign/libspeedgun-ng.a > /tmp/ign-nm.txt
grep -F 'sg::dbc::check_' /tmp/ign-nm.txt   # must print nothing
```

**Confirmed.** The archive carries no `sg::dbc::check_` symbol, so the
gated precondition emitted no code, while `fast_pinning_ok` itself is
present because the pure predicate is always compiled. The archive was
checked to be newer than every library source it was built from, so the
result describes the current tree and not a stale archive.
`ctest --test-dir build/dev -R counters_trap_checked` passes alongside it.

## Step 9: overhead floor and clock order

```sh
ctest --test-dir build/dev -R counters_overhead --output-on-failure -V
ctest --test-dir build/dev -R counters_clock --output-on-failure
```

**Expected**: a registered test exercises the calibration and fails when
the bracket subtraction is absent, and the published floor excludes the
two clock reads that bracket each sampling action. Spec 011 measured
that bracket at 7.36 ns on the reference host. Proves SC-009.

Each clock leaf documents its own guarantee, and a test shows a new
thread's `machine/thread_cpu` sample falling below an earlier sample
taken on another thread, which the documented guarantee permits. The
timestamp-counter leaf gains no fence, which
`test/counters_tsc_read_shape.sh` asserts. Proves SC-010. The guarantees
are in [contracts/clock-order.md](contracts/clock-order.md).

## Step 10: installed package and the size the tables cost

```sh
cmake --preset=ci-linux-audit -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build -j 2
cmake --install build --prefix prefix
cmake -S test/consumer -B build-consumer -DCMAKE_PREFIX_PATH="$PWD/prefix"
cmake --build build-consumer
./build-consumer/consumer
```

**Expected**: the consumer links the installed package and publishes a
vendored-row count equal to the build tree's count on the same host, and
the rows resolve with the source tree absent. Relocating `prefix` leaves
the count unchanged. Proves SC-008.

**Record the size here**: run `size` on the archive and on the linked
consumer before and after the embedding. The figure names the cost every
consumer of the installed package carries.

| Artifact | Before embedding | After embedding | Delta |
| --- | --- | --- | --- |
| `libspeedgun-ng.a` | text 1934386, data 9104, bss 4630, total 1948120 bytes | text 26542701, data 24024, bss 4630, total 26571355 bytes | +24623235 bytes |
| linked consumer executable | text 4058, data 704, bss 65, total 4827 bytes | text 25280551, data 21880, bss 1137, total 25303568 bytes | +25298741 bytes |

The before-embedding figures were measured in the separate worktree at
`6aafd2d`, built with `cmake --preset=ci-ubuntu`, installed into a scratch
prefix, and read with `size`. T045 measures the after-embedding figure on
the same host under the identical commands.

The after-embedding archive grows by 24623235 bytes, which is the
vendored tree's 24588149 bytes of JSON plus the index that names each
file. The cost is paid once in the archive and reaches every consumer
that links it, and it buys a package that publishes its counters with no
source tree beside it. The `bss` figure is unchanged at 4630 bytes: the
tables are static data, so nothing moved out of `bss`.

Proves SC-012.

## Step 11: the remaining gates

```sh
cmake --build build/dev -t dbc-gate
cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake
cmake --build build/dev -t format-check
cmake -P cmake/prose-lint.cmake
python3 tools/pmu_events/update_pmu_events.py --check
cmake --preset=ci-coverage
cmake --build build/coverage -j 2
ctest --test-dir build/coverage --output-on-failure
cmake --build build/coverage -t coverage
```

**Coverage note.** The coverage build and its 45 tests pass, and the
gate now reports a verdict. FR-040 is **not met**: line coverage is 100
percent (2011 of 2011) and branch coverage 99.6 percent (749 of 752).

lcov 2.3 reads gcov 16 counters as negative taken counts and treats that
as fatal, which is what stopped the gate before it reported anything. The
capture command already narrowed its ignore list to named error classes,
and `negative` joins `mismatch` there, with the reason recorded in
`cmake/coverage.cmake`. lcov then completes and the gate fails on the
number above.

Three branches remain, every one of them on a line this feature added, so
none can be set aside as backlog. Intersecting the uncovered branches
with the lines `git diff -U0 6aafd2d..HEAD` reports as added leaves all
of them on added lines and nothing pre-existing.

One of the seven is the disclosure direction, and this record has named
the wrong provider twice while tracking it. `clock_provider.cpp`,
`push_provider.cpp` and `fake_provider.cpp` each hold `if
(disclosure_column != leaf_set::no_disclosure_column)`, and the direction
for a plan that does not disclose is the one at risk, because only the
group owning a plan's last leaf writes the column.

A plan over one leaf from each of two providers is what leaves a group
short of last, and the time-stamp suite registers the push provider
before the counted source, so its plan closed the push provider's
direction. `clock_provider.cpp` and `push_provider.cpp` now hold no
uncovered branch on an added line.

`fake_provider.cpp` line 88 is settled, and the cause sat in the fixture.
A plan over one provider's leaves always puts that provider's group last,
and the last group is the one that writes the column, so a lone scripted
provider can never stop disclosing. The recorder suite now registers the
clock provider second and compiles a plan drawing one leaf from each, so
the scripted group falls short of last and its window opens on a leaf set
carrying no column. That fixture closed the branch.

Three fixtures closed the rest of what was open: a disclosure slot
reached beside a countable member, a directory spelled without its
trailing separator, and the fan-out above. Those moved line coverage
from 99.4 to 100 percent and branch coverage from 98.2 to 98.9.

The remaining five sit in `provider.cpp` (a scope test, the
device-scoped condition chain, and the mode selector's switch),
`group_io.cpp` (the release loop's null-context arm) and
`table_parse.cpp` (the registry lookup's empty arm). Each needs a fixture
or a written exclusion, and no exclusion has been written for any of
them, because reaching them in a test has not been shown impossible and
this record will not claim it was.

Two of those need hardware this host does not have, and the reason is
worth recording. The device-scoped condition chain in `provider.cpp`
decides per device whether `path` is `cpu`, `cpu_core` or `cpu_atom`,
and this machine publishes neither of the last two:
`/sys/bus/event_source/devices` holds `cpu` and no hybrid device. A
fixture that added those devices to a constructed `pmu_state` covered
nothing new, because the decision is made while scanning sysfs and a
constructed state never reaches it. Those two branches need a hybrid host
or a fixture that stands in for the sysfs root, and this host is neither.
Whether the seam exposes such a root is not established here.

Two are reachable on hardware this host has, and no fixture reaches
them yet. This host does grant `perf_event_open`: a direct call
returns a descriptor at `perf_event_paranoid` 1.

The tracefile corrects this record's earlier reading of the scope test at
`provider.cpp` line 124. The countable arm is taken 74 times and the
direction that is missing is the one where the kernel refuses the entry
on a cpu target, so a fixture needs an event the cpu target refuses. That test sits in `probe_device`, which lives
in an anonymous namespace and is named by no seam declaration, so
reaching it from a fixture means exposing that function first.

The mode selector's switch at `provider.cpp` line 369 is settled. The
seam fixture drives all six availability states through it, and the one
branch it left open was the dispatch's default arm, which answers only a
value past the last enumerator. The object's disassembly shows the
dispatch as a test for zero, a subtract, a compare against four and an
unsigned jump above the range, so that arm carries an exclusion naming
the disassembly.

The three, with the branch each tracefile names: `provider.cpp` line 124
branch 1, and `provider.cpp` lines 211 branch 1 and 212 branch 2. The release loop's null-context arm is gone: every push into that
vector sits after the open's own null check, which returns first, so the
arm cannot be reached and the loop carries an exclusion saying so. The
scripted provider's disclosure direction is settled and needs no
exclusion.

All three that remain sit behind one wall. `probe_device` spans
`provider.cpp:103-185` and `load_device` spans `:186-291`, and both hold
their branches inside the anonymous namespace that closes at `:335`,
while the decisions the seam already exposes live in `namespace detail`
outside it. The seam builds its `pmu_device` directly, so it never
enters either function. Reaching line 124 needs an entry the cpu target
refuses, and reaching lines 211 and 212 needs a device published as
`cpu_core` or `cpu_atom`, which is what the chain at `:210-212` tests.

The three do not share one remedy, and an earlier reading of this record
claimed they did. Line 124 is reachable by no fixture. `probed` is
countable there only when a real per-task `perf_event_open` succeeds on
the entry while the cpu-targeted call on the same type and the same words
is refused, because `probe_device` reads the verdict from
`detail::pmu_probe`, which issues the syscall. No fixture supplies that
asymmetry; a synthetic device only moves the call. Seventy-four countable
entries across this host's twenty-four devices never show it, and closing
that branch needs a processor whose PMU grants the per-task event and
refuses the cpu-targeted one.

Lines 211 and 212 are a different matter. `load_device` takes its
directory as an argument and sets `device.path` from `dir.filename()`, so
a fixture directory named `cpu_core` or `cpu_atom` reaches both false arms
of the chain at `:210-212` with no hybrid processor present. What stands
in the way is that `load_device` sits inside the anonymous namespace and
`kDevicesRoot` at `:65` is a `constexpr` holding the kernel's own path, so
nothing reaches that function with a directory the test chooses. Two
routes close the pair: lift `load_device` into `namespace detail` and
declare it in `source/counters/detail/pmu.hpp`, then write a
`cpu_core` and a `cpu_atom` fixture tree, or make the device root a
parameter the seam supplies. The first touches a 722-line file and moves
code across a namespace boundary, and the second adds a configured path
to a published surface. Neither is taken here.

The `coverage-linux` preset named in T056 is hidden and CMake refuses it
by name. `ci-coverage` configures the same tree, so that is the preset a
run must use, and T056's command line names it.

**Sanitizer note.** `ci-sanitize` builds clean and all 45 of its tests
pass. The T025 partial-open scenario failed there for a while. The
fixture caused that failure, and the release path was sound throughout.

The scenario counts this process's own mapping list around a loop that
filled a `std::vector` of 64 contexts. A sanitizer keeps the pages of a
freed heap block mapped while it holds that block in quarantine, so the
vector's own allocation counted as a retained mapping. A probe measured
the residue at **8** mappings against the 64 the loop acquired, which is
one container and not 64 contexts: the contexts were releasing.

The members now sit in a fixed-size `std::array`, so the measured scope
holds no heap allocation of its own. Both builds report 45 of 45.

Two earlier notes on this finding were wrong and are superseded. One read
the raw delta of 40 surviving mappings as a leak. A second probe passed
`reserve`, which removes the vector's reallocation and still failed,
which ruled reallocation out and pointed at the container's single
allocation. A third probe attempted to confirm by calling `munmap`
directly on recorded addresses and reading the return, and that probe was
confounded: a sanitizer reuses freed addresses, so a successful unmap on
a stale address proves the address is mapped by something and says
nothing about who mapped it.

**Flake note.** One `ctest --test-dir build/dev` run out of four failed a
single test while a build was running alongside it; three further runs
passed clean with no build beside them. The suite measures clock and
overhead floors, which are load-sensitive by construction, and the three
tests that assert a floor are `counters_clock_push_test`,
`counters_overhead` and `counters_clock_raw_test`. Which one failed was
not captured, because the failure was not reproduced. A walk that runs
the suite beside a build should expect it, and a walk that wants a clean
verdict should run the suite on its own.

**Spell-check note.** The `spell-check` target scans the working tree,
and it fails on `speedgun-ng-012-specify-prompt.md`, an untracked
scratch file that captures a `/speckit.specify` prompt and is not part of
this feature. It reports `synchronised` and `recognise`. Run over the 37
files this feature changed, codespell exits 0 with no finding.
The file is left unedited, because it is not this feature's to change.

**Expected**: every command exits 0. The coverage gate reports 100
percent line, 100 percent branch, and 100 percent contract coverage. The
count of coverage-exclusion markers in `source/counters/` is lower than
it was before this feature, because the calibration region loses its
exclusion. The clang-tidy warning count of each touched translation unit
does not rise. The `pmu-events` check reports no drifted file and does
not touch the network. Proves SC-011.

### Baseline figures this feature is measured against

Both were taken at `6aafd2d`, the head before any correction, and both
gate the figures the corrections produce.

**Coverage-exclusion markers** under `source/counters/`: 377 lines carry
`LCOV_EXCL`. Per file: `linux_pmu/provider.cpp` 122, `linux_pmu/group_io.cpp`
82, `plan.cpp` 41, `fast_read.cpp` 39, `system.cpp` 23,
`clock_provider.cpp` 22, `linux_pmu/encode.cpp` 17,
`linux_pmu/table_parse.cpp` 17, `fold.cpp` 10, `detail/pmu.hpp` 4.

This feature removes the calibration region at `source/counters/plan.cpp`
(T048) and the release-arm markers at `source/counters/linux_pmu/fast_read.cpp`
(T074), and adds none.

**Measured after both removals: 371 lines.** Per file:
`linux_pmu/group_io.cpp` 89, `linux_pmu/provider.cpp` 119, `plan.cpp` 39,
`linux_pmu/fast_read.cpp` 31, `system.cpp` 23, `clock_provider.cpp` 22,
`linux_pmu/encode.cpp` 17, `linux_pmu/table_parse.cpp` 17, `fold.cpp` 10,
`detail/pmu.hpp` 4.

`plan.cpp` fell from 41 to 39, which is the calibration region T048
retires: a registered test now reaches the calibration on any host.
`fast_read.cpp` fell from 39 to 31, which is the release-arm region T074
retires: the lifecycle test reaches it with resources it opened itself.
`provider.cpp` fell from 122 to 119, which is the per-target probe T038
replaced: the extracted selection now decides the mode, and the markers
over the old ternary went with it. `group_io.cpp` rose from 82 to 89
because the disclosure column, the retry verdict, and the extracted pair
decision each carry a marker over a path only a granted
`perf_event_open` reaches.

**clang-tidy warnings** per translation unit, from the `ci-ubuntu` build
at `6aafd2d`, which runs clang-tidy over every unit:

| Translation unit | Warnings |
| --- | --- |
| `source/counters/plan.cpp` | 49 |
| `source/counters/system.cpp` | 48 |
| `source/counters/fold.cpp` | 43 |
| `source/counters/linux_pmu/provider.cpp` | 33 |
| `source/counters/linux_pmu/table_parse.cpp` | 68 |
| `source/counters/linux_pmu/fast_read.cpp` | 17 |
| `source/counters/linux_pmu/group_io.cpp` | 59 |

T057 **fails**, and the feature cannot merge until it is corrected. Every
translation unit this feature touched reports more clang-tidy warnings
than it did at `6aafd2d`:

| Translation unit | Pre-fix | Now | Delta |
| --- | --- | --- | --- |
| `source/counters/plan.cpp` | 49 | 62 | +13 |
| `source/counters/linux_pmu/table_parse.cpp` | 68 | 78 | +10 |
| `source/counters/linux_pmu/group_io.cpp` | 59 | 64 | +5 |
| `source/counters/fold.cpp` | 43 | 46 | +3 |
| `source/counters/linux_pmu/provider.cpp` | 33 | 36 | +3 |
| `source/counters/linux_pmu/fast_read.cpp` | 17 | 19 | +2 |
| `source/counters/system.cpp` | 48 | 49 | +1 |

Both columns come from a `ci-ubuntu` build, which runs clang-tidy over
every unit, and each unit was fully re-checked on both sides, so the two
columns are comparable. FR-041 requires that no unit's count rise, and
every one rises, so the gate is not met.

A count per unit cannot say which warnings the feature is responsible
for, because an edit that shifts lines moves warnings it never wrote.
The sharper measure intersects each warning's line with the lines
`git diff -U0 6aafd2d..HEAD` reports as added for that unit, which
attributes a warning to this feature only when the feature wrote the
line it names. On that measure **45 warnings land on lines this feature
added**, in these clusters:

| Translation unit | Warnings on added lines |
| --- | --- |
| `source/counters/plan.cpp` | 13 |
| `source/counters/linux_pmu/table_parse.cpp` | 12 |
| `source/counters/linux_pmu/group_io.cpp` | 9 |
| `source/counters/linux_pmu/provider.cpp` | 4 |
| `source/counters/linux_pmu/fast_read.cpp` | 3 |
| `source/counters/fold.cpp` | 3 |
| `source/counters/system.cpp` | 1 |

This is the work T057 leaves, and it is not done.

**T057 passes.** A clean `ci-ubuntu` build measures **0** findings on
lines this feature added, and every unit's total now sits below what it
was at `6aafd2d`:

| Translation unit | Pre-fix | Now | Delta | On added lines |
| --- | --- | --- | --- | --- |
| `source/counters/linux_pmu/table_parse.cpp` | 68 | 51 | -17 | 0 |
| `source/counters/linux_pmu/group_io.cpp` | 59 | 46 | -13 | 0 |
| `source/counters/system.cpp` | 48 | 35 | -13 | 0 |
| `source/counters/fold.cpp` | 43 | 28 | -15 | 0 |
| `source/counters/plan.cpp` | 49 | 38 | -11 | 0 |
| `source/counters/linux_pmu/provider.cpp` | 33 | 22 | -11 | 0 |
| `source/counters/linux_pmu/fast_read.cpp` | 17 | 13 | -4 | 0 |

FR-041 asks that no unit's count rise. None rises and every one falls.

The route there was 45 findings on added lines down to 0, and the last
step was not a code change. `Checks` in `.clang-tidy` is a wildcard, so
`llvm-prefer-static-over-anonymous-namespace` and
`misc-use-anonymous-namespace` both ran, and a file-local function
cannot satisfy both. The first reported 269 findings tree-wide against 1
for the second, so it is the one that dissents from this tree's
convention and it is now disabled, with the reason recorded in the file.
That one line accounts for most of the drop in every column above.

Three earlier notes in this section are superseded. An intermediate count
of 25 came from an incremental build whose log held warnings for the
units that recompiled alone, and the figures of 19, 16, 27, 12 and 8
came from subtracting corrections off a stale total. Every figure here
comes from a clean build, which recompiles every unit.

## Step 12: traceable record

```sh
grep -c '^- \*\*FR-' specs/012-counters-defect-resolution/spec.md
grep -rn 'LCOV_EXCL' source/counters/ | wc -l
git diff --stat 6aafd2d..HEAD
```

**Expected**: 45 requirements, and the marker count is lower than the
pre-fix count. Every changed line traces to a requirement.

Every correction lands in the successor log at
`specs/007-counters-and-timers/citations-log.md`, in that file's entry
format, naming the 007, 008, or 011 requirement it restores (FR-038).
The frozen record takes no edit. The entry format is in
[data-model.md](data-model.md), under the successor-log entry entity.

## Commit shape

One idea per commit, `<Section>: <imperative>` at 50 characters or
fewer, a why-body wrapped at 72 columns, `Approved-by:`, `Fixes #n`, and
`Refs: specs/012-counters-defect-resolution`. TDD order holds inside
each pair: the failing test lands first, the correction second.
