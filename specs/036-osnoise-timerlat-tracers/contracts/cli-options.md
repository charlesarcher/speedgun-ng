# Contract: the `--os-noise` option

Phase 1 output. The addition to the option table of
`specs/015-benchmark-harness-core/contracts/cli.md`, the value grammar,
the parse point, the error surface, and the missing-window refusal
(spec FR-001 to FR-003, FR-011, FR-024, D-4). The shape stands the one
`specs/027-jitter-probe/contracts/cli-options.md` and
`specs/034-host-noise-precheck/contracts/cli-options.md` fix for the
probe family. The standing contracts stand amended by this delta in the
commit that lands the code (spec FR-024).

## Option table addition

| Long option | Argument | Effect | Requirement |
| --- | --- | --- | --- |
| `--os-noise` | one value in the grammar below | runs the named noise tracer across the pre-check window and prints the noise-by-source figures beside the pre-check block | specs/036 FR-002, FR-012 |

The row joins the table after the `--jitter-probe` row, takes no short
letter, and carries no environment name (the jitter family's own
shape). Its value stands held as the named constant
`kOsNoiseOptionValue` at `320` in
`source/harness/detail/options.hpp`, the first value of the tracer
block `320` to `329`; the block comment names `321` to `329` free for
tracers after the row joins. No standing name moves and no standing
value moves, and the name and value uniqueness tests carry the row
(`test/source/harness_cli_table_test.cpp`, spec FR-001).

## Value grammar

```text
--os-noise=osnoise|timerlat[,cpus=<cpu-list>]
```

| Part | Meaning | Default | Requirement |
| --- | --- | --- | --- |
| `osnoise\|timerlat` | the engine word, one of the two the kernel's tracing tree carries | none: the value names an engine | specs/036 FR-002, D-4 |
| `cpus=<cpu-list>` | one or more cpu indexes or closed `a-b` ranges, comma separated, the form the `--topology` listing prints, no cpu standing twice; every cpu stands inside the process affinity mask and the online set, the standing reader rules | absent: the pre-check's candidate set, one cpu per L3 group the plans touch | specs/036 FR-008, D-2 |

Each key stands once in a value; a second occurrence of one key stands
an invalid value with the standing error shape (spec FR-002). A
`cpus=` list naming a planned cpu runs as named: the run carries no
overlap refusal for it, and the docs carry the perturbation note
beside the option (spec FR-008, FR-021, the clarify answer of
2026-10-29).

## Parse point and stop rule

| Input | Behavior |
| --- | --- |
| `--os-noise=osnoise` | the osnoise engine runs across the pre-check window on the candidate set |
| `--os-noise=timerlat,cpus=2,9` | the timerlat engine runs on cpus 2 and 9 |
| `--os-noise` with no argument | the standing `getopt_long` missing-argument path, exit status 2 |
| An unknown flag | the standing unknown-option line and exit status 2 |
| No option at all | no tracer exists: the JSON carries the same bytes as master's apart from the `schemaVersion` value and the version figures the document carries, and the console prints no tracer line (specs/036 FR-015) |

The value parses where it settles, the shape the `--jitter-probe`
value follows (specs/027): the command line's value parses in
`speedgunMain` before the expansion, and the option file's value
parses where the gather settles it, so a file line's refusal names the
path and the line every file fault names. A rejected value stops the
batch with one line and status 2, and no benchmark runs (spec FR-003).

## Error surface

Every invalid value of this feature prints one line on standard error
and exits with status 2, before the first run starts (spec FR-003,
SC-003):

| Case | The line names | Status |
| --- | --- | --- |
| An engine word outside the two | the word | 2 |
| A `cpus` value that is no cpu index or range, or a cpu outside the process affinity mask, or a cpu the online cpu set does not hold | the token | 2 |
| A cpu that stands twice in one `cpus` list | the index and the whole value | 2 |
| A key repeated in one value | the key | 2 |
| An `--os-noise` value on a run whose batch carries no `--jitter-probe` option | the missing pre-check window | 2 |
| An `--os-noise` value on a run whose jitter-probe mode runs no pre-check window (`ALONGSIDE`) | the missing pre-check window | 2 |
| An invalid value from an option file line | the path, the line, and the token | 2 |

Message shape: the standing `--os-noise: <message>` line, carrying the
offending token and the reason; from an option file line the shape is
`<path>:<line>: --os-noise: <message>`, the origin every file fault
names. Every echoed token stands quoted, the way the option module
quotes a value. Tests assert the exit status, the one-line count and
the prefix; the wording stays unpinned.

The refusal reads the batch's probe options alone: a family-level
probe stands a different surface, and the compare mode carries no
probe, so it refuses the same way (research.md R-9).

## The disclosures that continue a run with status 0

| Case | Behavior |
| --- | --- |
| No tracefs mount at the standing path, no tracer engine, or no rights for the setup writes | the tracer lines print the standing `not available` disclosure, the user-space pre-check stands whole, and the run exits 0 (spec FR-016, D-5) |
| A tracer cpu whose count files state nothing at read time | that cpu's figure lines print the `not available` wording for the missing sources alone (spec Edge Cases) |
| A source the engine states no figure for | the same per-source disclosure (spec FR-010) |

## Exit statuses

| Status | Case |
| --- | --- |
| 0 | a run with the tracer, the disclosures included: the not-available host, the per-source absence (spec FR-016) |
| 2 | every case of the error surface, the missing argument, and an unknown flag (spec FR-003, FR-011) |
| 1 | the standing run failure statuses of feature 015, unchanged |

## Consumers

- `test/source/harness_noise_test.cpp`: the grammar, every status-2
  case of the error surface, and the source-order binding (spec FR-002,
  FR-003, FR-005, FR-011).
- `test/source/harness_cli_table_test.cpp`: the row uniqueness over the
  whole table (spec FR-001).
- `specs/015-benchmark-harness-core/contracts/cli.md` gains the option
  row in the same commit as the parser (spec FR-024).
