# Analysis round 1: specs/038-report-view-generators

Analyzed 2026-10-29 against the tree at branch `038-report-view-generators`.
Method: the constitution first, then the four artifacts, then the
grounding files. Every quoted line and number below was checked against
the checked-out tree.

## Findings

### F1. BLOCKER: the views' home target is incoherent across the artifacts, and the tasks' link lines cannot resolve `loadReport`

- Artifact: plan.md (Physical view, Public API surface), spec.md (D-2,
  FR-002), tasks.md (T001, T016).

One side, plan.md Physical view:

> The implementation stands under `source/views/`, linked into the
> `speedgun-ng` library target; the driver stands at
> `tools/report_view.cpp` as its own executable target linking the
> library.

The other side, spec.md FR-002 and the audit point:

> The project version SHALL stand at `0.25.0` with the harness
> `SOVERSION` at `16` and the archive `SOVERSION` at `4`

> The view functions join the exported library surface behind a public
> header, so the harness `SOVERSION` rises from `15` to `16`; the
> archive `SOVERSION` stays `4`

The tree: `loadReport` is implemented in
`source/harness/json_report.cpp` and compiles into the
`speedgun-ng_harness` target (the `GLOB_RECURSE` of
`source/harness/*.cpp` at `CMakeLists.txt:235`). The base target
`speedgun-ng_speedgun-ng` carries `SOVERSION 4`
(`CMakeLists.txt:91`); the harness target carries `SOVERSION 15`
(`CMakeLists.txt:320`). FR-002's numbers only add up when the views
join the harness archive. The plan says the views join the base
library, whose exported surface then grows with its hand-kept number
standing at 4, while the harness number moves for a harness surface
that never changed.

The tasks make the mismatch concrete. T001:

> Register the test executable `views_test` in `test/CMakeLists.txt`
> beside the standing harness tests (`add_executable`, link
> `speedgun-ng::speedgun-ng`, `add_test`)

T003's verify runs `loadReport` inside `views_test`, and T001's own
test list calls on a document with zero benchmarks. `loadReport`
resolves in the harness archive, so a link line of
`speedgun-ng::speedgun-ng` fails to link. T016 hands the driver the
same problem ("linking the library").

Also, `source/views/*.cpp` matches no standing source list: the base
target names its sources one by one, the harness target globs
`source/harness/*.cpp` only.

Remediation: rule the hosting decision once. The harness archive fits
FR-002 and keeps the views beside the reader they render for. Fix the
plan's physical view, D-2's wording, T001 and T016 to link
`speedgun-ng::harness`, and name the source-list seam (the harness
glob or an explicit list) in M-A. Move the `SOVERSION` record with the
same decision.

### F2. MAJOR: the drift gate's fixed tables must move with the new section, and no task names `tools/docs/example_drift.py`

- Artifact: plan.md (M-H), tasks.md (T020).

One side, plan.md M-H:

> The main page gains its section: one purpose sentence, one marked
> snippet from the example region, one result sentence, the
> quick-reference rows naming the public header (FR-022, IV.1).

The other side, `tools/docs/example_drift.py`: the gate holds a fixed
`SECTIONS` tuple and a fixed `NAME_SCAN_HEADERS` tuple
(`benchmark.hpp`, `barrier.hpp`, `execution_context.hpp`,
`counters_metrics.hpp`, `counters_system.hpp`, `hunt.hpp`), a
`PAGE_LINE_CAP` of 912, and a per-section cap of 25 lines. The checks
report:

> section '<title>' is not one of the N capability sections

> quick-reference name '<name>' is not in benchmark.hpp, ..., or
> hunt.hpp

A row naming `sg::renderHistogram` splits at `::` and every part must
appear in the scanned headers, so the row fails until `views.hpp`
joins `NAME_SCAN_HEADERS`. A new H3 section fails until it joins
`SECTIONS` and `SECTION_OWNERS`, and the page cap needs its measured
move. T020's verify (`ctest -R 'example_drift|snippet_compile'`
green) cannot pass without these edits, and no task names the file.

Remediation: extend M-H and T020 with the `example_drift.py` moves:
the section in `SECTIONS` and `SECTION_OWNERS`, `views.hpp` in
`NAME_SCAN_HEADERS`, the measured `PAGE_LINE_CAP` move recorded beside
it. One more seam worth naming there: the canonical prelude of
`test/snippet-compile/run.sh` is fixed
(`<cstdint>`, `<vector>`, `barrier.hpp`, `benchmark.hpp`), so the
marked region carries its own `views.hpp` include or the prelude moves
with the same task.

### F3. MAJOR: the docs example as described cannot pass the `example_run` gate, and the public surface carries no document writer

- Artifact: plan.md (M-H), tasks.md (T019).

One side, plan.md M-H:

> The example builds a report document through the library, loads it,
> renders the text view, and prints lines the `example_run` gate
> reads.

The other side, `test/example_run.sh`: the gate calls every
`docs_` binary with `--list` and fails the file when the list is
empty (`FAIL: docs_$name lists no registered benchmark`), runs it with
`--min-time=0.01`, and requires a printed row per registered
instance. A program that only renders a text view fails the gate
twice. The route "builds a report document through the library" also
has no path: the writer (`writeReport`) lives in
`source/harness/json_report.cpp` and is exported to no consumer; the
public surface carries the reader alone.

Remediation: state the example's shape in M-H and T019: a harness
example that registers a benchmark whose rows print, plus the render
path, with the document route named (the harness `--json` write then
`loadReport`, or a document the region writes to scratch).

### F4. MAJOR: the shared escaper lands one wave after its first users

- Artifact: plan.md (Modules and dependency order, Risks), tasks.md
  (Phase 3, T012 to T015).

One side, plan.md, wave 3 modules M-C and M-D with the escaper
assigned to M-F (wave 4):

> **M-F the HTML page** (`source/views/page.cpp`,
> `source/views/escape.hpp`)

> Depends on M-C and M-D.

The other side, the same plan and the spec's own edge cases:

> The SVG label text meets document-derived strings (benchmark names,
> labels): the escaper is shared and tested once (M-F), and the
> histogram and the plot call it

data-model.md: "document-derived text escaped (A-6)" for both SVG
figures; spec edge case: "the page and the SVG escape it". The SVG
writers of wave 3 call an artifact the plan schedules in wave 4, so
the module graph carries a cycle: M-F depends on M-C and M-D, and
M-C and M-D depend on M-F's `escape.hpp`.

Remediation: move the escaper to its own module beside M-B (wave 2 or
the head of wave 3) with T012 and T013; M-F keeps `renderPage` alone.

### F5. MINOR: the version print test has no writing task, and T002's verify cannot fail

- Artifact: plan.md (Wave 1, FR coverage map), tasks.md (T002).

One side, plan.md Wave 1 and the FR map:

> Wave 1 (M-A): the header compiles, the functions link, the version
> stands at `0.25.0` with `SOVERSION 16` / archive `4`.

> FR-002 | 1 | the version print test reads `0.25.0`; the commit
> records the check

The other side, tasks.md T002 verify:

> `cmake --preset=dev && cmake --build build` and
> `ctest --test-dir build -R version` green

No standing test name matches `version`, and `ctest -R` exits 0 when
nothing matches (measured on this tree), so the verify passes with no
check run. That breaks the binary-check rule of X.4.

Remediation: T002 writes the version and `SOVERSION` assertions into
`views_test` (or another named test) and the verify names that test.

### F6. MINOR: the physical view names a CLI test file no task creates

- Artifact: plan.md (Physical view), tasks.md (T017).

One side, plan.md Physical view:

> The tests stand at `test/source/views_test.cpp` and
> `test/source/report_view_cli_test.cpp`, registered with CTest

The other side, tasks.md T017: the CLI tests stand at
`test/report_view_cli.sh`, "the standing gate-script pattern", and no
task creates `test/source/report_view_cli_test.cpp`.

Remediation: replace the stale path with `test/report_view_cli.sh`.

### F7. MINOR: the documentation tree plans two quickstart files; the cited precedent carries one

- Artifact: plan.md (Project Structure, Documentation).

One side, plan.md Documentation tree:

> ├── quickstart.md        # the evidence index linking each record

> │   └── quickstart.md    # the links (the specs/036 shape)

The other side: `specs/036-osnoise-timerlat-tracers/` carries one
feature-root `quickstart.md` and the records under `results/`, with no
`results/quickstart.md`. T022 writes only the root file.

Remediation: drop the `results/quickstart.md` row.

### F8. MINOR: the evidence script D-5 requires has no creating task

- Artifact: spec.md (D-5, FR-021), tasks.md (T021).

One side, spec.md D-5 and FR-021:

> A script stands behind the runs; no CI gate carries them.

The other side, tasks.md T021: the task re-runs the commands and
records the documents; no task writes the script FR-021 states, and
T022 writes the quickstart only.

Remediation: T021 names the script's path and lands it in the tree, or
D-5 and FR-021 drop the script clause.

### F9. MINOR: the audit point cites `kOsNoiseOptionValue` at a line that holds a doc comment

- Artifact: spec.md (Audit point).

One side, spec.md:

> (`source/harness/detail/options.hpp:81` to `:107` name the standing
> constants; `kOsNoiseOptionValue` stands at `297`)

The other side: the constant stands at `options.hpp:93` and carries
the value 297; line 297 holds the doc comment of `opensWithBlank`.

Remediation: write "the value `297`" and cite `options.hpp:93` for the
constant.

### F10. MINOR: the coverage gate states two of the three legs VI and VIII carry

- Artifact: spec.md (FR-019, SC-005), plan.md (Wave 8), tasks.md
  (T023).

One side, spec.md FR-019:

> Coverage SHALL stand at 100% line and 100% branch with no
> `LCOV_EXCL` marker and no new `--ignore-errors` entry

The other side, constitution VI and VIII:

> Hard CI gates: 100% line, 100% branch, 100% DBC (II).

> Coverage gates of VI pass: 100% LOC, 100% branch, 100% DBC.

The constitution governs anyway, so the gate holds; the spec's own
statement of it drops the DBC leg, and T023's verify follows the spec.

Remediation: FR-019, SC-005 and T023 name the DBC leg, or state that
the standing coverage preset reads it and cite the read.

## Checks that passed

- Coverage: every FR-001 to FR-022, every SC-001 to SC-007 and every
  user story carries covering tasks; no orphan requirement and no
  orphan task stands outside the findings above.
- Consistency: the module list (M-A to M-I), the wave numbers, the
  function names (`viewSeries`, `renderHistogram`, `renderPlot`,
  `renderPage`, `renderText`, `ViewSeries`), the driver grammar, the
  refusal and disclosure table, and the decision references (D-1 to
  D-10, the clarify rulings) agree across the four artifacts.
- Constitution: TDD ordering holds per wave (red task before the
  implementation task it gates); the VIII gate list is represented in
  the plan's constitution check and wave 8; the planned identifiers
  pass V.1 (`ViewSeries` PascalCase, `viewSeries` camelBack,
  `binCount` camelBack, `source/views/` lower_case file names); the
  machine-checkable prose subset reads clean on all four artifacts.
- Grounding: every cited line number checks out against the tree
  (`benchmark.hpp` 90, 383, 627, 792, 2878; `CMakeLists.txt` 26, 91,
  320; `json_report.cpp:39` for schema `1.8`; the option values run to
  297, so the free block 298 to 319 stands); the presets
  (`ci-sanitize`, `ci-tsan`, `ci-ubuntu`, `ci-rocky`, the coverage
  preset) exist; `harness_cli_table_test` exists and T024's `-R`
  filter matches standing tests.
- Soundness: the dependency order is implementable once F1 and F4 are
  fixed; the verify commands run; the test plan covers the acceptance
  scenarios of every story; the contracts (A-1 to A-6, B-1 to B-6)
  match the FRs they name, and the exit classification of clarify
  1/Q2 stays consistent between FR-015, FR-016 and the contract's
  table. The `not available` composition rules (FR-012, FR-013,
  clarify 2/Q1) agree.

## Verdict

One blocker and three major findings stand between the artifacts and a
clean implement run. All four have one-place remediations inside the
artifacts; no finding touches the settled clarify rulings.
