# Contract: the command line and the console report

Phase 1 output. The surface `speedgunMain` parses with `getopt_long`
(FR-033), the report shape FR-035 fixes, and the exit statuses FR-036
fixes.

## Options (FR-034)

| Long option | Argument | Effect | Requirement |
| --- | --- | --- | --- |
| `--filter` | regular expression | runs exactly the benchmarks whose registered name matches | FR-034, US2 |
| `--list` | none | prints the matching names, runs no benchmark function | FR-034, US2 |
| `--repetitions` | integer ≥ 1 | repetition count | FR-007, FR-012 |
| `--min-time` | seconds, `> 0` | minimum time per run | FR-007, FR-015 |
| `--iterations` | integer ≥ 1 | explicit iteration count; calibration is skipped | FR-013 |
| `--warmup-time` | seconds, `≥ 0` | minimum warm-up time | FR-011 |
| `--dry-run` | none | one iteration, one repetition, no warm-up | FR-014 |
| `--counter` | catalog address, repeatable | attaches the leaf to every selected benchmark | FR-021 |
| `--catalog` | none | prints the catalog listing, then exits without running a benchmark | FR-037 |
| `--placement` | preset name: `none`, `core`, `l3`, `smt` or `compact` | runs the selected threaded benchmarks on the cpus the preset plans; the name is parsed once before the first run, and a name outside the five is a recoverable error that runs no benchmark (specs/025 FR-015, FR-016) | specs/025 FR-015, FR-016 |
| `--topology` | none | prints the layout listing of data-model.md E-10, then exits without running a benchmark; the listing stands ahead of selection, the way the catalog listing does | specs/025 FR-036 |
| `--compare` | `<baseline>,<contender>` | loads the two report documents, split at the first comma, answers the comparison table and the verdict, and runs no benchmark; a text with no comma, or an empty path on either side, prints one line naming the option and the text read (specs/026 FR-047, FR-048) | specs/026 FR-047, FR-048 |
| `--compare-rule` | one rule text, repeatable | adds one rule in the order given; a text the parse call rejects prints one line naming the option and the fault the parse call named (specs/026 FR-049, FR-051) | specs/026 FR-049 |
| `--compare-strict` | none | makes every `NOISY` row fail the run while the state word stays `NOISY` (specs/026 FR-044, FR-045) | specs/026 FR-044, FR-045 |

- A benchmark value set through the handle shall win over the command
  line (FR-015).
- An unparseable or negative count or time shall be a recoverable
  error, and so shall a zero repetition count, iteration count, or
  minimum time. A zero minimum warm-up time is valid, as the default of
  FR-007 requires. The executable reports the error, runs no benchmark,
  and exits nonzero (FR-034, clarification).
- `getopt_long` shall appear in the implementation alone; no public
  header names it (FR-050).

## Catalog listing (FR-037)

One line per published counter, six fields: the object path, the
counter name, the description, the unit, the read mode, and the
availability state, with the refusal kind for a leaf that cannot
count (PC-3, PC-9). The listing runs no benchmark function and exits
zero; a failed listing exits nonzero (US3, FR-036). Metrics join the
listing in the metric-formulas spec of the roadmap.

## Report shape (FR-026, FR-035)

Context lines above the rows carry the host, the cpu, the library
version, and the build type; a measured field comes from the counters
library, and a field the library does not publish stays out of the
line (R-08).

```text
host: <catalog fact, when published>   cpu: <catalog fact, when published>
speedgun-ng 0.6.0   build: <build type>

benchmark            iterations    time/iter (ns)   overhead floor (ns)   <metric columns>
BM_copy                    18432            311.4                  22.7   ipc=1.87 ratio=1.00 scaled=0 gap=0
```

- One row per repetition and one row per aggregate, in fixed columns
  (FR-035).
- Every measured row carries the plan's median sampling overhead in
  ns and states that each run window includes the cost of its two
  endpoint samples (FR-026).
- Each metric column carries the value with its running ratio, scaled
  flag, and gap state (FR-024).
- A skipped benchmark prints its reason and no statistics (FR-031).
- A failed benchmark prints the exception text (FR-032).
- Aggregates print when repetitions exceed 1: mean, median, standard
  deviation, coefficient of variation, min, max, for time and for each
  metric (FR-027).

## Exit statuses (FR-036)

Compare mode (specs/026) answers a verdict over two report documents
and runs no benchmark: `--compare` names the baseline and the
contender, split at the first comma (specs/026 FR-047, FR-048),
`--compare-rule` adds one rule, in order, repeatable (FR-049), and
`--compare-strict` turns every `NOISY` row into a failure while the
state word stays `NOISY` (FR-044, FR-045). The mode's rows join the
table below (FR-052).

The command line (specs/028) answers `--help` with the option list and
`--version` with the project version, each on standard output before any
benchmark runs (FR-015 to FR-017), and answers each usage fault the scan
reports with one line naming the rejected text: an unknown name, an
unknown name within two edits of a table name, an ambiguous prefix, and
an option given with no value (FR-007 to FR-009). Those rows join the
table below (FR-082).

The sources of specs/028 answer their own faults before a run starts: a
`SPEEDGUN_*` value, a `--threads` list and an `--interleave` seed each
print one line when the reader of the option they set refuses the text, a
line of the `--options` file or of the `--compare-rules` file the parse
refuses names its file, its line number and its text, a file the system
cannot open names its path, a second `--options` word names the option it
repeats, an `--env` word beside `off` names the option and the word, and
the two percentile variables standing together name both variables.
Those rows join the table below (FR-023, FR-027, FR-028, FR-032, FR-033,
FR-034, FR-036, FR-046, FR-054, FR-064, FR-065, FR-082).

| Condition | Status |
| --- | --- |
| a benchmark failed, the catalog listing failed, or SIGINT interrupted a run | nonzero |
| the report file named by `--json=` could not be opened or written | 2 (specs/023 FR-050) |
| the filter matched no benchmark | zero, with a message |
| list mode | zero |
| measured or skipped benchmarks | zero |
| `--placement` names a preset outside the five | 2, with one line naming the rejected text (specs/025 FR-016) |
| a family's handle `placement` names a preset outside the five | 2, with one line naming the family and the rejected text, before any instance runs (specs/025 FR-015) |
| a selected instance's preset cannot place its contexts | 2, with one line naming the preset, the place count and the requested count, before any run (specs/025 FR-018, Q-4) |
| a preset run meets a machine layout the harness cannot read | 2, with one line, whatever the context count (specs/025 FR-005, Q-6) |
| the `--topology` listing meets a machine layout the harness cannot read | 2, with one line (specs/025 FR-036, plan R-07) |
| the comparison verdict is a regression | 1 (specs/026 FR-052) |
| `--compare` names a document the load call rejects | 2, with one line naming the file and the reader's fault (specs/026 FR-051) |
| the two reports carry different `schemaVersion` texts | 2, with one line naming both versions (specs/026 FR-051) |
| a rule filter matches no benchmark in either report | 2, with one line naming the rule (specs/026 FR-039) |
| a value a rule names is absent, unanswered or uncountable, a zero baseline stands under a relative threshold, or a rule's whole scope holds no readable row | 2, with one line naming the rule, the benchmark and the report (specs/026 FR-031, FR-036 to FR-038, FR-040) |
| the comparison document could not be written | 2, with one line naming the path (specs/026 FR-074) |
| the comparison passes, or the pair matched nothing | 0 |
| `--help` prints the option list and runs no benchmark | 0 (specs/028 FR-015, FR-017) |
| `--version` prints the project version and runs no benchmark | 0 (specs/028 FR-016, FR-017) |
| an option name that matches no table row | 2, with one line naming the rejected text (specs/028 FR-007) |
| an unknown name within two edits of a table name | 2, with one line naming the rejected text and the nearest table name (specs/028 FR-007) |
| an option name that is an ambiguous prefix of table rows | 2, with one line naming the rejected text and the candidate names in table order (specs/028 FR-009) |
| an option given with no value | 2, with one line naming the rejected text (specs/028 FR-008) |
| `--options` stands twice on one command line | 2, with one line naming the option and no path (specs/028 FR-032) |
| `--env` names a word beside `off` | 2, with one line naming the option and the word as typed (specs/028 FR-028) |
| a `SPEEDGUN_*` variable holds a value the reader of its option refuses | 2, with one line naming the variable and the value (specs/028 FR-027) |
| `SPEEDGUN_PERCENTILES` and `SPEEDGUN_NO_PERCENTILES` stand together | 2, with one line naming both variables, whatever the command line holds (specs/028 FR-023) |
| a line of the `--options` file the parse cannot read | 2, with one line naming the file, the line number and the text (specs/028 FR-033, FR-036) |
| the `--options` file cannot be opened | 2, with one line naming the path (specs/028 FR-034) |
| a line of the `--compare-rules` file the rule parser refuses | 2, with one line naming the file, the line number and the parser's fault (specs/028 FR-064) |
| the `--compare-rules` file cannot be opened | 2, with one line naming the path (specs/028 FR-065) |
| a `--threads` list holds a malformed item, a count below one, or a range whose low bound stands above its high | 2, with one line naming the option and the text (specs/028 FR-045, FR-046) |
| `--interleave` names a text the seed reader refuses | 2, with one line naming the option and the text (specs/028 FR-054) |

## Interrupt behavior (clarification, FR-032)

SIGINT sets one flag. The runner reads it after each run, and the
current run completes. The benchmark reports as skipped with an
interrupt reason, and no later run starts. The plan, the recorder, and
every descriptor and mapping release. The process exits nonzero (R-06).
