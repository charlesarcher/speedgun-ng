# Quickstart: the area cell of a hunter run

Feature: 046-hunter-topdown. Spec: [spec.md](spec.md). Contracts:
[area-mapper.md](contracts/area-mapper.md),
[counters-grouping.md](contracts/counters-grouping.md),
[console-cell.md](contracts/console-cell.md),
[document-key.md](contracts/document-key.md). Audit point: `b1dd7670`.

This page is a run guide. It states no figure of a run that has not happened:
each number below names the clause that measured it at the audit point. The
acceptance report of the feature carries the head figures, and SC-005 to SC-007
name the commands it runs.

## Prerequisites

| Need | Check | Note |
|---|---|---|
| Linux, the supported platform | `uname -s` | `ci-rocky` (`CMakePresets.json:168`) writes `build` |
| The acceptance host | `lscpu` | `corsairone`, an Intel Skylake-X part (D-14) |
| The directory the loader selects | `cat /sys/devices/cpu/caps/pmu_name` | `skylake` there (`specs/042-topdown/quickstart.md:15`) |
| Counter access | `sysctl kernel.perf_event_paranoid` | `-1` there (`specs/042-topdown/quickstart.md:16`) |
| Toolchain | `c++ --version`; `cmake --version` | the pair that builds the lane today (`specs/042-topdown/quickstart.md:17`) |
| The preset list of a fresh clone | `cmake --list-presets` | `ci-rocky`, `ci-ubuntu` (`CMakePresets.json:157`), `prepush-quick` (`:214`) |

A CI runner is a different host. The AMD parts there hold no top-down row in
their vendored directory, so a CI run answers the refusal path (D-14).

## Build

```bash
cmake --preset=ci-rocky
cmake --build build -j"$(nproc)"
```

Once per feature, configure and build `ci-ubuntu` the same way. It carries
clang-tidy and cppcheck, and an optimized build is where an optimizer finding can
appear.

## The screen

Two hunter runs on the acceptance host, one memory-bound benchmark and one
compute-bound benchmark (SC-007):

```bash
./build/example/memory_walk --hunt
./build/example/first_benchmark --hunt
```

Both commands ran at the audit point, and AP-3 records what they answered. The
plan line reads `events=497 passes=249 method=PASSES`, and the table carries 20
ranked rows. AP-3 names the top rows: `topdown-slots-issued`,
`topdown-total-slots`, `topdown-slots-retired`, `ref-cycles` and
`branch-instructions`. The head screen holds, in this order:

1. the header line, with the version and the build mode;
2. the benchmark table rows;
3. the plan line, the cut line and the two disclosure lines;
4. the ranked rows, each with its ` area=` cell after the ` delta=` cell
   (FR-013, FR-014);
5. the skipped rows;
6. the summary line, `hunt: plan summary events=493/497 passes=249` on the second
   command (AP-3);
7. the level-1 block of the same run, after the hunt block closes (FR-016);
8. the lines that stood below the tables before this feature.

A hunter run prints no `topdown: core-wide coverage=` line, because that line
stays behind the option the user named (FR-035).

The area cell stands ahead of the flag cell or the mark cell of its own row
(FR-013).

The three kinds of answer split that table in a known way. AP-9 read 49
published nodes and 58 distinct leaf addresses for the level-6 ask of the same
run. On that read the 497 printed rows answer 58 node paths, 412 vendor file
stems and 27 `unmapped`. The 20 ranked rows answer 1, 10 and 9. SC-007 asks the
report for those three counts.

The block holds the four level-1 nodes and every rule the standing printer owns
(`contracts/console-cell.md`). The deep tree answers the areas, the level-1 tree
answers the block, and one run keeps the two levels apart (FR-033, D-17). The
companion costs one table read of its own. A hunt run sets no `topdownLevel` on
either of its two option records (`source/harness/hunt.cpp:420-427`,
`:1048-1053`), so it reads no top-down table today (AP-8). Round 6 states the
cost: that run pays the first read of its process, 503 ms measured (AP-12). A
second ask of the same read answers in 0.57 ms (AP-12).

## The document of the same run

```bash
./build/example/memory_walk --hunt --json=/tmp/hunt.json
```

`--json` states the path of the document (README.md:698). The file then carries
`topdownArea` after `delta` in each ranked event that holds an area (FR-019). A
record that holds no area writes no such key. The header reads the schema version
`1.14` (FR-020). A run with no `--hunt` differs from the document the base build
wrote in the schema version and in `speedgun.version` alone (FR-022).

## Gates, cheapest first

| # | Command | Verdict at the base |
|---|---|---|
| 1 | `python3 tools/docs/example_drift.py .` | 0; the bare form checks neither the page budget nor a quick-reference row (SC-006) |
| 2 | `python3 tools/docs/example_drift.py --sections --names --options --forbidden .` | 0 (AP-2); the head pays the README budget on this line (AP-5) |
| 3 | `cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake` | 0 findings; the range form at the branch point is a void run (AP-2b) |
| 4 | `cmake -P cmake/spell.cmake` | 0 (AP-2) |
| 5 | `cmake -P cmake/lint.cmake` | 0 (AP-2) |
| 6 | `bash test/time_source_gate.sh .` | clean; FR-030 asks for no clock, no sleep and no device the host lacks |
| 7 | `bash test/tier_labels.sh build ctest` | no fault; a new entry needs one label, one budget and no skip rule master lacks |

Then the two DBC gates, by hand, because the `dbc-gate` target cached a python
with no PyYAML (AP-2). A doxygen pass comes first, over an XML-only Doxyfile. Its
`INPUT` is `include/speedgun-ng` and its `OUTPUT_DIRECTORY` is `build/dbc-gate`
(AP-2). The `--xml` flag names the xml directory (AP-2):

```bash
/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml \
  --registry tools/dbc/macros.yaml --out /tmp/dbc-doc.json
/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng \
  --src2 source tools --registry tools/dbc/macros.yaml \
  --doc-matrix /tmp/dbc-doc.json --out /tmp/dbc-pair.json
```

Both answer 345 interfaces with 0 gaps at `b1dd7670` (AP-2). One new accessor and
one new record join the matrix, and SC-006 asks the report for the number the
head answers. The aggregate gate covers the same ground and more:
`tools/ci/prepush.sh --self-check` and `tools/ci/prepush.sh --full` each answer
0 (SC-006).

## The suite, and the two-cpu lane

```bash
ctest --test-dir build -j 24 --output-on-failure
taskset -c 0,1 ctest --preset prepush-quick -j 2
taskset -c 0,1 ctest --test-dir build -L quick -j 2 --timeout 120
```

The 24-way form answers 178 entries and 3 failures at the base, and those three
names stand in files the 045 lane owns (AP-1). The preset form writes
`build/prepush`. It answered 171 tests and the same 3 failures in 680 s at the
audit point. FR-030 names that command for the base and for the head. The
`-L quick` form adds `prose_gate_fixtures` and `report_shape_fwq_ftq_sample` for
5 failures over 171, and both pass alone under the same mask (AP-11). Its
`--timeout 120` flag also cuts `example_run`, which answers in 168.89 s at the audit
point. The head
adds no name to either list, and the report carries both lists (SC-005).

## A host that refuses the top-down read

A CI runner is an AMD part whose vendored directory holds no top-down row
(D-14). Its hunt screen holds:

- one refusal line per process, in the words the library text carries (FR-017);
- that one line for both asks, the `--topdown` ask and the companion of a hunt;
- the plan line and the ranked rows, each with ` area=unmapped` (FR-005);
- no level-1 block, because the standing printer returns for a record that
  stands unsupported (`source/harness/report.cpp:361-363`);
- the exit status the same run answered at the base (FR-017).

That host settles no area value. D-14 refuses a CI run as the acceptance of an
area. SC-001 and SC-002 carry the arm verdicts over a planted table and a planted
identity.

## Coverage

```bash
cmake --preset=ci-coverage
cmake --build build/coverage -j"$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage -t coverage
```

`tools/dbc/coverage_gate.sh` answers exit 3 here, because `lcov` dies in its own
perl at startup (AP-6). That exit names the tool and holds no verdict on a ratio.
The ratio then comes from the coverage job of the pull request, and the report
names the local exit 3 with its line (SC-004).
