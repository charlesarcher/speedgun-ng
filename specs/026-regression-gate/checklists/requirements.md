# Specification Quality Checklist: Regression Gate

**Purpose**: Validate specification completeness and quality before
proceeding to planning
**Created**: 2026-10-20
**Feature**: [spec.md](../spec.md)

**Note**: The house spec shape of this repository carries a Preconditions
section, a Citations table and a Dependencies section that name source
files and symbols, because the constitution's R-DCUT process (Principle
III) requires a plan to re-read each anchor on the base it runs against.
The "no implementation details" item below is read against the resolution
the repo settled on for specs/023: the `FR-0nn` list states behavior only
and names no source file or internal symbol, while code anchors live in
Preconditions, Dependencies and Citations. Gate names that are repository
law (`LCOV_EXCL`, `PAGE_LINE_CAP`, the time-source gate) stay in the
requirement text.

## Content Quality

- [x] No implementation details in the requirements: the `FR-0nn` list
  names no source file, no symbol and no signature. The command-line
  option and rule grammar a user types is user-visible surface, so it
  stands in the decisions and the requirements as behavior.
- [x] Focused on user value and business needs: every story is a CI or
  maintainer journey (a pass or fail answer, a named rule, one clear fault
  line, a noise refusal, a library and document reading, a page a reader
  can copy).
- [x] Written for non-technical stakeholders where the journey is stated:
  the stories and edge cases name reports, rules, thresholds and states;
  the code-facing sections carry the anchors separately.
- [x] All mandatory sections completed: User Scenarios & Testing with six
  prioritized independently testable stories, Edge Cases, Requirements with
  Key Entities, Success Criteria, Assumptions, plus the house sections
  (Run-time binding, Decisions, Audit point, Preconditions, Roadmap
  position, Parallel work, Constraints, Scope, Budget note, Dependencies,
  Citations, Open questions, Constitution Checklist).

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain: the prompt carried every
  open item as an accepted recommendation, and the five points the prompt
  left open are recorded as D-15 to D-19 with a recommendation each and
  listed again under Open questions for the owner to overturn.
- [x] Requirements are testable and unambiguous: each `FR-0nn` states one
  observable answer, and the fault forms name the line and the exit status
  they answer.
- [x] Success criteria are measurable: SC-001 to SC-013 name the input
  pair, the state word, the exit status, the gate command or the test
  oracle that answers the verdict.
- [x] Success criteria are technology-agnostic at the outcome level: each
  names a run, a printed state, a written document or a named repository
  gate, and none names a type, a signature or a source file.
- [x] All acceptance scenarios are defined: 26 Given/When/Then scenarios
  across the six stories, each with a named input and a named answer.
- [x] Edge cases are identified: 23 entries covering empty reports,
  unmeasured outcomes, fit results with no comparable row, single-row runs,
  a zero-sample aggregate, missing statistics, an aggregate on a
  statistics-free value, labels holding grammar characters, availability
  drift between the two reports, zero baselines and zero contenders,
  percentile point drift, threaded rows, name matching, a comma in a path,
  versioned builds, duplicate rules, filter interaction, a non-report JSON
  file and an unknown schema major.
- [x] Scope is clearly bounded: In lists FR-001 to FR-074; Out names N7,
  N8, N6 pass fields, non-primary percentile columns, per-context values,
  statistical tests beyond the thresholds, other report formats, and a gate
  that runs benchmarks.
- [x] Dependencies and assumptions identified: Dependencies lists the
  report surface, the result types, the noise predicate, the entry point,
  the printers, the aggregate builder, the pattern rule, the vendored JSON
  library, the docs gates and the constitution gates; Assumptions records
  the defaults chosen for the merged row, the threshold unit, the fixture
  path and the reserved statistic names.

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria: each
  `FR-0nn` maps to a scenario in a story, an edge case, or a success
  criterion, and the fault requirements name the one line and the status
  they answer.
- [x] User scenarios cover primary flows: the pass and fail answer, the
  rule grammar, the fault path, the noise guard, the library and document
  reading, and the documentation flow.
- [x] Feature meets measurable outcomes defined in Success Criteria:
  SC-001 to SC-006 cover the verdict flows, SC-007 and SC-008 the test and
  coverage bar, SC-009 the written document, SC-010 the codegen and shape
  gates, SC-011 the documentation gates, SC-012 the CI gate set, and
  SC-013 the schema pairing.
- [x] No implementation details leak into the requirements: the grammar a
  caller types and the exit statuses a CI job reads are the contract; the
  call names, type names and translation-unit names are recorded as open
  names for the plan and appear nowhere in the `FR-0nn` list.

## Notes

- The prompt's recommendations are recorded as D-1 to D-14, dated
  2026-10-20 and attributed to the owner, who accepted them in the prompt.
  D-15 to D-19 come from this session's audit of `aec8fce` and carry no
  approval. The clarification session of 2026-10-20 settled D-17 and D-19
  and added D-20 to D-22, all as this session's decisions; D-15, D-16 and
  D-18 stay listed in Open questions for the owner to overturn.
- Three prompt statements needed a code fact to become testable, and the
  spec records the resolution the wording needs: the schema
  version pairing (the reader accepts any minor of major 1, so the pairing
  rule is about the two reports at PC-4 and D-13), the noise guard's
  threshold bound for an absolute-only rule (D-19), and the value name
  space's treatment of percentile columns (D-16).
- Two prompt claims were checked and corrected at the audit point: labels
  carry no character rule, so the escape exists (PC-7, D-15), and the
  reader drops the noise warning a report carries, so the gate needs the
  reader to answer it (PC-2, D-17, FR-070).
- Items marked incomplete require spec updates before `/speckit.clarify`
  or `/speckit.plan`.
