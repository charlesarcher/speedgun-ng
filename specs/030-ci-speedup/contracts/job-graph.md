# Contract: CI Job Graph

**Surface**: `.github/workflows/ci.yml`, the job set, the `needs` edges,
the `concurrency` key, the step names, and the timing each step must
print. **Closes**: FR-014, FR-015, FR-016, FR-035 to FR-039, FR-045 to
FR-047, FR-063, FR-064.

## Job set

| Job | Trigger | `needs` | Runs on | Change |
| --- | --- | --- | --- | --- |
| `lint` | push, pull_request | none | `ubuntu-26.04` | unchanged, and green through the formatting-only commit (FR-065) |
| `coverage` | push, pull_request | `lint` | `ubuntu-26.04` | `ctest` goes to `-j 1` first (P-11) |
| `sanitize` | push, pull_request | `lint` | `ubuntu-26.04` | full-affinity parallelism, no matrix entries (D-5) |
| `tsan` | push, pull_request | `lint` | `ubuntu-26.04` | registers the five matrix entries |
| `test` | push, pull_request | `lint` | `ubuntu-26.04` | configures `ci-ubuntu-test`, keeps every step apart from the analyzer build |
| `test-rocky` | push, pull_request | `lint` | rocky container | registers the five matrix entries |
| `shared-audit` | push, pull_request | `lint` | `ubuntu-26.04` | unchanged job, cpu-count parallelism |
| `downstream-consumer` | push, pull_request | `lint` | `ubuntu-26.04` | unchanged job, cpu-count parallelism |
| `consumer-release` | push, pull_request | `lint` | `ubuntu-26.04` | unchanged job, cpu-count parallelism |
| `dbc-gate` | push, pull_request | `lint` | `ubuntu-26.04` | unchanged job, configures `ci-ubuntu` |
| `prose-lint` | push, pull_request | `lint` | `ubuntu-26.04` | unchanged job, configures `ci-ubuntu` |
| `docs` | push to master | `sanitize`, `test`, `test-rocky`, `static-analysis` | `ubuntu-26.04` | gains `static-analysis` at `:759` (FR-038) |
| `static-analysis` | push, pull_request | `lint` | `ubuntu-26.04`, three shards | new (FR-015, FR-018) |

No job of master disappears (FR-045). No test set shrinks apart from the
matrix entries that move out of `coverage` and `sanitize` (FR-046, D-5).

## `concurrency`

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: ${{ github.event_name == 'pull_request' }}
```

A second push to a pull-request ref cancels the older run. A master run is
never canceled, so a recorded master measurement cannot vanish (FR-039,
story 8).

## Analyzer job

Three shards build the three scopes P-08 names. The contract is the
finding list, the unit count and the verdict:

| Shard | Build scope | Must |
| --- | --- | --- |
| `library` | `speedgun-ng_speedgun-ng`, `speedgun-ng_harness` | analyze every library translation unit the default build compiles |
| `tests` | `cmake --build build/test` | analyze all 90 test units of `test/CMakeLists.txt` |
| `examples` | `cmake --build build/example` | analyze the two example units |

A finding of either analyzer fails its shard (FR-016), and
`readability-identifier-naming` stays the failing check (`.clang-tidy:21`,
FR-016). Each shard prints the count of units it compiled and uploads its
sorted `file:line:check` list (FR-018, FR-054). No analyzer result is
cached (FR-017).

## Timing each job must print

| Step | Printed reading | Read by |
| --- | --- | --- |
| every install step | its duration from the job record | FR-037, SC-002 |
| every `Build` step | its duration, plus `ccache -z` before and `ccache -s` after | FR-009, FR-051 |
| every `Test` step | its duration, plus the per-entry `ctest` times in the log | FR-051, SC-008 |
| `coverage` test step | its duration beside the base 334 s | P-11, FR-044 |

## Failure reading

After each push every job log is read, and this change's own failures are
fixed (FR-063). A red the base already carries is named in the pull
request with the base record and the owning spec (FR-064): at `f507b8f`
that is `lint` before FR-065 lands, `coverage` on a shortfall of 99.4%
lines and 97.5% branches, and `prose-lint` on one XI.2 finding (PC-10).

## Verify

```sh
gh api repos/charlesarcher/speedgun-ng/actions/runs/<id>/jobs \
  --jq '.jobs[] | [.name, .status, .conclusion,
        ((.completed_at | fromdate) - (.started_at | fromdate))] | @tsv'
gh run view <id> --log-failed
```

The job-name set of the branch equals the thirteen rows above, each
`needs` edge matches the table, and each install, build and test step
carries a duration.
