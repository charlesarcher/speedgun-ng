# The CI leg at the pull-request base

Run `588`, id `42156116508`, is the run that `master` at `02892b57` started at
2026-11-06T03:22:08Z. That commit is the base this branch diffs against, so its
verdicts are the numbers `T028` reads the head run against. Every line below
came from `gh api` and `gh run view` on the lane host, and the commands stand at
the foot of this record.

## The job verdicts

| Job | Verdict |
| --- | --- |
| `lint` | success |
| `prose-lint` | success |
| `dbc-gate` | success |
| `static-analysis (library)` | success |
| `static-analysis (examples)` | success |
| `static-analysis (tests)` | success |
| `downstream-consumer` | success |
| `shared-audit` | success |
| `consumer-release` | success |
| `test` | success |
| `tsan` | success |
| `test-rocky` | failure |
| `sanitize` | failure |
| `coverage` | failure |
| `docs` | skipped |

Eleven jobs pass, three fail, and one stands skipped. The same shape stands at
the first base `858c55d9` in run `41946446748`, so no commit between the two
bases changed that pattern.

## The entries each job ran

| Job | Entries | Result of the `Test` step |
| --- | --- | --- |
| `test` | 185 | 100 percent passed in 332.51 s |
| `tsan` | 181 | 100 percent passed in 528.71 s |
| `test-rocky` | 185 | 3 failed |
| `sanitize` | 180 | 1 failed |
| `coverage` | 180 | 1 failed |

`test-rocky` fails on `prose_gate_fixtures` at number 30, on
`harness_hunt_cli_test` at number 149, and on `harness_hunt_topdown_test` at
number 185. `sanitize` and `coverage` fail on `harness_hunt_cli_test` at number
144. The `coverage` job fails once more in its `Retain the coverage trace` step,
where `lcov` answers inconsistent for `source/harness/hunt_topdown.cpp:155`.

`test-rocky` reports `counters_pmu_test` (47), `counters_overhead` (62) and
`harness_topdown_run_test` (183) as entries that did not run. `sanitize` and
`coverage` report the same three at 42, 57 and 178. Those three need a kernel
counter, a privilege, or a top-down host, and the runner offers none of them.

## The toolchain of the sanitizer leg

The `sanitize` job sets `CXX: clang++` at `.github/workflows/ci.yml:153`, and
its Configure step answers `The CXX compiler identification is Clang 21.1.8`.
This host carries clang 23.1.4. That gap explains the one build difference this
lane met. `harness_limit_disclosure_test` builds and passes in that job, at
`Test #72` with `Passed 0.35 sec`, while clang 23.1.4 stops the same source with
`-Werror,-Wunused-but-set-global` at
`test/source/harness_limit_disclosure_test.cpp:131` and `:132`. `T030` asks for
this reading at the run of this branch, so it stays open until that run stands
read, and this record already answers it for `master`.

## What the head of this branch should answer

This lane registers one entry, `counters_limit_lifecycle_test`, so each job
above should read one entry more than the same job here: `test` 186, `tsan` 182,
`test-rocky` 186, `sanitize` 181, and `coverage` 181. No entry that passed here
may fail there. `harness_hunt_cli_test` should fail again on the same two
assertions, which this lane reads at
`test/source/harness_hunt_cli_test.cpp:3133` and `:3682`.

## The commands behind these lines

```text
gh api 'repos/<owner>/speedgun-ng/actions/runs?head_branch=master&per_page=3'
gh api repos/<owner>/speedgun-ng/actions/runs/42156116508/jobs
gh run view --log-failed --job 127690738360 --repo <owner>/speedgun-ng
gh api --allow-escape-sequences repos/<owner>/speedgun-ng/actions/jobs/<id>/logs
```

The three failed jobs read `127690738360` for `sanitize`, `127690738365` for
`test-rocky`, and `127690738499` for `coverage`. `gh api` refuses a log that
carries terminal escapes until the reader passes `--allow-escape-sequences`.