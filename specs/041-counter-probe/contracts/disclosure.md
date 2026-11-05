# Contract: the disclosure of a limit's source

**Feature**: specs/041-counter-probe | **Files**: `source/harness/runner.cpp`,
`source/harness/json_report.cpp`, `docs/schema/report-v1.json`,
`docs/pages/harness.md` | **Audit point**: `858c55d9`
**Spec**: FR-041, FR-043, FR-045, FR-050 to FR-053, D-3, D-3a

## What this contract covers

One run answers two questions about a limit: what the figure is, and where it came from.
This contract fixes the console line, the written record, the schema move, the reader's
answer for an older document, and the bytes that must not move. The figure's origin
stands in `provider-fill.md`, and the accessor a program reads stands in
`limit-source.md`.

## The bytes that stand still

The report of the audit point holds no counter limit anywhere: a read of
`docs/schema/report-v1.json` for `limit`, `programmable` or `slots` answers nothing. Two
printed texts name a limit today, and both keep every byte:

| Text | Where | Owner |
|---|---|---|
| `device '<dev>' publishes <n> slots and a hunt pass needs 3` | `source/harness/detail/hunt_selection.hpp:71-79` | `specs/029-hunter-core/contracts/hunt-cli.md` pins it |
| the pass disclosure of method, count and spread | the settled-method publish of `runner.cpp` | specs/024's contracts pin it |

`source/harness/pass_plan.cpp` prints nothing today, so no plan text carries a figure a
source word could join. FR-050 therefore opens a line of its own, and FR-051 opens a
record of its own.

## The console line

```text
counter limit: device '<device>' count <n> source probe
```

The line prints once for each needed device whose limit the search answered, within one
gathering of the limits (FR-050). One `Runner::run` call gathers once, so one instance
needing one probed device answers one line, and that is the case the unit pins. A hunt
gathers once per stage and once per pass, and a top-down companion gathers on a pass of
its own, so a hunt on a probed host prints the line again in each gathering. A run whose
devices all came from the decode prints nothing new, and a run with no limit prints
nothing new. The seat is the gathering loop of the limits (`runner.cpp:1578-1634`), which
already reads `Object::programmableCounters()` per source and already owns the one
publish point every later stop passes through.

```text
\pre a needed device published a limit
\post a run with a probed device prints one line per needed device the search answered
     in that gathering, and a run without one prints no new line (FR-050)
\post every line the same run printed at the audit point keeps its bytes (FR-043)
```

## The written record

One new run-level record, keyed `counterLimits`, holds one entry per needed device:

```json
"counterLimits": [
  { "device": "cpu", "limit": 6, "source": "probe" },
  { "device": "cpu_atom", "limit": null, "source": "none" }
]
```

- The key is absent for a run with no needed device, and the record is not `required`.
- `limit` is the published figure or `null`; `source` is `decode`, `probe` or `none`, the
  three words of `LimitSource` in `limit-source.md`.
- The record sits with the run-level records the writer already seats after `speedgun`
  and before `benchmarks` (`json_report.cpp:1154-1175` shows the `jitter` seat), and the
  order joins `x-order` in that place.
- `kSchemaVersion` moves from `1.14` to `1.15` (`json_report.cpp:49`), the schema gains
  the record in `properties`, and `docs/pages/harness.md:1395-1404` gains the key rows.

```text
\post a written report of a run with a limit names each needed device once (FR-051)
\post a document older than 1.15 loads and answers no counterLimits entry (FR-052)
\post the read-back answer stays inside the harness, so no exported layout of
     include/speedgun-ng/benchmark.hpp moves and the harness `SOVERSION` stays 21
     (FR-052, D-3)
```

## The plan and the hunt, unchanged

A probed figure reaches `settleMethod` and `planPasses` as the same `std::optional`
value a decoded figure reaches (FR-045). The three routes read the value, never its
source: the method settling, the planner, and the hunt refusal. A probed count of 2
refuses a hunt pass with the pinned line above, byte for byte, and `lastRefusalMessage()`
answers it (FR-045).

## Verification

| Claim | Machine |
|---|---|
| the line prints once per probed device, and no base line moves (`test/CMakeLists.txt:851-856` registers the unit) | `ctest --test-dir build -R harness_limit_disclosure_test --output-on-failure` |
| one plan from the same count by either route | `ctest --test-dir build -R harness_pass_plan_test --output-on-failure` |
| the record writes, loads and answers 1.15 and 1.14 shapes | the three rows `harness_json_report_test`, `harness_json_schema_test`, `harness_json_load_test` |
| the pinned refusal bytes stand | `specs/029-hunter-core/contracts/hunt-cli.md` scenarios in the same run |
| `SOVERSION` 21 holds and 6 moves to 7 | the `soVersionNumbers` test and `results/acceptance.md` |

## Amendment, round 2 (2026-11-05)

SC-006's 100 percent claim belongs to the written record. The `counterLimits` entry of
FR-051 lists every needed device with its source word, so a decoded limit names `decode`
there, and the console keeps one line for a probed limit alone. A decoded run prints no
new byte, so no golden, no pinned refusal text and no pass report moves.

`harness_limit_disclosure_test` registers a scripted device tree under the real
provider, reads the figure that provider published beside the finder the catalog holds,
and names the finder of its scenario through the internal `recordLimitSource` only where
a figure stands. A scripted device publishes a `type` number of the fixture's own, so a
real `perf_event_open` against it lands on whatever PMU the host owns at that number,
and no scripted device tree can make the search answer a chosen figure at the harness
layer. `counters_limit_fill_test` and `counters_group_probe_test` drive the search
through the kernel seam, and `counters_limit_source_test` covers the seat table. No
member joins `FakeProvider`, because this lane's archive number rises for one exported
accessor alone (FR-053, D-3a).
