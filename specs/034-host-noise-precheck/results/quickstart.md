## Recorded run, 2026-10-27, the recording host (24 cores, 4 L3 groups)

| Scenario | Command | Exit | Observation |
| --- | --- | --- | --- |
| Feature suite | `ctest --test-dir build -R harness_precheck` | 0 | 1 test, ok |
| Real-host pre-check | (above) | 0 | 3 measured cpu blocks; pinning: pinning candidates: cpu 0 248.61 cpu 24 283.23 |
| Serial shape | (above) | 0 | 3 blocks, planned before candidates |
| Usage error precheck=searial | (above) | 2 | one line: `--jitter-probe: the precheck 'searial' is not a positive number` |
| Usage error precheck=0 | (above) | 2 | one line: `--jitter-probe: the precheck '0' is not a positive number` |
| Usage error precheck=4294967296 | (above) | 2 | one line: `--jitter-probe: the precheck '4294967296' is not a positive number` |
| Strict refusal | (above) | 2 | `--jitter-probe: cpu 0 is noisy: gapsPerSecond 14610718.96 maxNs 8778` |
| JSON round trip | (above) | 0 | keys: ['schemaVersion', 'speedgun', 'jitter', 'precheck', 'benchmarks'] |


## The suite under `taskset -c 0,1`

The suite ran with the process mask held to two cpus: 140 of 141
tests passed. The one failure is the `report_shape_jitter_sample`
docs gate, whose recorded run names a candidate cpu a two-cpu mask
cannot plan: the spec edge case the run itself covers, and the
sample stays the shape one full-mask host records. No timing figure
moves: the pre-check windows close through the injected reader
(FR-011), so the contamination signal never reaches the figures the
tests read, and the margin between the tests' 1 ms windows and any
real scheduling signal stands unbounded by construction.

## The CI run of the pushed head

The CI run of the pushed head
[40621765799](https://github.com/charlesarcher/speedgun-ng/actions/runs/40621765799)
closed green: `lint`, `test`, `test-rocky`, `sanitize`, `tsan`, the
three `static-analysis` jobs, `coverage`, `dbc-gate`, `prose-lint`,
`shared-audit`, `downstream-consumer`, and `consumer-release` all
passed. The coverage gate read 100.0% lines (12780 of 12780) and a
branch shortfall of 16 rows, within the D-14 recorded allowance of
19. The `docs` job skips on a pull request run.
