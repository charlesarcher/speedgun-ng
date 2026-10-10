# Contract: selection, list mode, and the `DISABLED_` prefix

The H1 `--filter` and `--list` options
(`specs/015-benchmark-harness-core/contracts/cli.md:11-12`) match
instance names. Expansion completes before selection, so the filter
and list mode see every instance (`source/harness/cli.cpp:142`, R-03).

## Selection over instance names (FR-003, FR-011)

```text
speedgunMain                       source/harness/cli.cpp:142
  expandRegistry()                 added: families to instances (R-03)
  --filter / --list selection      H1 path, cli.cpp:227-248
  list mode prints names           cli.cpp:250-255
  the runner walks the suite-grouped order (R-12)
```

| Rule | Requirement |
| --- | --- |
| The filter and list mode shall match instance names. | FR-011 |
| Expansion shall complete at the start of `speedgunMain`, before the filter and the first run (R-03). | FR-003 |
| A filter that matches one instance name exactly shall run exactly that instance. | FR-011 |
| The filter is a regular expression, and list mode prints one name per line, as H1 defines (`015 contracts/cli.md:11-12`). | FR-011 |
| The filter compiles one regular expression and selects by substring match over the instance name, as H1 leaves it (`source/harness/cli.cpp:233-247`). | FR-011 |
| The `--catalog` path returns before selection and prints no instance name (`source/harness/cli.cpp:222-225`). | FR-011 |
| A duplicate instance name found at expansion prints one line on the standard error stream and leaves the exit status unchanged (FR-012). | FR-012 |
| The option spellings, the parse, and the exit statuses of H1 shall stay unchanged. | FR-022 |

## The `DISABLED_` prefix (FR-020, R-11)

| Rule | Requirement |
| --- | --- |
| An instance whose expanded instance name starts with `DISABLED_` shall register and never run. | FR-020 |
| The prefix shall be tested on the expanded instance name at selection time in `cli.cpp` (R-11). | FR-020 |
| The filter shall not match a disabled instance, and list mode shall not print it, as `benchmark_register.cc:181` treats it. | FR-020 |
| A family whose name carries the prefix expands to disabled instances. | FR-020 |
| A fixture method named `DISABLED_x` shall run: that instance name starts with the fixture class (R-11). | FR-020 |
| A `DISABLED_` family as the only match of a filter leaves the executable to report no match, run nothing, and exit zero, as H1 fixes it (015 `spec.md:550`, FR-036). | FR-020 |
| List mode prints instance names, and the printed set excludes every disabled instance (R-11). | FR-020 |
| Disabled instances stay in the registry and expand with their family; the gate acts at selection (R-11). | FR-020 |

## Exit statuses (unchanged from H1)

| Condition | Status |
| --- | --- |
| a benchmark failed or SIGINT interrupted a run | nonzero (015 `contracts/cli.md`) |
| the filter matched no benchmark, a `DISABLED_`-only match included | zero, with a message |
| list mode | zero |
| measured or skipped benchmarks | zero |
| unknown option or unparseable value | nonzero (015 `contracts/cli.md`) |

## Console row shape (FR-021)

| Rule | Requirement |
| --- | --- |
| The console row shall keep the H1 shape: one name column carrying the full instance name. | FR-021 |
| The row keeps the fixed H1 columns for iterations, time per iteration, overhead floor, and metric columns (015 `contracts/cli.md`). | FR-021 |
| A skipped benchmark prints its reason and no statistics, and a failed benchmark prints the exception text, as H1 leaves it (015 `contracts/cli.md`). | FR-021 |
| The suite and case split shall reach the caller through row order and the fields of `BenchmarkResult` (`result-fields.md`). | FR-021 |
| Suites shall print in the order of their first registered instance, and rows within one suite shall keep registration order (R-12). | FR-021 |
