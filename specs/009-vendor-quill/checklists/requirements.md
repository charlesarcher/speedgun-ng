# Specification Quality Checklist: Vendor quill as a Private, Pinned Submodule

**Purpose**: Validate specification completeness and quality before proceeding to planning
**Created**: 2026-09-30
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

- **Items marked incomplete require spec updates before `/speckit.clarify` or `/speckit.plan`.**
- **Deviations from the generic checklist wording, settled by the repository constitution, which outranks Spec Kit defaults:**
  - *No implementation details* and *technology-agnostic success criteria*: naming the third-party library, its upstream version tag and pinned commit, its build switches, and the audit commands is subject matter for
a dependency-ingestion feature. It does not constitute implementation leakage. The five prior vendor specifications set this precedent. The alternative, a specification that cannot name the artifact it vendors or state the commit it pins, would be untestable and unauditable. The specification still states only observable outcomes: what builds, what fails, what leaks, what a consumer sees.
  - *Written for non-technical stakeholders*: the audience is the maintainer and the reviewer who must audit the non-exposure contract. Requirements are phrased as testable obligations because this feature
delivers no user-visible behavior by design (Fixed decision 7). A user
journey would describe behavior this feature does not have.
- **Zero [NEEDS CLARIFICATION] markers by deliberate decision.** Three candidates were settled from in-repo
precedent and not escalated, and a later clarification session settled three more against measurement:
  1. *Scope of "make it available"*: whether the feature delivers a public logging API or only the vendored dependency. Settled by Fixed decision 7, since all five prior imports deliver zero public API and the brief asks for the same treatment.
  2. *Version-check granularity*: major-only or the full three-part version. Settled by FR-003, since the four prior imports carrying a compile-time tripwire all pin the exact version.
  3. *Platform priority*: settled by Fixed decision 5, identical to the prior five imports.
- **Central design deviation recorded, and left visible.** quill is an interface-only, header-carrying dependency: it compiles nothing, produces no archive, and defines every symbol it declares in the same unit that declares it. Every archive-level mechanism the four compiled prior imports established (archive merge, archive sanitizer exclusion, cross-member link proof, three-way `nm` proof) has no quill counterpart. Measured against the pinned tree, an archive proof of that shape cannot pass, because the required undefined reference never exists at any optimization level, and forcing one costs 563,104 bytes against 3,744 bytes for the version constants alone, a factor of about 150, in a benchmarking library's shipped archive. Per the Clarifications of 2026-09-30 the proof is a runnable check outside the archive instead. FR-008, FR-008a, SC-009, and SC-009a state it, and research R-008 records the measurement.
- **A second deviation, equally forced.** The four compiled brackets keep vendored warnings out of the strict set by clearing flag variables inside their own bracket, which works because each dependency compiles targets there. quill compiles nothing, so that technique reaches no compilation. FR-010a supplies the mechanism that does reach the headers and FR-010b records why the earlier technique is absent here. Research R-006.
- **Deferred by scope, named so they are inherited knowingly:** quill's backend spins a polling thread and calibrates a hardware timestamp counter on first use (Fixed decision 8 keeps both out of the shipped library), and that calibration carries a signed-overflow defect instrumentation would report. Both belong to the future specification that first emits a log record.
- **Prose conformance verified by hand.** The mechanical prose gate does not cover `specs/`, but Principle XI binds these documents anyway. Passes over the specification and the plan artifacts removed every em dash, contrastive construction, and filler word the gate bans elsewhere in the tree. The house `---` story separator matches the prior vendor specifications and stays.
- **Planning debt, now discharged.** The non-exposure audit identifiers restart at A1 in each specification, confirmed against specs/004, specs/005, and specs/006, so this feature allocates its own A1 through A8. The sequence is per-specification
and does not extend across specifications. The dependency-classification branch is research R-015, and the plan lists the files to touch.
