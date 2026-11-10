# Specification Quality Checklist: Fix the counters and FWQ/FTQ gates

**Purpose**: Validate specification completeness and quality before
proceeding to planning
**Created**: 2026-11-03
**Feature**: [spec.md](../spec.md)

## Content Quality

- [ ] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [ ] Written for non-technical stakeholders
- [x] All mandatory sections completed

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [ ] Success criteria are technology-agnostic (no implementation details)
- [x] All acceptance scenarios are defined
- [x] Edge cases are identified
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria
- [x] User scenarios cover primary flows
- [x] Feature meets measurable outcomes defined in Success Criteria
- [ ] No implementation details leak into specification

## Notes

- Validation pass 2 (2026-11-03, after the analyze phase): the four
  unticked Content Quality and Feature Readiness items stay unticked
  on purpose. This feature is a fix lane: the FRs name files, symbols,
  line anchors and test entries because the acceptance evidence the
  feature description fixes demands them. The Notes paragraph below
  the checklist in the original pass recorded the same concession.
  The items pass for a feature spec in general and fail for this
  artifact class by design.
- Validation pass 1 (2026-11-03): all items pass. The spec carries no
  [NEEDS CLARIFICATION] marker, because the feature description
  recommends one answer per open question and the spec records each
  answer under Decisions (D-1 to D-9). The two owner questions stay in
  "Open, owner" with no ruling, as the description directs.
- The file and symbol references the FRs carry (seam field, marker
  lines, test names) describe the observable behavior under test and
  the acceptance evidence, which the description fixes; the design
  phase owns the implementation detail.

## Phase 13 validation (2026-11-07, owner ruling G-25)

**Feature**: [spec.md](../spec.md), section "Phase 13: Repair the
coverage-data fault"

### Content Quality

- [ ] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [ ] Written for non-technical stakeholders
- [x] All mandatory sections completed

### Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [ ] Success criteria are technology-agnostic (no implementation details)
- [x] All acceptance scenarios are defined
- [x] Edge cases are identified
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

### Feature Readiness

- [x] All functional requirements have clear acceptance criteria
- [x] User scenarios cover primary flows
- [x] Feature meets measurable outcomes defined in Success Criteria
- [ ] No implementation details leak into specification

### Notes on the phase 13 pass

- Validation pass 1 (2026-11-07): the four unticked items stay unticked
  for the same reason the 2026-11-03 pass recorded them. This is a fix
  lane: FR-016 to FR-022 name commands, files, symbols and line
  anchors because the acceptance evidence the ruling fixes demands
  them, and a reader cannot verify the repair without them.
- Two clarification rounds ran on 2026-11-07 with three readers each
  lens. Every question resolved to an answer the tree or a measurement
  supports, and the phase states each answer in the Clarifications
  section. No [NEEDS CLARIFICATION] marker stands in the phase.
- One item stands open with its evidence recorded: the capture list
  keeps `--ignore-errors inconsistent`, because that read's complaint
  is the end-line record of the function `SG_BENCHMARK_F` generates and
  its repair reaches the public macro family. FR-017 states the rule,
  and O-4 carries the open owner question.
- Main-page impact check (IV.1): no page change. No user-visible
  behavior lands, and the quickstart gains the reproducible command
  example SC-013 names.
## Phase 16 validation (2026-11-10, owner ruling G-35)

**Feature**: [spec.md](../spec.md), section "Phase 16: Match the
probe's printed units to the counter ticks"

### Content Quality

- [ ] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [ ] Written for non-technical stakeholders
- [x] All mandatory sections completed

### Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [ ] Success criteria are technology-agnostic (no implementation details)
- [x] All acceptance scenarios are defined
- [x] Edge cases are identified
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

### Feature Readiness

- [x] All functional requirements have clear acceptance criteria
- [x] User scenarios cover primary flows
- [x] Feature meets measurable outcomes defined in Success Criteria
- [ ] No implementation details leak into specification

### Notes on the phase 16 pass

- Validation pass 1 (2026-11-10): the three unticked items stay
  unticked for the reason the 2026-11-03 pass recorded. This is a fix
  lane: FR-038 to FR-042 name files, symbols and gate names because
  the acceptance evidence the ruling fixes demands them, and a reader
  cannot verify the repair without them. The design phase owns the
  implementation detail.
- The choice between disclosure and scale stands open by decision:
  D-28 fixes the contract (the printed unit matches the ticks the
  read answers) and leaves the mechanism to the plan phase, taken on
  the measured factor and the shape-gate surface. No [NEEDS
  CLARIFICATION] marker stands in the phase, because no answer blocks
  the specification; the plan records the choice with its reason.
- Main-page impact check (IV.1): no page change by default. The fix
  reaches the probe's report rows, and the shape gates pass numbers;
  a fix that moves a quoted block or a recorded line stands a shape
  finding reported with the scope it needs (edge cases).
- The dated correction lines on D-27 and the phase 15 O-8 row stand
  applied in this pass, per FR-041 and the owner ruling.
- Validation pass 2 (2026-11-10, clarify round 1): five questions stood
  and each answer stands encoded in the phase text (FR-039, FR-040,
  SC-035, SC-036 and the Clarifications section). All five print-site,
  JSON, threshold, calibration-target and tick-rate answers carry
  their grounding in the tree. No [NEEDS CLARIFICATION] marker stands.
- Validation pass 3 (2026-11-10, clarify round 2): the round found no
  remaining plan-blocking question. Three consistency findings, all
  one root cause, stand fixed: D-28 settles on the scale reading,
  FR-039 states the conversions unconditionally, and the `tsc` edge
  case names the series rows alone. The phase stands ready for the
  plan phase.
