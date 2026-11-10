# Data Model: Raw Event-Code Support (G-31)

Entities as the feature names them (spec Key Entities), with the fields,
rules, and transitions the requirements state. Line anchors repeat the
base shapes each entity mirrors.

## E-1. Raw event-code address

A counter address whose leaf part spells the perf raw spelling.

| Part | Rule | Source |
|---|---|---|
| device part | the text before the last `/`; names the PMU device, exactly as a catalog address does | `source/harness/runner.cpp:81-89`, `source/counters/linux_pmu/group_io.cpp:167-172` |
| leaf part | a lower-case `r`, then one or more hex digits of either case | FR-001; the same shape the hunter contract pins at `source/harness/detail/hunt_selection.hpp:106-131` and `specs/029-hunter-core/contracts/hunt-cli.md:59` |
| hex part | one or more digits, of either case, is raw-shaped (FR-001); the accepted width is sixteen (D-2), and a longer run stays raw-shaped and answers the width refusal at the leaf-match stage, ahead of the device lookup and of any probe (FR-005) | FR-001, FR-005 |

Validation rules:

- `R4000`, `r`, `rgg`, and `r4g` are not raw-shaped; they follow the
  catalog-name route with the standing refusals (edge list, FR-007).
- Identity is the address string as typed (the standing leaf-model rule):
  `cpu/r0043` and `cpu/r43` are two addresses; `cpu/r43` written twice is
  one address with two columns over one shared measured leaf (FR-008,
  edge list; dedup at `include/speedgun-ng/counters_measurement.hpp:132-141`).
- The hex text becomes the 64-bit perf config word read as hexadecimal;
  nothing reinterprets or truncates it (FR-001, D-2).
- The device part must name a host-published device: `System::object`
  answers the standing `no object at path` refusal for an unknown device
  (`source/counters/system.cpp:305-312`), and the counters arm answers the
  same wording for a path whose directory holds no PMU device (research
  R-06).

Relationships: one address selects one PMU device and one config word
(one raw code, one word; the spec's config1/config2 case stays on catalog
rows); one address feeds one synthesized E-2 record; the address string is
the key of E-3, the label of the report column, and the value in all four
disclosure carriers (E-5).

## E-2. Synthesized raw entry

The provider-side entry for one raw address (spec Key Entity). It enters
no catalog listing (FR-015).

| Field | Content | Mirrors |
|---|---|---|
| name | the leaf as typed | `source/counters/detail/pmu.hpp:261` (`PmuEntry::name`) |
| words | one pair, config word 0 holding the parsed hex value | `pmu.hpp:266` (`words`), placed by `group_io.cpp:207-219` |
| availability | the E-4 verdict of the probe | `pmu.hpp:268` |
| mode | the device read mode the seeded records state | `provider.cpp:291`, `pmu.hpp:742` (research R-07) |
| probed kinds | the kinds the probe settled, mask form | `pmu.hpp:270-275` |
| ratio pair / provider index | the device settings from the records seeded under that node | `source/counters/detail/core.hpp:30-39`, seeded at `source/counters/system.cpp:63-78` |
| exclusion, group-read, time-pair attr bits | not stored; `fillAttr` sets them for every leaf alike | `group_io.cpp:220-224` |

Invariant: an entry exists only after its probe settled a state, and its
`words`, availability, and kinds come from that one probe (research R-05).

## E-3. Raw-entry table

One table beside the frozen tree (FR-015, D-5).

- Key: the canonical address `<device>/<leaf>` the resolved leaf carries
  (`include/speedgun-ng/counters_measurement.hpp:99`), the keying the
  probed-kinds records use (`source/counters/system.cpp:165-177`,
  `pmu.hpp:278-292`, writer key at `provider.cpp:838-841`). The column
  label keeps the text as typed (FR-006).
- Value: the E-2 record in `LeafRecord` shape.
- Lifecycle: written once per address by the raw probe step; read-only
  afterward. The fill-before-read rule is the one the probed-kinds comment
  states at `system.cpp:170-172`.
- Readers, and only these: the core lookup (raw arm), the plan's
  leaf-record read (`source/counters/plan.cpp:445-466`), and the provider
  window resolve (`source/counters/linux_pmu/group_io.cpp:178-187`).
- Never-read surfaces: `PmuProvider::enumerate` and every `CatalogSeed`
  stream (`provider.cpp:814-842`), the near-miss candidate loops
  (`system.cpp:306-312`, `system.cpp:582-585`), the `--catalog` print
  (`source/harness/catalog.cpp:43-60`), and the `--list` family. The tree
  gains no leaf: the seeded leaf count answers unchanged after any raw
  resolve (FR-015).

## E-4. Probe verdict (by reference)

The standing outcome, unchanged by this lane. Values are the closed
`Availability` enumeration (`include/speedgun-ng/counters_core.hpp:87-95`);
the printed state words are the harness table
(`source/harness/report.cpp:32-39`).

| Situation | Verdict | Producer |
|---|---|---|
| scripted or real open grants | `COUNTABLE` | `source/counters/linux_pmu/provider.cpp:704-708` |
| `EINVAL`, `EOPNOTSUPP`, `ENOENT` | `NOT_ENCODABLE` | the one errno table, `provider.cpp:722-729` |
| every other errno | `PERMISSION_BLOCKED` | `provider.cpp:727-728` |
| permission refusal on a cpumask-scoped device, no kind settled | `SCOPE_REFUSED` | `scopeSettledState`, `provider.cpp:537-557` |

Transition for a raw address: unresolved (no table row) → probed (per-kind
probe on the `probeDevice` pattern, `provider.cpp:258-285`) → settled
(errno table, then the scope relabel where it applies). A settled state
prints the standing `listed as <state> (FR-023)` line
(`source/harness/runner.cpp:1012-1013`) at status 2
(`runner.cpp:1027`); the run ends before measurement (edge list). A
refused state carries no target kind, the standing rule the extracted
decision states (`provider.cpp:516-535`, used at `provider.cpp:269`, and
`plan.cpp:668-692`).

No new state, no new errno mapping, and no host-side check joins the path
(D-3).

## E-5. Disclosure carriers

The address string appears in four places with no new key (FR-006, D-4):

1. the option-disclosure line of a run whose option came from a file or
   the environment (`source/harness/cli.cpp:106-128`; JSON twin at
   `source/harness/json_report.cpp:1118-1121`),
2. the console column label (`source/harness/report.cpp:727-736` over
   `runner.cpp:1437-1443`),
3. the benchmark `metricLabels` list (`runner.cpp:1586-1597`;
   `json_report.cpp:873-880`),
4. each row's `metrics[].label` (`json_report.cpp:532-538`; schema
   `docs/schema/report-v1.json:689-696`).

The schema document is unchanged at minor 1.16 (`json_report.cpp:58`), and
`counterLabels` keeps carrying only `State::setCounter` names
(`json_report.cpp:881-888`), untouched by this feature.

## E-6. Option carriers

- `CounterAddress {origin, address}` rows stay the shape at
  `source/harness/detail/internal.hpp:640` and
  `source/harness/detail/options.hpp:250`; the gather adds no shape check
  (`source/harness/options.cpp:1441-1462`, `:1631-1653`).
- The `counter` option row text changes once (FR-010): option table
  `options.cpp:560-565`, guide row `docs/pages/harness.md:675`, README row
  `README.md:970`.

## Validation summary (one line per refusal)

| Input | Line | Status |
|---|---|---|
| `nosuch/r4000` | `<origin>: nosuch/r4000: no object at path 'nosuch'`, the three-field print at `runner.cpp:1022-1026` with the wording from `system.cpp:311`, pinned in that form at `test/source/harness_cli_test.cpp:731` | 2 |
| `cpu/r<17 digits>` | `<origin>: <address>: raw event code exceeds 16 hex digits` (FR-005 text) | 2 |
| probe refusal (encoding) | `... listed as not-encodable (FR-023)` | 2 |
| probe refusal (permission) | `... listed as permission-blocked (FR-023)` | 2 |
| scoped device permission refusal | `... listed as scope-refused (FR-023)` | 2 |
| `--hunt=cpu/r4000` | the pinned hunter line, byte-identical (`hunt-cli.md:83`) | unchanged |
