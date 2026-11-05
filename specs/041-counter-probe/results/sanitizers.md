# The sanitizer rows (T055)

Host `gary`, Linux 7.3.1-1-cachyos x86_64, c++ (GCC) 16.2.1 20260810. Head `99f3d650`.
Trees: `build/sanitize` (`ci-sanitize`, Address and Undefined) and `build/tsan`
(`ci-tsan`, Thread). The `build` directory holds the release developer-mode tree the rest
of this lane reads, and no sanitizer row touches it. Logs:
`/tmp/041-notes/p8-san-suite.log`, `/tmp/041-notes/p8-tsan-suite.log`, and the earlier
`p6-sanitize-ctest.log`, `p6b-san-ctest.log`, `p6-tsan-ctest.log`, `p6b-tsan-ctest.log`.

## The rows at this head

| Tree | Configure | Build | Suite | Sanitizer reports |
|---|---|---|---|---|
| `ci-sanitize` | exit 0 | exit 0 | exit 0: 100 percent of 180 passed, 0 failed, 0 `Not Run`, 1 skipped | 0 lines |
| `ci-tsan` | exit 0 | exit 0 | exit 0: 100 percent of 181 passed, 0 failed, 0 `Not Run`, 1 skipped | 0 lines |

The report greps are `ERROR: AddressSanitizer`, `runtime error`, `LeakSanitizer`,
`SUMMARY` for the first row and `ThreadSanitizer`, `SUMMARY: ThreadSanitizer`,
`LeakSanitizer` for the second. Each answers 0 lines over its whole suite log, so no
sanitizer said anything across 361 entry-runs of this branch.

Entry counts differ by registration only: the release tree registers 185, `ci-tsan` 181
and `ci-sanitize` 180. The five `dbc_matrix_*` entries need `SPEEDGUN_DBC_MATRIX`, which
neither sanitizer preset sets, and four allocation-heavy units stand unregistered under a
sanitizer. The one skipped entry in both rows is `harness_topdown_run_test`, the skip it
carried at the base: it reserves its skip code for a host that grants the top-down nodes,
and `test/tier_labels.sh` holds its row.

## The three readings that carried no verdict, and why they are here

Two rows were retaken to reach the table above. Each earlier reading failed on a host
carrying two suites at once, and the unit that failed was never a file this branch
touches:

| Reading | Failure | What a solo re-run answered |
|---|---|---|
| `p6-sanitize-ctest.log` | `harness_jitter_cli_test` failed, 0 report lines | passes alone in 55 s |
| `p6-cover-ctest.log` | `harness_jitter_window_test`, `harness_precheck_test`, `harness_jitter_cli_test` failed, 0 report lines | all three pass alone, the last in 188 s under instrumentation |
| `p6b-tsan-ctest.log` | `harness_execution_context_test`, verdict `Timeout`, 0 report lines | 100 percent of 181 in the row above |

Those units spawn child runs and measure spans, so they are the first entries a contended
machine stops. A suite that failed carries no verdict, and the table above is the reading
taken with the host idle. Disk was never a limit: 811 G free, and the sanitizer tree
stands at 16 G.

## The rows retaken on the tree this branch carries

Both trees were rebuilt against the content this commit carries and run again on `gary`,
2026-11-05, one suite at a time.

| Tree | Preset | Suite | Report lines |
| --- | --- | --- | --- |
| `build/sanitize` | `ci-sanitize`, Address and Undefined | 100 percent of 180 passed, exit 0 | 0 |
| `build/tsan` | `ci-tsan`, Thread | 100 percent of 181 passed, exit 0 | 0 |

The report count is the grep for `ERROR: AddressSanitizer`, `runtime error`,
`LeakSanitizer` and `SUMMARY` over the Address and Undefined log, and the grep for
`ThreadSanitizer` and `LeakSanitizer` over the Thread log. Both answered 0, so neither
tree reported anything to report. Logs: `/tmp/041-notes/final-san.log` and
`/tmp/041-notes/final-tsan.log`.

The Thread tree carries one entry more than the Address tree because
`harness_topdown_run_test` builds there and reports its skip. That skip is the base's own,
and `test/tier_labels.sh` holds its row.
