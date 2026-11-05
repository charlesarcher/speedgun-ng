# Contract: the counter probe example, its recorded run, and the readers of
both

**Feature**: specs/041-counter-probe | **Files**: `example/docs/counter_probe.cpp`
with `example/docs/counter_probe.expected`, and one `add_docs_example` line in
`example/CMakeLists.txt` | **Audit point**: `858c55d9`
**Spec**: FR-061 to FR-064, FR-073, US2, SC-003, SC-007, D-2, D-3a, D-9, ruling G-20

## What this contract covers

The example prints one line per core device. The line carries the device, its
counter limit, where that limit came from, and the figure the example's own group
probe reached beside it. This contract fixes the program's shape, the console
shape on three kinds of host, the recorded file, the one build line, and the readers
that check them. `group-probe.md` covers the library's probe, and the example calls
none of it: D-3a states the reason, and the consequence the lane accepts stands
below with it.

## The readers of these two files

| Reader | What it requires of this example | Where it says so |
|---|---|---|
| `test/example_run.sh` | a built `docs_counter_probe` binary, a non-empty `--list` answer, no line carrying `FAILED`, and exit 0 | `test/example_run.sh:39-46`, `:48-56`, `:58-70` |
| `tools/docs/example_drift.py` | every marked region equals, byte for byte, any fenced block that quotes it; the pages it opens are `README*`, `AGENTS.md` and `docs/**/*.md`; the main page holds 1004 lines or fewer | `tools/docs/example_drift.py:20` (the marker form), `:272-279`, `:281-314`, `:423-441`, `:256` |
| the acceptance record | the recorded run equals the `.expected` region on the host that recorded it, and the record names that host | FR-061, SC-007, ruling G-20 |

FR-061 rules out one reader on purpose: the lane SHALL register no `report_shape_`
fold test for this example. That tool compares a page's sample block with a real
run, and its fold keeps letters, so an entry for this example would let a runner
that refuses `perf_event_open` fail a compare it was never asked to win
(`tools/docs/report_shape.py:140-165`, `:264-302`). Three of the twelve goldens of
`example/docs/` already stand with no entry of theirs: `hunt_ranks.expected`,
`regression_gate.expected` and `report_views.expected`.

## The one build line

One line lands in `example/CMakeLists.txt` after `:91`:

```cmake
add_docs_example(counter_probe)
```

The function at `example/CMakeLists.txt:46-54` names the target
`docs_counter_probe` and the runner `run_docs_counter_probe`, links
`speedgun-ng::speedgun-ng` and `speedgun-ng::harness`, and pins `cxx_std_23`.
`test/example_run.sh:41` finds the binary at `$BUILD_DIR/example/docs_counter_probe`.
No other line of that file moves, and no line lands in `test/CMakeLists.txt`.

## The name of the recorded file

The house spells a recorded run `<name>.expected` beside `<name>.cpp`. Twelve
files under `example/docs/` carry that spelling, among them
`hunt_topdown.expected` and `jitter_probe.expected`. The file therefore stands at
`example/docs/counter_probe.expected`.

The spec's US2 independent-test line names `counter_probe.cpp.expected`. That name
matches no file the tree's readers build: `tools/docs/report_shape.py:313` builds
`<example>.expected` from its `--example` word, and the twelve goldens follow the
same form. The tool and the twelve standing files are the authority, and the file
name follows them.

A recorded file is needed only where the lane commits a console reading.
`example/docs/counter_metrics.cpp` carries two marked code regions and no
`.expected` file, and that is right for it: its regions are `cpp` blocks the main
page quotes, and no reader runs them as console text.

## The program

| # | Fact | Rule and citation |
|---|---|---|
| 1 | It registers one benchmark, `SG_BENCHMARK(bmProbeTick);`, one instance and no argument list. | the run gate reads the registered set and fails a file that lists nothing (`test/example_run.sh:48-56`); the one-benchmark shape stands at `example/docs/topdown.cpp:38` |
| 2 | It registers the clock and PMU providers before its first catalog query. | `example/docs/metric_passes.cpp:50-58` |
| 3 | It calls `sg::speedgunMain` first and keeps that status as its exit code. | `example/docs/topdown.cpp:61-66` |
| 4 | It prints its device lines after the report, and prints nothing when the command line holds `--list`. | the guard of `example/docs/topdown.cpp:40-46` and `:63`, and `example/docs/hunt_topdown.cpp:121-140` |
| 5 | It reads two exported names for the device's own figure: `Object::programmableCounters()` and `objectLimitSource(path)`. | `include/speedgun-ng/counters_system.hpp:85-86`, and `limit-source.md` for the accessor (M3, FR-040a) |
| 6 | It runs its own group-open probe over `perf_event_open`, with the same fit test, the same cap and the same closure rule as the library's probe. | FR-073 allows real opens here and in `specs/041-counter-probe/tools/group_probe.c`; the shape to copy is `group_probe.c:69-97` |
| 7 | It keeps the core-device name rule in this file: `cpu`, `cpu_core`, `cpu_atom`, and a device part that starts with `armv8_`. | FR-031; the same four-part spelling `source/harness/hunt.cpp:39-44` keeps |
| 8 | It adds no option, reads no file under `/proc`, and prints no word about kernel settings. | D-2, FR-062 |
| 9 | It exits with the harness' status on every shape, including a host where every open refuses. | FR-062, US2 scenario 3 |

Two of those rows need a word each, because both look like a shortcut and neither
is one.

Row 6: the example cannot call the library's probe. The counters archive builds
with `CXX_VISIBILITY_PRESET hidden` (`CMakeLists.txt:112`) and publishes `include/`
as its only interface directory (`CMakeLists.txt:148-152`), so
`sg::counters::detail::probeGroupLimit` reaches no reader outside the archive, and
that same reading is D-3a's reason for exporting `objectLimitSource` at all. A unit
cannot take the example's place either, because FR-073 keeps real opens out of
every unit. D-2 keeps check mode off the command line, so no option can ask the
library for one probe. The example therefore owns a small copy of the search: the
same one group per size, the same leader, the same one enabled read, the same
`time_running == time_enabled` with `time_enabled` past zero, and a close of every
descriptor on every path. `group-probe.md` owns the rule, and this file owns one
call of it in a program a reader can run.

Row 7: `isProbeCoreDevice` of M2 is a counters internal, and D-3 admits one
exported accessor. The example's four-name rule is the price of that decision, and
it is data a reader sees on the page beside the line it prints.

## The console shape

One line per core device the catalog publishes, in catalog order, after the report
rows:

```text
counter limit: <device> limit=<count>|none source=<word> probe=<count>|no limit
```

| Field | Its source | Its word when there is no answer |
|---|---|---|
| `<device>` | `Object::path()` of the enumerated device | a line stands for every core device the host lists |
| `limit=` | `Object::programmableCounters()` (`include/speedgun-ng/counters_system.hpp:85-86`) | `none` |
| `source=` | `objectLimitSource(path)`: `decode`, `probe`, `none` (FR-040a) | `none` |
| `probe=` | the example's own search of that device | `no limit` |

The shapes the spec asks for:

| Host | The line | Rule |
|---|---|---|
| a core device whose decode is silent and whose probe answers | `counter limit: armv8_pmuv3_0 limit=4 source=probe probe=4` | FR-040, FR-050, US1 scenario 1 |
| a core device whose decode answers and whose probe answers, the check mode of D-2 | `counter limit: cpu limit=6 source=decode probe=6` | US2 scenario 1, US2 scenario 2, F-4, SC-003 |
| a host that refuses every `perf_event_open`, and a device the decode answers | `counter limit: cpu limit=6 source=decode probe=no limit` | FR-061, FR-062, US2 scenario 3 |
| a host that refuses every `perf_event_open`, and a device the decode cannot answer | `counter limit: cpu limit=none source=none probe=no limit` | FR-043, US4 |

FR-061 and US2 scenario 3 ask for `no limit` where no `perf_event_open` answers,
and the shape prints it in the `probe=` field on both of the last two rows. A CPUID
decode needs no permission, so a host that refuses every open keeps its decoded
limit, and a line that read `no limit` alone could print there only by hiding a
figure the run really holds. The example prints that figure, prints `no limit` for
the probe, and prints nothing else (FR-062).

## The recorded region

The whole committed file `example/docs/counter_probe.expected`, recorded from the
acceptance host the pull request body names (ruling G-20, FR-061):

```text
# sg-example:begin report
speedgun-ng 0.31.0 build=Release
bmProbeTick              iterations=  87341205  real time/iter (ns)=    1.193  cpu time/iter (ns)=    1.190  overhead floor (ns)=  680.000  endpoint samples=2
counter limit: cpu limit=6 source=decode probe=6
# sg-example:end report
```

Four rules about those bytes:

- The markers carry the `#` spelling, which the marker pattern admits beside the
  `//` spelling (`tools/docs/example_drift.py:20`), and the region name `report`
  carries the name every other recorded run uses.
- `read_regions` drops trailing blank lines from a region
  (`tools/docs/example_drift.py:310-314`), so the file ends at the closing marker.
- Any page block that quotes the region must carry these five lines exactly,
  because `compare` is a line-for-line byte compare
  (`tools/docs/example_drift.py:423-441`).
- The figures are the acceptance host's: version `0.31.0` at the base, and the AMD
  Zen 5 figure of 6 from both paths (F-4, SC-003). M8 moves the project version to
  `0.32.0`, and SC-007 owns the compare as an acceptance reading on the host that
  recorded the run, so the record states the header line it read.

## What a host can move, and what it cannot

No registered entry folds a run of this example against the region (FR-061), so the
reading of the compare belongs to the acceptance record. The fold the other
examples use still states which differences a host accounts for, and a record that
reports a difference names one of the ones in the right column
(`tools/docs/report_shape.py:140-165`: a digit run folds to `#`, a whitespace run
folds to one space, and the `build=` field folds to `build=#`; letters keep their
bytes).

| Line | A host moves | A host cannot move |
|---|---|---|
| `speedgun-ng 0.31.0 build=Release` | the version digits, the build word | the word `speedgun-ng`, the `build=` field |
| the `bmProbeTick` row | every digit: iterations, both times, the overhead floor, the sample count | the name field, padded to the 25-column field every other recorded run uses, and the six field labels |
| the device line | the two figures after `limit=` and `probe=` | the device name, the word after `source=`, the words `no limit`, and the number of device lines |

The last row is the one that decided FR-061's ruling. The core-device count is a
host fact: an AMD or Intel non-hybrid host lists one core device, a hybrid pair
lists `cpu_core` and `cpu_atom`, and an Arm board lists one `armv8_` device with
another name. A host that refuses `perf_event_open` keeps its device names and moves
two words. A registered fold entry would read either change as a failure, because
the fold keeps letters and collapses only the repeated per-cpu lines of three named
shapes (`tools/docs/report_shape.py:106-110`, `:212-262`).

## The page block, and who lands it

- A block quoting this region cannot stand in `README.md`. The main page allows one
  sample-output block and requires it to name `first_benchmark.expected#report`
  (`tools/docs/example_drift.py:649-659`).
- The region may stand unquoted, the way `hunt_ranks.expected`,
  `regression_gate.expected` and `report_views.expected` stand today. That keeps M6
  inside the three files this contract names, and FR-064 still asks the drift gate
  to pass with the new build line in place.
- The figure may join the guide instead, beside the N6 rows at
  `docs/pages/harness.md:1568-1600`, the area FR-060 rewrites. Then the block takes
  the marker form the page already uses
  (`docs/pages/harness.md:1591`: `<!-- sg-example: <path>#<region> -->`), and the
  sample-output marker stays off it, because nothing folds a run against it. The
  drift gate's byte compare then holds the page and the region equal, and that
  compare reads two files and no host.
- That page edit belongs to M7, the module the plan names for those lines, and it
  lands in the same hunk as FR-060's rows. The prose near a block on that page
  carries no `FR-` id, no `T` task id, no principle number, no spec id and no
  contract label: the drift gate scans that page with the docs classes
  (`tools/docs/example_drift.py:483`, `:491-503`).

## What this contract does not cover

The library's probe and its closed answer stand in `group-probe.md` (M1). The
device and event rule stands in `device-rule.md` (M2). The accessor and its three
values stand in `limit-source.md` (M3). The fill order and the once-per-process rule
stand in `provider-fill.md` (M4). The plan-text line and the one schema key stand in
`disclosure.md` (M5). The N6 rows of the guide and the `README.md` line count stand
with M7.

## Verification

| Claim | Machine |
|---|---|
| the example builds, registers one instance, runs clean, prints no `FAILED` row and exits 0 on a host that refuses every open | `ctest --test-dir build -R example_run --output-on-failure` (FR-061, FR-062) |
| the recorded region equals a real run on the host that recorded it, and the two figures of the core device are equal there | the acceptance reading of SC-007, on the host the pull request body names (SC-003, ruling G-20); no `report_shape_` entry owns it (FR-061) |
| the new build line breaks no region and no page | `python3 tools/docs/example_drift.py` (FR-064, FR-075) |
| no unit opens a real event | `git grep -l perf_event_open test/source/` names no file (FR-073) |
| the main page holds 1004 lines or fewer after the lane lands | `wc -l README.md`, with both counts in the record (FR-063) |
| the example prints no word about kernel settings | the region holds five lines, and the probe refusal answers `probe=no limit`; the fold keeps letters, so any word a later change adds moves the recorded line and the record states it (FR-062) |

## Amendment, round 2 (2026-11-05)

Two names and one claim moved. The golden is `example/docs/counter_probe.expected`:
`tools/report_shape.py:313` builds the name from the target, so the `.cpp` spelling is
unreachable. The target is `docs_counter_probe` and the binary is
`./build/example/docs_counter_probe`, the names `example/CMakeLists.txt:46-54` gives a
docs example.

SC-003 compares the decoded figure with a search the example runs itself: FR-030 keeps the
library from probing a device its decode answered, and the example links the public
headers alone. The acceptance record carries a second reading taken through the attribute
the shipped probe composes, from
`results/requirement-1-shipped-shape.txt`, so a host figure stands behind the composed
attribute itself and no reader takes the example's copy for the shipped path.
