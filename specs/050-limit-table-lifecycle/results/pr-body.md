# Counters: give each System its own limit table

One `System` owns one programmable-counter limit table, and no second
`System` reads it. The table ends when its `System` ends, and a read
answers the table of the `System` that owns the handle. A `System` that
a program built therefore answers its own limits and never the limits of
the process-local root. The base held one static table for the whole
process, so a `System` that scripted nothing could read the value
another `System` scripted (`results/base-state.md`, FR-006). The branch
rebases onto `02892b57`, so every reading below names the sha it
measured.

## The rule as shipped

- `System::Impl` holds the table, and registration writes it
(`source/counters/system.cpp:293`). - `Object::programmableCounters()`
reads through the owner of its node, so one handle reaches one table
(`source/counters/system.cpp:437`). - `System()` and `~System()` stand
public now, so a program can build and end a `System` of its own (D-5,
FR-008, FR-010). - `Object::parent()` and `Object::children()` follow
that same owner, so a walk stays inside the tree that owns the handle
(FR-009). - The counters passage of `docs/pages/harness.md` states the
rule, and the class text of `System` states it again. - The version
takes the next free minor, `0.33.0`. The counters header changes, so
hard rule 12 moves its archive number, and the harness header stands
still:

```text
SOVERSION 8
SOVERSION 21
```

## The acceptance readings

Every reading stands in the acceptance record, and each row below names
the sha whose code that reading measured. Each such sha names a tree
this lane measured: the squash of T025 and the amend of T027 rewrote the
branch, so the head of this request is the commit that ships, and the
record holds the command behind every number. That record is
`specs/050-limit-table-lifecycle/results/acceptance.md`, beside the five
records `results/rebase-base.md`, `results/rebase-analyzer.md`,
`results/rebase-sanitizers.md`, `results/rebase-coverage.md` and
`results/ci-base.md`.

| Reading | Head | Verdict |
| --- | --- | --- |
| `-j 20`, first base | `858c55d9` | 180 of 181 entries, 94.3 s |
| pinned, first base | `858c55d9` | 180 of 181 entries, 367.4 s |
| `-j 20`, this base | `02892b57` | 183 of 185 entries, 130.04 s |
| `-j 20`, head | `fba814d2` | 185 passed of 186 entries, 117.67 s |
| pinned, head | `fba814d2` | 185 passed of 186 entries, 367.25 s |
| five gates | `fba814d2` | each one exits 0 |
| `dbc-gate` | both shas | 348 interfaces, then 349, 0 gaps |
| checked build | both shas | 22, 19 and 5 positions, equal |
| `ci-sanitize` | `fba814d2` | 181 entries, no sanitizer report |
| `ci-tsan` | `fba814d2` | 182 entries, no race report |

`harness_hunt_cli_test` is the one failure under both head commands, and
it fails at both bases for its own two assertions. The run at this base
loses one further entry to its own layout: `views_test` walks up from
its working directory for the root `CMakeLists.txt`, and a build tree
outside the source tree holds none. That entry passes when the same
binary runs from the source tree, and `results/rebase-base.md` states
both readings.

The new entry `counters_limit_lifecycle_test` passes under both
commands. The wave readings that an earlier sha produced stand in the
acceptance record beside that sha, and the rebase section names them.

## The coverage figures

- `source/counters/system.cpp`: 281 of 281 lines, 137 of 140 branches. -
`source/counters/detail/core.hpp`: 79 of 79 lines, 31 of 34 branches. -
`include/speedgun-ng/counters_system.hpp`: 20 of 20 lines, 6 of 6
branches.

Every line this lane wrote reads covered, and six dark branch rows stand
in the first two files. The base carries the same six, so no dark row
joined either file.

```text
branch shortfall 6 rows (two files, host trace)
```

That line names its own measure: it counts the dark `BRDA:` rows of the
host trace over the two changed files that carry dark rows. The
allowance of 21 rows at `tools/dbc/coverage_gate.sh:102` is a different
measure. The in-tree gate compares it with the tree-wide count of the
union trace that the CI `coverage` job builds. On this host that gate
exits 3 before its line checks, because the host `lcov` reads three
records as inconsistent, so the CI `coverage` job carries the verdict.
The host trace of this head leaves 318 dark `BRDA:` rows tree-wide,
against 537 in the trace of the first base
(`results/rebase-coverage.md`, `results/coverage.md`).

## Two toolchain readings a reader should hold

`test/source/harness_limit_disclosure_test.cpp:131` and `:132` trip
`-Werror,-Wunused-but-set-global` under clang 23.1.4, so
`harness_limit_disclosure_test` builds on no sanitizer tree of this host
and CTest reports it as `Not Run`. That file arrives with
specs/041-counter-probe and stands byte-identical here and at this base,
and a rebuild of that one target at the base under the same compiler met
the same two diagnostics. GCC carries no such diagnostic, so the plain
tree and the coverage tree build the entry and pass it. The `sanitize`
job of run `42156116508`, the run at the base of this request,
configures `Clang 21.1.8`, and there that entry builds and passes as
test number 72. `results/ci-base.md` records that reading with its
commands. The run of this request still carries the verdict for this
branch.

specs/041-counter-probe also keeps the source of a published limit as
process state beside the tree, and its postcondition pairs that label
with what `Object::programmableCounters()` answers. This change gives
each `System` its own limits, so for a second `System` that pairing no
longer holds. specs/050-limit-table-lifecycle does not rewrite the
shipped rule of another feature, so the pairing waits for its own
ticket, and `tasks.md` carries it as a convergence task.

## The follow-on tables this lane leaves

| Table that waits | Where it stands |
| --- | --- |
| `probedKindTable()` | `source/counters/system.cpp:173` |
| the resolved metric table | `metricHostCurrent()` |
| the source of a published limit | `objectLimitSource()` |

`probedKindTable()` holds the same lifetime defect, and its writer sits
in another translation unit at
`source/counters/linux_pmu/provider.cpp:838` (D-8).
`metricHostCurrent()` caches the address of `System::local()` at
`source/counters/linux_pmu/metric_formula.cpp:1611`, so the rows
resolved against it name one `System` for the whole process.
`objectLimitSource()` reads a store that spans the process, and the
paragraph above states the pairing that waits. The request names the
limit table, so this change leaves the three alone, and `spec.md`
records the readings beside them.

## The base failures and the CI leg

CI run `41946446748` ran the first base commit `858c55d9`, and eleven
jobs pass there. Three jobs fail: `test-rocky` on `prose_gate_fixtures`,
`harness_hunt_cli_test` and `harness_hunt_topdown_test`; `sanitize` on
`harness_hunt_cli_test`; and `coverage` where `lcov` answers
inconsistent for `source/harness/hunt_topdown.cpp:155`. The `docs` job
stands skipped, and the `tsan` job passes.

CI run `42156116508` ran `02892b57`, the commit this request diffs
against, and answers the same shape. Eleven jobs pass there,
`test-rocky`, `sanitize` and `coverage` fail, and `docs` stands skipped.
There `test` passes 185 entries in 332.51 s and `tsan` passes 181 in
528.71 s. `test-rocky` fails three of 185, while `sanitize` and
`coverage` fail one of 180 each on `harness_hunt_cli_test`. This lane
registers one entry, so each of those jobs should read one entry more on
this head. `results/ci-base.md` holds the job table, the skipped entries
and the commands that read them.

`harness_hunt_cli_test` fails on this host at both bases and at the
head. It fails under both suite commands and in both sanitizer legs, on
two assertions at `test/source/harness_hunt_cli_test.cpp:3133` and
`:3682`. This change touches none of that code and fixes none of it, so
the branch carries every one of these failures forward unchanged.

The head CI verdict stands on this pull request. A branch push starts no
run, because `.github/workflows/ci.yml` triggers `push` on `master`
alone. The run that reads this head starts when the request against
`master` opens. `results/acceptance.md` records the host readings and
claims nothing about that run. No `Fixes #` line stands here, because
`gh issue list --state all` names no issue that this change closes.

Refs: specs/050-limit-table-lifecycle T-LTL Approved-by: OmO
