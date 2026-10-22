# Contract: the hunt command line

**Feature**: specs/029-hunter-core | **Surface owner**: `source/harness/hunt_cli.cpp`
with the table rows of `source/harness/cli.cpp` | **Audit point**: `bd10d76`
**Spec**: FR-030 to FR-033, FR-080 to FR-082, D-1, D-9, D-11, D-12, D-21, D-22,
hard rules 2, 3, 9 and 10

## What this contract covers

Eight long options, no short letters, values 270 to 279. `cli.cpp` gains the
eight rows of the one `option longOptions[]` table at `cli.cpp:346` and eight
named constants; every parse, range check, refusal message, `@file` reader, glob
matcher and conflict check lives in `hunt_cli.cpp` (D-22). The table stays the
single source `tools/docs/example_drift.py:458-465` reads for the README option
rows, so a hunt option with no README row fails `example_drift --options`.

Values 256 to 259 are taken at `bd10d76` (`cli.cpp:69-71` for the three
comparison options, `cli.cpp:345` for `kMetricMethodOption`), so 270 to 279 stand
free (spec PC-5).

## The option table

| Option | `getopt_long` row | Value | Default | Accepted | Refusal |
|---|---|---|---|---|---|
| `--hunt` | `{"hunt", optional_argument, nullptr, kHuntOption}` | 270 | off | no value or `=all`, a comma list of catalog addresses, one glob pattern, or `@path` | unknown address, pattern that matches nothing, empty address line, `=none`, raw code |
| `--hunt-norm` | `required_argument` | 271 | `instr` | `instr`, `cycle`, `iter` | any other word |
| `--hunt-rank` | `required_argument` | 272 | `rate` | `rate`, `cv` | any other word, `sensitivity`, `delta`, `score` included (D-14) |
| `--hunt-top` | `required_argument` | 273 | `20` | integer 1 to 10000 | 0, 10001, non-integer |
| `--hunt-noise` | `required_argument` | 274 | `10` | percent CV threshold above `0.0` | `0`, a negative value, non-numeric |
| `--hunt-repeats` | `required_argument` | 275 | `10` | integer 1 to 1000 | 0, 1001, non-integer |
| `--hunt-budget` | `required_argument` | 276 | none | `<n>s` seconds or `<n>p` passes, `<n>` above 0 | no suffix, `0s`, `0p`, negative, non-numeric |
| `--hunt-seed` | `required_argument` | 277 | `0` | any unsigned 64-bit value | non-numeric, a value a `std::uint64_t` cannot hold |

Values 278 and 279 stay unclaimed for the later options of this feature. Every
constant is named in `cli.cpp` beside the comparison constants, in the form the
comment at `cli.cpp:66-68` states: a long option with no letter of its own takes
a number of 256 or more, so it cannot collide with a short option.

`--hunt` is the tree's first `optional_argument` row. `getopt_long` reads an
optional argument only in the attached form, so `--hunt=all` selects all events
and `--hunt all` selects nothing and leaves `all` as a positional argument; the
contract states the attached form as the spelling, and the docs example uses it.

## Selection forms

| Form | Reading |
|---|---|
| `--hunt` or `--hunt=all` | every entry of every core device in the catalog (D-1, D-8) |
| `--hunt=a/b,a/d` | a comma-separated list of catalog addresses, in the order of the text, reaching any device, uncore included (FR-005) |
| `--hunt=uncore/*` | one glob pattern over the catalog addresses; `*` matches any run of characters including none, `?` matches one character, and the match is case-sensitive |
| `--hunt=@events.txt` | the file's catalog addresses, newline or comma separated; blank lines and surrounding whitespace are ignored (FR-006) |

A raw event code of the shape `r[0-9a-fA-F]+` is refused wherever it appears,
including inside a list, inside a pattern and on a line of a `@file`. The shape
check runs before the catalog is consulted, so `r4a03` is refused on every host
including one whose kernel would encode it (D-11; spec PC-1 measured that no
catalog entry carries an address-shaped `event` field to resolve it with).

Expansion runs after the option scan and before the first run, where
`speedgunMain` has registered the clock and PMU providers
(`cli.cpp:326-339`). The shape rules (raw code, malformed pattern, empty
address line) are checked first, so the same text gives the same refusal on a CI
host without PMU access and on a host that counts everything.

## Refusals

Every refusal prints exactly one line naming the value to stderr and exits with
status 2 (FR-081). The message form follows the percentile list refusal at
`cli.cpp:73-81`, which prints the option and the text it was given.

| Input | Line |
|---|---|
| `--hunt=none` | `--hunt: expected all, a catalog address list, a pattern or @file, got 'none'` |
| `--hunt=cpu/nope` | `--hunt: 'cpu/nope' is not a catalog address` |
| `--hunt=uncore*` | `--hunt: 'uncore*' is not a well-formed pattern` |
| `--hunt=no/such*` | `--hunt: pattern 'no/such*' matches no catalog address` |
| `--hunt=@missing.txt` | `--hunt: '@missing.txt' cannot be opened` |
| `--hunt=@blank.txt` | `--hunt: '@blank.txt' holds no catalog address` |
| `--hunt=r4a03` or `--hunt=cpu/r4a03` | `--hunt: 'r4a03' is a raw event code; --hunt takes catalog addresses` |
| `--hunt-norm=ipc` | `--hunt-norm: expected instr, cycle or iter, got 'ipc'` |
| `--hunt-rank=sensitivity` | `--hunt-rank: expected rate or cv, got 'sensitivity'` |
| `--hunt-top=0` | `--hunt-top: expected 1 to 10000, got '0'` |
| `--hunt-top=10001` | `--hunt-top: expected 1 to 10000, got '10001'` |
| `--hunt-noise=0` | `--hunt-noise: expected a percentage above 0, got '0'` |
| `--hunt-repeats=1001` | `--hunt-repeats: expected 1 to 1000, got '1001'` |
| `--hunt-budget=120` | `--hunt-budget: expected <n>s or <n>p, got '120'` |
| `--hunt-budget=0p` | `--hunt-budget: expected a count above 0, got '0p'` |
| `--hunt-seed=abc` | `--hunt-seed: expected a whole number, got 'abc'` |
| `--hunt` with `--counter=cpu/instructions` | `--hunt cannot combine with --counter` |
| `--hunt` with `--metric=ipc` | `--hunt cannot combine with --metric` |
| `--hunt` with `--compare=a.json,b.json` | `--hunt cannot combine with --compare` |

A hunt option given twice keeps the last value, the way every other option in
the table behaves. A hunt option with no `--hunt` is a refusal naming the option
it needs: `--hunt-top: needs --hunt`.

`--hunt` combined with `--list`, `--catalog` or `--topology` is not a refusal:
those modes print and stop before any run, so the hunt plan never forms and no
`hunt:` line prints. `--hunt` with `--filter` applies the hunt to the instances
the filter selected. `--hunt-repeats` sets the repeats of the hunt stages, and
`--repetitions` keeps its present meaning for the benchmark's own repetitions,
which are the rows the report prints.

## Conflict check

`hunt_cli.cpp` owns `validateHuntCombination(const RunOptions&)`, which `cli.cpp`
calls once after the scan. It reads the settled members of
`detail/internal.hpp:434-476`: `counterAddresses` (line 444), `metricNames`
(line 455) and `compareBaseline` (line 474). The check runs
after the scan because a `--counter` that stands after `--hunt` on the command
line is the same conflict as one that stands before it.

## Console output

All hunt lines go to stdout behind the `hunt:` prefix; refusals go to stderr
(D-20). The order is fixed.

Before the run, once per hunt run:

```text
hunt: plan events=42 passes=14 method=PASSES estimate=1248000ns budget=120s
hunt: plan budget cut stage=2 reason=fits-budget
```

After the benchmark tables of the run:

```text
hunt: rank 1 cpu/instructions_retired_any instr=1042.300 cv=1.20
hunt: rank 2 cpu/l2_lines_out.any_core instr=6.910 cv=2.05 multiplexed
hunt: rank - cpu/mem_load_stalled_on_l2 instr=2.110 cv=24.80 mark=noisy
hunt: rank - cpu/page_walker_walk_completed instr=0.310 cv=0.00 mark=unrepeated
hunt: skipped cpu/l1_dc_misses permission-blocked stage=2
hunt: skipped armv8_cycle group-refused
hunt: plan summary events=40/42 passes=14 spend=1184000ns budget=120s
```

| Rule | Detail |
|---|---|
| Ranked table size | at most `--hunt-top` ranked rows, then the excluded tail with `rank=-` and a `mark=` of `noisy`, `unrepeated` or `beyond-top` (D-14, D-7) |
| `MULTIPLEX` | a run whose plan settled `MULTIPLEX` prints the method word `MULTIPLEX` in the plan line and the word `multiplexed` on every row (FR-022) |
| No anchor | the plan line carries ` normalized=iter`, and the rate column is labelled `iter=` (D-3) |
| No estimate | `estimate=not available`, because the estimate is calibration data and a hunt run can reach the plan before any calibration answered (FR-021) |
| Host that refuses everything | the plan line, no ranked row, the skipped list with one line per refused event, and the summary reading `events=0/N` (D-12) |
| No hunt | no `hunt:` line at all, and no `hunt` key in the document (FR-040, I-8) |
| `--dry-run` | the plan line prints and nothing else does: a dry run measures one iteration, so the ranked table, the excluded tail and the summary have no samples to print |

## Option table test

`test/source/harness_cli_table_test.cpp` exists here and nowhere at `bd10d76`,
and hard rule 10 gives it to the first lane that merges it. It reads `cli.cpp` through a compiled-in `SG_CLI_SOURCE` path, the registration
pattern `SG_SCHEMA_PATH` uses at
`test/source/harness_json_schema_test.cpp:21-22`, and asserts:

- no option name repeats in the table,
- no option value repeats, counting the named constants as their numbers,
- values 270 to 277 are claimed by the hunt rows and 278 to 289 stand free,
- the fixture twin of a table with a planted duplicate name and a planted value
  clash is reported.

## Verification

| Behaviour | Test |
|---|---|
| Each refusal line of the table above, one case per row, status 2 and one stderr line | `test/source/harness_hunt_cli_test.cpp` |
| In-process `speedgunMain` status checks, and forked children where the refusal path calls `std::exit`, following `test/source/harness_cli_test.cpp:800,1166,1417` | `harness_hunt_cli_test.cpp` |
| Selection forms: `all`, list, pattern, `@file`, dedup across a list and a file | `harness_hunt_cli_test.cpp`, `harness_hunt_plan_test.cpp` |
| The conflict check for `--counter`, `--metric`, `--compare` and `--dry-run`, in both command-line orders | `harness_hunt_cli_test.cpp` |
| Console order and the eight line shapes | `test/source/harness_hunt_report_test.cpp` |
| Unique option names and values | `test/source/harness_cli_table_test.cpp` |
| README option rows for all eight options | the `example_drift` test with `--options` |
| One interrupt line and no second flag read after the hunt edit | `bash test/cli_shape.sh` |

The CLI tests script the machine through `FakeProvider`, so no test needs PMU
access, and none sleeps, reads a clock or assumes a CPU count (FR-092). Every
branch in `hunt_cli.cpp` is reachable from one of the rows above, so the file
carries no exclusion marker (FR-091).
