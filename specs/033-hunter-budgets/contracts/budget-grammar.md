# Contract: the `--hunt-budget=` clause grammar (033)

Extends the standing row of `source/harness/hunt_cli.cpp`
(`parseHuntBudget`, FR-004, R-01). The row stays one; no short
letter; the hunt family, command-line only, carries one whole value.

## Value grammar

```text
--hunt-budget=<clause>[,<clause>...]   each kind at most once, any order
<clause>  := <count>s | <count>p | <seconds>e | <count>g
<count>   := whole number, digits alone, above 0 (s, p: above 0; g: 1 to 10000)
<seconds> := finite decimal seconds above zero, fraction allowed, at most
             kMaxWholeSeconds whole seconds, nothing below one nanosecond
             (the parseSeconds rule hoisted into detail/options.hpp and
             shared by both parsers)
```

| Clause | Meaning | Unit kept | Standing rule |
| --- | --- | --- | --- |
| `s` | two-stage: whole hunt, seconds | integer count | unchanged (P-1) |
| `p` | two-stage: whole hunt, passes | integer count | unchanged (P-1) |
| `e` | sample budget: one event, one stage | whole nanoseconds | new (FR-001) |
| `g` | multiplex cap: events per group | whole count 1 to 10000 | new (FR-003) |

A value with one `s` or `p` clause parses exactly as spec 029 shipped
it. Examples: `120s`, `8p` (standing); `0.5e`; `32g`;
`120s,0.5e,32g`; `32g,120s` (order stands free).

## Refusals (each one line quoting the whole value, exit 2)

| Input | Line names |
| --- | --- |
| empty value | `--hunt-budget` |
| empty clause (`120s,,32g`) | the whole value |
| repeated kind (`0.5e,2e`) | the whole value |
| missing or unknown suffix (`120`, `120x`) | the whole value |
| count or seconds not a number (`xe`) | the whole value |
| `e` below one nanosecond or above the bound | the whole value |
| `g` below 1 or above 10000 | the whole value |
| leading or trailing blank | the whole value |

The refusal keeps the standing sentence family
(`--hunt-budget: expected ..., got '<value>'`).

## Derived behavior (plan-side, FR-001 to FR-005)

- Stage repeats: `min(stageRule, max(1, floor(sampleNs /
  (calibrationNsPerPass × passRuns(plan, stage)))))`, `stageRule`
  the stage's own rule count when a figure prices
  the stage.
- Estimate: re-priced from the derived repeats before the standing
  stage-drop decision.
- No figure: the stage rule repeats stand; the record says `unpriced`.
- Fit below one: one repeat; the record says the overrun.
- Group split (`MULTIPLEX` only): consecutive groups of at most `g`
  addresses in plan order; `PASSES` plans carry no groups.
