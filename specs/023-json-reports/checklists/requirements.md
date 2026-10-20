# Specification Quality Checklist: JSON Reports

**Purpose**: Validate specification completeness and quality before
planning
**Created**: 2026-10-20
**Feature**: [spec.md](../spec.md)

**Note**: This checklist is maintained by `/speckit.specify` and
`/speckit.clarify`. `checklists/requirements.md` has that separate
lifecycle; `/speckit.implement` reads the markers as a gate and must not
modify them.

## Content Quality

- [x] No implementation details in the requirement list: FR-001 to
  FR-050 state behavior. Code anchors live in Preconditions,
  Dependencies and Citations, and the paths that stay in the FR list are
  repository law or documentation artifacts (see Notes).
- [x] Focused on user value and business needs: each story names what a
  developer can do after the feature that they cannot do now, and each
  decision names the reason it exists.
- [x] Written for a stakeholder who does not write the code: the stories
  and the Scope section read without any C++ knowledge; the code facts
  sit in the sections a plan reads.
- [x] All mandatory sections completed: User Scenarios & Testing,
  Requirements, Success Criteria, plus the house sections (Run-time
  binding, Decisions, Audit point, Preconditions, Roadmap position,
  Parallel work, Constraints, Scope, Assumptions, Budget note,
  Dependencies, Citations, Open questions, Constitution Checklist).

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain. The prompt carried every
  open point as a recommendation Charles accepted, so the spec records
  them at D-1 to D-11 and D-13. The two points the prompt left open were
  answered in the 2026-10-20 clarification session and stand at D-12,
  FR-018, FR-030 and FR-050.
- [x] Requirements are testable and unambiguous: each FR names the
  observable outcome (a file that validates, two byte-identical files,
  one error naming a field, exit status 2, zero allocations in a
  window), and the Edge Cases list carries the input each FR must
  answer for.
- [x] Success criteria are measurable: SC-001 to SC-012 each name a
  count, a byte comparison, an exit status, a coverage percentage, or a
  named gate verdict.
- [x] Success criteria are verifiable without guessing: each names the
  oracle that answers it (the schema file, a byte comparison of two
  written files, the drift gate, the loop-shape gate, the coverage
  gates). See Notes on the template's technology-agnostic wording.
- [x] All acceptance scenarios are defined: six stories with 20
  scenarios in total, in Given/When/Then form.
- [x] Edge cases are identified: 20 entries covering an empty selection,
  list and catalog modes, dry run, every non-measured outcome, an
  unavailable percentile, a manual-time run, a threaded run, a placement
  text longer than the console field, a fit row, an empty string field,
  an interrupt, three path failures, three option failures, and four
  malformed-file shapes.
- [x] Scope is clearly bounded: In lists FR-001 to FR-050; Out names
  N5, N7, N8, N2, N4, N12, a load mode on the command line, and
  compression or a file per benchmark.
- [x] Dependencies and assumptions identified: Dependencies names the
  N1 surface, the result types, the entry point, the console printers,
  the vendored library and its privacy contract, the docs machinery and
  the constitution gates. Assumptions records the eight defaults the
  prompt left open.

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria: each
  FR maps to a scenario, an edge case, or a success criterion, and the
  seven requirement groups C-1 to C-7 plus the surface and discipline
  block carry the five capabilities the prompt names.
- [x] User scenarios cover primary flows: write a file, read a file
  back, ask for the raw values, fail cleanly on a bad option or a bad
  file, compare two runs, read the page.
- [x] Feature meets measurable outcomes defined in Success Criteria:
  SC-001 to SC-005 cover the writer, the loader and the failure paths;
  SC-006 to SC-008 cover the cost discipline; SC-009 to SC-010 cover the
  diff and threaded properties; SC-011 to SC-012 cover the docs and the
  gate set.
- [x] No implementation details leak into the requirement list: the
  writer's library choice, the builder call, the schema dialect and the
  new translation units sit in Decisions, Preconditions and Citations,
  and the FR list states what the file and the loader do.

## Notes

Two template items meet a house convention, and the resolution is the
one the repository settled for spec 021:

1. "No implementation details (languages, frameworks, APIs)". The
   requirement list states behavior and carries no internal symbol
   name. What stays in the FR text is repository law and shipped
   artifact paths: `LCOV_EXCL`, `gcov -b`, `PAGE_LINE_CAP`,
   `SOVERSION`, the time-source and loop-shape gates,
   `docs/schema/report-v1.json`, `docs/pages/harness.md` and
   `contracts/cli.md`. A spec that could not name those could not state
   the gate it owes. The library choice, the builder call and the new
   translation units live in Decisions, Preconditions and Citations.
2. "Success criteria are technology-agnostic". The house rule makes a
   success criterion verifiable by a named gate or test oracle, so
   SC-007 names the loop-shape gate and SC-011 names the drift gate.
   Each still states an outcome a reader can check without reading the
   implementation.

Three findings the audit point produced, recorded in the spec and worth
a reader's attention before `/speckit.plan`:

- PC-3: the chunk value columns exist only inside the runner at
  `c7f1769`, so `--json-raw=chunks` needs a home for them. The
  2026-10-20 clarification settled it: the columns travel beside the
  result, and the public result layout keeps its fields.
- PC-5: the harness target reaches the vendored headers nowhere today,
  and the vendored privacy rules of specs/004 FR-014, FR-015 and FR-016 hold the
  install tree. The build change is a build-interface-only dependency.
- PC-4: a report write that fails exits with status 2, and neither the
  entry point's postcondition nor the status table of `contracts/cli.md`
  carries such a row today. FR-050 puts the contract edit in the same
  change.

- Items marked incomplete require spec updates before `/speckit.clarify`
  or `/speckit.plan`. None is incomplete.
