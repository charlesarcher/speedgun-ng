# Quickstart: Hunter mode core (event ranking)

**Feature**: specs/029-hunter-core | **Audience**: the implementer and the
reviewer of the PR | **Audit point**: `bd10d76`

This guide builds the change, runs the tests that cover it, and shows each
behaviour with the output the code must produce. Every expectation below comes
from [contracts/hunt-cli.md](contracts/hunt-cli.md),
[contracts/hunt-json.md](contracts/hunt-json.md),
[contracts/hunt-api.md](contracts/hunt-api.md) and
[contracts/report-schema-key.md](contracts/report-schema-key.md).

## Prerequisites

- Linux x86-64. Linux is the supported platform: `ci-rocky` on Rocky Linux 10
  and `ci-ubuntu` on Ubuntu 26.04 are the two release presets, and a future
  specification is what adds a platform
- GCC and Clang, CMake 3.20 or newer, `python3` for the documentation gates,
  `doxygen` for `dbc-gate`, and `lcov` plus `gcov` for the coverage target
  (Principle VIII)
- No PMU access for the tests: every scripted case drives `FakeProvider`. A host
  that answers `/proc/sys/kernel/perf_event_paranoid` above 2 runs the whole test
  plan (AC-5); only the two recorded AC-10 hardware runs need a counting host
- No network at build, test or run time
- A fresh clone carries no `CMakeUserPresets.json`; `ci-rocky` and `ci-ubuntu`
  are the only two build routes this guide uses

## Build

```bash
cmake --preset=ci-rocky
cmake --build build
```

The `ci-ubuntu` preset carries clang-tidy and cppcheck and runs once per feature
(Principle IX):

```bash
cmake --preset=ci-ubuntu
cmake --build build
```

## Run the hunt tests

```bash
ctest --test-dir build --output-on-failure -R 'hunt|cli_table' -j 2
```

Expected: `harness_hunt_plan_test`, `harness_hunt_rank_test`,
`harness_hunt_collect_test`, `harness_hunt_cli_test`,
`harness_hunt_report_test`, `harness_hunt_json_test` and
`harness_cli_table_test` all pass. `harness_cli_table_test` reports option names and option values unique across
the whole table, every hunt value inside 270 to 279, the block the lane
reserves, and the planted clash in the fixture twin of the table.

## Walkthrough: US1 plan, collect, rank

Drive the scripted plan decision table and the scripted collection.

```bash
ctest --test-dir build -R 'harness_hunt_plan_test' -V
ctest --test-dir build -R 'harness_hunt_collect_test' -V
```

Expected, one case per row of the plan decision table in
[contracts/hunt-api.md](contracts/hunt-api.md):

- one column per event, each holding the event plus the two anchor leaves of the
  same device, and no third column shape
- limit 6 with 200 events gives 50 passes, 4 events in each, both anchors
  claimed once per pass, and `planPasses` returns no error for the width
- limit 3 gives one event per pass; limit 1 or 2 with both anchors present prints
  one refusal line naming the device and the limit and exits 2 (FR-081)
- `--hunt-norm=cycle` on a host whose cycles anchor stands refused prints one
  refusal line and exits 2, while the unset norm falls back to `iter` in the
  open (D-13)
- no published limit settles `MULTIPLEX`, names every event `multiplexed` and
  prints the plan line with the word `MULTIPLEX`
- a high-rate event ranks above a flat one, a flat event whose CV is above the
  threshold prints `noisy`, stays out of the top N and stays in the JSON
- the skipped list carries `permission-blocked`, `not-encodable`, `absent` and
  `scope-refused`, each from a scripted `Availability`
- `isCoreDevice` answers true for a fixture device whose path starts with
  `armv8_`, and `--hunt=all` selects that device's entries (D-1)
- `setGroupLeafLimit` refuses the wide pass, the run retries its events one at a
  time inside the same pass loop, `retriedPasses` counts the pass, an event that
  still refuses joins the skipped list with the kind of the state its refusal
  names (a refusal reporting `not_encodable` answers `not-encodable`, one
  reporting `permission_blocked` answers `permission-blocked`, and one that
  names no state answers `absent`, which is the answer the scripted refusal of
  this host gives), and the run exits 0
- two contexts: the window-open count per pass read from the scripted provider
  equals one per context per pass, with no sleep and no clock read

The rate, CV, noise and rank rows come from `harness_hunt_rank_test`:

```bash
ctest --test-dir build -R 'harness_hunt_rank_test' -V
```

Expected: `instr` divides by the `instructions` count of the same pass, `cycle`
by `cpu-cycles`, `iter` reports the raw per-iteration median, and the median and
CV a case reports come from the shared `statisticsOf` call on the same sample
list: a reported `cv` is exactly one hundred times the printed `cv=` ratio
(I-6). The single-sample case reports `cv` `0.0`, no rank, and the `unrepeated`
mark.

## Walkthrough: US2 budget and printed plan

```bash
ctest --test-dir build -R 'harness_hunt_cli_test' -V
ctest --test-dir build -R 'harness_hunt_report_test' -V
```

Expected for the fixed seed, the fixed 200-event catalog and the fixed estimate:

```text
hunt: plan events=200 passes=50 method=PASSES estimate=96000000ns budget=8p
hunt: plan budget cut stage=2 reason=over-budget
```

Calibration answers 1,920,000 ns per pass, and the plan line states the stage-1
figure: 1,920,000 x 50 passes x 1 repeat = 96,000,000 ns, whole numbers
throughout (D-2). The 8-pass budget cannot fit a stage-2 sweep on top of the
50-pass stage-1 sweep, so the plan drops stage 2 entirely and the run prints one
`budget cut` line naming the reason. Stage 1 never shrinks: all 200 events still
run at one repeat (D-2, FR-013). A budget that fits both stages prints no cut
line: `60p` hosts the 50-pass stage 1 plus a 12-event stage-2 set that takes 3
passes at `--hunt-repeats` 3, nine more pass runs, 59 in total. A budget of
`120` with no suffix, `0s`, a negative count, `--hunt-repeats=0`,
`--hunt-top=10001`, `--hunt-rank=sensitivity` and `--hunt=cpu/r4a03` each print
one line naming the value and exit 2. No estimate prints
`estimate=not available`. The plan line prints before the first row.

## Walkthrough: US3 JSON objects and schema

1. Run the hunt JSON suite and the JSON suites around it.

   ```bash
   ctest --test-dir build -R 'harness_hunt_json_test' -V
   ctest --test-dir build -R 'harness_json' -V
   ```

2. Read the expectations those two suites state for a hunt document.

   Expected: a hunt document carries the top-level `hunt` object after
   `benchmarks` and a per-entry `hunt` object after `spreadWarning`;
   `checkReport` over the schema at `SG_SCHEMA_PATH` answers clean for the hunt
   document; the round trip through `loadReport` reads every field back equal,
   including a `rank` that is null and a `budget` that is null; one test per
   refused reader code fails the load with the pointer named; a document
   without hunt keys loads with both hunt members empty (FR-050, FR-051,
   AC-6).

3. Read the two `hunt` keys in `docs/schema/report-v1.json`. Both keys stand in
   `x-order` and in `properties` of the root object and of the benchmark entry
   object, and in neither `required` list (FR-053, D-17).

4. Grep the two FR-052 facts off the document that a run with no `--hunt`
   writes.

   ```bash
   ./build/example/docs_json_reports --json=/tmp/hunt-free.json
   grep -c '"hunt"' /tmp/hunt-free.json
   grep -oE '"schemaVersion":"[0-9.]+"' /tmp/hunt-free.json
   ```

   The first grep answers `0` and exits 1: no hunt key stands in the document
   of a run without the hunt option (FR-052). The second answers the version
   this feature reserves, `1.4` (FR-053, D-17). The byte golden of
   `harness_json_report_test.cpp` carries the byte, console and exit-status
   comparison behind those two lines (AC-6). T048 moved `kSchemaVersion` to
   `1.4` and T049 moved the rows that name it, so the tree stamps `1.4`.

5. Grep every line that names a schema version, so a red line there points at
   the version.

   ```bash
   grep -rnE '"[0-9]\.[0-9]|schema [0-9]\.[0-9]|`[0-9]\.[0-9]' \
     source/harness/json_report.cpp \
     test/source/harness_json_load_test.cpp \
     test/source/harness_compare_cli_test.cpp \
     docs/pages/harness.md
   ```

   The pattern matches the value wherever a file spells it, in the quoted form
   the writer and a written document use, in the escaped form the loader test
   builds, in the `schema 1.1` console words, and in the backticks of the page.
   Four lines carry the value the bump moves: `json_report.cpp:32` the writer
   constant, `harness_json_load_test.cpp:113` the current-version document,
   `harness_json_load_test.cpp:529` the kind refusal row beside it, and
   `docs/pages/harness.md:1067` the sentence of the page.
   `harness_compare_cli_test.cpp:516` states the header that
   `test/data/gate_schema_11.json` carries, a frozen fixture, and keeps `1.1`.
   T048 moves the writer constant and T049 moves the three rows that name it.

## Walkthrough: US4 documentation chain

```bash
ctest --test-dir build \
  -R 'example_drift|example_drift_fixtures|snippet_compile|report_shape|example_run' \
  --output-on-failure
python3 tools/prose/prose_gate.py --check prose --mode tree
```

Expected: `SECTIONS` holds `"Hunter mode"` with `hunt_rank.cpp` as its
`SECTION_OWNERS` entry; `PAGE_LINE_CAP` carries the measured README length with
the move note beside the comment block
(`tools/docs/example_drift.py:100-155`); `snippet_compile` compiles the copied
block; `example_run` runs `docs_hunt_rank`, which registers benchmarks that print
rows and prints its skipped list from the scripted refusal on every host;
`report_shape_hunt_rank_sample` compares that run with
`example/docs/hunt_rank.expected` by shape; the prose gates pass on `README.md`,
`docs/pages/harness.md` and every file under `specs/029-hunter-core/`.

## Coverage and sanitizers

```bash
cmake --preset=ci-coverage
cmake --build build/coverage --target coverage
```

The preset is master's `ci-coverage`, unchanged, and its tree is
`build/coverage`. This feature adds no preset and touches no workflow file
(hard rule 14).

Expected: line and branch coverage of 100 percent with no exclusion marker and
no waiver, and `dbc-gate` at 100 percent. Confirm no marker entered the new
files:

```bash
git grep -n 'LCOV_EXCL' -- source/harness example test
```

A gap is a stop: capture `gcov -b` for the file and report the cause with the
evidence (FR-091). The sanitizers run as CI jobs, `ci-sanitize` and `ci-tsan`;
the threaded hunt case is the one this feature adds to the thread-sanitizer
route, and it reads window counts from the scripted provider; no barrier wait
joins the case (hard rule 1, FR-092).

## Remediation for a failed check

| Failing check | Remediation |
|---|---|
| `harness_hunt_plan_test` limit row wrong | the column shape is not one event plus the two anchors of the same device: read `planHunt` against the `ColumnLeaves` contract at `detail/pass_plan.hpp:41-107`, never a copy of the planner |
| A hunt CV stops standing at one hundred times the printed `cv=` column | `rankEvents` computed a median or deviation itself; it must call the shared `statisticsOf` (`detail/statistics.hpp`) and nothing else |
| `harness_cli_table_test` reports a clash | an option name or value repeats: the eight hunt constants take 270 to 277 and the lane reserves 270 to 279 (D-22) |
| `checkReport` rejects a hunt document | a `hunt` key stands outside `x-order`, or stands in a closed hunt node's document and not in its `properties` |
| the byte-identity of a non-hunt run moved | the seat route changed for a run with no hunt columns: `SG_INVARIANT` at `runner.cpp:2287-2292` still serves that route, and the hunt branch is taken only when the plan carries hunt columns |
| `example_drift` fails on the option table | an option row is missing from the main page: `example_drift.py:681-693` compares every README row with every row of `cli.cpp`'s table |
| `example_run` fails on `docs_hunt_rank` | the example printed no row, or its skipped list moved: the scripted refusal owns the output and no host fact does |
| the AC-1 or AC-2 hardware record misses | read `perf list` and the device's `programmableCounters` answer on the host, and read the plan line's `method=` word: an `armv8_` device publishes no limit and plans `MULTIPLEX` |

## Evidence to record

| Evidence | Where it goes |
|---|---|
| The limit decision table of `harness_hunt_plan_test`, 1, 3, 4, 6, 8, no limit, and 200 events at limit 6 | the PR body |
| The scripted rank table with the expected order and marks | the PR body |
| The scripted group-refusal retry: pass fell back, retried passes, skipped list, exit status 0 | the PR body |
| The fixed-seed, fixed-estimate two-stage plan for AC-7 | the PR body |
| The non-hunt byte comparison, console and document, with `schemaVersion` excluded | the PR body |
| The `harness_cli_table_test` result over the tree table and over the planted fixture | the PR body |
| The coverage lines for the new files with no exclusion | the PR body |
| AC-10: the recorded hunt run on `corsairone` and on `gary`, each with the host's `perf_event_paranoid`, the device parts, the published limits, and the `hunt: plan` line | `specs/029-hunter-core/results/` |

## Timing on the development host

| Step | Time |
|---|---|
| `cmake --preset=ci-rocky` | under a minute |
| `cmake --build build -j` | about 2 minutes |
| `ctest -R 'hunt\|cli_table' -j 2` | a few seconds: every hunt case is scripted |
| the full suite | minutes on a release build, and far longer under an instrumented build, so give a sanitizer lane a timeout above 3600 seconds; the default is too small |
| the coverage target | minutes; it runs the suite again with instrumentation |

## Notes

- The scripted plan, rank, budget and refusal cases run identically with and
  without PMU access, so a locked-down CI host sees the same expected values.
- A hunt test never sleeps, never reads a clock and never assumes a CPU count. A
  test that needs a CPU set calls `sched_getaffinity`, the pattern
  `harness_threads_test.cpp` carries (FR-092).
- `--hunt` reads its value in the attached form: `--hunt=all`. `getopt_long`
  gives an `optional_argument` option no value when a space separates them, so
  `--hunt all` selects nothing and leaves `all` as a positional argument.
- The hunt reads the limit from `Object::programmableCounters()` and the events
  from `Object::counters()`, and composes an address as `Object::path()` plus `/`
  plus the entry name. No hunt code reads sysfs or CPUID (D-23).
- The AC-1 `L - 2` shape is reachable on hardware only through a device that
  publishes a limit: `limitForDevice` at
  `source/counters/linux_pmu/counter_limits.cpp:288` answers for
  `cpu`, `cpu_core` and `cpu_atom`, so an `armv8_` device publishes none and
  plans `MULTIPLEX`. The scripted limit is the covering check for the shape.
- The two other lanes share `cli.cpp`, `runner.cpp`, `benchmark.hpp`,
  `internal.hpp`, `README.md`, `example_drift.py`, `docs/pages/harness.md` and
  `docs/schema/report-v1.json`. The rebase rules are in
  [plan.md](plan.md), and every gate runs after each rebase (FR-094).
