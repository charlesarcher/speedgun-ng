# Quickstart record: Host Validation

Recorded here: the two runs of the `validate` target (T030), the docs
gate family over the two pages (T049) and every scenario of the guide, the
committed-set entry included (T050). The narrow-mask suite stands in
`taskset.md` and the pushed-head verdict in `ci-run.md`.

## Recorded runs, 2026-10-29, the recording host (`GenuineIntel-6-C6-2`, 20 cpus, one L3 group)

Task T030, FR-001, FR-002, FR-006, FR-035, SC-001, SC-002. The shape
follows `specs/034-host-noise-precheck/results/quickstart.md`. Both runs
went while `031-host-validation` stood at `3f8c315` with nothing
uncommitted in the working tree. Each lane of that working tree runs its
own build tree, so the commands below name `build-val` where the guide
names `build`, and `VALIDATE_OUT` points inside it. No record stands
committed at this head, so the reader line names the record the run wrote
(T031 commits it).

| Environment | Value |
| --- | --- |
| Preset | `ci-rocky`: developer mode on, `Release`, Unix Makefiles |
| Toolchain | GCC 16.2.1, CMake 4.4.5, Linux 7.3.0-4-cachyos |
| Tree root | the working tree, carrying `docs/schema/host-record-v1.json` and `external/pmu-events/arch/x86/mapfile.csv` |
| Output root | `$PWD/build-val/validation`, the `VALIDATE_OUT` cache entry, absent before run 1 and stood by the script |
| Affinity | the full mask `0-19`, the mask the record carries in its machine block |

The run needs a configured developer-mode tree and the host's own CPUID,
topology and mapfile reads. No token, no network, no PMU and no
`perf_event_paranoid` read stands before it (FR-004, FR-026).

## The two runs

```sh
cmake --preset=ci-rocky -B build-val -D VALIDATE_OUT=$PWD/build-val/validation
cmake --build build-val --target validate
cmake --build build-val --target validate
```

`$PWD` names the working tree root both runs started in, and the two lines
the programs print carry the record path absolute under it. Run 1, exit 0:

```text
[  0%] Built target quill
[  5%] Built target hwloc_vendor-ep
[  5%] Built target simdjson
[ 15%] Built target zlibstatic
[ 21%] Built target hdr_histogram_static
[ 39%] Built target yaml-cpp
[ 81%] Built target speedgun-ng_speedgun-ng
[ 97%] Built target speedgun-ng_harness
[100%] Built target host_record_write
[100%] Built target host_record_check
[100%] Writing and checking the host record
host-record: GenuineIntel-6-C6-2  $PWD/build-val/validation/GenuineIntel-6-C6-2.json
$PWD/build-val/validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid
[100%] Built target validate
```

Run 2, exit 0, its log line for line as run 1's:

```text
[  0%] Built target quill
[  5%] Built target hwloc_vendor-ep
[  5%] Built target simdjson
[ 15%] Built target zlibstatic
[ 21%] Built target hdr_histogram_static
[ 39%] Built target yaml-cpp
[ 81%] Built target speedgun-ng_speedgun-ng
[ 97%] Built target speedgun-ng_harness
[100%] Built target host_record_write
[100%] Built target host_record_check
[100%] Writing and checking the host record
host-record: GenuineIntel-6-C6-2  $PWD/build-val/validation/GenuineIntel-6-C6-2.json
$PWD/build-val/validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid
[100%] Built target validate
```

## What the second run left standing

| Check | Reading |
| --- | --- |
| Records under the output root | one file: `GenuineIntel-6-C6-2.json` |
| Bytes | 315 after each run |
| Written at | 23:31:53 after run 1, 23:31:55 after run 2, so the second run rewrote the one file (SC-002) |
| `recordVersion` | `1.0` |
| `cpu` | `GenuineIntel`, family 6, model 198, stepping 2, key `GenuineIntel-6-C6-2`, directory `arch/x86/arrowlake/` |
| `machine` | 1 package, 1 L3 group, 20 cores, 20 threads, mask `0-19`, one L3 row `0-19`, one node row `0-19` |
| `evidence` | empty, the list run 1 left standing (FR-006, FR-041) |
| Exit statuses | 0 and 0 |

SC-001: run 1 met an absent output root, stood it, wrote one record named
by the rendered key with the `.json` suffix, printed the CPU type and the
path, and exited 0. SC-002: run 2 left one record whose identity and
machine facts match its own run, with the evidence list of the first run
kept. The identity matches the lane host row of
`specs/031-host-validation/results/plan.md`, and the empty evidence list is
the list that plan records for this CPU type (FR-041).

## Docs gates, 2026-10-30, the same host (T049)

Task T049, FR-036, FR-040, SC-010. The gate family that
`specs/031-host-validation/tasks.md` names for the two pages ran over the
tree at `f43e1bf`, the head carrying the root-page block, the moved page
cap and the build page section. The working tree held two files another
lane had left in flight
(`source/harness/detail/host_record.hpp` and
`tools/hostcheck/host_record_write.cpp`). No reading below names either:
the docs gates read the pages, the example regions and the built examples,
and the prose gate reads every source, where both files stand clean.

| Gate | Command | Exit | Reading |
| --- | --- | --- | --- |
| `example_drift` | `python3 tools/docs/example_drift.py .` | 0 | no finding, the page standing 974 lines against the cap of 974 at `32dffcbb` |
| `example_drift` | the same command with `--sections`, `--forbidden`, `--names`, `--options` | 0, 0, 0, 0 | the section spans, the substance list, the quick-reference rows and the option rows each stand scanned |
| `example_drift_fixtures` | `python3 test/example-drift-fixture/run_example_drift_fixtures.py .` | 0 | every planted drift named, so a clean verdict cannot come from a scan that inspects nothing |
| `snippet_compile` | `bash test/snippet-compile/run.sh .` | 0 | 34 cpp snippets compiled; the shell block of the new region stands outside the emitted set |
| `example_run` | `bash test/example_run.sh build-val` | 0 | every docs example ran with its declared skips kept |
| `report_shape` | `python3 tools/docs/report_shape.py --binary build-val/example/first_benchmark --readme README.md` | 0 | the sample block matches a real run by shape |
| `prose-lint` | `cmake -P cmake/prose-lint.cmake` | 0 | 199 sources, 7476 units, 0 findings, 0 skipped |
| `prose_gate_fixtures` | `ctest --test-dir build-val -R '^prose_gate_fixtures$'` | 0 | 1 test passed |
| `spell-check` | `cmake -D SPELL_COMMAND=<codespell> -P cmake/spell.cmake` | 65 as written, 0 over the checked-in content | the gate reads the note below |

The same family as CTest registers it ran in one line,
`ctest --test-dir build-val -I 137,149`, and reads `100% tests passed out
of 13`: the drift and shape fixture entries, `snippet_compile`,
`report_shape` beside the six recorded runs of the user guide page, and
`example_run` with its fixture entry.

The spell gate carries one environment note. `cmake/spell.cmake` runs the
`SPELL_COMMAND` entry, `codespell` by default, and that name stands off
this machine's `PATH`, so the gate cannot run as written here. Pointed at a
fetched codespell it answers 65 on files under the build trees this
worktree holds (`build-val`, `build-hdr`, `build-wtr`): `.codespellrc`
skips the tree names a lane configures (`build`, `build-tidy`,
`build-shared`), and no CI checkout holds any of them. The same checker
over `git ls-files`, the content a runner gets, answers 0 and prints no
finding.

The `validate` target ran again at this head, ahead of the family: exit 0,
the writer printing `host-record: GenuineIntel-6-C6-2` beside the record
path and the reader printing that path with `valid`. The record stands 315
bytes with an empty evidence list, the shape of the two runs above. The
path composition fault that lane carried earlier on 2026-10-30 stands
fixed by `a7a987f`, which takes the catalog root three levels above the
mapfile, so no reading here waits on that target.

No page, region, cap line or note needed a change: every gate above read
clean on its first run at the head named.

## Both pages read back, 2026-10-30 (T051)

Task T051, FR-036, FR-040, FR-042. `README.md`, `docs/pages/building.md`
and `example/quickstart.sh` stand unchanged since `cfde998`, the commit
that closed the page work, so the read-through below covers the pages as
they shipped. Each row names the run that answers the claim. `build-val`
names the lane build tree, and the planted records stood under scratch roots
outside the repository and stand deleted now.

| Page claim | Run | Reading |
| --- | --- | --- |
| developer mode adds the `validate` target | `cmake -S . -B build-val-off -D CMAKE_BUILD_TYPE=Release`, then `cmake --build build-val-off --target validate` | the cache reads `speedgun-ng_DEVELOPER_MODE:BOOL=OFF` and the build answers `No rule to make target 'validate'.` at exit 2 |
| the same target stands in a developer build | `cmake --build build-val --target validate` | exit 0, the writer line beside the record path and the reader line beside the same path with `valid` |
| the reader stands at `build/test/host_record_check` | `ls build-val/test/host_record_check` | the file stands there, and the page form `--root . --records validation` ran from a tree root of its own |
| one record per CPU type at `validation/<key>.json` | `cmake -D VALIDATE_BIN=<the two programs> -P cmake/validate.cmake` in a clean clone, and one fresh developer-mode cache | the clone holds `validation/GenuineIntel-6-C6-2.json` at 315 bytes, and `VALIDATE_OUT` reads `<tree root>/validation` |
| one line per record, exit 0 when all read valid | the reader over a root holding the record above | `validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid`, exit 0 |
| staleness rule 1, the version behind the schema | a planted record with `recordVersion` `0.9` | `stale  the record speaks 0.9 and the format speaks 1.0`, exit 2 |
| rules 2 and 3, an evidence path absent, a cited line absent | one planted record carrying both rows | `stale  test/no_such_case.cpp stands absent from the tree; README.md holds no line the record cites`, exit 2 |
| rule 4, the mapfile answers no row | a planted record keyed `GenuineFake-9-9-9` | `stale  GenuineFake-9-9-9 selects no catalog directory: vendor GenuineFake, family 9, model 9`, exit 2 |
| a broken document | a planted record without `machine.mask` | `broken  host-record: /machine: the key 'mask' stands missing`, exit 2 |
| an absent schema | the reader with `--root` at a tree without `docs/schema` | `error: the record schema '<root>/docs/schema/host-record-v1.json' cannot be read`, exit 2 |
| an unreadable mapfile | the reader with `--root` at a tree whose mapfile carries mode 000 | `error: the record run cannot read '<root>/external/pmu-events/arch/x86/mapfile.csv'`, exit 2 |
| a record root with no `.json` entry | `--records validation` where no record stands committed yet | `error: the record root 'validation' holds no record`, exit 2 |
| an unusable argument, each program | `--bogus` on the reader and on the writer | `error: the option '--bogus' names no arm of the reader` and the same line ending `of the write path`, exit 2 each |

Every status the two pages state is one of these readings, and both pages
name the target as `validate` and the reader as
`build/test/host_record_check`. The counts FR-036 asks hold too: the root
page addition spans 10 lines inside `## Quick start`, the build page
section counts 25 lines under `## Quality gates`, and a scan of every line
these commits add to the tree names no host, no person and no public name.
The one string there that reads like a name is the `.omo` entry of the
spell skip list, a directory name the file carried before this feature.

Two readings bound the shipped surface and no page claims either way. A
developer-mode tree configured with `BUILD_TESTING=OFF` fails the CMake
generate step, because the `validate` target depends on the two programs
the test directory builds, so such a tree never reaches the target. And the
second command of the root page block answers 2 on this head, where no
record stands committed under `validation/` yet, the case its own result
sentence names.

## The quickstart scenarios, 2026-10-30 (T050)

Task T050 runs every scenario of
[the guide](../quickstart.md) and records the command, its exit status and
its observation, one row each. The head stood `fa68c1d` with no source file
uncommitted. Each lane of this working tree runs its own build tree, so the
commands name `build-val` where the guide names `build`, configured with
`-D VALIDATE_OUT=$PWD/build-val/validation`. `$PWD` stands for the working
tree root: the two programs print the record path absolute under it, and the
substitution keeps the account of this host out of the record (FR-042). The
mask stood `0-19`, the full mask of the host, in every row except the two
that name the narrow mask.

| Root | What it holds |
| --- | --- |
| `$PWD` | `docs/schema/host-record-v1.json`, `external/pmu-events/arch/x86/mapfile.csv` and the committed `validation/GenuineIntel-6-C6-2.json`, so the reader resolves its guard here |
| `/tmp/broken` | one record, the committed one with `machine.mask` removed |
| `/tmp/no-records` | one `README` entry, which is no record |
| `/tmp/stale` | one record, the committed one with one evidence row appended |
| `/tmp/tree` | the schema under `docs/schema`, the record under `validation`, and `external/pmu-events/arch/x86/mapfile.csv` of one row: `99,99,9,family,model,0` |
| `/tmp/no-mapfile` | the schema and the record, with no mapfile anywhere under it |

| Scenario | Command | Exit | Observation |
| --- | --- | --- | --- |
| Build and test | `ctest --test-dir build-val -R host_record --output-on-failure` | 0 | six entries pass in 0.02 s, one process each: `harness_host_record_schema_test`, `harness_host_record_test`, `harness_host_record_check_test`, `host_record_check`, `host_record_check_bogus_option`, `host_record_write` |
| The write on a host the catalog names | `cmake --build build-val --target validate` | 0 | the writer prints `host-record: GenuineIntel-6-C6-2  $PWD/build-val/validation/GenuineIntel-6-C6-2.json`, the reader prints that path with `valid`, and one file of 315 bytes stands under the output root |
| The refresh | `cmake --build build-val --target validate` | 0 | one file still, 315 bytes, its modified time moved and its content line for line what the first run wrote, the evidence list empty as the first run left it (FR-006) |
| The reader over the committed records | `./build-val/test/host_record_check --root . --records validation` | 0 | `validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid`, the one `.json` entry of the root the run names, read against the schema that root carries (FR-024, FR-029) |
| The committed set as a CTest entry | `ctest --test-dir build-val -R '^host_record_check$' --output-on-failure` | 0 | the registration runs `build-val/test/host_record_check --root $PWD --records $PWD/validation` and its pattern pins the per-file line `$PWD/validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid`; both arguments stand absolute in the registration, so the entry passed with the same line when the same command ran with `/tmp` as the start directory |
| A planted break | `./build-val/test/host_record_check --root . --records /tmp/broken` | 2 | `/tmp/broken/broken.json  broken  broken  host-record: /machine: the key 'mask' stands missing`, one line, the missing key named (FR-019, FR-030) |
| The record root with no record | `./build-val/test/host_record_check --root . --records /tmp/no-records` | 2 | `error: the record root '/tmp/no-records' holds no record`, and the `README` entry there reads as nothing (FR-029) |
| A planted stale record | `./build-val/test/host_record_check --root . --records /tmp/stale` | 2 | `stale`, naming the evidence path it could not confirm: `specs/031-host-validation/results/no-such-run.md stands absent from the tree` (FR-025, FR-041) |
| A planted mapfile that names no row | `./build-val/test/host_record_check --root /tmp/tree --records /tmp/tree/validation` | 2 | `stale  GenuineIntel-6-C6-2 selects no catalog directory: vendor GenuineIntel, family 6, model 198`, the schema read from the planted root beside the record, no hardware read (FR-020, FR-025) |
| The same mapfile against the write path | `./build-val/test/host_record_write --root /tmp/tree --out /tmp/tree/out` | 2 | `error: the identity 'GenuineIntel-6-C6-2' names no CPU: the mapfile names no directory`, and no file stands under the output root (FR-010) |
| The tree that holds no mapfile | `./build-val/test/host_record_check --root /tmp/no-mapfile --records /tmp/no-mapfile/validation` | 2 | one refusal line, `error: the record run cannot read '/tmp/no-mapfile/external/pmu-events/arch/x86/mapfile.csv'`, and no per-record line beside it (FR-004, FR-026) |
| The same tree against the write path | `./build-val/test/host_record_write --root /tmp/no-mapfile --out /tmp/no-mapfile/out` | 2 | `error: the mapfile '/tmp/no-mapfile/external/pmu-events/arch/x86/mapfile.csv' cannot be read` (FR-004) |
| A write aimed at a root that stands absent | `./build-val/test/host_record_write --root $PWD --out $PWD/build-val/validation-absent` | 2 | `error: the record file '$PWD/build-val/validation-absent/GenuineIntel-6-C6-2.json' cannot be opened: No such file or directory`, so a run straight against the writer needs its output root standing first (FR-002) |
| The reader's usage arms | `--bogus`; `--root` alone; a trailing `extra` | 2 | one line each: `error: the option '--bogus' names no arm of the reader`, `error: the option '--root' stands without its value`, `error: the argument 'extra' names no option of the reader`, each ahead of any record read (FR-004) |
| The writer's usage arms | `--bogus`; `--out` alone; a trailing `extra` | 2 | one line each: `error: the option '--bogus' names no arm of the write path`, `error: the option '--out' stands without its value`, `error: the argument 'extra' names no option of the write path`, each ahead of any host read (FR-004, FR-037) |
| The host the catalog cannot name | `ctest --test-dir build-val -R 'harness_host_record_test\|host_record_write' --output-on-failure` | 0 | two entries pass: the unit that plants an identity the catalog cannot name, a layout read made to fail and an output root that refuses the write, and the entry that pins the writer's refusal line of the malformed invocation (FR-010, FR-011, SC-004) |
| The narrow host | `taskset -c 0,1 ctest --test-dir build-val -R host_record --output-on-failure` | 0 | the same six entries pass under a mask of two cpus, `0-1`, and `taskset.md` carries the whole suite under that mask |
| A masked write takes a root under the build directory | `taskset -c 0,1 ./build-val/test/host_record_write --root $PWD --out $PWD/build-val/validation-masked` | 0 | its machine block names the mask and nothing outside it: 1 package, 1 L3 group, 2 cores, 2 threads, mask `0-1`, one L3 row `0-1`, one node row `0-1`; the committed record kept the 315 bytes and the digest it held before the run (FR-013, FR-034) |
| The gates | `ctest --test-dir build-val -R 'example_drift\|snippet_compile\|example_run\|report_shape' --output-on-failure` | 0 | 13 entries pass in 53.85 s, `example_drift`, its fixtures, `report_shape` and its six sample folds over both pages, `snippet_compile` over 34 snippets, `example_run` and its fixtures (FR-036, SC-010) |

Three readings carry the weight here. The committed-set entry and the
direct reader run answer the same line, one from the registration with two
absolute arguments, one from a shell, so the committed record reads valid
from any start directory (FR-005, FR-026). Every planted root answers the
reason its own files give, which is the shape FR-035 asks of the suite: no
scenario needs a particular CPU, and the two refusal arms name the mapfile
they could not open. The masked write shows why FR-034 pins an output root
under the build directory: the same command that rewrites the record of the
host rewrites it with two cores and the mask `0-1` when the process holds
two cpus, and only a root under the build tree keeps those machine facts
out of the committed record.\n\n## The lint gates, 2026-10-30 (T059)

Task T059, FR-022, FR-042, SC-012. The four gates ran over the tree at
`b855388`. The host answers `clang-format 23.1.4` while `.github/workflows/ci.yml`
installs 18.1.8, so both tools came off pinned launchers,
`uvx --from clang-format==18.1.8` and `uvx --from codespell==2.4.3`, named
through the `FORMAT_COMMAND` and `SPELL_COMMAND` cache entries the targets
read. The versions the run used print as 18.1.8 and 2.4.3.

| Gate | Command | Exit | Reading |
| --- | --- | --- | --- |
| Format | `cmake --build build-val --target format-check` | 0 | no finding; a direct `--dry-run --Werror` pass of the pinned formatter over the ten sources this feature adds answers the same, so the gate is not blind to them |
| Spell | `cmake --build build-val --target spell-check` | 0 | no finding, with `.codespellrc` read and its skip list carrying the lane build trees, which is what leaves a clean reading here meaningful |
| Prose | `cmake --build build-val --target prose-lint` | 2 | 8461 units over 235 sources answer two findings, both on a commit message: `CM.SECTION-UNKNOWN` for the section token of `c60d208`, outside the list at `tools/prose/prose_rules.yaml:17-30`, and `CM.BODY-WRAP` for one body line of 73 columns against `body_wrap: 72` |
| Prose fixtures | `cmake --build build-val --target prose-lint-fixtures` | 0 | the matrix plants three findings and the gate reports all three, which is the assertion the entry makes |
| Purity | `ctest --test-dir build-val -R '_purity_scan$\|counters_header_purity'` | 0 | seven entries pass in 19.10 s: the six vendored scans, `simdjson_purity_scan` among them, and the counters header scan |

The six files T055 names stand clean under every content gate: the three
units under `test/source/`, `docs/schema/host-record-v1.json`,
`example/quickstart.sh`, `README.md` and `docs/pages/building.md`. A tree
mode run of the prose gate over those files from the merge base with
`origin/master`, which reads whole files where the target reads the added
lines, answers no finding in them either.

The two findings stood on one message, now `c60d208`. It carried the section
token `Build`, which the configured list does not hold, and one body line of
73 columns. Its change stands in `tools/dbc/dbc_pair_gate.py`, whose token
reads `dbc`, and the line rewraps inside 72 columns without a word moving.
Both fixes are message-only, and 28 commits stood above that commit, so the
move waited for the rebase, where a rewrite reaches every commit another lane
builds on. The rebase landed it: the message now opens with `dbc`, and its
longest body line counts 72 columns, so neither finding stands.
