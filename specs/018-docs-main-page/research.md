# Research: Main Page Documentation Rewrite

Phase 0 output for `specs/018-docs-main-page/spec.md`. The spec carries
no open clarification: its clarify session of 2026-10-13 settled the
amendment form (sub-rule IV.1, MINOR), the quick-reference scope (the
harness surface alone), the drift-gate scope (every marked block on any
prose page, marking obligation on the main page alone), the compiler
claim (C++23 plus the CMake minimum, no version named), and the SC-001
devices (a fresh model per change plus three new readers once, neither a
CI gate). The open questions that remain belong to the plan, and each one
below is settled here against the audit-point tree.

## D-01: The block-to-region identity rides the fence info string

**Decision**: A marked block opens with an info string whose first token
is the language, followed by `sg-example=<file>#<region>` or, for the one
sample block, `sg-sample-output=<target>`. The example file marks the
region with line comments `// sg-example:begin <region>` and
`// sg-example:end <region>`. One early task renders both forms through
the `docs` target and the GitHub preview before the rewrite spreads.

**Rationale**: FR-031 requires the file path and region name to render
invisibly. GitHub's markdown pipeline uses the first info token for
highlighting and treats the rest as opaque attributes, so the token costs
no visible text and stays one line per block. The pair stays inside the
block and its region, so a move or a copy carries its identity. Line
comments add no identifier, so Principle V.1 binds nothing at the marker,
and the compiler ignores them. The `sg-` namespace matches the project's
macro prefix family and keeps the token grep-stable for the gate.

**Alternatives considered**: an HTML comment line above each fence,
rejected as the first choice because Doxygen's markdown path treats raw
HTML inconsistently and the marker then sits outside the block it names;
kept as the recorded fallback if the render spike shows the token text on
either surface. A central manifest file mapping blocks to regions,
rejected because it is a third place that can drift from the two it
binds.

## D-02: The drift checker is a python tool with a ctest wrapper

**Decision**: `tools/docs/example_drift.py` takes the repo root, scans the
prose pages in prose-gate scope, compares every marked block, and runs
the name and option scans. `test/CMakeLists.txt` registers it as
`example_drift` unconditionally and its fixture harness
`test/example-drift-fixture/run_example_drift_fixtures.py` as
`example_drift_fixtures`. Exit codes follow the prose gate: 0 clean, 1
findings, 2 usage.

**Rationale**: The tool compares text across two trees of files, exactly
the job `tools/prose/prose_gate.py` does in python, with the fixture
harness pattern of `test/prose-gate-fixture/` as its covering test set.
FR-035 requires the check to run with `BUILD_EXAMPLES=OFF`; a python
script needs no configure, so the `lint` job can run it directly as the
build-independent proof while `test` and `test-rocky` run it through
ctest. FR-033 names the script-gate pattern, and `test/time_source_gate.sh`
supplies the liveness rule the fixtures carry: a clean run must not come
from a scan that inspects nothing.

**Alternatives considered**: a bash/awk gate script in the style of
`test/loop_shape.sh`, rejected because multi-file block extraction and
line-pair reporting are the exact spots where awk grows unreviewable, and
the shell gates here all analyze one generated file; none spans two
trees. A
C++ test, rejected because it would couple a source-text check to a
configured build, contradicting FR-035's reason.

## D-03: Comparison is byte-exact, and failures name four things

**Decision**: The region lines (strictly between the markers) must equal
the fence body lines exactly: same order, same characters, same trailing
whitespace state, same count. Each failure prints the prose file, the
block, the example file, the region, and each differing line pair with
line numbers on both sides. A one-character difference fails (US5).

**Rationale**: The page promises an LLM reader that a copied snippet is
the code that compiles (FR-011, "Style for the two readers"). Only
byte-equality keeps the promise true, and the promise is the gate's
reason to exist. The four-part naming is FR-033's requirement, and the
line pairs turn a failure into a paste-ready fix.

**Alternatives considered**: whitespace-normalized comparison, rejected
because it silently blesses exactly the kind of edit (an indentation
change) that the spec's move tolerance rule separates from real drift; a
diff -u blob, rejected because FR-033 names differing lines with the
block and region identified.

## D-04: The sample output block is shape-checked with the suite's own fold

**Decision**: The single `sg-sample-output` block is exempt from the
equality rule and checked by shape. `tools/docs/report_shape.py` runs the
quick start binary (path passed by ctest through `$<TARGET_FILE:...>`,
test registered only when the target exists), captures stdout, folds both
captures with the row-shape mask (digit run to one `#`, whitespace run to
one space, other characters copied), and compares the masked line
sequences. Golden pairs in the fixture harness pin the python fold to the
C++ fold in `test/source/harness_cli_test.cpp`.

**Rationale**: FR-034 names the row-mask mechanism, and its comment
records the measured reason widths stay out of the comparison: a
sanitizer run prints one field wider, and only the columns and their
order are the property the check owns. A golden-pair fixture is the
shared-spelling mechanism the repository already uses for
machine-consumed values, so the python copy cannot silently diverge from
the C++ owner.

**Alternatives considered**: extracting the C++ fold into a test header
and compiling a small checker, rejected because it moves code owned by an
existing test to save a ten-line fold, and Principle X.3 warns against
that kind of drive-by refactoring; comparing the raw text on a pinned
fake-provider run, rejected because the harness example runs on real
providers and the numbers are host values by nature.

## D-05: The quick start owns a complete example of its own

**Decision**: A new `example/first_benchmark.cpp`: includes, one small
benchmark body, one registration, `main` returning
`sg::speedgunMain(argc, argv)`. Added through `add_example` with a second
link line for `speedgun-ng::harness`, mirroring `benchmark_example` in
`example/CMakeLists.txt`. The quick start shows the whole file as one
region, plus its build and run commands and its sample output. FR-031
binds every main-page block to a region, so the command blocks read
their regions from `example/quickstart.sh` and the `find_package` and
link-line block reads its region from
`example/quickstart/CMakeLists.txt`, the same CMake text the clean-room
consumer of the test plan builds.

**Rationale**: FR-005 requires a complete program from a file under
`example/` that CI builds and runs, and the run command, sample output,
and shape check all name that one binary. A dedicated minimal file keeps
the first screen teachable and the report block short. `add_example`
already yields the `run_first_benchmark` target, so CI compiles and can
run it with zero new wiring beyond the harness link.

**Alternatives considered**: teaching `benchmark_example`, rejected
because its thirteen families bury a first benchmark and lengthen the
sample block past the 25-line section budget; teaching `empty_example`,
rejected because it is a counters example with no harness registration
and no `speedgunMain`.

## D-06: Compiled example sources carry the C++ regions, and two quick-start files carry the command regions

**Decision**: `example/benchmark_example.cpp` gains region markers at its
existing sites (run loop, argument family, template, fixture, timing
modes, unit, chained registration, `addMetric`, `DISABLED_` prefix) with
comment-only edits. A new `example/docs_snippets.cpp` carries the
capabilities that file lacks, one region per page snippet: capture,
the template fixture macro family, the ranges family (`range`, `ranges`,
`rangeMultiplier`, `denseRange`, `argsProduct`, `argName`), the `setup`
and `teardown` callbacks, run control (`minTime`, `iterations`,
`repetitions`, `warmupTime`), `clobberMemory` in the run loop, skips
(`skipWithError`, `skipWithMessage`), and `setDefaultTimeUnit`. It is one
`add_example` target, harness-linked, whose benchmarks are registered with
the `DISABLED_` prefix or are run through `--dry-run` in the `run_` target
so the aggregate run stays short.

**Rationale**: Every page snippet then compiles as pasted inside a file
CI builds (FR-011, SC-003), and every region is one grep away from the
capability it teaches. Comment-only edits to `benchmark_example.cpp` honor
FR-036's allowance for region markers. Keeping teaching snippets in one
new file bounds the blast radius and keeps `benchmark_example.cpp` the
capability showcase the harness features built.

**Alternatives considered**: one new file per teaching area (six files),
rejected as file sprawl with no rule each file would enforce; putting the
snippets inside `benchmark_example.cpp`, rejected because its output is
the shape-checked showcase and extra families would churn that block.
DISABLED_ registration inside `docs_snippets.cpp` was checked against the
spec's own teaching of the prefix: it is the shipped mechanism, so the
file is honest about staying out of aggregate runs, and the `--dry-run`
run target still proves the program runs. The two quick-start region
files (`example/quickstart.sh`, `example/quickstart/CMakeLists.txt`) are
not example targets, so FR-032's build obligation does not reach them;
their truth is byte equality under the drift gate plus the clean-room
build of the CMake text.

## D-07: The build page lands at `docs/pages/building.md` with an audience-ordered index

**Decision**: New `docs/pages/building.md` receives all nine dropped
sections with commands and tool names intact (section map in plan.md).
`docs/conf.py.in` `LINKS_NAVBAR1` gains the page groups: user pages first
(about, building, harness), internals second (barrier rule, counters
overhead, dbc migration, dbc overhead, rename map). `docs/pages/harness.md`
retitles its H1 to drop the spec identifiers, keeping the filename.

**Rationale**: FR-022 and decision Q-3 name the destination directory;
`docs/pages` is both prose-gate scope and Doxygen `INPUT`, so moved
material stays checked and published. `LINKS_NAVBAR1` is where the
documentation index is actually composed (`docs/conf.py.in`), so FR-025's
ordering rule lands there. PC-6 records that `harness.md` titles carry
spec identifiers; the audience-ordered index reads by reader, so the
title follows the same rule while the file path stays stable for inbound
links.

**Alternatives considered**: splitting the build page into a build page
and a gates page, rejected because FR-022 names one receiving page and
the maintainer walks one route; moving maintainer material into
`AGENTS.md`, rejected because FR-022 requires the docs index to list the
receiving page, and `AGENTS.md` is agent-facing and unpublished.

## D-08: The amendment and its gate entry land as one change

**Decision**: One commit series adds: sub-rule IV.1 under Principle IV
with FR-027's text; the example compile gate as one Principle VIII
hard-gate entry whose text enumerates its components (`example_drift`,
`example_drift_fixtures`, `snippet_compile`, `report_shape`,
`report_shape_fixtures`, the lint-job direct step, and the example build
obligation), because the spec's component table groups FR-031 to FR-035
as that one gate; lineage row 2.19.0, footer version, and footer date; a
Sync Impact Report comment in the form the file uses, closed by the
re-anchoring note; and one main-page checklist line in each of
`.specify/templates/spec-template.md`
and `.specify/templates/tasks-template.md`, both carrying the pinned
phrase "Main-page impact check (IV.1)": the spec-template gains the line
"Main-page impact check (IV.1): does this feature change what the root
`README.md` teaches? If yes, update the page and its `example/` regions
in the same change." and the tasks-template gains the same sentence as a
checkbox item.

**Rationale**: Principle VIII states that changing the gate set itself
requires a constitution amendment, and FR-030 applies the rule: a gate
with no entry in the law is a preference. Governance classifies a new
sub-rule plus a new gate item as MINOR, and FR-028 pins 2.19.0. The
amendment and the gate must land in the same change so no commit carries
a gate outside the law or a law naming a gate that does not exist; 2.13.0's
report sets that atomicity precedent.

**Alternatives considered**: landing the gate first and amending after,
rejected on the atomicity rule above; a new top-level principle, rejected
by the spec's own clarification Q-1 since Principle IV already owns the
subject; a CI meta-check rejecting any future gate addition without a
matching amendment, rejected because FR-030 names the amendment rule, not
a second-order gate, and IV.1's checklist line plus the reviewer already
serve later features (Principle X forbids speculative machinery).

## D-09: CI wiring rides existing steps

**Decision**: No new CI job. The `test` and `test-rocky` jobs run the
drift gate, its fixtures, and the shape check through their existing
`ctest` steps. The `lint` job gains one step running
`python3 tools/docs/example_drift.py .` directly, the build-independent
proof FR-035 asks for. The `docs` job (push to master) remains the CI
home of FR-026's build. Example-target compilation already happens in
`test` and `test-rocky` through the `dev-mode` inheritance of
`ci-ubuntu` and `ci-rocky`, which is FR-032's existing guarantee.

**Rationale**: Every obligation maps to a job that already configures the
tree it needs. The `lint` job configures nothing, which is exactly the
`BUILD_EXAMPLES=OFF` tree FR-035 describes. FR-034 asks for one job that
runs the binary and compares the shape; the `test` job's ctest step does
both once the shape test is registered.

**Alternatives considered**: a dedicated `docs-gates` job, rejected as
matrix cost for three tests that fit existing jobs; putting the direct
drift step in `prose-lint`, rejected because that job checks out without
submodules and its scope is the commit range.

## D-10: The first screen is prose plus a six-line fact list

**Decision**: The opening part holds two or three sentences and a list of
exactly the six facts FR-002 names: C++23, Linux, Google Benchmark
lineage, counters library as the source of every time and counter value,
hardware counter metrics on a benchmark, contract checks on the API. The
part stays inside about 20 lines so the screen answers US2's three
questions unaided.

**Rationale**: SC-001 is judged from the screen alone by a fresh model
and by new readers; a list of the six facts is the smallest shape that
answers what, where, and whence deterministically. The clarify session
keeps both devices out of CI, so the plan carries them as pull-request
review steps in quickstart.md scenario 2.

**Alternatives considered**: a hero paragraph in prose alone, rejected
because a model given only the screen must extract three facts, and
prose-length extraction is the least reliable device; a feature matrix
table, rejected as duplicate of the quick reference below.

## D-11: The option table and quick reference are generated-once, scanned-often

**Decision**: The two quick-reference tables are written once from the
audit-point lists (PC-4 for names, the long-option table of
`source/harness/cli.cpp` for options) and kept honest by the drift tool's
`--names` and `--options` scans inside the default ctest run.

**Rationale**: SC-009 and FR-014 ask for a checked match to the source,
and one tool run inside the existing test step is the cheapest owner.
Later features carry their rows through IV.1, so the tables cannot lag
the source tables (Edge Cases).

**Alternatives considered**: generating the tables from headers at build
time, rejected: the page is prose for humans and models, generation would
decide row wording by template, and the row set is deliberately curated to
the harness surface (clarify session).

## D-12: Teaching the disabled prefix, skips, and the unavailable-counter host is text plus one region each

**Decision**: The page teaches the `DISABLED_` prefix from the region
that already carries it in `benchmark_example.cpp`; the skips section
teaches both calls from a region in `docs_snippets.cpp` and states what
the report prints; the metrics section states the unavailable-counter host
behavior in prose and links the harness detail page for the disclosure
fields.

**Rationale**: FR-009 lists all three capabilities; Edge Cases pin the
behaviors. Each fact lands where the reader meets the name, and the
detail stays on the detail page per FR-017.

**Alternatives considered**: a skip-with-error demonstration that exits
in a run target, rejected: the `run_` target would fail on a skipped
family's exit status, buying a red run for one printed word.

## D-13: The main page keeps no maintainer material, and nothing is deleted

**Decision**: The rewrite deletes no fact. Every dropped section moves to
`docs/pages/building.md` through the plan's section map, and the map is
the SC-006 audit worksheet. The name-origin paragraph shrinks to one
sentence in the tail (spec decision Q-5).

**Rationale**: US4's priority reason is the maintainer regression; the
map plus the walkthrough scenario is the receipt. Q-5 and FR-038 keep the
tail at one sentence.

**Alternatives considered**: moving maintainer material to a new file
under a new `maintainer/` tree, rejected on FR-022's named destination and
the docs-index rule.

## D-14: Line and section budgets land as binary caps checked at acceptance

**Decision**: SC-011's stated numbers become the caps themselves: 400
total lines and 25 lines per feature section (heading line included).
The page-rewrite acceptance and quickstart scenario 10 both check the
caps as executable assertions; no CI gate owns them.

**Rationale**: The spec says about, and Principle X.4 prefers a binary
check. Fixing the cap at the spec's own number keeps the check inside
the spec bound, makes the review reading a pass/fail verdict, and puts
the tolerance where it belongs: prose gets tightened at acceptance, and
the cap stays inside the bound. A CI
line-count gate still buys nothing beyond the acceptance check.

**Alternatives considered**: caps at 420/30, rejected on review because
a cap above the stated bound re-reads the spec through the plan;
uncapped "about", rejected because a success criterion nobody can
falsify is not a criterion.
