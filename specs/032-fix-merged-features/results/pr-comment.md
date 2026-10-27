# Pull-request record: Fix Merged Features

The one comment FR-065 requires: each item of the request with its
commit or its question, the coverage numbers, and the CI run. The
comment names no approval, no acceptance and no person (FR-065,
SC-003). Figures stand in `results/record.md` and come from the CI run
of the last pushed commit (FR-061).

## Items and their commits

| Item | Commit | State |
| --- | --- | --- |
| Spec artifacts, plan, research, record | `Docs: Stand the fix-merged-features lane` | closed |
| FR-028, D-01: one precedence for the probe value | `Harness: Take the probe value by one rule` | closed with test |
| FR-021, D-02: the affinity-mask fallback | `Harness: Fall back to the affinity mask` | closed with test |
| the seam clear and the set width | `Harness: Clear the seam before the mask check` | closed with test |
| FR-030: the compare cells quoted | `Harness: Quote the name and rule cells` | closed with test |
| FR-017: one top-level `x-order` key | `Harness: Name the key order once` | closed with test |
| FR-034, FR-045, D-07: the ambient refusals | `Harness: Run the hunt beside the probe` | closed with tests |
| FR-035, D-08: the hunt beside the probe | `Harness: Run the hunt beside the probe` | closed with test and fix |
| FR-041 to FR-050: the hunter review items | `Harness: Stand the review items with their tests` | closed with tests |
| FR-024, FR-025: the requirement language | `Docs: Carry requirements in the named targets` | closed |
| FR-001, FR-004: the version lane | `Meta: Take the 0.20.1 version lane` | closed |
| FR-018: the samples and the sweep | `Meta: Take the 0.20.1 version lane` | closed, zero disagreements |
| FR-022: the family-probe print | `Harness: Take the probe value by one rule` | the print left; the root cause stands a question |
| FR-020: the loop shape under GCC 14 | question | this host carries no GCC 14; the gate passes on GCC 16.2.1 and on the test job's GCC 15.2.0; the reproduction routes to the Rocky lane |
| FR-023: the rebase blocks | recorded | the reconcile surface of nine files reads against master; the 38-block count and the eight files stand on the request's record |
| FR-049: the real-PMU test | question | this host carries no real PMU; the catalog is the fixture's own |

## Coverage

The trace of the coverage build reads 100.0 percent for lines (11978
of 11978), with no unhit line in any application file, and the branch
shortfall stands at 18 rows, within the recorded allowance of 19
(SC-002). After the affinity-mask fallback landed, the four
`--omit-lines` patterns of the count arm in `cmake/coverage.cmake:95`
to `:98` match no line of code, and lcov 2.6 answers its `unused`
error class over each of them, so the gate holds open on a host that
carries lcov 2.6 until the removal stands approved (D-12, FR-026).
The `cpus.empty()` pattern and the `SG_` macro pattern stay live.
This comment asks before any removal.

## CI

The branch head's run stands here when the push lands: every job of
the run reports success, and each figure above comes from that run
(FR-061). The base head's run `40438223916` read green job by job
before the work started (FR-064).
