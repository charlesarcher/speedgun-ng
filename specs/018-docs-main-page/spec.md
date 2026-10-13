# Feature Specification: Main Page Documentation Rewrite

**Feature Branch**: `018-docs-main-page`

**Created**: 2026-10-13

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

> Docs main page, spec D1 of the speedgun harness roadmap. D1 runs
> before H4. The main page of speedgun-ng is the root `README.md`.
> Doxygen uses the same file as its main page. Today the page says
> little about what speedgun-ng does, and it shows no benchmark. Most
> of its lines tell a maintainer how to re-pin vendored dependencies
> and run gates. This spec gives the main page one purpose: teach what
> speedgun does and how you do it. The page teaches simply, briefly,
> and concisely, with working examples. It serves two readers: a
> human developer and an LLM that writes speedgun benchmarks. Third-party integration
> details move off the main page. The spec also adds a constitution
> principle: every later feature spec updates its main-page section
> with a working example. [...]

## Clarifications

### Session 2026-10-13

- Q: Should the docs-with-every-feature rule be a numbered sub-rule
  under Principle IV, or a new top-level principle? → A: A numbered
  sub-rule IV.1 under Principle IV, with the MINOR bump to 2.19.0.
- Q: Which API names should the quick-reference table carry? → A: The
  harness surface alone, counting the `doNotOptimize` and
  `clobberMemory` pair a benchmark body calls. Counters facility names
  stay out of the table.
- Q: Which code blocks should the drift gate compare against example
  regions? → A: Every marked block on any prose page. The obligation to
  mark binds the main page alone, so a marked detail-page block is
  checked and an unmarked one is left alone.
- Q: What compiler requirement should the quick start state? → A: A
  C++23 compiler and the CMake minimum, with no compiler version named
  on the page.
- Q: Who judges SC-001, the check that the first screen teaches what
  the library does? → A: Both devices. A model given the screen alone
  answers it on every change to the page, and three readers new to the
  repository answer it once before merge. Neither device is a CI gate.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `86552012620eeb305c158631e88d2153d0f152ca` |
| Audit-point date | 2026-10-12 09:58:38 -0500 |
| Default branch at run time | `master`, tip `8655201`, equal to the `origin/master` tip |
| Working tree at run time | clean at `8655201` before this feature directory was created |
| Feature number at run time | 018, the next free number under `specs/` |
| Short name at run time | `docs-main-page` |
| Project version at the audit point | 0.8.0 (`CMakeLists.txt`, `project()` version) |
| Harness `SOVERSION` at the audit point | 4 (`CMakeLists.txt`, harness target) |
| Counters `SOVERSION` at the audit point | 2 (`CMakeLists.txt`, counters target) |
| Constitution version at the audit point | 2.18.0, lineage top row and footer equal |

The audit read the tree of `8655201`, the squash commit of pull
request 35, "Feature 017 review fixes", merged 2026-10-12T16:06:55Z.
Citations below name a file and the symbol or section inside it. No
citation carries a line number: the prompt that opened this spec
requires name citations, because line numbers in a document that
outlives the audit point become false.

## Preconditions

Each precondition was checked in the tree at the audit point. The
result and its evidence follow.

### PC-0: H1, H2, and H3 have merged. PASS

- Pull request 32, "Harness: Add the benchmark harness core", merged
  2026-10-09T19:26:31Z. It is H1, feature `015-benchmark-harness-core`.
- Pull request 33, "Harness: Add families, fixtures and callbacks",
  merged 2026-10-10T15:29:18Z. It is H2, feature
  `016-harness-registration-and-fixtures`.
- Pull request 34, "017 harness timing modes", merged
  2026-10-11T05:04:25Z, and pull request 35, "Feature 017 review
  fixes", merged 2026-10-12T16:06:55Z. Together they are H3, feature
  `017-harness-timing-modes`.
- The three feature directories are present at the audit point, each
  with `spec.md`, `plan.md`, `tasks.md`, `research.md`,
  `data-model.md`, `contracts/`, and `checklists/`.
- The task ledgers are closed: `015` reports 77 closed boxes and zero
  open boxes, `016` reports 64 closed and zero open, `017` reports 33
  closed and zero open.
- The shipped surface is in the tree: `include/speedgun-ng/benchmark.hpp`,
  `include/speedgun-ng/barrier.hpp`, and the harness translation units
  `source/harness/registry.cpp`, `family.cpp`, `runner.cpp`,
  `report.cpp`, `catalog.cpp`, `cli.cpp`.

No H1, H2, or H3 feature falls to the planned list. Every one of them
is taught by the rewritten page.

### PC-1: the current main page. PASS

`README.md` holds 275 lines and ten second-level sections. Each
section is tagged by audience.

| Section | Audience |
| --- | --- |
| Title, one-line summary, name-origin paragraph | history |
| `Build` | user |
| `Contracts` | maintainer |
| `Re-pinning hwloc` | third-party |
| `Re-pinning simdjson` | third-party |
| `Re-pinning HdrHistogram_c` | third-party |
| `Re-pinning zlib` | third-party |
| `Re-pinning yaml-cpp` | third-party |
| `Re-pinning quill` | third-party |
| `Re-pinning pmu-events` | third-party |
| `Quality gates` | maintainer |

None of the ten sections shows a benchmark program. Seven of the ten
are re-pinning instructions for vendored dependencies. The only block
a benchmark author can act on is the `find_package` snippet under
`Build`. That snippet names `speedgun-ng::speedgun-ng` alone. The page
shows no benchmark program, no run loop, no report output.

Doxygen settings, `docs/Doxyfile.in`: `USE_MDFILE_AS_MAINPAGE` names
the root `README.md`, and `INPUT` names `README.md`, `include`, and
`docs/pages`. One rewrite therefore updates the GitHub page and the
documentation home page together.

Pages under `docs/pages/` and their audience:

| Page | Audience |
| --- | --- |
| `about.dox` | user, tooling note written as a Doxygen page |
| `barrier-rule.md` | internals, a rule with spec identifiers in its title |
| `counters-overhead.md` | internals, a measurement record |
| `dbc-migration.md` | internals, a migration map |
| `dbc-overhead.md` | internals, a measurement record |
| `harness.md` | user reference for the harness surface, titled with spec identifiers |
| `identifier-rename-map.md` | internals, a rename map |

### PC-2: the targets a benchmark links. PASS

`CMakeLists.txt` publishes two alias targets: `speedgun-ng::speedgun-ng`
over the library target and `speedgun-ng::harness` over the harness
archive. `example/CMakeLists.txt` proves the pairing: its
`add_example` function links `speedgun-ng::speedgun-ng` for every
example, and the harness example carries a second
`target_link_libraries` line for `speedgun-ng::harness`. A program
that registers a benchmark needs both targets. The `Build` section of
`README.md` names the first one alone, so a reader who copies it gets
link errors.

### PC-3: the example programs. PASS

`example/` holds four programs, each one a build target through the
`add_example` function: `empty_example`, `counters_giraffe_example`,
`counters_standalone_example`, `benchmark_example`. The same file
defines a `run_<name>` target per example and the aggregate
`run-examples` target. `CMakeLists.txt` gates the tree behind
`BUILD_EXAMPLES`, whose default is the developer-mode value.
`CMakePresets.json` sets that value ON in the `dev-mode` preset, and
`ci-ubuntu` and `ci-rocky` each inherit `dev-mode`, so the two presets
the CI matrix drives build the example tree. The `test` and
`test-rocky` jobs configure those presets and build the whole tree, so
every example target compiles in CI. The `test` job also inspects
`build/example/counters_standalone_example` with `readelf` for the
standalone link manifest.

Two facts bound what CI does today. No CI job runs
`build/example/benchmark_example`. No check compares a code block in a
document with a region of a source file. Both belong to this feature.

`benchmark_example.cpp` already carries one benchmark per shipped
capability: a plain accumulator, `bmArgs` with `argNames` and `args`,
`bmFill` through `SG_BENCHMARK_TEMPLATE`, `TableFixture` with the
`setUp` and `tearDown` pair, `bmProcessCpu`, `bmRealTime`, `bmManual`,
`bmPaused`, `bmMillis` with `unit`, a handle that calls `addMetric` on
an instructions-per-cycle expression, and `main` that returns
`sg::speedgunMain(argc, argv)`.

### PC-4: the public harness surface. PASS

Every name below was read in a public header at the audit point. All
of them are shipped, so all of them are taught by the page. Names are
listed by header.

`include/speedgun-ng/benchmark.hpp`, namespace `sg`:

| Name | Shape |
| --- | --- |
| `State` | the object a benchmark body receives: `iterations`, `range`, `rangeCount`, `skipWithError`, `skipWithMessage`, `pauseTiming`, `resumeTiming`, `setIterationTime` |
| `BenchmarkHandle` | the registration handle and the chain root: `minTime`, `warmupTime`, `repetitions`, `iterations`, `measureProcessCpuTime`, `useRealTime`, `useManualTime`, `unit`, `addMetric`, `arg`, `args`, `range`, `rangeMultiplier`, `ranges`, `denseRange`, `argsProduct`, `argName`, `argNames`, `setup`, `teardown` |
| `Fixture` | the fixture base with the virtual `setUp` and `tearDown` pair |
| `ScopedPauseTiming` | the RAII pause guard |
| `SG_BENCHMARK` | register a function |
| `SG_BENCHMARK_CAPTURE` | register a function with a captured value |
| `SG_BENCHMARK_TEMPLATE` | register a function template over type arguments |
| `SG_BENCHMARK_F` | define and register a fixture method |
| `SG_BENCHMARK_DEFINE_F` | define a fixture method |
| `SG_BENCHMARK_REGISTER_F` | register a defined fixture method |
| `SG_BENCHMARK_TEMPLATE_F` | define and register a fixture template method |
| `SG_BENCHMARK_TEMPLATE_DEFINE_F` | define a fixture template method |
| `SG_BENCHMARK_TEMPLATE_METHOD_F` | register a method of an instantiated fixture |
| `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` | instantiate a fixture and register a method |
| `setDefaultTimeUnit` | set the default unit for printed times |
| `speedgunMain` | the entry point that parses the command line and runs the suite |
| `BenchmarkResult` | the report value, with `suite` and `caseName` beside the instance name |

`include/speedgun-ng/barrier.hpp`, namespace `sg`:

| Name | Shape |
| --- | --- |
| `doNotOptimize` | keep a value live across the barrier |
| `clobberMemory` | force pending stores to complete |

The header also defines scaffolding macros that a benchmark author
never writes: `SG_BENCHMARK_REGISTRAR`, `SG_BENCHMARK_EXPAND`,
`SG_BENCHMARK_CAPTURE_REGISTRAR`, `SG_BENCHMARK_CAPTURE_EXPAND`,
`SG_BENCHMARK_TEMPLATE_REGISTRAR`, `SG_BENCHMARK_TEMPLATE_EXPAND`,
`SG_BENCHMARK_FIXTURE_DECLARE`, `SG_BENCHMARK_FIXTURE_REGISTER`,
`SG_BENCHMARK_TEMPLATE_INSTANTIATE_REGISTRAR`, and
`SG_BENCHMARK_TEMPLATE_INSTANTIATE_EXPAND`. The quick reference
excludes them.

Two shipped behaviors reach the page without a call of their own.
The `kDisabledPrefix` constant in `source/harness/cli.cpp`
is the `DISABLED_` prefix that keeps an instance out of a run and out
of the filter and the listing. The `suite` and `caseName` fields of
`BenchmarkResult` carry the suite and case split of an instance name.
The page teaches both.

`speedgunMain` reads its options from the long-option table in
`source/harness/cli.cpp`. The table holds nine entries: `--filter`,
`--list`, `--repetitions`, `--min-time`, `--iterations`,
`--warmup-time`, `--dry-run`, `--counter`, `--catalog`. The table
carries a short letter for each entry, and the short-option string that
`getopt_long` receives is empty, so the command line accepts the long
form. A usage error is fatal: a bad count, a bad duration, an invalid
`--filter` expression, and an unrecognized option each report one line
and finish the process with exit status 2.

### PC-5: the constitution. PASS

`.specify/memory/constitution.md` is version 2.18.0. The lineage top
row and the footer agree on it.

- Principle IV, "Documentation and Self-Documenting Code", exists with
  six bullets and no numbered sub-rule. It requires self-documenting
  code, forbids content-free comments and TODOs, and requires doxygen
  on every interface. It holds no rule about user-facing documentation
  per feature.
- Principle VIII, "CI Quality Gates", holds the hard gate list. It
  closes with the clause that weakening a gate is forbidden and that
  changing the gate set requires a constitution amendment.
- Principle XI holds XI.1 to XI.6 and XI.7, "Simplified Technical
  English".
- Governance records the amendment rule: MINOR for a new principle or
  materially expanded guidance. Each amendment carries a Sync Impact
  Report: an HTML comment at the head of the file, opened by
  `Sync Impact Report (<version>, <MAJOR|MINOR|PATCH>):` with a one-line
  summary, then the rationale paragraphs, then the re-anchoring note.

The prose gate scope covers the pages this feature touches.
`tools/prose/prose_gate.py` treats a root-level `README*`, a
root-level `AGENTS.md`, and anything under `.specify/memory/`,
`specs/`, and `docs/` as prose in scope. Both the main page and every
page under `docs/pages/` are scanned, and code fences, inline code
spans, blockquote lines, and markdown structure lines are
auto-exempt.

### PC-6: `docs/pages/harness.md`. PASS

The page holds 441 lines. Its title names the spec identifiers it
came from, and its 18 section headings are named by capability
identifier: `C-1: Argument families`, `C-4: Capture`, `C-6: Fixtures`,
and the rest, followed by the timing-mode sections. The body is the
detailed reference for the harness surface: instance-name grammar,
argument access, capture rules, fixture lifetime, the two time
columns, the overhead floor, the disclosure fields, and the
re-execution checks. It is the natural detail page behind a teaching
main page. Its titles carry spec identifiers, which the main page must
not carry.

## Roadmap position

D1 is the documentation leg of the harness roadmap. It runs before H4,
the harness user counters and statistics feature. Three consequences
follow.

- H4 gains a precondition: D1 is merged. H4 then writes its main-page
  sections in the shape this spec defines.
- The constitution rule in C-8 makes the main-page section part of
  every later feature spec, H5 onward.
- The H3 leftovers stay with H4. This spec carries no library change,
  so the pause allowance and the default-unit timing fix keep their
  home.

## Target page structure

The main page has two parts, then a quick reference, then the tail.

1. **What speedgun does.** One screen. A C++23 benchmark framework for
   Linux in the Google Benchmark style. Every time value and every
   counter value comes from the counters library of this project.
   Hardware counter metrics attach to a benchmark. Contract checks
   guard the API.
2. **How you do it.** One short section per feature. Each section has
   the same three parts: one purpose sentence, one snippet, one or two
   sentences on what the output shows.
3. **Quick reference.** One table for the API: name, what it does, the
   section that teaches it. One table for the command line: option,
   what it does.
4. **Planned work.** One short list, by feature name.
5. **Tail.** One link to the build and dependencies page, then a
   one-sentence note on the name and its origin.

## Material the main page drops

- Vendored dependency notes, re-pinning steps, and the distribution
  package install commands.
- Gate command listings and the gate internals that a contributor runs
  after they read a failure.
- Commit hashes, kernel refs, and every pinned version string.
- Spec numbers, requirement identifiers, success-criterion
  identifiers, task identifiers, and owner directives.
- Constitution text and principle numbers.
- CI job names.
- Source paths, except the paths of example programs.

The dropped facts move. None of them is deleted. See C-6, FR-022,
FR-023, and SC-006.

## Style for the two readers

- The two readers are a C++ developer who meets the library for the
  first time, and an LLM that must write a correct benchmark from this
  page alone. Both read the same text.
- Every feature section uses the same heading level and the same three
  parts.
- Every snippet is complete enough to paste into a benchmark file that
  already carries the includes the quick start shows. No ellipsis
  stands inside a code block.
- An API name appears in code font, spelled exactly as its header
  spells it.
- One idea per sentence. Prose follows Principle XI and XI.7, with
  zero prose gate findings on every changed file.
- The page states what a report shows. It carries no performance
  claim without a number, a platform, and a distribution.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Write and run a first benchmark from the page (Priority: P1)

A C++ developer opens the repository page on a Linux host. They know
Google Benchmark. They have not read a spec, the constitution, or the
CI setup. They read the quick start, copy the first benchmark program,
build it with the commands on the page, and run it. A report row
prints.

**Why this priority**: This is the page's job. Everything else on the
page is decoration next to a reader who builds and runs a benchmark
with the page alone.

**Independent Test**: On a clean Linux checkout, follow the quick start
verbatim: configure, build, install, then build the first program
against the installed package with the link line the page gives, then
run it. Success is a report row on standard output with exit status 0.
The test reads no file outside the page except the example program the
page names.

**Acceptance Scenarios**:

1. **Given** a clean Linux host with a C++23 compiler, **When** the
   reader runs the configure and build commands on the page, **Then**
   the build finishes and the install command produces a package the
   page's `find_package` call finds.
2. **Given** the installed package, **When** the reader builds the
   first benchmark program with the link line on the page, **Then**
   the link succeeds, which requires that the page names both targets
   a benchmark needs.
3. **Given** the built benchmark, **When** the reader runs it,
   **Then** the console shows a report row and the program exits 0.
4. **Given** the printed report, **When** the reader compares it with
   the sample output on the page, **Then** the columns match and only
   the numbers differ, because the page marks the sample as one host.

---

### User Story 2 - Learn what the library does in one screen (Priority: P2)

A reader who is deciding whether to adopt the library reads the first
screen. They can say what speedgun does, what platform it supports,
and what makes its numbers different from a hand-rolled timer loop.

**Why this priority**: A page that teaches a feature the reader never
looks for still has to say what the product is. The opening carries
that, and it costs one screen.

**Independent Test**: Run two devices. First, give the first screen
alone to a fresh model with no access to the rest of the repository,
and ask three questions: what is this, where does it run, where do the
numbers come from. Second, ask the same three questions of three
people new to the repository. Success is a correct answer to all three
questions on both devices.

**Acceptance Scenarios**:

1. **Given** the first screen, **When** a reader answers what the
   library is, **Then** the answer names a C++23 benchmark framework
   for Linux in the Google Benchmark style.
2. **Given** the same screen, **When** the reader answers where the
   times come from, **Then** the answer names the counters library of
   this project.
3. **Given** the same screen, **When** the reader answers what else
   the library reports, **Then** the answer names hardware counter
   metrics that attach to a benchmark.

---

### User Story 3 - Find one feature and copy a snippet that builds (Priority: P2)

A developer who already runs benchmarks needs the pause guard, the
argument family, or the manual time call. They open the section for
that feature, read one purpose sentence, copy the snippet into their
own benchmark file, and compile. The section tells them what the
resulting report row shows.

**Why this priority**: This is the repeat use of the page. It decides
whether the page stays the reference or the reader goes to the headers.

**Independent Test**: For each feature section listed in FR-009,
compile its snippet with the includes the quick start shows. Success
is 100% of snippets compiling in CI. Then check each snippet against
the example region it came from; a section whose block drifted fails
the drift gate.

**Acceptance Scenarios**:

1. **Given** a feature section, **When** the reader copies its snippet
   into a file with the quick-start includes, **Then** the file
   compiles and links with the page's build command.
2. **Given** the same section, **When** the reader looks up a name in
   the quick reference, **Then** the row for that name exists and
   links back to the section.
3. **Given** a shipped feature with no section, **When** the page is
   checked, **Then** the check fails, because every shipped feature of
   FR-009 owns a section.
4. **Given** a section that describes a report, **When** the reader
   runs the snippet, **Then** what prints matches the result sentence
   in shape: same columns, same aggregate lines.

---

### User Story 4 - Recover the maintainer facts on the build page (Priority: P2)

A maintainer re-pins a vendored dependency or reproduces a gate
locally. The main page no longer holds that material. One link
reachable from the main page leads to a page that holds every fact the
old main page held, in the same detail.

**Why this priority**: The rewrite moves real content. A moved-away
fact that no page carries is a regression for the people who need it
most often.

**Independent Test**: Walk the section-by-section map this feature
produces. For each of the nine dropped sections of PC-1, read the
old text and the new page side by side. Success is every fact present
on the new page: each command, each tool name, each tripwire
description, each version-pin mechanic.

**Acceptance Scenarios**:

1. **Given** the dropped section on the old page, **When** the
   maintainer searches the new build page, **Then** the page holds the
   section with its commands intact.
2. **Given** the main page, **When** the maintainer looks for the
   route to that page, **Then** one link near the end of the page
   reaches it.
3. **Given** the new page, **When** the documentation build runs,
   **Then** the page appears in the documentation index, and the
   prose gate scans it.

---

### User Story 5 - Docs ship with the feature (Priority: P3)

A contributor writes the spec for a later feature. The constitution
rule makes the main-page section part of that spec, and the compile
gate makes a stale block impossible to merge. The contributor changes
a main-page block without touching its source region, and the test
suite fails with the file, the region, and the differing lines.

**Why this priority**: Nothing in this feature depends on it. It is
how the page stays correct after this feature merges, so the roadmap
made it part of D1.

**Independent Test**: Edit one character inside a marked main-page
block, leave its example region alone, and run the test suite. Failure
with a named region is the pass. Then delete a region marker and run
again: an unmarked block on the main page must fail too.

**Acceptance Scenarios**:

1. **Given** a main-page block that differs from its region, **When**
   the drift gate runs, **Then** it fails and names the block, the
   example file, and the region.
2. **Given** a main-page block with no region marker, **When** the
   drift gate runs, **Then** it fails.
3. **Given** a later feature spec that changes user-visible behavior,
   **When** `/speckit.analyze` reads it, **Then** the rule in the
   constitution and the checklist line in the templates require its
   main-page section.
4. **Given** an example target that no longer compiles, **When** CI
   builds, **Then** the build fails, because the example tree is part
   of the build the gates run.

---

### Edge Cases

- A host publishes no hardware counter the benchmark asked for. The
  metrics section states what the run reports and what it does not
  report on such a host.
- A metrics snippet needs a counters name that the quick-reference
  table leaves out. The metrics section carries the expression form
  inside its snippet and links to the detail page, so the table stays
  the harness surface alone.
- A detail page holds a code block with no region marker. The gate
  leaves it alone, because the marking obligation binds the main page
  alone.
- A benchmark skips itself through `skipWithError` or
  `skipWithMessage`. The page shows both and names what the report
  prints.
- A name a benchmark calls is absent at the audit point. The quick
  reference and the snippets carry names that exist at the audit point
  alone; the drift gate and the example compile gate catch the rest.
- An example program prints numbers that differ from the sample output
  on another host. The sample output block is compared by shape through the
  mask mechanism the test suite already uses, so a digit change cannot
  fail the gate and a column change can.
- A snippet omits an include. The quick start owns the include list,
  and each snippet compiles with that list in a file that CI compiles.
- A reader is on macOS or Windows. The page states that Linux is the
  supported platform and offers no build for either.
- The reader copies the link line and the harness target is missing.
  The page names both targets, so the copy is correct the first time.
- A later feature adds an option. Its spec carries the quick-reference
  row, so the option table on the page cannot lag the option table in
  the source.
- The Doxygen main page and the repository page diverge. Both read the
  same file through `USE_MDFILE_AS_MAINPAGE`, so one edit updates both.
- A block moves inside its example file and the region markers move
  with it. The drift gate compares text, so a move that keeps the
  text equal passes.
- The prose gate and a code identifier disagree. Inline code spans and
  fenced blocks are auto-exempt, so an accurate API spelling never
  costs a finding.
- The example tree is off in a consumer build. The gate that reads the
  main page needs source text only, so it runs in a tree with
  `BUILD_EXAMPLES` off; the compile half needs the example targets and
  runs where the tree is on.

## Requirements *(mandatory)*

### Opening and quick start (C-1, C-2)

- **FR-001**: The main page MUST open with a part titled "What speedgun
  does".
- **FR-002**: That part MUST state, in one screen: the language
  standard, the supported platform, the Google Benchmark lineage, the
  counters library as the source of every time value and counter
  value, hardware counter metrics that attach to a benchmark, and
  contract checks that guard the API.
- **FR-003**: The main page MUST carry a quick start that states the
  supported platform, the C++23 requirement, and the CMake minimum,
  then the configure, build, and install commands. The quick start
  names no compiler version.
- **FR-004**: The quick start MUST give the `find_package` call and a
  link line that names every target a benchmark needs, which is
  `speedgun-ng::speedgun-ng` and `speedgun-ng::harness`.
- **FR-005**: The quick start MUST show one complete first benchmark
  program: includes, body, registration, and `main`. The program MUST
  come from a file under `example/` that CI builds and runs.
- **FR-006**: The quick start MUST give the build command and the run
  command for that program.
- **FR-007**: The quick start MUST show the console output of that
  program, labeled as a sample from one host.

### Feature sections (C-3)

- **FR-008**: The main page MUST carry a part titled "How you do it".
- **FR-009**: That part MUST hold one section for each shipped
  capability, and the list at the audit point is: the run loop with
  `doNotOptimize` and `clobberMemory`; registration through the
  registration macros, the handle chain, capture, templates, the
  `DISABLED_` prefix, and suite and case naming; argument families and
  ranges through `arg`, `args`, `range`, `ranges`, `rangeMultiplier`,
  `denseRange`, `argsProduct`, `argName`, and `argNames`; fixtures
  with `setUp` and `tearDown` and the `setup` and `teardown`
  callbacks; run control through `minTime`, `iterations`,
  `repetitions`, and `warmupTime` with the repetition aggregates;
  timing modes with real time, process CPU time, and manual time;
  pause and resume through `pauseTiming`, `resumeTiming`, and
  `ScopedPauseTiming`; the time unit through `unit` and
  `setDefaultTimeUnit`; hardware counter metrics through `addMetric`;
  skips through `skipWithError` and `skipWithMessage`; the command
  line through `speedgunMain`; and how to read the report.
- **FR-010**: Every feature section MUST use the same heading level and
  the same three parts, in order: one purpose sentence, one snippet,
  one or two sentences on what the output shows.
- **FR-011**: Every snippet MUST compile as pasted, given the includes
  the quick start shows. No code block MAY contain an ellipsis in
  place of code.
- **FR-012**: Every API name on the page MUST appear in code font with
  the spelling its header uses, and MUST exist at the audit point.
- **FR-013**: The metrics section MUST state the behavior on a host
  where the requested counter is unavailable.
- **FR-014**: The command-line section MUST list every long option in
  the option table of `speedgunMain`, name the accepted form, state
  that a usage error stops the program with exit status 2, and state
  what each option does.
- **FR-015**: The report section MUST explain each column, the
  aggregate rows, and the disclosure fields a row carries.
- **FR-016**: The page MUST NOT teach a name a benchmark author never
  calls. The scaffolding macros listed in PC-4 are excluded.
- **FR-017**: Each feature section MAY link to the detail page for its
  subject. The main page MUST stay a teaching page: the detail stays
  on the detail page.

### Quick reference (C-4)

- **FR-018**: The main page MUST carry a quick reference with one table
  that maps each API name to what it does and to the section that
  teaches it. The table holds the harness surface of PC-4 alone: the
  registration macros, `State`, `BenchmarkHandle`, `Fixture`,
  `ScopedPauseTiming`, `setDefaultTimeUnit`, `speedgunMain`, and the
  `doNotOptimize` and `clobberMemory` pair. A name from the counters or
  the DBC facility is outside that table.
- **FR-019**: The quick reference MUST carry a second table with each
  command-line option and what it does.
- **FR-020**: The quick reference MUST carry names and no source paths
  and no line numbers.

### Planned work (C-5)

- **FR-021**: The main page MUST carry a short planned-work list with
  the planned features named by name: user counters on the state,
  bytes and items throughput, labels, custom statistics, aggregate-only
  report output, complexity fitting, structured machine-readable
  output, and histogram output. The list MUST carry no spec number and
  no date.

### Build and dependencies page (C-6)

- **FR-022**: One new page under `docs/pages/` MUST receive every
  dropped fact named in "Material the main page drops", with its
  commands and its tool names intact.
- **FR-023**: The plan MUST carry a section-by-section map from each
  dropped section of the old main page to the place on the new page
  that holds it. Every fact in a dropped section MUST have a
  destination in that map.
- **FR-024**: The main page MUST link that page once, near the end.

### Documentation home (C-7)

- **FR-025**: The documentation page index MUST list the user pages
  first and MUST group pages that record internal measurements,
  migration maps, or rename maps under one internals heading.
- **FR-026**: The documentation build MUST pass with the rewritten main
  page as its main page.

### Constitution amendment (C-8)

- **FR-027**: The constitution MUST gain a numbered sub-rule IV.1,
  titled "Main-page documentation per feature", that states: every
  feature spec that adds or changes user-visible behavior MUST add or
  update its main-page section, with a working example in the shape
  FR-010 defines, with its quick-reference rows, and the spec is
  incomplete until it does.
- **FR-028**: The amendment MUST take the next minor version above the
  audit-point version, which is 2.19.0. It MUST update the version
  lineage table, the footer version, and the footer date, and MUST
  carry a Sync Impact Report in the form the file already uses.
- **FR-029**: The spec template and the tasks template MUST gain a
  checklist line that matches IV.1, so a plan and a task list carry
  the obligation where a contributor reads it.
- **FR-030**: The same amendment MUST add the example compile gate of
  C-9 to the Principle VIII hard gate list. An amendment is the only
  route to a gate change, and a gate with no entry in the law is a
  preference.

### Example compile gate (C-9)

- **FR-031**: Every code block on the main page MUST come from a
  marked region of a file under `example/`. A region marker is a
  comment in the example file that opens and closes the region and
  names it, and the block on the page carries the example file path
  and the region name in a form that renders invisibly. A page other
  than the main page MAY hold an unmarked block, and a marked block on
  such a page is held to the same rule.
- **FR-032**: Every example target MUST build in CI. The existing
  `BUILD_EXAMPLES` default already gives this; the feature MUST keep
  it.
- **FR-033**: The test suite MUST gain one gate that scans every prose
  page in scope for marked blocks and compares each marked block with
  its region. The gate MUST fail on any text difference, MUST fail on a
  main-page block with no region, MUST leave an unmarked detail-page
  block alone, and MUST name the file, the region, and the differing
  lines. It MUST run through `ctest`, following the script-gate pattern
  the suite already uses.
- **FR-034**: One CI job MUST run the harness example binary and
  compare its printed report with the sample output block on the page by
  shape, through the row-mask mechanism the harness tests already use.
  A digit difference MUST NOT fail the gate, and a missing or renamed
  column MUST fail it.
- **FR-035**: The gate of FR-033 MUST run in a tree with
  `BUILD_EXAMPLES` off, because the comparison needs source text alone.

### Discipline and version

- **FR-036**: This feature MUST change no library file, no public
  signature, and no build option. It MAY add example files, example
  targets, region markers, and gate scripts.
- **FR-037**: The project version MUST stay 0.8.0, because no library
  file changes. The harness and counters `SOVERSION` values MUST stay
  unchanged for the same reason.
- **FR-038**: The main page MUST carry no commit hash, no kernel ref,
  no pinned version string, no compiler version, no spec identifier, no
  requirement or success-criterion or task identifier, no owner
  directive, no constitution principle number, no CI job name, no
  re-pinning step, and no source path except an example program path.
- **FR-039**: Every example this feature adds MUST use the public
  headers, and every time value in it MUST come from the counters
  library, as the constitution already requires of every shipped
  program.
- **FR-040**: The prose gate MUST report zero findings on every file
  this feature changes.

### Capability to requirement map

| Capability | Requirements |
| --- | --- |
| C-1 Opening | FR-001, FR-002 |
| C-2 Quick start | FR-003 to FR-007, FR-034 |
| C-3 Feature sections | FR-008 to FR-017 |
| C-4 Quick reference | FR-018 to FR-020 |
| C-5 Planned work | FR-021 |
| C-6 Build and dependencies page | FR-022 to FR-024 |
| C-7 Documentation home | FR-025, FR-026 |
| C-8 Constitution amendment | FR-027 to FR-030 |
| C-9 Example compile gate | FR-031 to FR-035 |
| Discipline and version | FR-036 to FR-040 |

## Key Entities *(include if feature involves data)*

- **Main page**: the root `README.md`. It is the repository landing
  page and the documentation home page at the same time. Its purpose
  is to teach what speedgun does and how you do it.
- **Feature section**: one teaching unit inside "How you do it". Three
  parts in a fixed order: purpose, snippet, result. It owns one
  capability of the shipped surface.
- **Example region**: a named span inside a file under `example/`,
  opened and closed by markers. It is the single source of truth for
  one snippet on the page.
- **Page block**: a fenced code block on the main page. It names its
  example file and its region. It equals its region.
- **Sample output block**: the printed report of the first benchmark,
  marked as one host's run. It is compared by shape.
- **Quick-reference row**: one API name, one line of meaning, one
  section link. Every row names a shipped API.
- **Option row**: one command-line option and what it does. The rows
  equal the option table of `speedgunMain`.
- **Build page**: the page under `docs/pages/` that holds the vendored
  dependency and gate material the main page drops.
- **Page-index group**: the documentation index heading a page sits
  under. Two groups exist: user pages and internals.
- **Rule IV.1**: the constitution sub-rule that binds a main-page
  section to every later feature spec.
- **Drift gate**: the test that compares every marked page block with
  its example region across the prose pages, and fails on a difference
  or on a missing region on the main page.
- **Section map**: the plan artifact that pairs each dropped section of
  the old main page with the destination that holds its facts.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Both devices of User Story 2 pass from the first screen
  alone. A fresh model given that screen and nothing else answers all
  three questions correctly, and the run repeats on every change to the
  page as a review check at the pull request. Three readers new to the
  repository answer the same three questions correctly once before
  merge. Neither device is a CI gate.
- **SC-002**: On a clean Linux host, the quick start commands and the
  first benchmark build and run, with the page alone. The run exits 0
  and prints a report row.
- **SC-003**: Every capability in FR-009 has a section, and 100% of the
  page snippets compile in CI from the regions they name.
- **SC-004**: The drift gate reports zero differences between the main
  page and the example regions, and zero differences between any other
  marked block and its region, and it fails on a planted one-character
  change in any block.
- **SC-005**: A scan of the main page finds zero commit hashes, zero
  pinned version strings, zero spec identifiers, zero constitution
  principle numbers, zero CI job names, and zero re-pinning steps.
- **SC-006**: Every fact of the nine dropped sections appears on the
  build page, shown by the section map, which covers 100% of them with
  a named destination.
- **SC-007**: The documentation build passes, and its index lists user
  pages before the internals group.
- **SC-008**: The constitution carries IV.1 at version 2.19.0, the
  lineage table and the footer agree on it, and both the spec template
  and the tasks template carry the checklist line.
- **SC-009**: Every quick-reference row names an API that exists at the
  audit point, checked against a header name scan.
- **SC-010**: The prose gate reports zero findings on every file this
  feature changes.
- **SC-011**: The main page fits about 400 lines including the quick
  reference, and no feature section exceeds about 25 lines, so one
  sitting covers the page.
- **SC-012**: The harness example runs in CI and its report matches the
  sample output block by shape, so the page cannot show a report the harness
  no longer prints.

## Scope

In scope:

- The root `README.md`, rewritten.
- One new page under `docs/pages/` for the build and dependency
  material, and the retitling and grouping decisions that make the
  documentation index read by audience.
- Example programs, example targets, and region markers that carry the
  page's snippets.
- The drift gate script, its `ctest` registration, and the CI step
  that runs the harness example.
- The constitution amendment: IV.1, the Principle VIII gate entry, the
  version row, the Sync Impact Report, and the checklist line in the
  spec and tasks templates.

Out of scope:

- Any change to a library header, a library source file, a public
  signature, or a build option.
- The H3 leftovers, the pause allowance and the default-unit
  assignment. H4 owns them.
- JSON output, histogram output, user counters, labels, custom
  statistics, aggregate-only output, and complexity fitting. Those are
  later features, and each one writes its own section.
- New documentation tooling, a documentation site redesign, and any
  change to the publishing mechanism.
- macOS and Windows support text beyond the one-line statement that
  Linux is the supported platform.

## Assumptions

- The reader has a C++23 compiler and CMake on a Linux host, which is
  what the quick start asks for. The repository states no compiler
  floor beyond the language requirement, so the page states the
  language requirement and the CMake requirement, and names no
  compiler version it cannot support with a citation. Confirmed in the
  clarify session of 2026-10-13.
- The reader installs the package and consumes it through
  `find_package`, which is the path the downstream consumer gates
  already prove.
- One example program can carry the first benchmark of the quick start,
  and the existing `benchmark_example.cpp` pattern can carry the rest
  of the snippets, one file per teaching area, chosen in the plan.
- The report shape stays stable during this feature, because no library
  file changes. That is what lets the sample output block be a
  shape-checked constant.
- The prose gate treats code fences, inline code spans, and blockquote
  lines as auto-exempt, so a page built from accurate identifiers and
  commands costs no findings for its own accuracy.
- The `USE_MDFILE_AS_MAINPAGE` setting stays, so the repository page
  and the documentation home page remain one file.
- Later feature specs inherit IV.1 through the templates. The roadmap
  carries the docs line for each of them, and this spec adds no
  main-page text for a feature that has not shipped.

## Dependencies

- H1, H2, and H3 are merged, per PC-0. The page teaches their surface
  as it stands at the audit point.
- The documentation build, which consumes `README.md` through
  `USE_MDFILE_AS_MAINPAGE` and reads `docs/pages` as input.
- The prose gate, which owns the vocabulary and the exemption rules
  that the new prose must satisfy.
- The example tree and its `BUILD_EXAMPLES` gate, which is how the
  snippets get compiled in CI.
- The row-mask mechanism in the harness test suite, which the sample
  output check reuses.
- The `ctest` script-gate pattern, which the drift gate follows.
- The amendment process in Governance, which sets the MINOR bump and
  the Sync Impact Report form.
- H4 depends on this feature: it writes its main-page sections in the
  shape this spec defines, and its spec gains the precondition that
  this feature is merged.

## Citations

Read at the audit point. Each citation names a file and the symbol or
section inside it.

- `README.md`: the ten sections listed in PC-1, the `find_package`
  snippet, the name-origin paragraph.
- `docs/Doxyfile.in`: `INPUT`, `USE_MDFILE_AS_MAINPAGE`.
- `docs/pages/harness.md`: the title and the eighteen capability
  sections.
- `docs/pages/barrier-rule.md`, `counters-overhead.md`,
  `dbc-migration.md`, `dbc-overhead.md`, `identifier-rename-map.md`,
  `about.dox`: titles and audience.
- `CMakeLists.txt`: the two `ALIAS` lines, the `SOVERSION` values, the
  project version, the `BUILD_EXAMPLES` option and the `example`
  subdirectory.
- `example/CMakeLists.txt`: `add_example`, the four example targets,
  the `run-examples` target, the harness link line for
  `benchmark_example`.
- `example/benchmark_example.cpp`: the benchmark bodies, the chained
  registration sites, `addMetric`, `sg::speedgunMain`.
- `include/speedgun-ng/benchmark.hpp`: `sg::State`,
  `sg::BenchmarkHandle`, `sg::Fixture`, `sg::ScopedPauseTiming`,
  `sg::setDefaultTimeUnit`, `sg::speedgunMain`, the registration
  macros, and the scaffolding macros.
- `include/speedgun-ng/barrier.hpp`: `sg::doNotOptimize`,
  `sg::clobberMemory`.
- `source/harness/cli.cpp`: the long-option table, the usage-error
  exits with status 2.
- `.specify/memory/constitution.md`: Principle IV, Principle VIII,
  Principle XI with XI.7, the amendment rule and the Sync Impact
  Report form, the version footer and the lineage table.
- `tools/prose/prose_gate.py` and `tools/prose/prose_rules.yaml`: the
  markdown roots in scope, the auto-exempt constructs, the rule set.
- `.github/workflows/ci.yml`: the `test`, `test-rocky`, and `docs`
  jobs, the standalone example link-manifest audit step.
- `test/CMakeLists.txt`: the `add_test` pattern for script gates.
- `specs/015-benchmark-harness-core`,
  `specs/016-harness-registration-and-fixtures`,
  `specs/017-harness-timing-modes`: feature directories, task ledgers,
  and the `contracts/cli.md` option table.

## Decisions

Each decision carries the recommendation this spec adopts as its
working default. `/speckit-clarify` may confirm each as written or
replace it.

- **Q-1 Form of the docs rule**: Recommendation: a numbered sub-rule
  IV.1 under Principle IV, "Main-page documentation per feature", with
  a MINOR bump. Principle IV already owns documentation, so a new
  top-level principle would split one subject across two places.
  Confirmed in the clarify session of 2026-10-13 and adopted in FR-027
  and FR-028.
- **Q-2 Example compile gate**: Recommendation: a hard gate. Each page
  block names its example file and its region, the example targets
  build in CI, and a `ctest` script fails on any difference between a
  block and its region. Building the examples alone leaves the page
  free to drift from the file it quotes, so that alternative is
  rejected. Confirmed in the clarify session of 2026-10-13 with the
  marker-driven scope: the gate checks every marked block on a prose
  page, and the marking obligation binds the main page alone. Adopted
  in FR-031 to FR-035 and FR-030.
- **Q-3 Build and dependencies page location**: Recommendation:
  `docs/pages/building.md`. That directory is inside the prose gate
  scope and inside the documentation input, so the moved material
  stays checked and published. Adopted in FR-022.
- **Q-4 Line budget**: Recommendation: about 400 lines for the main
  page including the quick reference, and about 25 lines per feature
  section. The plan records the budget. Adopted in SC-011.
- **Q-5 Project history paragraph**: Recommendation: one sentence on
  the name and its origin, in a short footer. A longer history belongs
  in neither part of a teaching page. Adopted in the page structure
  tail.
- **Q-6 `docs/pages/harness.md`**: Recommendation: keep it as the
  detailed harness reference, retitle its sections by feature name,
  and link it from the sections that need detail. The main page
  teaches; that page gives detail. Adopted in FR-017 and FR-025.
- **Q-7 Version effect**: Recommendation: no project version change,
  because no library file changes. The constitution takes the next
  minor version, 2.19.0. Adopted in FR-028 and FR-037.
- **Q-8 Upkeep**: Decided by C-8. Each later feature spec owns its
  main-page section, its quick-reference rows, and its example regions
  as tasks of its own. Adopted in FR-027 and FR-029.
