# Contract: the placement and topology options

Phase 1 output. The additions to the option table of
`specs/015-benchmark-harness-core/contracts/cli.md`, and the listing mode
they carry (FR-015 to FR-018, FR-036).

## Option table additions

| Long option | Argument | Effect | Requirement |
| --- | --- | --- | --- |
| `--placement` | one preset name | places the contexts of every threaded run of the process by that preset | FR-015, FR-016 |
| `--topology` | none | prints the layout listing, runs no benchmark, exits 0 | FR-036 |

The handle option `BenchmarkHandle::placement(name)` states the preset
for one family. Where a handle and the command line both stand, the
handle wins, the standing rule for a benchmark value over the command
line (FR-015).

## Parsing and stop rule

| Input | Behavior |
| --- | --- |
| `--placement=l3` | the process default preset becomes `l3` |
| `--placement=l2` | one line on standard error, exit status 2, no benchmark runs |
| `--placement=0-3` | one line on standard error, exit status 2, no benchmark runs |
| `--placement=none` | accepted as the preset name it is: no plan, no topology line, no fit check, the behavior feature 020 ships, exit 0 (FR-010, FR-019) |
| `--placement` with no argument | the standing `getopt_long` missing-argument path, exit status 2 |
| `--topology` | the listing prints, exit 0, no benchmark runs, no report block prints |
| `--topology --filter=x` | the listing prints and exits 0: the listing runs before selection, the way `--catalog` does, beside the `--catalog` block at `source/harness/cli.cpp:321` and ahead of the expansion and filter at `:326-357` |
| An unknown flag | the standing unknown-option line and exit status 2 |
| `--placement=l3` over a layout the harness cannot read | one line on standard error, exit status 2, no benchmark runs, whatever the context count (FR-005, Q-6) |

The preset name is parsed once, before the first run, so a bad name stops
before any context starts (FR-016, FR-018).

## Listing output

```text
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
```

| Rule | Requirement |
| --- | --- |
| The counts equal the topology line of a preset run on the same host. | FR-036, SC-12 |
| One line per L3 group names the cpus of that group inside the process mask. | FR-036 |
| No memory node appears in the listing. | spec Scope, N12 |
| A layout the harness cannot read makes the listing print one line and exit with status 2. | plan decision R-07 |

## Exit statuses

| Status | Case |
| --- | --- |
| 0 | a normal run, and the listing on a readable layout |
| 2 | an unknown preset name, a cpu list, a missing argument, an unfittable context count, an unknown flag, a preset run over a layout the harness cannot read, and the listing on an unreadable layout |
| 1 | the standing run failure statuses of feature 015, unchanged |

## Consumers

- `test/source/harness_cli_placement_test.cpp` drives the table above
  with the option scanner and the fit check, and asserts each status.
- `example/docs/command_line.cpp` keeps its region: it names no placement
  option, so its bytes stand. A new row for `--placement` and
  `--topology` joins the option table of
  `specs/015-benchmark-harness-core/contracts/cli.md` in the same commit
  as the parser, the way the metric rows are planned to join it.
