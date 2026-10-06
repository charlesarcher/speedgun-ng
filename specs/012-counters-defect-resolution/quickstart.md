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
tables are static data, not common symbols, so nothing moved out of
`bss`.

Proves SC-012.

## Step 11: the remaining gates

```sh
cmake --build build/dev -t dbc-gate
cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake
cmake --build build/dev -t format-check
cmake -P cmake/prose-lint.cmake
python3 tools/pmu_events/update_pmu_events.py --check
cmake --preset=coverage-linux
cmake --build build/coverage -j 2
ctest --test-dir build/coverage --output-on-failure
cmake --build build/coverage -t coverage
```

**Coverage note.** The coverage build and its 45 tests pass. The gate
itself then stops inside lcov, which errors on its own instrumentation:
`Unexpected negative taken count '-40202' for branch 1 while capturing
from clock_provider.cpp.gcda`. That is a tool defect, and it means the
gate reports no verdict here, so FR-040 is unmeasured on this host rather
than met. The `coverage-linux` preset named in this feature's commands is
also hidden, and CMake refuses it by name; `ci-coverage` inherits it and
configures the same tree, so that is the preset a run must use.

**Sanitizer note.** `ci-sanitize` builds clean and 44 of its 45 tests
pass, but `counters_linux_pmu_seam_test` fails there and passes in the
dev build, on the T025 partial-open scenario: "a partial open releases
every mapping it acquired". A standalone probe against the sanitized
archive reproduces it and names the figure: 64 contexts acquired and
released leave `/proc/self/maps` at 131 lines against 91 before the
loop, so roughly forty mappings survive. The same loop in the dev build
returns to its starting count.

The gap between the two builds is unexplained and T061 stays open. The
candidates are a release the sanitizers change and a defect the
optimized build hides. Nothing here records which, because nothing here
has established it. The scenario is the one T025 added, so the finding
belongs to this feature's own test and not to a pre-existing gate.

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

T057 **fails**, and the feature cannot merge until that is corrected. Every
translation unit this feature touched reports more clang-tidy warnings
than it did at `6aafd2d`:

| Translation unit | Pre-fix | Now | Delta |
| --- | --- | --- | --- |
| `source/counters/plan.cpp` | 49 | 62 | +13 |
| `source/counters/linux_pmu/table_parse.cpp` | 68 | 75 | +7 |
| `source/counters/linux_pmu/group_io.cpp` | 59 | 64 | +5 |
| `source/counters/fold.cpp` | 43 | 46 | +3 |
| `source/counters/linux_pmu/fast_read.cpp` | 17 | 19 | +2 |
| `source/counters/system.cpp` | 48 | 49 | +1 |
| `source/counters/linux_pmu/provider.cpp` | 33 | 34 | +1 |

Both columns come from a `ci-ubuntu` build, which runs clang-tidy over
every unit, and each unit was fully re-checked on both sides, so the two
columns are comparable. FR-041 requires that no unit's count rise, and
every one rises, so the gate is not met.

The counts by check, across the seven units, are 57
`cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`, 45
`llvm-prefer-static-over-anonymous-namespace`, 38
`readability-trailing-comma`, 35 `misc-include-cleaner`, 22
`readability-identifier-length`, 17
`cppcoreguidelines-pro-bounds-pointer-arithmetic`, and 13
`readability-math-missing-parentheses`, with smaller counts for the
remainder. The correction for each is a mechanical pass over the lines
this feature added, and it is not done.

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
