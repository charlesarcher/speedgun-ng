# Contract: The Compare Mode on the Command Line

## The options

Three entries join the `longOptions` table of `speedgunMain`
(`source/harness/cli.cpp:293` to `:310`):

| Option | Argument | Meaning |
| --- | --- | --- |
| `--compare` | `<baseline>,<contender>` | the two report documents, split at the first comma (FR-048) |
| `--compare-rule` | one rule text | adds one rule, in order, repeatable (FR-049) |
| `--compare-strict` | none | a `NOISY` row fails the run while the state word stays `NOISY` (FR-044, FR-045) |

The compare mode stands off until `--compare` is given. With the mode
on, `--filter` scopes the comparison (FR-050) and `--json=` writes the
comparison document (FR-057); the run options keep their spellings and
answer nothing the comparison reads.

## Parsing and the usage faults

The option scan parses as it goes, the pattern of the existing usage
errors (`source/harness/cli.cpp:371`, `:385`, `:406`, `:426`):

- `--compare` with no comma, or with an empty side, prints one line
  naming the option and the text read, and answers status 2.
- `--compare-rule` whose text the parse call rejects prints one line
  naming the option and the rule text and the fault the parse call
  named, and answers status 2 (FR-051).
- An invalid batch filter pattern prints the run path's line
  (`source/harness/cli.cpp:462` to `:466`) and answers status 2.

## The compare branch

The branch sits after the option scan closes (`source/harness/cli.cpp:456`)
and before registry expansion (`:518`), so the mode runs no benchmark
and reads no clock (SC-005):

```cpp
if (!options.compareBaseline.empty()) {
  const auto exit = detail::runCompareMode(options);
  SG_ENSURE((exit.status != 0) == exit.failed,
            "the compare status follows the table of contracts/cli.md "
            "(FR-052)");
  return exit.status;
}
```

`detail::runCompareMode` lives in `source/harness/compare.cpp` and
answers `CompareExit { int status; bool failed; }`: `failed` is set
from the verdict or the fault, `status` from the table below, the two
computed independently so the `SG_ENSURE` carries content (FR-052,
D-15). The closing postcondition of the run path
(`source/harness/cli.cpp:639`) keeps its predicate. The shape gate
anchors of `test/cli_shape.sh:31` to `:41` stand untouched (PC-11).

## Scoping and the empty pair

`runCompareMode` loads both documents through `loadReport`, applies the
batch filter to the loaded values (D-12), then calls `compareReports`.
When the filtered pair shares no name, the run prints one line saying
the pair matched nothing and answers status 0 (D-19).

## Exit statuses

The exit status table of the CLI contract
(`specs/015-benchmark-harness-core/contracts/cli.md:72` to `:82`) gains
these rows (FR-052):

| Condition | Status |
| --- | --- |
| the comparison verdict is a regression | 1 |
| `--compare` names a document the load call rejects | 2, with one line naming the file and the reader's fault (FR-051) |
| the two reports carry different `schemaVersion` texts | 2, with one line naming both versions (FR-051) |
| a rule filter matches no benchmark in either report | 2, with one line naming the rule (FR-039) |
| a value a rule names is absent, unanswered or uncountable, or a zero baseline stands under a relative threshold, or a rule's whole scope holds no readable row | 2, with one line naming the rule, the benchmark and the report (FR-036 to FR-038, FR-031, FR-040) |
| the comparison document could not be written | 2, with one line naming the path (FR-074) |
| the comparison passes, or the pair matched nothing | 0 |

## What the mode never does

- It never runs a benchmark, reads a clock or samples a sensor
  (FR-007, SC-005).
- It never reads a report document except through `loadReport`
  (FR-006).
- It never prints a partial table: a fault answers one line and no
  table (FR-003).
