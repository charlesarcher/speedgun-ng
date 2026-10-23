# Contract: The Environment Source

**Surfaces:** `detail::environmentRows()`, the environment argument of
`detail::gatherOptionSources` (research D-04, D-09, D-10, D-11; data
model E-02, E-04). **Covers:** FR-019 to FR-028; SC-002.

## The seam

```cpp
auto detail::environmentRows() -> std::vector<EnvironmentRow>;

auto detail::gatherOptionSources(
    const CommandLineOptions& scan,
    const std::vector<EnvironmentRow>& environment,
    const std::optional<OptionFileText>& optionFile)
    -> std::expected<ResolvedOptions, std::string>;
```

**Precondition:** `environment` holds the names and values of the
sources as data; the gather reads no process environment.
**Postcondition:** the answer holds `ResolvedOptions` (E-07) or one
fault line, and the call touches no file system and no process state.

`cli.cpp` calls `environmentRows()` once, through `environ`, and only
when `--env=off` stands absent. A test builds the vector by hand
(FR-012).

## The flagged rows

A row with the environment flag takes its default from `SPEEDGUN_` plus
the long name upper-cased with `-` changed to `_`, computed from the row
(FR-019, D-09). The flagged rows are exactly these twelve (FR-020):

| Option | Variable | Value form |
| --- | --- | --- |
| `filter` | `SPEEDGUN_FILTER` | one pattern |
| `repetitions` | `SPEEDGUN_REPETITIONS` | one count |
| `min-time` | `SPEEDGUN_MIN_TIME` | one duration |
| `iterations` | `SPEEDGUN_ITERATIONS` | one count |
| `warmup-time` | `SPEEDGUN_WARMUP_TIME` | one duration |
| `counter` | `SPEEDGUN_COUNTER` | items split on `;` |
| `metric` | `SPEEDGUN_METRIC` | items split on `;` |
| `percentiles` | `SPEEDGUN_PERCENTILES` | points split on `;` |
| `no-percentiles` | `SPEEDGUN_NO_PERCENTILES` | a flag |
| `placement` | `SPEEDGUN_PLACEMENT` | one preset name |
| `threads` | `SPEEDGUN_THREADS` | items split on `;` |
| `interleave` | `SPEEDGUN_INTERLEAVE` | a seed, FR-022 |

Fifteen options carry no flag: `list`, `catalog`, `topology`, `dry-run`,
`json`, `json-raw`, `compare`, `compare-rule`, `compare-strict`,
`compare-rules`, `metric-method`, `options`, `env`, `help`, `version`
(FR-021). The clause about an option another lane owns becomes checkable
at rebase, when those rows join the table (research D-27).

## The value forms

A repeatable read from the environment splits on `;` into items, each
read under the command-line rule (FR-024). A flag takes `1`, `0`,
`true`, `false` (FR-025). Every other value goes through the same reader
the command line uses (FR-027, D-11).

`SPEEDGUN_INTERLEAVE` turns `--interleave` on with the seed its reader
answers, and `SPEEDGUN_INTERLEAVE=0` means given with seed 0 (FR-022,
FR-054).

## The faults

Each answers one line on standard error at status 2.

| Case | Line |
| --- | --- |
| the percentile pair stands together | `SPEEDGUN_PERCENTILES and SPEEDGUN_NO_PERCENTILES cannot both stand` |
| a flag value outside the four words | `SPEEDGUN_NO_PERCENTILES: expected 1, 0, true or false, got '<value>'` |
| a value its own reader refuses | `SPEEDGUN_<NAME>: ` then the reader's own line, which names the option and the value |

The pair check reads the environment pair alone and runs ahead of the
precedence rule, so a command-line half does not hide it (FR-023, spec
D-5, spec D-27). The command-line pair keeps the master answer: the last
word wins and the status stays 0.

A `SPEEDGUN_*` variable that matches no flagged row is ignored, with no
line and no status change (FR-026). A variable that matches a flagged
row reaches that row's reader with the text as spelled, empty included,
and the reader's own rule decides what that text means. A text the reader
refuses answers its one line and status 2 even when a higher source names
the same option with a text that reads (FR-027, FR-046, FR-054, SC-003).

## `--env=off`

`--env` takes the word `off` and turns the environment off for one run;
any other word answers `--env: expected 'off', got '<word>'` at status
2 (FR-028). The option stands on the command line alone: the option
file refuses its name (FR-031) and it carries no environment flag. With
the environment off, `environmentRows()` is not called and no
environment use is disclosed.

## Oracle

`harness_cli_sources_test` builds `EnvironmentRow` vectors and asserts
every rule above: the twelve flagged names and their values reaching the
run options, the fifteen unflagged names ignored, an unknown
`SPEEDGUN_*` name ignored, the `;` split, the four flag words and the
refusal line, the pair line, the seed through the variable, `--env=off`
and its refusal line, and the precedence of FR-037 across the three
sources. No test calls `setenv` and no test reads the process
environment (FR-081). The end-to-end inheritance check forks the test
binary and execs it with an explicit `envp` carrying the rows, the
pattern of research D-04.
