# Specification Quality Checklist: CI Speed-Up

**Purpose**: Validate specification completeness and quality before proceeding to planning
**Created**: 2026-10-25
**Feature**: [spec.md](../spec.md)

**Note**: This checklist is the `/speckit.specify` quality gate for this
feature. Mark an item `[x]` only when the criterion is satisfied for the
spec as written.

## Content Quality

- [x] No implementation details in the user-facing sections: the eight
  stories state wait times, verdicts and command outcomes a contributor
  observes. File, preset and flag names sit in the Spec identity,
  Baseline, Preconditions, Residual need, Decisions, Constraints and
  Dependencies sections by design, on the house precedent of specs/026
  and specs/029, because the feature is build configuration and its
  scope boundary can only be stated by naming the files it binds.
- [x] Focused on user value: the stories are reader outcomes (get the
  verdict in 15 minutes, run one local command in 5, read the analyzer
  verdict as its own job, see which matrix cell broke, pick a tier, see
  a slow test fail where it was written, keep an install step from
  owning the clock, cancel a superseded run).
- [x] Written for a non-technical reader at the story level; mechanism
  names appear as defined entities in Key Entities, each settled by a
  Decision.
- [x] All mandatory sections completed (User Scenarios with eight
  prioritized stories, Edge Cases, Requirements, Key Entities, Success
  Criteria, Assumptions, Constitution Checklist).

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain. The one marker the first
  draft carried, the disposition of the three reds the base already
  carries (`lint` on `test/source/harness_cli_table_test.cpp`, the
  coverage shortfall of 63 lines and 121 branches at run 39941934864, one
  `prose-lint` XI.2 finding), was answered by the owner on 2026-10-25 and
  is recorded in the Clarifications section: FR-064 leaves each red to
  its owning spec, FR-065 carries the one formatting-only exception, and
  SC-003 and SC-012 read the equality against the base tree.
- [x] Requirements are testable and unambiguous: each FR names a trigger
  and an observable reading (a `gh` field, a `ctest -N` list, an
  artifact diff, an exit status, a line count).
- [x] Success criteria are measurable: SC-001 to SC-012 each name a
  number, a list equality, an exit code or an artifact comparison.
- [x] Success criteria are technology-agnostic at the outcome level
  (wall time in minutes, budget compliance, list equality, exit status,
  line and branch percentages). SC-003 and SC-005 name their measuring
  instruments, following the specs/022 and specs/029 precedent for
  gate-verifiable criteria on an infrastructure feature.
- [x] All acceptance scenarios are defined (eight stories, twenty-three
  Given/When/Then scenarios).
- [x] Edge cases are identified (fifteen, including a cold cache, a
  poisoned cache key, an analyzer finding on a warm cache, a shared
  resource at `-j $(nproc)`, thread oversubscription, the unlocked
  `gcda` merge, a compiler bump, a hang, a shard that would drop a
  target, a compiler-version record change, a docs deploy after a failing
  analyzer, a canceled run, a cache-restore miss, and a wrong
  clang-format version).
- [x] Scope is clearly bounded: host validation, any option, environment
  variable or schema key, a container image push, version and SOVERSION
  changes, any new ignore class, exclusion marker, skipped test, cached
  analyzer result or raised timeout are each named out of scope, and the
  file list of the Constitution Checklist states what the diff may touch.
- [x] Dependencies and assumptions identified (Dependencies and
  Citations lists every `gh` read and every repository read behind the
  baseline; Assumptions records the 4-vCPU runner, the compiler-family
  grouping, the shared 10 GB cache budget and the pinned formatter).

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria (trace:
  C-1, C-2, C-7 and C-8 to US1 and US8 via SC-001 and SC-002; C-3 to US3
  via SC-005 and SC-011; C-4 to US4 via SC-004; C-5 to US2 via SC-007
  and SC-010; C-6 to US6 via SC-008; C-9 to SC-003, SC-006 and SC-009;
  C-10 to SC-001, SC-002, SC-004, SC-005, SC-006; C-11 to SC-009).
- [x] User scenarios cover primary flows (a push, a local pre-push run,
  the analyzer verdict, the matrix, the tiers, a budget failure, an
  install step, a canceled run).
- [x] Feature meets measurable outcomes defined in Success Criteria: the
  request's AC-1 to AC-9 map one to one to SC-001 to SC-009, and
  SC-010 to SC-012 carry the docs, gate-bite and base-red obligations the
  hard rules add.
- [x] No implementation details leak into specification beyond the
  evidence sections and the obligations the constitution requires by
  name (preset names, cache key shape, `PROCESSORS`, `RESOURCE_LOCK`).
- [x] The request's residual-need obligation is met: the Residual need
  table covers all twelve reverted commits, each with a verdict and the
  in-scope route a live need takes, with the CI evidence named.

## Notes

- No marker is open. The spec is ready for `/speckit.plan`. The plan
  carries one ordering consequence of the clarification: the branch's
  first commit is the formatting-only commit FR-065 states, because every
  other job waits behind `lint`.
- The Preconditions section carries its evidence now, at the base commit
  `f507b8f`, with two corrections against the request: the matrix script
  path (`tools/dbc/semantics_matrix.sh`) and the count of `TIMEOUT`
  entries at the base (nine, eight at 120 s and `example_run` at 300 s).
  PC-6 is re-read at the branch audit point before the compiler-family
  split of D-5 is trusted.
- Items marked incomplete require spec updates before `/speckit.clarify`
  or `/speckit.plan`.
