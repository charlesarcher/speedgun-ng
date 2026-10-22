# Contract: the `--jitter-probe` option

Phase 1 output. The addition to the option table of
`specs/015-benchmark-harness-core/contracts/cli.md`, the value grammar,
the parse point, the error surface, and the `--json-raw` extension
(FR-008, FR-009, FR-011 to FR-014, FR-028, FR-037, FR-038, D-13).

## Option table addition

| Long option | Argument | Effect | Requirement |
| --- | --- | --- | --- |
| `--jitter-probe` | one value in the grammar below | runs the jitter probe over the batch by that value | FR-011 |

The option takes no short letter (FR-011): every short value of master
and of the two merged lanes names its own option. Its value stands
at 256 or above, held as the named constant `kOptJitterProbe` in the
option table (`source/harness/cli.cpp`), and one test asserts every
value in the option table is unique (FR-011).

## Value grammar

```text
--jitter-probe=<cpu-list|idle>[,threshold=<ns>][,mode=alongside|precheck|both][,precheck=<ms>][,gaps=<n>][,strict]
```

| Part | Meaning | Default | Requirement |
| --- | --- | --- | --- |
| `<cpu-list\|idle>` | one or more cpu indexes or closed `a-b` ranges, comma separated, the form the `--topology` listing prints; or the word `idle` | none: the value names the cpus together with the option | FR-011, FR-016, D-4 |
| `threshold=<ns>` | the gap threshold in nanoseconds, at or above 1; a negative value is a grammar rule of the value, where the string carries the sign (the field is `std::optional<std::uint64_t>`); a value stands as given, an explicit `threshold=200` staying exactly 200 ns | when the option names no threshold value: 200 ns, or 10 times the measured loop period when that stands larger | FR-007, FR-008, R-05 |
| `mode=` | `alongside`, `precheck`, or `both` | `alongside` | FR-012, FR-014, D-1 |
| `precheck=<ms>` | the pre-check duration in milliseconds, at or above 1 | 200 ms | FR-014, D-13 |
| `gaps=<n>` | the gap buffer size per probe cpu, 1 to 16,777,216 | 65,536 | FR-009 |
| `strict` | a flag: an unavailable TSC stops the run | absent: an unavailable TSC is disclosed and the run continues | FR-037 |

Each key stands once in
a value; a second occurrence of one key is an invalid value with the
standing error shape (FR-038).

## Parse point and stop rule

| Input | Behavior |
| --- | --- |
| `--jitter-probe=idle` | the `idle` preset resolves one probe cpu per L3 group the plans touch (D-4) |
| `--jitter-probe=2,5` | the probe runs on cpus 2 and 5 |
| `--jitter-probe=idle,mode=precheck` | the pre-check phase runs on the resolved cpus before the first benchmark (D-13, R-10) |
| `--jitter-probe` with no argument | the standing `getopt_long` missing-argument path, exit status 2 |
| An unknown flag | the standing unknown-option line and exit status 2 |
| No option at all | no probe exists: the console report and the JSON carry the same fields and shape as master's (FR-013, D-1) |

The value parses once in `speedgunMain`, before the expansion and before
any run, where the `--placement` parse stands today; a rejected value
stops the batch with one line and status 2, and no benchmark runs
(R-06). In `precheck` mode the probe uses the benchmark cpus, because it
stops before they start (D-3); in `alongside` mode and in the alongside
phase of `both`, the overlap check of the error surface binds (D-3,
R-10).

## Error surface

Every invalid value of this feature prints one line on standard error
and exits with status 2, before the first run starts (FR-038, SC-002):

| Case | The line names | Status |
| --- | --- | --- |
| A cpu token outside the list grammar, or a cpu outside the process affinity mask | the token | 2 |
| A head that names no cpu list and no `idle` | the head | 2 |
| `threshold=0`, a negative value, a value that is not a number | the value | 2 |
| A mode word outside the three | the word | 2 |
| `gaps=0`, a value above 16,777,216, a value that is not a number | the value | 2 |
| `precheck=0` or a value that is not a number | the value | 2 |
| A key repeated in one value | the key | 2 |
| No free cpu at all under `idle` | the preset word | 2 |
| An overlap in `alongside` mode or in the alongside phase of `both`: a probe cpu that is a planned cpu of a benchmark context, or an SMT sibling of one | the cpu and the benchmark | 2 |
| An unpinned benchmark whose affinity mask the probe cpus empty | the benchmark | 2 |
| The `machine/tsc` leaf missing, with `strict` | the availability fact | 2 |

Message shape: `--jitter-probe: <message>`, one line, carrying the
offending token and the reason, the shape the `--json-raw` rejection
prints today (`source/harness/cli.cpp:378`). The overlap line names the
cpu index and the benchmark name (FR-018). Tests assert the exit status,
the one-line count, and the prefix; the wording stays unpinned.

The disclosures that continue a run with status 0, each one line in the
disclosure region of `report-json.md` and the console contract of the
plan (FR-025):

| Case | Behavior |
| --- | --- |
| The leaf missing, without `strict` | the report discloses `not available` for the probe and the run continues with status 0 (FR-037, D-10) |
| A group with no free cpu under `idle` | the group is skipped, the report discloses it, and the run continues (FR-017) |
| An unpinned benchmark with cpus left after the probe cpus leave | the mask change is disclosed and the run continues (D-5) |
| Cpusets limit the probe cpus | the report discloses the limit (D-12, FR-036) |

## `--json-raw=gaps`

| Rule | Requirement |
| --- | --- |
| The closed value list of `--json-raw` gains `gaps` beside `chunks` and `none` (`source/harness/cli.cpp:309`, `:383`). | FR-028 |
| Only `gaps` writes the gap list into the document; `chunks` keeps the chunk lists and `none` writes no raw content. | FR-028 |
| Any other word keeps the standing rejection: one line naming the closed value list, exit status 2 (`source/harness/cli.cpp:378`). | FR-028 |

The gap list is a per-probe-cpu array of kept records, each carrying the
stamp and the gap in ns; its shape stands in `report-json.md`.

## Exit statuses

| Status | Case |
| --- | --- |
| 0 | a run with the probe, the disclosures included: an unavailable TSC without `strict`, a skipped group, a disclosed mask change, a cpuset limit (FR-037, FR-017, D-5, D-12) |
| 2 | every case of the error surface, the missing argument, and an unknown flag (FR-038) |
| 1 | the standing run failure statuses of feature 015, unchanged |

## Consumers

- `test/source/harness_jitter_cli_test.cpp`: the grammar, every status-2
  case of the error surface, the mode machine, the option-value
  uniqueness test over the whole table, and the probe-free no-probe
  report shape (FR-008, FR-009, FR-011 to FR-013, FR-038).
- `specs/015-benchmark-harness-core/contracts/cli.md` gains the option
  row in the same commit as the parser (plan, Files outside the feature
  directory).
