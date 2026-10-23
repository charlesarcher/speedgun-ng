# Specification Quality Checklist: Full Command Line

**Purpose**: Validate specification completeness and quality before
proceeding to planning
**Created**: 2026-10-22
**Feature**: [spec.md](../spec.md)

**Note**: The house spec shape of this repository carries a Preconditions
section, a Citations table and a Dependencies section that name source
files and symbols, because the constitution's R-DCUT process (Principle
III) requires a plan to re-read each anchor on the base it runs against.
The "no implementation details" item below is read against the resolution
the repo settled on for specs/023 and kept through specs/026: the `FR-0nn`
list states behavior only and names no source file or internal symbol,
while code anchors live in Preconditions, Dependencies and Citations.
Names that are repository law (`LCOV_EXCL`, `PAGE_LINE_CAP`,
`SECTION_LINE_CAP`, the time-source gate, the loop-shape gate,
`test/cli_shape.sh`, `tools/docs/example_drift.py`) stay in the requirement
text, and the option spellings and the environment variable names a user
types are user-visible surface, so they stand in the requirements as
behavior.

## Content Quality

- [x] No implementation details in the requirements: the `FR-0nn` list
  names no source file, no symbol and no signature. The two exceptions the
  repository law forces are named and bounded: FR-060 quotes the one run
  loop line the entry-point shape gate locates, and FR-075 names the drift
  gate that must follow the option table when it moves. Both name a gate
  anchor.
- [x] Focused on user value and business needs: every story is a CI or
  maintainer journey (one help text for one table, a setting taken from the
  environment, one fault line per bad value, thread counts from the command
  line, a seeded run order that still diffs, rules kept in a file, a page a
  reader can copy).
- [x] Written for non-technical stakeholders where the journey is stated:
  the stories and edge cases name options, sources, values, exit statuses
  and printed lines; the code-facing sections carry the anchors separately.
- [x] All mandatory sections completed: User Scenarios & Testing with seven
  prioritized independently testable stories, Edge Cases, Requirements with
  Key Entities, Success Criteria, Assumptions, plus the house sections
  (Run-time binding, Decisions, Audit point, Preconditions, Roadmap
  position, Parallel work and the version rule, Constraints, Scope, Budget
  note, Dependencies, Citations, Open questions, Constitution Checklist).

## Requirement Completeness

- [x] No open-question markers remain: the prompt carried twelve
  recommendations, recorded as D-1 to D-16, because recommendation 2 is
  D-2 to D-7, and the eleven points the audit of `bd10d76` gives are
  recorded as D-17 to D-27, each naming the fact it rests on. Four open
  questions stand: D-22, D-25, D-27, and the percentile column qualifier
  specs/026 D-16 left for N7.
- [x] Requirements are testable and unambiguous: each `FR-0nn` states one
  observable answer, and every fault requirement names the one line it
  prints, the source and value that line carries, and the exit status 2 it
  answers.
- [x] Success criteria are measurable: SC-001 to SC-014 name the option
  list, the source pair, the exit status, the printed line, the seed, the
  coverage percentage, the gate command or the test oracle that answers the
  verdict.
- [x] Success criteria are technology-agnostic at the outcome level: each
  names a run, a printed line, a written document, a table read by a test
  or a named repository gate, and none names a type, a signature or a
  source file.
- [x] All acceptance scenarios are defined: 32 Given/When/Then scenarios
  across the seven stories, each with a named input and a named answer.
- [x] Edge cases are identified: 26 entries covering prefix resolution and
  ambiguity, a repeated option in one source and across sources, an empty
  option file, a last line with no newline, a forbidden option-file line, a
  comment marker after text, an empty environment value, a malformed
  environment list, an unknown `SPEEDGUN_*` variable, `--env=off` and
  `--env=on`, a repeated thread count, a family pushed past
  `kMaxFamilySize`, a count above the affinity count beside a preset that
  cannot seat it, an inverted range, `--interleave` beside a mode option,
  interleaving with repetitions, an interrupt before the first row, a rule
  that names no benchmark, two faults in two sources at once, a `run`
  object read by a later reader, and a source option beside `--interleave`.
- [x] Scope is clearly bounded: In lists FR-001 to FR-086; Out names the
  `--benchmark_*` aliases, N9 context groups, N8 generators, the N6
  multi-pass fields already merged, the option rows the N13a and N14a lanes
  own, repetitions spread across instances, shell completion, the rule
  grammar of specs/026, the percentile column qualifier, environment
  variables outside the twelve rows, and any short option.
- [x] Dependencies and assumptions identified: Dependencies lists the entry
  point and scanner, the thread-count surface, the run loop and printers,
  the comparison surface of specs/026, the report surface of specs/023, the
  documentation gates and their budgets, the shape gates, the existing CLI
  tests whose expected text changes, the vendored JSON library and the
  constitution gates; Assumptions records the defaults chosen for prefix
  matching, one option file per run, the disclosed value form, holding the
  rows of a shuffled run, the thread-count replacement rule,
  and the one disclosure line.

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria: each
  `FR-0nn` maps to a scenario in a story, an edge case, or a success
  criterion, and the fault requirements name the one line and the status
  they answer.
- [x] User scenarios cover primary flows: the table and the help text, the
  environment and file sources with their disclosure, the fault path, the
  thread counts, the seeded run order, the rules file, and the
  documentation flow.
- [x] Feature meets measurable outcomes defined in Success Criteria:
  SC-001 covers the table and its uniqueness, SC-002 and SC-009 the sources
  and the disclosure, SC-003 and SC-013 the fault path, SC-004 the thread
  counts, SC-005 the run order, SC-006 the rules file, SC-007 and SC-008 the
  unchanged run and the help and version answers, SC-010 the coverage bar,
  SC-011 the documentation gates, SC-012 the codegen and shape gates, and
  SC-014 the CI gate set.
- [x] No implementation details leak into the requirements: the option
  spellings, the environment variable names and the exit statuses a CI job
  reads are the contract; the parse unit name, the table variable name, the
  environment seam type and the shuffle helper are left to the plan
  and appear nowhere in the `FR-0nn` list.

## Notes

- The prompt's recommendations are recorded as D-1 to D-16: D-1 to D-16
  restate recommendations 1 to 12 of the feature prompt (recommendation 2
  is D-2 to D-7). D-17 to D-27 come from the audit of `bd10d76` and each
  names the code fact it rests on. Four open questions stand: D-22, D-25,
  D-27, and the percentile column qualifier specs/026 D-16 left for N7.
- The spec takes four readings, each stated in the requirement text: the
  `run` object and the console line name their members and their form
  (D-8, D-21, FR-039, FR-040, FR-043, SC-009), the percentile pair reads
  the environment pair alone (D-5, D-27, FR-023), the scan takes the first
  of `--help`, `--version` or a usage fault (D-14, FR-017, SC-003, SC-008),
  and one seed reader covers both seed forms (D-4, D-17, FR-022, FR-051,
  FR-054). A repeatable list is replaced whole by the higher source, the
  comparison rules of `--compare-rule` and `--compare-rules` included
  (D-1, D-23, FR-038, FR-061, FR-063).
- Three prompt statements needed a code fact to become testable, and the
  spec records the resolution the wording needs: `--interleave` needs an
  optional argument because the prompt requires both the bare and the
  seeded form (D-17, FR-051, FR-052); the option table cannot stay in
  `source/harness/cli.cpp` without breaking the drift gate that reads the
  table out of that path (D-20, FR-002, FR-075); and the `run` disclosure
  object needs the schema to name it as an optional key, because the
  top-level object is open today and an unnamed key would leave the schema
  silent about a key the writer emits (D-21, FR-043, FR-086).
- Two prompt claims were checked at the audit point and one needed a
  number: `README.md` stands at 773 lines with `PAGE_LINE_CAP` at 773, so
  the page has no room and the cap must move (PC-5, FR-071, Budget note);
  and the section `The command line` already sits in `SECTIONS`, so this
  feature adds no section title and no entry in the section map (PC-5,
  FR-073).
- The roadmap file the prompt names,
  `/workspace/speedgun-ng-harness-roadmap.md`, is absent from this machine,
  so the prompt text is the authority for scope and the Audit point says so
  (Assumptions).
- Items marked incomplete require spec updates before `/speckit.plan`
  runs.
