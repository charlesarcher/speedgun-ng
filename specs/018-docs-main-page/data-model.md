# Data Model: Main Page Documentation Rewrite

Phase 1 output for `specs/018-docs-main-page/spec.md`. The entities are
the Key Entities of the spec, restated with the field rules, validation
rules, and transitions the gates enforce. Anchors name a file and a symbol
or section; no anchor carries a line number.

## E-01: Main page

The root `README.md`. One file, two roles: the repository landing page on
GitHub and the documentation home page through `USE_MDFILE_AS_MAINPAGE`
(`docs/Doxyfile.in`).

| Part | Rule |
| --- | --- |
| Order | "What speedgun does", then the quick start, then "How you do it", then Quick reference, then Planned work, then the tail (target page structure items 1 to 5) |
| First screen | One screen states: C++23, Linux, Google Benchmark lineage, the counters library as the source of every time and counter value, hardware counter metrics on a benchmark, contract checks on the API (FR-001, FR-002) |
| Quick start | Platform, C++23 requirement, CMake minimum, configure, build, install, `find_package`, the two-target link line, the complete first benchmark program, its build command, its run command, its sample output (FR-003 to FR-007) |
| Forbidden content | Commit hashes, kernel refs, pinned version strings, compiler versions, spec identifiers, requirement/success-criterion/task identifiers, owner directives, constitution principle numbers, CI job names, re-pinning steps, source paths except example program paths (FR-038) |
| Budget | Binary caps (D-14): 400 lines including the quick reference; no feature section over 25 lines counting its H3 heading (SC-011) |
| Audience prose | Principle XI and XI.7; zero prose gate findings (FR-040, SC-010) |

Validation: a forbidden-token scan over the file finds zero matches
(SC-005). The file stays inside `USE_MDFILE_AS_MAINPAGE` scope, so no
setting changes (FR-026).

## E-02: Feature section

One teaching unit inside "How you do it", one per shipped capability.

| Field | Rule |
| --- | --- |
| Heading level | Same level in every section (FR-010) |
| Part 1 | One purpose sentence |
| Part 2 | One snippet in one fenced block; complete under the quick-start include list; no ellipsis in place of code (FR-011) |
| Part 3 | One or two sentences on what the output shows (FR-010) |
| Detail link | MAY link the detail page; the detail stays there (FR-017) |

The closed section list (FR-009 at the audit point): run loop with
`doNotOptimize` and `clobberMemory`; registration through the registration
macros, the handle chain, capture, templates, the `DISABLED_` prefix, and
suite and case naming; argument families and ranges (`arg`, `args`,
`range`, `ranges`, `rangeMultiplier`, `denseRange`, `argsProduct`,
`argName`, `argNames`); fixtures with `setUp`/`tearDown` and the `setup`
and `teardown` callbacks; run control (`minTime`, `iterations`,
`repetitions`, `warmupTime`) with the repetition aggregates; timing modes
(real time, process CPU time, manual time); pause and resume (`pauseTiming`,
`resumeTiming`, `ScopedPauseTiming`); the time unit (`unit`,
`setDefaultTimeUnit`); hardware counter metrics through `addMetric`; skips
(`skipWithError`, `skipWithMessage`); the command line through
`speedgunMain`; and reading the report.

Validation: every name taught exists in a public header at the audit point
(FR-012); scaffolding macros stay off the page (FR-016).

## E-03: Example region

A named span inside a file under `example/`, opened and closed by line
markers (grammar in `contracts/example-drift.md`).

| Rule | Statement |
| --- | --- |
| Naming | Region names are unique inside one file; begin and end markers carry the same name |
| Balance | Every begin marker has its end marker; an unbalanced pair is a gate error |
| Ground truth | The region is the single source of truth for the page block that names it |
| Granularity | A C++ region wraps a complete top-level declaration (function, class, `main`) or the one complete program (the quick-start program region), so its block compiles as pasted under the canonical include list (FR-011; `contracts/example-drift.md` R-10); non-C++ regions wrap lines of text truth (report lines, command lines, CMake lines) and carry no declaration obligation |
| Compilability | C++ regions live in example sources the example targets compile in CI (FR-032) and each compiles standalone from the page block (`contracts/example-drift.md` R-10); the quick-start command and CMake regions (`example/quickstart.sh`, `example/quickstart/CMakeLists.txt`) and the sample region (`example/first_benchmark.expected`) carry text truth only, because FR-032's build obligation binds example targets. C++ example sources use public headers only, and C++ example code takes every time value from the counters library (FR-039) |

## E-04: Page block

A fenced code block on the main page that names its example file and its
region in the fence info string, which renders invisibly on GitHub and in
the Doxygen output (FR-031).

| Rule | Statement |
| --- | --- |
| Equality | The block body equals its region text exactly (drift gate, FR-033) |
| Obligation | Every block on the main page carries file and region; a block without a region fails the gate |
| Detail pages | A marked block on another prose page is held to the same rule; an unmarked one is left alone |
| Move tolerance | A region that moves inside its file and keeps its text passes, because the gate compares text (Edge Cases) |

## E-05: Sample output block

The printed report of the quick start program, labeled as one host's run
(FR-007).

| Rule | Statement |
| --- | --- |
| Shape | A CI check runs the quick start example binary and compares its report to the block through the row-shape mask the harness tests use: a digit run folds to one `#`, a whitespace run folds to one space (`test/source/harness_cli_test.cpp`) |
| Pass | A number differs |
| Fail | A column is missing, renamed, or reordered |
| Stability | The report shape stays fixed during this feature because no library file changes (FR-036, spec Assumptions) |
| Budget | The block's lines count toward the 400-line page total (C-F rule 1); the 25-line cap binds feature sections (C-F rule 2), where this block does not sit |

## E-06: Quick-reference row

One API name, one line of meaning, one link to the section that teaches it
(FR-018, FR-020). The row set is the harness surface alone: the
registration macros, `State`, `BenchmarkHandle`, `Fixture`,
`ScopedPauseTiming`, `setDefaultTimeUnit`, `speedgunMain`,
`doNotOptimize`, `clobberMemory`. Counters and DBC names stay out
(clarify session, 2026-10-13). A name scan proves every row names a header
name that exists at the audit point (SC-009).

## E-07: Option row

One long option of `speedgunMain` and what it does (FR-019). The row set
equals the long-option table of `source/harness/cli.cpp` at the audit
point: `--filter`, `--list`, `--repetitions`, `--min-time`, `--iterations`,
`--warmup-time`, `--dry-run`, `--counter`, `--catalog`. The page names the
accepted long form and states that a usage error stops the program with
exit status 2 (FR-014). A later feature adds its row through its own spec
under IV.1, so the table cannot lag the source table.

## E-08: Build page

The page under `docs/pages/` (`docs/pages/building.md` per spec decision
Q-3) that receives every dropped fact (FR-022).

| Rule | Statement |
| --- | --- |
| Completeness | Every command, tool name, tripwire description, and version-pin mechanic of the nine dropped sections appears (SC-006; section map in plan.md) |
| Route | The main page links it once, near the end (FR-024) |
| Index | The documentation index lists it among user pages (FR-025) |
| Prose | It is inside prose gate scope as a `docs/` page (FR-040) |

## E-09: Page-index group

The m.css navigation group a documentation page sits under
(`docs/conf.py.in`, `LINKS_NAVBAR1`). Two groups exist after this feature:
user pages first (about, building, harness), internals second (the
measurement records, the migration map, the rename map, the barrier rule)
(FR-025, SC-007).

## E-10: Rule IV.1 and the amendment record

A numbered sub-rule "Main-page documentation per feature" under Principle
IV (FR-027). The amendment record carries: the IV.1 text, the Principle
VIII hard-gate entry for the example compile gate (FR-030), version 2.19.0
in the lineage table and the footer with the footer date (FR-028), a Sync
Impact Report in the form the file uses, and one checklist line in each of
the spec and tasks templates (FR-029).

## E-11: Drift gate

The ctest-registered check that scans the prose pages in scope
(`README*`, `AGENTS.md` at the root, `docs/` per `tools/prose/prose_gate.py`
scope) for marked blocks (FR-033).

| Input | Verdict |
| --- | --- |
| Marked block equals its region | pass |
| Marked block differs from its region | fail; print the file, the region, and the differing lines |
| Main-page block with no region | fail; name the block |
| Marked block on a detail page with no region | fail under the same rule as any marked block; an unmarked detail-page block is ignored |
| Any tree configuration, including `BUILD_EXAMPLES=OFF` | the gate runs; it reads source text only (FR-035) |

## E-12: Section map

The plan artifact (section "Section map" of plan.md) pairing each dropped
section of the old main page with its destination, as FR-023 requires. It
is the audit worksheet for SC-006 and carries no runtime role.

## Transitions

| Object | From | To | Trigger | Guard |
| --- | --- | --- | --- | --- |
| Page block | absent | paired | A feature adds the block with its region in the same change | IV.1 checklist line; drift gate |
| Page block | paired | drifted | Either side is edited alone | drift gate fails; CI blocks the merge |
| Page block | drifted | paired | The pair is realigned | drift gate passes |
| Option row | absent | present | A later spec adds the option and its quick-reference row | IV.1 |
| Main page | maintainer page | teaching page | This feature's rewrite | every E-01 rule holds; SC-001..SC-012 receipts |
