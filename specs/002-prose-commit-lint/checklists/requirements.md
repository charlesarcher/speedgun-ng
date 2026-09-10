# Requirements Quality Checklist: Prose and Commit-Message Lint Gate

**Purpose**: Validate specification completeness and quality before planning
**Created**: 2026-09-10
**Feature**: [spec.md](../spec.md)

**Note**: This custom checklist is a reviewer-owned requirements-quality review artifact.
**Review Ownership**: Mark an item `[x]` only when the reviewer determines the requirements-quality criterion is satisfied.
**Marker Semantics**: `[x]` means the criterion has been reviewed and satisfied for requirements quality. It does not mean implementation work is complete.

## Content Quality

- [ ] CHK001 No implementation details leak in: the spec names surfaces, thresholds, and verdicts, and leaves interpreter, library, file layout, and algorithm to the plan
- [ ] CHK002 Focused on user value: each story states the maintainer or contributor outcome it delivers
- [ ] CHK003 Written for a non-technical reader: rules are described by behavior and by the constitution subsection they implement
- [ ] CHK004 All mandatory sections completed: User Scenarios, Requirements, Success Criteria, Assumptions

## Requirement Completeness

- [ ] CHK005 No `[NEEDS CLARIFICATION]` markers remain
- [ ] CHK006 Requirements are testable and unambiguous: each FR states a decidable condition, and each acceptance scenario is a Given/When/Then over an observable outcome
- [ ] CHK007 Success criteria are measurable: counts, thresholds, and fixture coverage carry numbers
- [ ] CHK008 Success criteria are technology-agnostic: SC-001 to SC-006 name no framework, language, or tool
- [ ] CHK009 All acceptance scenarios are defined for all three stories
- [ ] CHK010 Edge cases are identified, including the false-positive corpus (substrings, URLs, commands, ranges), the self-quoting constitution section, empty ranges, invalid encoding, and merge commits
- [ ] CHK011 Scope is clearly bounded: tree-wide sweep, editor hooks, chat-reply checking, historical message rewriting, and spell-check are each named out of scope
- [ ] CHK012 Dependencies and assumptions identified, including the dependency-parity assumption about CI runner tooling and the two accepted mechanical approximations

## Feature Readiness

- [ ] CHK013 Every functional requirement has at least one acceptance scenario or success criterion that verifies it
- [ ] CHK014 User scenarios cover the primary flows: enforcement on pull request (P1), commit template (P2), local parity (P3), each independently deliverable
- [ ] CHK015 Measurable outcomes in Success Criteria correspond to the enforcement the stories promise
- [ ] CHK016 Governance requirement present: FR-022 forces the constitution amendment that closes both deferrals in the same landing change, so no stale deferral survives the merge

## Notes

- Author self-review ran against these criteria before opening the pull request and found no `[NEEDS CLARIFICATION]` markers, no mandatory section left empty, and one deliberate open question resolved in the Clarifications section (the machine stand-in for the `genuinely trivial` exemption). Items stay unchecked for reviewer disposition.
- Two mechanical approximations carry known imprecision, stated openly: the changed-line threshold for a missing body, and the reject-list treatment of imperative mood. A reviewer who wants either tightened should push it into the rule data through the plan, and the imperative-mood case may justify a constitution amendment if full grammatical detection is ever wanted.
- Items marked incomplete require spec updates before `/speckit.clarify` or `/speckit.plan`.
