# Refuse a document whose pass fields disagree

`loadReport` now compares the pass fields of a report entry with each other. A
method word, a pass count, a pass spread and a pass tag form three pairs. A pair
that disagrees in a way the writer cannot produce answers one error that names
the field and quotes its partner's path, and the read stops there. The writer,
the schema file, the schema checker and the comparison stand unchanged, so every
document the harness writes keeps loading, and a stored baseline of schema 1.0
keeps loading.

Roadmap item T-LCF. Spec `specs/047-loader-cross-field/spec.md`.

## The lane

- 10 commits over `master`, base `b1dd7670`, head `2ffba1dd`.
- 18 files changed, 2950 insertions, 17 deletions.
- Source: 48 lines at the end of `readEntry` in `source/harness/json_load.cpp`.
- Tests: nine functions in `test/source/harness_json_load_test.cpp`, one in
  `test/source/harness_compare_cli_test.cpp`.
- The law lands first: FR-038 of `specs/024-multipass-collection/spec.md` carries
  the three checks, the pair rule, the presence reading and the rule-1 waiver
  before the reader enforces them.
- Version `0.30.1`. Schema `1.13`, harness `SOVERSION 20` and counters archive
  `SOVERSION 5` stand, so no public layout moves and no number moves twice in one
  unreleased window.

## The host

- Host `chilly`, kernel `Linux 7.3.1-1-cachyos x86_64`.
- AMD Ryzen 9 7950X3D, 16 cores, 32 threads.
- gcc 16.2.1 (20260810), clang-format 18.1.8 pinned, cmake 4.4.4, lcov 2.x.

## The gates

- Red, then green: the load binary printed `13 checks failed` with the reader
  absent, and the compare command answered two failures. Both binaries print `ok`
  with the reader present: 36 cases in the load test, 38 in the compare test.
- Full suite at the head: 178 tests, 176 passed, 1 failed, 1 skipped. The same
  suite pinned to two cores: the same 178, the same one entry, the same skip. The
  failing entry and the skip stand at the base.
- ASan and UBSan: 173 tests, 171 passed, 1 failed, 1 skipped. ThreadSanitizer:
  174 tests, 171 passed, 2 failed, 1 skipped. The extra entry is a jitter row,
  and the one race tsan reports sits in the FWQ probe, in files this lane leaves
  byte-identical.
- Lint with the pinned formatter, codespell, the example drift check, the prose
  gate over the range and over the feature tree, the dbc doc gate and pair gate
  at 345 interfaces with no gap, and the clang-tidy and cppcheck build: all exit
  0.

## The coverage

| Reading | Lines | Functions | Branches |
| --- | --- | --- | --- |
| Base `b1dd7670`, host | 99.5% (17484 of 17565) | 97.3% (1804 of 1854) | 98.8% (8593 of 8701) |
| Head, host | 99.5% (17519 of 17600) | 97.3% (1804 of 1854) | 98.8% (8635 of 8743) |
| Head, `source/harness/json_load.cpp` | 99.2% (2194 lines) | 100.0% (178 of 178) | all arms reached |

The lane adds 35 instrumented lines and 42 branches, and the uncovered counts
stand unchanged at 81 lines, 50 functions and 108 branches: every line and every
branch the lane adds is covered. The branch shortfall of 108 rows is the standing
host figure at the base, and the gate's allowance is 21 rows
(`tools/dbc/coverage_gate.sh:102`); D-12 of the spec carries the written
justification. The coverage job's capture does not run on this matrix, at the
base or at the head: the job stops on the standing jitter row, and its upload
step answers `No files were found with the provided path:
build/coverage/coverage.info`. The host reading therefore carries the coverage
evidence for the merge.

## What the matrix reads

Run `41727692423` reads at the head, and the five stopped jobs ran a second time.
`lint`, `prose-lint`, `dbc-gate`, `shared-audit`, `downstream-consumer`,
`consumer-release` and all three `static-analysis` shards pass on both readings.
`test`, `test-rocky`, `sanitize`, `tsan` and `coverage` stop on suite entries, and
no loader unit and no compare unit stands among them: `harness_json_load_test`
and `harness_compare_cli_test` pass in every job of both readings. The base's run
`41580787441` stops on the same jitter rows on the same matrix. The two entries
the head adds over that set are PMU-permission rows on the runners, the provider
row that answers with the kernel's own text and the `docs_metric_passes` row that
answers `permission_blocked` for `cpu/cache-misses`; both move between jobs
across the two readings.

`specs/047-loader-cross-field/results/acceptance-2026-11-03.md` carries the
commands, the counts, the gate exits, the per-job readings and the host
conditions end to end.
