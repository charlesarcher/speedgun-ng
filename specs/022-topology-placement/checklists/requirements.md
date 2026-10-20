# Specification Quality Checklist: Topology Placement

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

- No [NEEDS CLARIFICATION] marker was raised. The five open points the
  request carried arrived as recommendations Charles accepted, so they
  stand as D-1 to D-5 with the requirement each one lands in, and the
  spec's Open questions section carries no owner decision. Two points
  were left to the plan inside those bounds: the option spellings, now
  fixed by Clarifications Q-3 and plan decision R-07, and the translation
  unit that owns the hwloc include under the single-unit rule of
  specs/003-vendor-hwloc FR-021, now fixed by Clarifications Q-1 and plan
  decision R-01. Each behavior they carry is already fixed by FR-015 to
  FR-018 and FR-001 to FR-006.
- The subject of this feature is where a context runs, so the requirement
  text names the words a user types: the preset names `none`, `core`,
  `l3`, `smt`, `compact`, and the placement names of the surface feature
  020 shipped (`PlacementSlot`, `PlacementFn`, `setPlacementFunction`).
  Those are user-visible vocabulary. The spec carries no hwloc call, no
  class body and no data layout: the source of the layout is stated as a
  constraint and the mechanism is left to the plan. The one build
  statement is the privacy constraint that the vendored include and link
  stay `PRIVATE` and unexported.
- The audit point, Preconditions, and Citations sections name files and
  symbols, with the line number left out: the house spec style of 015 to
  021. Clarifications Q-8 is the one exception, citing `cmake/dbc.cmake:24`
  and `include/speedgun-ng/dbc.hpp:198-214`, because the rejected candidate
  turns on the dispatch body itself. Those sections record what exists at the audit point, and they
  prescribe no implementation.
- The preset list is a recorded decision with its evidence beside it. The
  naming check ran against Google Benchmark, where no placement option exists at
  `main` or at `v1.5.0`, `v1.8.5`, `v1.9.1`, `v1.9.4`, and against the
  placement vocabulary of OpenMP, Intel's compiler, Slurm, `numactl`, and
  hwloc. The reference read section carries the table and the final list.
- Success criteria are host-bound where the host is the point: SC-01 and
  SC-02 name thrawn's measured layout, read from sysfs at run time and
  recorded under PC-7, because the fit limits a user meets are the
  machine's. SC-04 keeps the rest of the proof host-independent through
  the injected layout of FR-006.
- The prose gate reads tracked paths only, because `--mode tree` walks
  `git ls-files`, and these artifacts stand untracked at the audit point.
  That command reports 0 sources and 0 units over this directory, so it
  cannot clear them yet: the zero it prints is an empty scan. The
  artifacts were written against the Principle XI rule set, and the run
  that can report a pass lands with the commit that tracks them.
