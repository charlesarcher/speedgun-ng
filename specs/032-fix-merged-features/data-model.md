# Data Model: Fix Merged Features

**Branch**: `032-fix-merged-features` | **Date**: 2026-10-27

Phase 1 output. The feature adds no entity, no field and no
relationship of its own: it closes defects in the entities the three
merged features landed. This file records the touched entities, the
validation rules the fixes keep or introduce, and the one state
transition each fix owns.

## Entities touched

### `SkippedKind` (`include/speedgun-ng/hunt.hpp:158`)

- **Kind**: public enum class, `std::uint8_t`, the harness ABI.
- **Values**: `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`,
  `SCOPE_REFUSED`, `GAP`.
- **Change**: a new value joins only when an item needs one (D-09,
  FR-036); the edit moves the project version to `0.21.0` and the
  harness `SOVERSION` to `13` (FR-001, FR-002), and the ABI check is
  recorded in that commit.
- **Validation**: the console prints every kind with the spelling
  `report.cpp availabilityName` prints; a new value joins that table in
  the same commit.

### The option table (`source/harness/detail/options.hpp`)

- **Kind**: the one option row table with the value lanes.
- **Change**: no row is added and no value moves (FR-003); the lanes
  hold: `260` to `269` the jitter-probe family, `270` to `279` the
  hunter family, `290` to `319` the command-line family.
- **Validation**: the standing uniqueness test over the option values
  keeps running; the environment-name derivation at
  `options.hpp:109` to `:115` matches no row named `json` (SC-007).

### The probe value precedence

- **Kind**: a rule over three sources of one `--jitter-probe` value:
  the command line, the option file, the family call.
- **State transition**: the settlement order changes. Before: an
  option-file value overrides a family's own probe. After: the
  family's own probe wins over both other sources; an option-file
  value stands as a command-line value would; when the command line
  and the option file both carry a value, the command-line value
  stands (D-01, FR-028).
- **Validation**: one test per source (FR-028); the duplicate-cpu
  refusal stands for the list shape `0,0` and the range shape `0-1,1`,
  one line quoting the value, exit 2 (D-04, FR-029).

### The online cpu set seam (`source/harness/topology.cpp:327`)

- **Kind**: an internal seam: `installTestOnlineCpus`,
  `onlineCpuSet()`, read by `execution_context.cpp:655` and
  `jitter_probe.cpp:711`.
- **State transition**: the unreadable-line fallback at
  `topology.cpp:358` to `:365` changes from filling the set by index
  from `onlineCpuCount()` to answering the process affinity mask
  (D-02, FR-021). A count is forbidden as the fallback source.
- **Validation**: the tests install a set with a gap, for example
  `0-3` and `8-11`, and check both bounds against the gap (FR-014,
  FR-021). A placement cpu is a membership check in the set (D-03).
  The comments that describe the count fallback move to the mask
  wording in the same commit (research R-07).

### The compare table row (`source/harness/compare.cpp:1906`)

- **Kind**: the compare output table cell format.
- **State transition**: the name cell and the rule-text cell gain one
  quoting call with `detail::quotedText` at the print site; the reason
  cell stays unquoted (FR-030).
- **Validation**: a baseline name with a quote character prints the
  cell quoted once; the reason cell is unchanged (User Story 4).

### The report document (`docs/schema/report-v1.json`)

- **Kind**: the JSON report schema, version `1.5`.
- **State transition**: the two top-level `x-order` keys at `:7` and
  `:8` become one, `["schemaVersion", "speedgun", "run", "jitter",
  "benchmarks", "hunt"]` (FR-017); the writer order is checked against
  it; a test fails when a document carries a duplicate key.
- **Validation**: the schema test of the standing `report_shape`
  family; the mutation anchor reads the version once at `1.5` (FR-006).

### The hunt option surface

- **Kind**: the `--hunt`, `--hunt-noise`, `--counter`, `--metric`
  parse and refusal rules of spec 029.
- **State transitions** the fixes keep or introduce:
  - an ambient `SPEEDGUN_COUNTER`, `SPEEDGUN_METRIC` or
    `SPEEDGUN_PLACEMENT` value stops a hunt with exit 2 and one quoted
    line naming the source, from every source (D-07, FR-034, FR-045);
  - an unresolvable `--counter` address exits 2 from every source,
    including a valid PMU path this host lacks (D-05, FR-032);
  - an uncountable leaf exits 2 (D-06, FR-033);
  - a refused combination is refused from every source (FR-045);
  - `--hunt` with `--jitter-probe` runs and reports both records
    (D-08, FR-035);
  - the hunt's first pass carries no retry (D-10, FR-037).
- **Validation**: one test per rule and per source, written first
  (FR-055); no sleep, no wall-clock read, no made-up cpu (Constraints).

### The coverage keep-set (`cmake/coverage.cmake:92` to `:98`,
`tools/dbc/coverage_gate.sh:60`)

- **Kind**: the gate configuration: five `--omit-lines` patterns and a
  branch allowance of 19.
- **State transition**: the set stands until the approval D-12 names;
  after FR-021 lands the pull-request comment lists each pattern and
  row that no longer matches code, with the trace and the zero list as
  evidence (FR-026). The exception-cleanup sides D-13 accepts carry
  their `gcov` block and arc data in the spec (FR-031), and the one
  gate change such a proof buys is the narrowest `lcov` option that
  removes exactly the proven sides, in `cmake/coverage.cmake` alone.
- **Validation**: the gate reads 100.0 percent lines and holds the
  branch shortfall to the accepted sides and the keep-set (SC-002).

## Relationships

- The seam serves both readers: the execution-context bound and the
  option-value parse bound read one `onlineCpuSet()`; the fix changes
  the seam, not the readers (D-02, D-03).
- The keep-set protects the seam's fallback patterns; the fallback fix
  is what lets FR-026 ask for their removal (research R-03).
- The schema key order, the writer order and the drift test stand in
  one chain: the single `x-order` key names the writer's order, and
  the duplicate-key test fails a hand-edited document (FR-017).
- The version lane ties together: the project version, the harness
  `SOVERSION`, the archive `SOVERSION` and the docs sample versions
  agree in each file the merge touched (FR-004).

## Out of the model

- The host-validation feature (spec 031): the hunter acceptance rows
  move there (FR-041), no entity of this feature belongs to it.
- The landed history of PR 46, PR 47 and PR 49: accepted as it is
  (D-15), no entity is reshaped to match it.
