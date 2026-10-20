# Specification Quality Checklist: Metric Formulas

**Purpose**: Validate specification completeness and quality before proceeding to planning
**Created**: 2026-10-19
**Feature**: [spec.md](../spec.md)

## Content Quality

- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [x] Success criteria are technology-agnostic (no implementation details)
- [x] All acceptance scenarios are defined
- [x] Edge cases are identified
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria
- [x] User scenarios cover primary flows
- [x] Feature meets measurable outcomes defined in Success Criteria
- [x] No implementation details leak into specification

## Notes

- No `/speckit.clarify` pass is needed. The request of 2026-10-19 carried
  its open points as accepted recommendations, and D-1 to D-9 record each
  one with the reading it settles. The spec carries no
  [NEEDS CLARIFICATION] marker.
- The subject of this feature is a metric formula, and the request names
  the surface a user reads: a metric by its perf name, the `--catalog`
  listing, one command-line option, `MetricResult`, the unit text. Those
  names stay in the requirement text because a user reads them. The spec
  carries no parser structure, no grammar table, no class body, and no
  build recipe.
- The Audit point, Preconditions, Vendored table inventory read and
  Citations sections name files and symbols. That is the house spec style
  of 015 to 020 and the evidence Principle VIII and IX ask for. Each anchor
  was read at the audit point `ab32400` and is named by file and symbol,
  with the line number left out. Those sections record what exists. They
  prescribe no implementation.
- Three measurements corrected the request's recommendations, and the
  change is recorded in the open (X.1). The clarify session of the same
  date settles the C-6 question this note raised: the fix ships with N2.
  The pinned tree carries the
  conditional `if` and `else` form in 267 rows, `has_event` in 35 and
  `source_count` in 31, and `#`-prefixed literal constants in 275 rows, so
  D-4 and D-9 widen the grammar and add the literal-constant rule the
  first reading of the request had as a footnote. The lowercase spellings
  the request names, `#slots` and `#smt_on`, are absent: the tree spells
  the SMT constant `#SMT_on`. Scope records what stays out.
- PC-1 passes for the parser and, as landed, passes for host
  reachability too: D-7 recorded `mapfileKey` omitting the CPU
  stepping, so a model-55 part selected no vendored table, and the
  machine this spec was written on is a model-55 part. C-6 shipped the
  fix, FR-034 to FR-036 its tests, and the landed record witnesses the
  reachability: `--catalog` lists 279 metric lines on that machine,
  128 published and 151 refused, and `metricByName` answers them by
  name (data-model.md, Real-host run). The one scope change this spec
  recorded is closed; no PARTIAL remains in this feature.
- PC-5 passes with the warning the spec acts on: `README.md` stands at 638
  lines against the 638-line page cap, so the added lines cannot land
  without the recorded move FR-031 names. D-6 records the placement choice
  and the measurement that can still overturn it.
- SC-02, SC-04, SC-07 and SC-08 name this project's own gates and option.
  The house convention for a success criterion in 015 to 020 is a check a
  contributor runs in this repository, and each of those four is binary and
  runnable here. The user-facing reading of each stays in SC-01, SC-03,
  SC-05 and SC-06.
