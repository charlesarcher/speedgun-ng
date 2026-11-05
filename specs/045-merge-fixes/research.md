# Phase 0 research: 045-merge-fixes

Spec: [spec.md](./spec.md). Base: master `b1dd7670`. Line numbers name
the state at that sha.

The research ran as two lanes, each with its own annex:

- Part A (counters): [research-part-a.md](./research-part-a.md)
- Part B (jitter probe): [research-part-b.md](./research-part-b.md)

Each annex states one decision per requirement with the code facts and
the rejected alternatives. The table below routes the reader.

| Requirement | Decision (one line) | Annex section |
| --- | --- | --- |
| FR-001 | One internal `KernelCalls` field returns `CpuidWords`; `pmuIdentCurrent` drops its static cache and reads the field per call; the limit-register probe stays real | Part A, FR-001 |
| FR-002 | Script every kernel refusal through the seam (`gRefuseAt`/`gErrno`); one real malformed-attribute refusal check stays (`counters_kernel_calls_test.cpp:133-140`) | Part A, FR-002 |
| FR-003 | Remove the `counters_provider.hpp:111` marker; swap LINE to BR at `group_io.cpp:306`/`:432` with the dark-line fallback; cover `gapped` lines `:288` and `:402`; keep `:426` and the retry block with ledger reasons | Part A, FR-003 |
| FR-004 | lcov removes a marked line from numerator and denominator; the new scenarios carry every repaired line; one short branch position enters the allowance | Part A, FR-004 |
| FR-005 | The ledger reads 84 at the Part A head, down from 94: 10 marker lines removed, the six G-21 markers stay, one reason per kept marker | Part A, FR-005 |
| FR-006 | The scripted clock's tail delta goes from 1 to `kTargetUnitTimeNs` (1000) under one named constant; expected magnitudes stand unchanged; no probe change | Part B, FR-006 |
| FR-007 | The sine arm computes `windowStep + llround(amplitude*sin)` in `std::int64_t` and clamps at 0; assertions stay relational | Part B, FR-007 |
| FR-008 | A per-cpu latch-time snapshot (prefix, kept count, dropped count, calibration figures, method) published through one atomic flag; the mid-run folds read the snapshot only; the run-level folds stay join-ruled; the `tsc` gap-fold choice lands in the commit B2 body, and the FR-011 mapping confirms it | Part B, FR-008 |
| FR-009 | The runner imports `prose_gate.py` as a module for every gate call except the timing case and self-times the whole entry against the unchanged 10 s bound | Part B, FR-009 |
| FR-010 | The lane ticks T048-T049 and T051-T056 when green; T050 stays open naming the allowance; T057 and T061 stay open naming the owning 043 lane; no 043 verify text changes | Part B, FR-010 |
| FR-011 | Each dark line and short branch maps to a named scripted arm in the four test files; a line no test can reach stops the work and is reported | Part B, FR-011 |
| FR-012 | The two `CMakeLists.txt` comment sites credit the archive SOVERSION 5 move to PR #70 (T-LCOV); comments alone | Part B, FR-012 |
| FR-013 | The ban binds the seam-driven counters tests; the watchdog and counter-limit probes stay real, unasserted; Part B stands under FR-006 to FR-009 | Part A, FR-013 |
| FR-014 | Standing behavior this lane changes nothing about; the commit A1 gate task asserts the option-error entry's exit 2 with one quoted line | tasks.md, T018 |
| FR-015 | "Added for coverage" means net-new exclusion lines and new exception classes; the FR-003 swap stands outside the ban | Part A, FR-015 |

No NEEDS CLARIFICATION item remains: the clarification sessions
recorded 25 accepted answers in the spec, and this research resolves
the one open design site the answers left (the `tsc` gap-fold site of
FR-008), by naming the two admissible shapes and putting the choice
under the FR-011 coverage mapping.
