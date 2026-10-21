# Contract: Options and Exit Status

## The options

| Option | Argument | Effect |
|---|---|---|
| `--json=<path>` | required | writes one report document at `path` after the batch |
| `--json-raw=<list>` | required | selects the chunk columns the document carries |

`<list>` for `--json-raw` is one of `none`, `chunks`, or a comma-separated
list of those two. The last mention of a value wins, the rule the counter
option already takes for a repeated name. The default is `none`, so
`--json=<path>` alone writes a document with no `raw` key.

The options reach `RunOptions` as `jsonPath` (an empty string when the
option stands off) and `jsonRaw` (a `RawSetting` bit set), beside the
percentile fields at `source/harness/detail/internal.hpp:324-341`.

## No family handle

The report is a property of the batch. A family carries no report setting
of its own, so the feature adds no `BenchmarkHandle` method. `--json=` selects the file for every
instance of the run, and a family cannot ask for a file of its own. This
differs from the percentile output, where the family selects and the
command line overrides (`specs/021-percentiles-raw-capture/contracts/
percentile-options.md`).

## Parsing and the exit-2 forms

The scanner parses both options in the shape the existing parsers use: one
line on standard error naming the option and the offending text, then
`std::exit(2)`, before any benchmark runs (`source/harness/cli.cpp:50-108`,
`:332`).

**Exit-2 forms the tests cover**: an empty path (`--json=`), a path with a
NUL, an empty `--json-raw=` list, an empty element (`chunks,,none`), an
unknown value (`chunks,samples`), and `--json-raw` with no argument where
the parser requires one.

**Accepted forms**: `--json=out.json`, a path with a directory prefix,
`--json-raw=chunks`, `--json-raw=none`, `--json-raw=none,chunks`, and a
repeated option whose later mention wins.

## The capture gate the raw setting sets

`--json-raw=chunks` turns chunk capture on for every instance of the run
even when the percentile output stands off (FR-020, D-14). The gate that
picks the chunk plan reads the percentile output or the raw setting; the
chunk plan, the recorder capacity and the loop cursor cycle stay exactly
as 021 settled them, so `test/loop_shape.sh` reads the same cycle it reads
today.

`--json-raw=none`, or the option absent, leaves the gate as 021 set it: a
run with the percentile output off captures nothing.

## Precedence with the percentile options

The two families of options are independent. `--no-percentiles` turns the
percentile lines off and leaves `--json-raw=chunks` capturing, so the
document carries chunk columns no console line printed (FR-020).
`--percentiles=50,90` with `--json-raw=none` writes the percentile cells
and no `raw` key.

## Exit status

| Situation | Status |
|---|---|
| every benchmark measured, report written or no report asked | 0 |
| a benchmark failed or was interrupted, report written | 1 |
| the report asked and the write failed, whatever the benchmarks did | 2 |
| an option error | 2, before any benchmark runs |

Status 2 wins over status 1: a user who asked for a report and did not get
one sees the failure of the artifact they asked for, and the run's own
failures stay visible in the document and on the console (FR-030, D-10).

The postcondition at `source/harness/cli.cpp:423` reads today:

```cpp
SG_ENSURE((status != 0) == (anyFailed || anyInterrupted));
```

It gains the third case, and `specs/015-benchmark-harness-core/contracts/
cli.md` gains the matching row of its status table (FR-050):

```cpp
SG_ENSURE((status != 0) == (anyFailed || anyInterrupted || reportFailed));
SG_ENSURE(status != 2 || reportFailed || optionError);
```

The exact spelling is the implementer's, the two conditions are not.
