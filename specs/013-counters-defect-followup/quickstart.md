# Quickstart: validating counters defect follow-up

Every command below runs unprivileged on Linux at `perf_event_paranoid` 2,
the level the continuous-integration matrix runs at. Nothing here needs an
Intel host, a privileged event, or a real uncore device (012 FR-034).

**The audit point is the pre-fix head**: `0dea082`. Every "fails at the
audit point" line below was observed there before its correction landed.
That is the property SC-010 requires and F-05 names as the defect in the
merged tests.

## Prerequisites

```bash
cmake --preset=dev
cmake --build --preset=dev -j "$(nproc)"
```

Confirm the tree is the one the plan describes:

```bash
git rev-parse HEAD
test -d specs/013-counters-defect-followup
```

## Step 1: observe the four wrong-value defects before correcting them

Each command runs one test that passes at the audit point and fails once
rewritten. Run them at the audit point first if you want to see the green
baseline that F-05 names.

```bash
ctest --test-dir build/dev -R counters_recorder_test --output-on-failure
ctest --test-dir build/dev -R counters_objects_test --output-on-failure
ctest --test-dir build/dev -R counters_linux_pmu_seam_test --output-on-failure
ctest --test-dir build/dev -R counters_pmu_test --output-on-failure
```

What each one shows, and what it shows after its rewrite:

| Test | At the audit point | After the rewrite, before the fix |
| --- | --- | --- |
| `counters_recorder_test` | passes, with every count starting at zero | fails: a fold that ends at a gap point reports a delta near 2^64 |
| `counters_objects_test` | passes, with the disclosure column named to the plan's last leaf | fails: a PMU gap is lost when the clock provider registered second |
| `counters_linux_pmu_seam_test` | passes, with no scale field in its fixture | fails: the disclosed ratio does not match the header recipe |
| `counters_pmu_test` | passes, with one host-wide fast verdict | fails: an uncore entry carries the fast mode its device refuses |

**The two rewritten 012 tests are the gate that was missing.** Both pass at
the audit point and both fail once rewritten, because the recorder fixture
starts every count at zero so the gap's zero equals the first real point,
and because the multiplex fixture has no field for `cap_user_time`,
`time_shift`, `time_mult`, or `time_offset`. That is F-05, and SC-010
forbids it.

## Step 2: the gap state and the fold's two reads

```bash
ctest --test-dir build/dev -R counters_recorder_test --output-on-failure
```

What to look for:

```bash
./build/dev/test/counters_recorder_test 2>&1 | grep -i 'gap\|availability'
```

The rewritten fixture drives cumulative counts above zero before the gap
point, so the zero the gap writes differs from the first real point. Assert
three windows: a gap at the end point, a gap at the start point, and a gap
strictly inside. The first two report no value and publish `gap`; the third
reports a correct delta, because the counts are cumulative. A caller reads
the state with no column index in its source.

## Step 3: per-window disclosure under both registration orders

```bash
ctest --test-dir build/dev -R counters_objects_test --output-on-failure
```

The plan holds a PMU leaf and a clock leaf, drives a PMU gap, and reads the
fold result under each of the two provider registration orders. Both orders
must reach the result. Under the audit point the clock-second order loses
the gap.

## Step 4: the kernel's time recipe

```bash
ctest --test-dir build/dev -R counters_linux_pmu_seam_test --output-on-failure
```

The fixtures are synthetic event pages, so this runs identically on every
host:

- a page whose `cap_user_time` is set, whose enabled time differs from its
  running time, and whose scale, offset, and multiplier are non-trivial
- a second page whose `cap_usr_time_short` is set, whose cycle value needs
  the `time_cycles` and `time_mask` correction
- a page that states neither, which copies the raw pair unchanged

The recipe the header documents at
`/usr/src/linux-cachyos/include/uapi/linux/perf_event.h:674-688` and
`:713-723`:

```text
quot  = cyc >> time_shift
rem   = cyc & ((1 << time_shift) - 1)
delta = time_offset + quot * time_mult + ((rem * time_mult) >> time_shift)

enabled += delta
if (index)
  running += delta
```

and, where `cap_usr_time_short` is stated:

```text
cyc = time_cycles + ((cyc - time_cycles) & time_mask)
```

The disclosed ratio must match within one tick. Both fixtures fail at the
audit point, because the fixture had no field for any of the six values and
the reader copies the raw pair.

## Step 5: the register filter and the encodable counts

```bash
ctest --test-dir build/dev -R counters_linux_pmu_seam_test --output-on-failure
```

```bash
./build/dev/test/counters_linux_pmu_seam_test 2>&1 | grep -i 'encodable rows'
```

Before and after, per directory:

| Directory | At the audit point | After this feature |
| --- | --- | --- |
| skylake | 576 | 587 |
| icelake | 342 | 346 |
| alderlake | 521 | 563 |
| sapphirerapids | 1685 | 2222 |
| amdzen4 | 326 | 326 |
| amdzen5 | 322 | 322 |

The "at the audit point" column was measured at `0dea082`, from
`seam encodable rows:` lines against the synthetic list, and matches the
reference host list figure for figure. The "after" column is the target,
and step 5 is what settles whether it was reached.

Two things to check beyond the figures:

- the synthetic device's format list grows `ldlat` and `frontend` at
  `config1:0-15` and `config1:0-23`, the ranges the kernel publishes at
  `arch/x86/events/intel/core.c:6606` and `:6608`. Without them, the 122
  rows naming those formats flip to `not_encodable` and the count falls
  below the audit-point figure.
- a row carrying a register filter encodes that filter. The pinned tree's
  570 such rows across the four Intel directories publish as `countable`
  with no filter at the audit point, so a spot check that one row's config
  carries its register value is the direct assertion.

## Step 6: device routing, fast verdicts, and target scope

```bash
ctest --test-dir build/dev -R counters_pmu_test --output-on-failure
```

The fixtures are synthetic device trees, so this runs on a host that
publishes no uncore and no RAPL device, which this one does not:

- a core device and an uncore device on a fast-capable host: the fast mode
  lands on the core entries alone, and the uncore leaf reads with no gap
- devices named `uncore_cha_0`, `uncore_cha_1`, `uncore_imc_0`, `amd_df`,
  `amd_l3`, and `amd_umc_0`, each receiving the rows its unit names, with
  no uncore row on a core device
- a model-specific-register device whose per-task probe succeeds: the
  thread target bit lands on its entries
- a refused fast read: a gap is disclosed and no syscall read follows

`counters_pmu_test` carries `SKIP_RETURN_CODE 2`, so a host that publishes
no device skips and no failure follows. The synthetic fixtures must
avoid that path: a fixture that skips is a fixture that did not run.

## Step 7: the package, the version, and the installed count

```bash
cmake -P cmake/spell.cmake
grep -n 'COMPATIBILITY' cmake/install-rules.cmake
grep -n 'VERSION' CMakeLists.txt | head -3
```

```bash
cmake --preset=ci-ubuntu
cmake --build build -j "$(nproc)"
ctest --test-dir build -C Release --output-on-failure --no-tests=error
```

What to confirm:

- the package config compatibility reads the minor-version compatibility
  while the project major version stays 0
- the project version reads 0.4.0 and the shared-object version reads 1
- the version lineage names both declarations `cd5cbd1` removed, with the
  bump, and restores neither

The consumer-count step lives in the audit job and has no local shortcut.
To exercise it without the workflow, run the consumer and the build tree's
test on the same machine and compare their printed counts:

```bash
./build/test/consumer            # prints the catalog entry count
```

The workflow step compares those counts and exits non-zero on a
difference. A runner that publishes no device prints zero on both sides
and the step holds.

## Step 8: the overhead figure this feature gates

```bash
cmake --preset=dev
cmake --build --preset=dev -j "$(nproc)"
ctest --test-dir build/dev -R counters_overhead --output-on-failure
```

The release-build median must stay within five percent of the median
measured at the audit point. Record the new median in
`docs/pages/counters-overhead.md` beside the one it replaces, with the same
recipe the page already documents: 64 warm-up actions per plan, 257 timed
actions sorted then reported as min, median, and max, on a pinned
processor.

Nothing this feature adds runs on the sampling path. The disclosure is
written once per action per window, which the clock window already does;
the fold's two extra reads happen once per figure. If the median moves by
more than five percent, the correction is wrong and the figure is
right.

### At the audit point

Measured at `0dea082` on the release preset, on this host, an AMD Ryzen 9
9950X3D running Linux 7.2.4-1-cachyos at `perf_event_paranoid` 1, with
the TSC measured at 4.300 GHz. The page's own recipe governs all of it:
1000 actions x 64 repeats per regime, the bare bracketing included.

| Regime | min ns | median ns | max ns |
| --- | --- | --- | --- |
| library sampling path | 8.8 | 8.8 | 10.7 |
| bare rdtsc pair (the bracketing) | 8.6 | 8.6 | 10.5 |
| clock, syscall (vDSO) | 20.0 | 20.0 | 30.0 |
| clock raw, syscall (vDSO) | 20.0 | 20.0 | 30.0 |
| gated clock, monotonic | 23.7 | 23.7 | 30.0 |
| pmu group, fast_rdpmc | 50.0 | 60.0 | 60.0 |
| gated core PMU group | 60.0 | 60.0 | 64.8 |
| pmu single, fast_rdpmc | 30.0 | 40.0 | 60.0 |

The two figures this feature gates are the library sampling path, at
8.8 ns median, and the gated core PMU group, at 60.0 ns median. Each is
held within five percent, so 9.3 ns and 63.0 ns respectively. The figure
a host reports depends on its own TSC and its own kernel, so the gate is
the ratio on the same host under the same recipe, never this table
copied onto another one.

Two figures at the audit point are already off the page's published floor:
the clock regime reports a 20.0 ns median against a published floor of
40.0 ns, and the same is true of every fast regime below it. That is
pre-existing and out of scope here; this feature does not move a
published floor, it holds the sampling-path median.

## Step 9: every remaining gate

```bash
ctest --test-dir build/dev --output-on-failure --no-tests=error
cmake --build build/dev -t dbc-gate -j 2
cmake -P cmake/spell.cmake
python3 tools/prose/prose_gate.py --check prose --mode tree
cmake --build build/dev -t prose-lint-fixtures
ctest --test-dir build/dev -R prose_gate_fixtures --output-on-failure
```

The address and thread sanitizer presets, and the coverage gate:

```bash
cmake --preset=ci-sanitize
cmake --build --preset=ci-sanitize -j "$(nproc)"
ctest --preset=ci-sanitize

cmake --preset=ci-tsan
cmake --build --preset=ci-tsan -j "$(nproc)"
ctest --preset=ci-tsan

cmake --preset=ci-coverage
cmake --build build/coverage -j 2
ctest --test-dir build/coverage --output-on-failure --no-tests=error
cmake --build build/coverage -t coverage
```

The coverage gate holds at 100 percent line, branch, and contract
coverage, and the exclusion-marker count in `source/counters/` does not
rise. This feature adds no exclusion marker, which is why D-01, D-02,
D-03, D-07, D-09, and D-10 each need their own test. No exclusion
covers them.

The header purity scan, which no public platform term may enter:

```bash
ctest --test-dir build/dev -R counters_header_purity --output-on-failure
```

The availability field is a plain enumeration already declared in
`counters_core.hpp`, so this gate holds without an edit.

### At the audit point

Measured at `0dea082`, so a later run can tell a pre-existing finding
from a regression this feature introduces:

| What | At the audit point | After this feature |
| --- | --- | --- |
| `grep -ro 'LCOV_EXCL_[A-Z]*' source/counters/ \| wc -l` | 366 | 366 |
| badly formatted files, from `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | rc 1, 21 files | the same 21 files, no new file on the list |
| `ctest --test-dir build/dev -R 'counters_recorder_test\|counters_objects_test\|counters_linux_pmu_seam_test\|counters_pmu_test'` | 4 of 4 pass | 4 of 4 pass after each is rewritten |
| `counters_pmu_test` target-mask line | `target masks: 0 of 40 entries cpu-only` | the same 40 entries, with the mask figures the correction publishes |
| `counters_pmu_test` availability line | `pmu availability: 10 countable, 0 permission_blocked, 30 not_encodable, 0 scope_refused, 8 fast_rdpmc` | the recount step 5 measures |

The 21 badly formatted files at the audit point are:

```text
include/speedgun-ng/counters_measurement.hpp
include/speedgun-ng/counters_provider.hpp
include/speedgun-ng/counters_system.hpp
source/counters/clock_provider.cpp
source/counters/detail/pmu.hpp
source/counters/fake_provider.cpp
source/counters/fold.cpp
source/counters/linux_pmu/embedded_tables.hpp
source/counters/linux_pmu/encode.cpp
source/counters/linux_pmu/fast_read.cpp
source/counters/linux_pmu/group_io.cpp
source/counters/linux_pmu/provider.cpp
source/counters/linux_pmu/table_parse.cpp
source/counters/plan.cpp
source/counters/push_provider.cpp
source/counters/system.cpp
test/source/counters_clock_push_test.cpp
test/source/counters_linux_pmu_seam_test.cpp
test/source/counters_noalloc_test.cpp
test/source/counters_pmu_test.cpp
test/source/counters_provider_ext_test.cpp
```

Every file this feature touches was already on that list except
`include/speedgun-ng/counters_core.hpp`, which is clean at the audit
point and has to stay clean. Reformatting the 21 is out of scope: this
feature holds the list flat and adds no file to it.

## Baseline figures this feature is measured against

The four counters tests the defect rewrite targets all pass at the audit
point, which is the green baseline SC-010 names as the gate that was
missing:

```text
counters_linux_pmu_seam_test .....   Passed    0.15 sec
counters_objects_test ............   Passed    0.00 sec
counters_recorder_test ...........   Passed    0.00 sec
counters_pmu_test ................   Passed    0.21 sec
100% tests passed out of 4
```

Each fails once its fixture is rewritten, and the rewritten fixture is
where the failing run is observed.

Measured at the audit point, and reproduced here so a reader can check the
plan's numbers without rerunning anything:

| Directory | Rows | Non-zero register value | Offcore index | Load latency | Frontend | `Counter` parses | `Deprecated` parses |
| --- | --- | --- | --- | --- | --- | --- | --- |
| skylake | 587 | 287 | 260 | 8 | 19 | 4 | 1 |
| icelake | 346 | 96 | 71 | 8 | 17 | 2 | 2 |
| alderlake | 563 | 86 | 46 | 19 | 21 | 13 | 30 |
| sapphirerapids | 2693 | 101 | 71 | 9 | 21 | 447 | 9 |
| amdzen4 | 502 | 0 | 0 | 0 | 0 | 0 | 0 |
| amdzen5 | 579 | 0 | 0 | 0 | 0 | 0 | 0 |

Across the whole pinned tree, `0x1a6,0x1a7` appears on 5045 rows,
`0x1A6` on 810, `0x3F6` on 379, `0x3F7` on 307, `0x1a6, 0x1a7` with a
space on 64, `0x1a6` on 60, and `0x1a7` alone on 18. Every one resolves
under the kernel's own map, taken at its first index.

## The Intel confirmation stays deferred

Every step above runs on synthetic inputs. Nothing here needs an Intel
host, and 012's deferred confirmation stays deferred (012 FR-039). The one
figure that depends on the running kernel is the availability of
`offcore_rsp`, `ldlat`, and `frontend` in a device's own format list: this
host publishes none of the three, so every register-filter row publishes
`not_encodable` here, which is a correct answer and not a defect. That is
why the pinned encodable figures are measured against the synthetic list.

## Walk result

A reviewer who runs every step in order and lands on a green tree has
verified: five wrong-value defects closed with tests that failed at the
audit point, four event-usability defects closed against synthetic device
trees, four package and gate items closed, the encodable counts moved to
their predicted figures, the sampling-cost median within five percent, and
every hard gate green with no new exclusion marker and no new clang-tidy
warning.

## Commit shape

One commit per issue, each carrying its own failing test, each named
`<Section>: <one-line imperative>` at fifty characters or fewer with a
why-body wrapped at seventy-two columns, an `Approved-by:` footer, and
`Refs: specs/013-counters-defect-followup`. Fourteen issues, fourteen
commits, so each is bisectable and each revert is a defect the next test
run catches.
