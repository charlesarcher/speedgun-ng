# Implementation Plan: Fix Rocky PMU Stack Smashing and the Harness Jitter Window Flake

**Branch**: `fix/rocky-pmu-stack` | **Date**: 2026-11-01 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/044-fix-rocky-pmu-stack/spec.md`

## Summary

Two intermittent `test-rocky` failures, each fixed where its cause sits.

B-1: `FastContext` (`source/counters/detail/pmu.hpp`) declares a destructor
and no copy or move operations, so the compiler writes a copy assignment;
the partial-open scenario's assignment releases one descriptor and one
mapping twice, and four test sites hand the context a stack address the
destructor's `munmap` then addresses. The fix completes the rule of five:
deleted copy operations, user-defined `noexcept` move operations that
transfer all five fields and empty the source's resource triple, an
explicitly defaulted default constructor, and the four test sites moved to
owned anonymous mappings. Contracts ride the move operations as
`SG_ENSURE` pairs; tests count releases with the existing descriptor and
mapping count helpers and add the compile-time not-copyable assertion.

B-2: both jitter-window gate bodies wait on `provider.readActions()`, one
process-wide atomic that every fake-window read on any thread increments,
so their margins unblock on reads the probe never made; under cpu load the
pass's window closes with no kept gap at the pass's magnitude
(`diagnosis-b2.md`: 264 of 500 loaded runs). The fix installs a counting
reader through the reader-install seam (`installTestReader`) over the fake
provider's own scripted `machine/tsc` leaf, one delegate window per probe
thread, so the waits end on the probe's own read count with targets
derived from the documented 1,024 reads per probe cpu; the drift margin
and `passGateBase()` leave the test; the spin bound and the yield stay.

## Technical Context

**Language/Version**: C++23, GCC and Clang on Linux x86-64 (the only
supported platform)

**Primary Dependencies**: none beyond the standard library and the
vendored trees under `external/` (unaffected by this change)

**Storage**: N/A

**Testing**: the repository's own frameworkless `check()`/`fail()` test
binaries driven by `ctest`; the coverage, sanitizer, and lint gates of
Principle VIII

**Target Platform**: Linux x86-64 (Rocky Linux 10 and Ubuntu runners; all
acceptance local per FR-017)

**Performance Goals**: none new; the read path gains nothing (the
contract facility drops its checks from release builds, and the gate
bodies are test code)

**Constraints**: no CI change (FR-013), no public ABI change (FR-014),
coverage at the existing 100% gate with the allowance at 21 (FR-015), no
sleep, clock read, timeout, or retry in any test (FR-016, FR-012)

**Scale/Scope**: one internal seam header, one library source file, two
test files, and this feature's spec directory

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **I (standard-first)**: the move operations use only standard C++23;
  the C++23 aggregate rule the design surfaces ([dcl.init.aggr]) is
  standard-conformant behavior the tests must adapt to; the standard
  states the rule.
- **II (DBC)**: each new move operation carries the doxygen `\pre`
  row and `\post` rows of `contracts/move-contracts.md` in the seam
  header, enforced with `SG_ENSURE` beside the out-of-line
  definition; the move assignment's different-values precondition
  carries one `SG_REQUIRE` with its reason.
  `contracts/move-contracts.md` states the exact pairs.
- **III (R-DCUT)**: the design adds no abstraction beyond the counting
  reader the defect class needs; rejected alternatives stand recorded in
  `research.md`.
- **IV (documentation)**: no user-visible behavior changes, so the root
  page needs no update (spec Constitution Checklist).
- **V (style)**: the change builds under `ci-ubuntu`, where clang-tidy
  and cppcheck run on every translation unit.
- **VI (coverage)**: the moved-from and move-assignment tests count
  releases through the existing helpers; no `LCOV_EXCL` marker, no
  `--omit-lines`; the contract macros sit outside the measurement as the
  facility already does.
- **VII (performance)**: the library's read path is untouched; the test
  waits replace one atomic read with one atomic read.
- **VIII (gates)**: every hard gate runs on the branch before the pull
  request opens (see quickstart.md and the spec's SC-004).
- **IX (SDD)**: this plan, its research, data model, contracts, and the
  task list stand inside `specs/044-fix-rocky-pmu-stack/`.
- **X (anti-slop)**: no self-move guard (the release empties the fields,
  so the transfer ends owning nothing), no moved-from flag, no shared
  ownership; each rejection carries its reason in `research.md`.
- **XI (prose)**: all four artifacts were written under the XI rules and
  stand scanned clean.

## Project Structure

### Documentation (this feature)

```text
specs/044-fix-rocky-pmu-stack/
├── spec.md                    # the specification (/speckit.specify output)
├── plan.md                    # this file
├── research.md                # Phase 0 output: both approaches, rejected alternatives, risks
├── data-model.md              # Phase 1 output: E-01 FastContext, E-02 counting reader
├── contracts/
│   └── move-contracts.md      # Phase 1 output: the SG_ENSURE/SG_REQUIRE pairs
├── quickstart.md              # Phase 1 output: build, run, and acceptance commands
├── diagnosis-b2.md            # the B-2 diagnosis record the spec's FR-009 cites
├── clarify-critic.md          # clarify round 1: critic questions and answers
├── clarify-evidence.md        # clarify round 1: code-evidence findings
├── clarify-round2.md          # clarify round 2: convergence verdict
├── checklists/
│   └── requirements.md        # the specification quality checklist
└── tasks.md                   # Phase 2 output (/speckit.tasks, next)
```

### Source Code (repository root)

```text
source/counters/detail/pmu.hpp        # FastContext: deleted copies, move
                                      # declarations, doxygen contract pairs
source/counters/linux_pmu/fast_read.cpp  # move definitions, SG_ENSURE
                                      # enforcement, beside fastContextClose
test/source/counters_linux_pmu_seam_test.cpp  # four owned mappings, the
                                      # partial-open move, two count tests,
                                      # the static assertion
test/source/harness_jitter_window_test.cpp    # CountingReader, the install
                                      # and clear, the four waits re-targeted
source/harness/detail/jitter_probe.hpp # kPassReads moves here (internal)
```

**Structure Decision**: the change touches the two files the defects live
in and the two test files that measure them; the `kPassReads` move touches
`source/harness/jitter_probe.cpp` and
`source/harness/detail/jitter_probe.hpp`; no new translation unit, no
new target, no public header.

## Complexity Tracking

> No constitution check has a violation to justify; the table carries no row.
