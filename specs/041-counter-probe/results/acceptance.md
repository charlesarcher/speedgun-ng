# The acceptance record of the counter-count group probe

**Pull request**: #82 | **Branch**: `041-counter-probe` | **Base**: `origin/master` 858c55d9 | **Head before the squash**: `115157ef` (the 19 commits whose tree this commit preserves)
**Host**: gary | **CPU**: AMD Ryzen 9 9950X3D, family 26 model 68 | **Kernel**: 7.3.1-1-cachyos
**Compiler**: c++ (GCC) 16.2.1 20260810 | **lcov**: 2.3.1 | **Date**: 2026-11-05
Paths are build-relative or `$HOME`-relative. Every row names a figure or names itself pending.

## The success criteria

| Row | Reading | Where |
|---|---|---|
| SC-001 a scripted device that runs 6 copies answers 6 | `counters_group_probe_test`, 9 scenarios, green | `results/red-readings.md` |
| SC-002 a decoded device pays 0 probe opens | `counters_limit_fill_test`, the decode rows count opens | same |
| SC-003 the decoded figure and a searched figure for `cpu` are equal | both 6: `counter limit: cpu limit=6 source=decode probe=6`, and the shipped attribute reads 6 | `results/requirement-1-shipped-shape.txt`, `example/docs/counter_probe.expected` |
| SC-004 every refusal shape answers no limit | `counters_group_probe_test` (`EACCES`, size-1 short run, short read, cap-wide fit) and `counters_limit_fill_test` through the fill | same |
| SC-005 one search per device across two plans | `counters_limit_fill_test`, the settled answer with the open count at 0 | same |
| SC-006 every published limit names its source in the record | `harness_limit_disclosure_test` plus `harness_json_report_test` and `harness_json_load_test`; the console line stays probe-only | `results/red-readings.md` |
| SC-007 the example builds, runs, and refuses politely | `docs_counter_probe` exits 0 and prints `probe=no limit` where the open is refused | `example/docs/counter_probe.expected` |
| SC-008 lines at 100 percent, branch shortfall stated, markers at most 84 | lines 100.0 percent (18019 of 18019, 0 dark); branches 9395 of 9448, so 53 dark against the allowance of 21, this host's reading where the base reads 53; markers 84 | `results/coverage-head.txt` |
| SC-009 the search costs 28 opens inside the stated band | 28 opens, 7 reads, 28 closes, 0.080 ms with a spread of 0.068 to 0.095 ms | `results/requirement-1-shipped-shape.txt` |
| SC-010 the four rows with no host stand pending | Intel non-hybrid check, `armv8_*`, Intel hybrid with leaf 0x23, Zen 4: pending, seam rows only | `specs/041-counter-probe/spec.md` |

## The suite rows

Both rows read 100 percent of 185 passed, 0 failed, 0 `Not Run`, 1 skipped
(`harness_topdown_run_test`, the skip it carried at the base):

```text
ctest --test-dir build --output-on-failure
taskset -c 0,1 ctest --test-dir build --output-on-failure
```

## The sanitizer rows

Address and Undefined: 100 percent of 180 passed, 0 sanitizer report lines. Thread
sanitizer: 100 percent of 181 passed, 0 report lines. Three earlier readings failed on a
host carrying two suites at once, on entries this branch never touches, and each named
entry passes alone; `results/sanitizers.md` names all three and their solo re-runs,
because a suite that failed carries no verdict.

## The instrumented row

Lines read 100.0 percent, 18019 of 18019 on the content this commit carries, one
executable line fewer than the 18020 of the earlier read, and the count is complete either
way. Branches read 9395 of 9448: 53 dark, which is
the base host figure, so this lane adds no dark branch and closes the two it opened at
`group_probe.cpp:124` and `limit_fill.cpp:66` along with the 8 dark reader lines it
opened in `json_load.cpp`. Markers stand at 84, the FR-074 figure, and no lane file holds
one. `results/coverage-head.txt` carries the capture and filter route (the project's own
two cache commands), the per-file dark rows beside the base figures, the gate's exit 3 as
issue #76 records it, and the two discarded readings with their reasons.

## The analyzer row

`cmake --preset=ci-ubuntu` over a forced rebuild from clean: the one error-class finding
was a naming defect in this lane's own scenario name, which broke rule N-2 and stopped the
build, and the pass after the rename reports none anywhere in the tree. Both cppcheck
error-severity findings reproduce on the base file with the same command. `results/analyzer.md`
carries the attempt that recompiled nothing, the pass that stopped, the base readings beside
both cppcheck findings, and the warning background sized.

## The two facts a reader should not have to dig for

The harness unit that scripts a probed run reads the figure its own host published
beside the finder the catalog holds, and names the finder of its scenario through
the internal `recordLimitSource` only where a figure stands, because the accessor
answers `NONE` exactly where no figure stands. Where the host publishes no figure,
the scenario seats nothing and its assertion is that pairing rule. The harness layer
cannot earn the pair for real, because a scripted device publishes a `type` number
of the fixture's own and a real `perf_event_open` against it lands on whatever PMU
the host owns at that number. The search itself runs behind the scripted kernel in
`counters_group_probe_test` and in `counters_limit_fill_test`. The fill's own probed
arm is proven in `counters_limit_fill_test`, which takes the decoded figure as an
argument, so the arm reaches a case on any host. The seat table the report reads is
process-wide, so a scripted run's device stands for the rest of that binary, and the
unit's seating check says so.
