# Contract: the `hwlat` words of `--jitter-probe` and `--os-noise`

Phase 1 output. The delta over two standing contracts: the `method=`
row of `specs/027-jitter-probe/contracts/cli-options.md` and the engine
word of `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md`.
One word names two surfaces: `hwlat` stands as a method of
`--jitter-probe` and as an engine of `--os-noise`, and no option row
joins the table (spec FR-002, FR-018, D-1, D-2, D-6). The standing
grammars, the standing error shape and the standing exit statuses stand
unchanged. The amendments this delta names join the two standing files
in the commit that lands the code (spec FR-039).

## The option table

| Long option | Internal value | Row and constant | Requirement |
| --- | --- | --- | --- |
| `--jitter-probe` | `260` | `source/harness/options.cpp:680`, `source/harness/detail/options.hpp:88` | spec FR-002 |
| `--os-noise` | `320` | `source/harness/options.cpp:686`, `source/harness/detail/options.hpp:96` | spec FR-018 |

No row joins the table, no internal value moves, and no short letter
joins. The block comment at `source/harness/detail/options.hpp:79` to
`:81` keeps `321` to `329` free for tracers, and this lane claims none
of them. `test/source/harness_cli_table_test.cpp` keeps its name and
value uniqueness checks over the unchanged table.

## Value grammar

The `--jitter-probe` value takes the fourth method word in the head
position of the standing grammar:

```text
--jitter-probe=<cpu-list|idle>[,threshold=<ns>][,mode=alongside|precheck|both][,precheck=<ms>|serial][,gaps=<n>][,strict][,method=tsc|fwq|ftq|hwlat][,quantum=<us>][,work=<n>][,noise=<percent>]
```

The `--os-noise` value takes the third engine word in the head
position of its standing grammar:

```text
--os-noise=osnoise|timerlat|hwlat[,cpus=<cpu-list>]
```

| Part | Meaning | Default | Requirement |
| --- | --- | --- | --- |
| `method=hwlat` | the user-space method that reads the SMI count and the NMI count beside the standing gap loop | absent: the value keeps `tsc` | spec FR-002, D-1, D-11 |
| `hwlat` as the `--os-noise` head | the kernel's hardware-latency detector, run through tracefs across the pre-check window | none: the value names an engine | spec FR-018, D-2 |
| `threshold=<ns>` | the gap threshold in nanoseconds, at or above 1, unchanged by the new method | 200 ns, or 10 times the measured loop period when that stands larger | spec D-11, A-4 |
| `cpus=<cpu-list>` | the tracer cpus, the standing list grammar, one key per value | absent: the pre-check's candidate set | specs/036 FR-008 |

Every other part of the two grammars stands unchanged. Each key stands
once in a value, and the new words change no bound of any key (spec
FR-002, FR-018).

## The threshold rule the new method keeps

| Rule | Detail | Requirement |
| --- | --- | --- |
| The default holds at 200 ns | `kDefaultThresholdNs` at `include/speedgun-ng/benchmark.hpp:2426` stands 200, and `method=hwlat` changes none of it | spec D-11, A-4 |
| The rise holds at ten times | `kRiseFactor` at `source/harness/jitter_probe.cpp:57` stands 10, and the effective threshold at `:538` to `:541` takes the larger of 200 ns and the rise | spec D-11 |
| The key overrides both | an explicit `threshold=` value stands as given, the standing rule at `source/harness/jitter_probe.cpp:1636` to `:1642` | specs/027 FR-007 |

A reader who wants a low threshold types `threshold=` with the new
method: `--jitter-probe=idle,method=hwlat,threshold=50`.

## Parse point and stop rule

| Input | Behavior |
| --- | --- |
| `--jitter-probe=idle,method=hwlat` | the user-space method runs on one probe cpu per L3 group the plans touch |
| `--os-noise=hwlat` | the detector runs across the pre-check window on the candidate set |
| `--os-noise=hwlat,cpus=2,9` | the detector runs on cpus 2 and 9 |
| `--jitter-probe=idle,method=hwlat --os-noise=hwlat` | both surfaces run, each recording on its own keys (spec D-6) |
| A `--jitter-probe` value from the command line | parses once in `speedgunMain` at `source/harness/cli.cpp:849` to `:854`, before the expansion and before any run |
| A `--jitter-probe` or `--os-noise` value from an option file line | parses where it settles, at `source/harness/options.cpp:1873` and `:1896`, and the refusal names the path and the line |
| `--os-noise` with no argument | the standing `getopt_long` missing-argument path, exit status 2 |
| An unknown flag | the standing unknown-option line and exit status 2 |

A rejected value stops the batch with one line on standard error and
exit status 2, and no benchmark runs (spec FR-005).

## Error surface

Every invalid value prints one line on standard error and exits with
status 2 before the first run starts (spec FR-005). The rows the new
words move stand first:

| Case | The line names | Status |
| --- | --- | --- |
| A method word outside the four | the word | 2 |
| A `method` key that stands twice in one value | the key | 2 |
| An engine word outside the three | the word | 2 |
| A `--os-noise` value on a run whose batch carries no `--jitter-probe` option | the missing pre-check window | 2 |
| A `--os-noise` value on a run whose mode runs no pre-check window (`ALONGSIDE`) | the missing pre-check window | 2 |
| An invalid value from an option file line | the path, the line, and the token | 2 |

Every other row of the two standing error surfaces stands unchanged.

## The refusal texts, assembled

Each line below stands the way the builder assembles it, with the
values one run inserts. The command line prints a `--jitter-probe`
message through the one `fprintf` at `source/harness/cli.cpp:852`.

| Builder | The line as printed |
| --- | --- |
| `source/harness/jitter_probe.cpp:1676`, with the word set the new branch adds | `--jitter-probe: the method 'smi' is not tsc, fwq, ftq or hwlat` |
| `source/harness/jitter_probe.cpp:1632` | `--jitter-probe: the key 'method' stands twice in the value` |
| `source/harness/noise_tracer.cpp:55`, with the word set the new branch adds | `--os-noise: the engine 'hwlato' is not osnoise, timerlat or hwlat` |
| `source/harness/cli.cpp:191` | `--os-noise: the batch carries no pre-check window for the tracer` |
| `source/harness/cli.cpp:1265` | `--jitter-probe: the machine/tsc leaf is not available` |
| `source/harness/options.cpp:270` | `--json-raw: 'chunks', 'gaps' and 'none' are the values the option names` |

The standing texts hold two words each, so the new branches move both
lists: the method refusal names four words and the engine refusal names
three. The accepted-word list a refusal prints stands in the same
branch that reads the word, so a test that adds a word to one list adds
it to the other (spec FR-005).

An option file line carries the origin in front of the same line, the
shape `source/harness/options.cpp:1539` to `:1541` builds and
`:2055` prefixes: `'<path>':<line>: --jitter-probe: <message>`. The
path stands quoted by `quotedText`, and the line number stands after
the colon.

## The root and tracefs cases, which refuse nothing

| Case | Behavior | Status |
| --- | --- | --- |
| `/dev/cpu/<cpu>/msr` answers `EACCES`, or stands absent | the console states `SMI count not available` once, every gap of that cpu carries no mark, and the run continues | 0 |
| A part states no documented SMI counter, an AMD part among them | the same disclosure, and the feature names no substitute counter | 0 |
| `/proc/interrupts` holds no `NMI` row, which is what an unprivileged reader gets | the console states `NMI count not available` once, and the SMI figures stand on their own | 0 |
| No tracefs stands at `sys/kernel/tracing`, the writes answer `EACCES`, or the kernel holds no detector | `printPrecheckReport` prints the standing `tracer: not available` line at `source/harness/report.cpp:1586`, the user-space figures stand whole | 0 |
| A setup write fails after an earlier write stood | every file the run wrote stands restored, the tracer lines state the failure | 0 |

The feature owns no root refusal: it states no line that asks for root,
it changes no exit status for the absence of root, and it carries no
setuid helper (spec FR-011, FR-023, FR-024). The only status-2 paths
for the two new words are the value refusals above. The status-2 line
at `source/harness/cli.cpp:1265` stands the standing `strict` refusal
of the missing TSC leaf, which the new method reaches the same way the
standing methods do.

## Exit statuses

| Status | Case |
| --- | --- |
| 0 | a run with either new word, the disclosures included: an unreadable `msr` node, no `NMI` row, no documented counter, no tracefs, no rights, a failed restore write |
| 2 | every case of the error surface above, the missing argument, and an unknown flag |
| 1 | the standing run failure statuses of feature 015, unchanged |

## Standing contract lines this delta amends

| File and line | Standing line | New line |
| --- | --- | --- |
| `specs/027-jitter-probe/contracts/cli-options.md:23` | `[,method=tsc\|fwq\|ftq]` | `[,method=tsc\|fwq\|ftq\|hwlat]` |
| `specs/027-jitter-probe/contracts/cli-options.md:35` | `` `tsc`, `fwq`, or `ftq`: one method per run, the words the value takes `` | `` `tsc`, `fwq`, `ftq`, or `hwlat`: one method per run, the words the value takes `` |
| `specs/027-jitter-probe/contracts/cli-options.md:97` | `A method word outside the three` | `A method word outside the four` |
| `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md:31` | `--os-noise=osnoise\|timerlat[,cpus=<cpu-list>]` | `--os-noise=osnoise\|timerlat\|hwlat[,cpus=<cpu-list>]` |
| `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md:36` | `` `osnoise\|timerlat` \| the engine word, one of the two the kernel's tracing tree carries `` | `` `osnoise\|timerlat\|hwlat` \| the engine word, one of the three the kernel's tracing tree carries `` |
| `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md:71` | `An engine word outside the two` | `An engine word outside the three` |

The sentence at `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md`
lines 60 to 61 keeps its statement that a rejected value stops the batch
with one line and status 2. The option table of
`specs/015-benchmark-harness-core/contracts/cli.md` gains no row.

## The engine word and the tracefs write

| Rule | Detail | Requirement |
| --- | --- | --- |
| One builder names the word | `engineWordOf` at `source/harness/noise_tracer.cpp:306` answers the text the engine writes into `current_tracer`, and it gains the third branch | spec FR-019, D-2 |
| The word the file holds | `hwlat`, the name `kernel/trace/trace_hwlat.c:863` gives the tracer | spec FR-019 |
| The standing two-way choice | `source/harness/noise_tracer.cpp:310` reads `engine == TracerEngine::OSNOISE ? "osnoise" : "timerlat"`, so the new engine needs its own branch before it runs | spec FR-018 |
| The mode word | the run writes `hwlat_detector/mode` with `per-cpu`, the word `kernel/trace/trace_hwlat.c:67` states, before the engine word, so each tracer cpu carries its own detector thread | spec FR-019, D-9, P-14 |
| The kernel default the write replaces | the kernel starts in `round-robin` at `kernel/trace/trace_hwlat.c:117`, which moves one thread across the mask | spec D-9 |
| The write order | `tracing_cpumask`, `hwlat_detector/mode`, `hwlat_detector/width`, `hwlat_detector/window`, `tracing_thresh`, `current_tracer`, `tracing_on` at `1` | spec FR-019 |
| The saved set | one value per file the run saved: `current_tracer`, `tracing_cpumask`, `hwlat_detector/mode`, `hwlat_detector/width`, `hwlat_detector/window`, `tracing_thresh` | spec D-8, FR-021 |
| `tracing_on` stands outside the set | the restore writes `0`, the standing shape of `source/harness/noise_tracer.cpp:379`, so a run that stopped a tracer that stood on leaves tracing off | spec D-8, SC-007 |
| The restore order | the restore writes `tracing_on` at `0`, then each saved value in the reverse of the write order, so the engine word goes back before the threshold | spec FR-021, P-8 |
| The standing engines keep their setup order | `source/harness/noise_tracer.cpp:332` to `:344` writes `current_tracer`, then the mask, then `tracing_on`, and the reverse rule of FR-021 flips that pair at the restore, where the standing restore writes `current_tracer` first at `:380` | spec Out of scope, D-15 |

## Consumers

- `test/source/harness_jitter_cli_test.cpp`: the fourth method word, the
  method refusal text with four words, and the repeated-`method` refusal
  (spec FR-002, FR-005).
- `test/source/harness_hwlat_tracer_test.cpp`: the third engine word, the
  engine refusal text with three words, and the missing-window refusal
  (spec FR-005, FR-018).
- `test/source/harness_noise_test.cpp`: the standing grammar over the
  moved engine word list (specs/036 FR-002, FR-003).
- `test/source/harness_cli_table_test.cpp`: the unchanged name and value
  uniqueness checks (spec FR-002).
- `specs/027-jitter-probe/contracts/cli-options.md` and
  `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md` carry
  the rows above in the commit that lands the parser branches (spec
  FR-039).
