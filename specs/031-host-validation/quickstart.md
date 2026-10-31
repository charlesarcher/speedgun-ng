# Quickstart: Host Validation

Validation guide for `specs/031-host-validation`. The scenarios below
prove the feature end to end; the contracts and the data model carry the
shapes. Implementation detail stands in `tasks.md`.

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build -R host_record --output-on-failure
```

Expected: three test executables and two program entries pass, none of
them naming a cpu or reading a clock (spec FR-031 to FR-035). Each entry
runs one process: `host_record_check` validates the committed records from
files alone, and `host_record_write` runs one malformed-argument
invocation whose refusal line a passing pattern pins. The usage arms of
both entry points run in-process, the writer's through
`hostRecordWriteMain` with its facts seam and the reader's through
`hostRecordCheckMain` with its chooser factory, each parsing before any host
read (spec FR-004, FR-037).

## The write on a host the catalog names

```sh
cmake --preset=ci-rocky -D VALIDATE_OUT=$PWD/build/validation
cmake --build build --target validate
cat build/validation/*.json
```

Expected: the target runs the write, then the check, so one run prints
the line naming the CPU type and the file it wrote, then one line per
committed record, and exits 0. One file stands under `build/validation`,
named `cpu.key` with the `.json` suffix. The script of the target stands
that directory before it invokes the writer, so a tree carrying no
`validation/` still runs, and a run straight against `host_record_write`
needs its output root standing first (spec FR-002, FR-011).

The reviewer compares the identity block against the record's own
fields. `cpu.key` equals the model key built from `cpu.vendor`,
`cpu.family` and `cpu.model` in uppercase hex, plus the one hex digit of
`cpu.stepping` (`source/counters/linux_pmu/table_parse.cpp:673`,
`:684-692`). `cpu.directory` names the directory the mapfile under the
tree root selects for that ident. On this lane's host the model key
reads `GenuineIntel-6-C6`, the record key `GenuineIntel-6-C6-2` and the
directory `arch/x86/arrowlake/`.

The `machine` block carries the counts, the mask, the group rows and the
node rows `--topology` prints (`source/harness/topology.cpp:551`,
`:555`, `:571`, `:596`). The rows `--catalog` prints carry the metric
names of the directory the ident selects, so they confirm the mapping and
name no vendor, family, model or stepping (spec FR-001 to FR-008,
FR-012).

## The refresh

```sh
cmake --build build --target validate
```

Expected: exit 0, one file still for that CPU type, the identity and
machine facts rewritten, and the evidence list standing as the first run
left it (spec FR-006).

## The reader over the committed records

```sh
./build/test/host_record_check \
  --root . --records validation
```

Expected: one line per committed record naming the CPU type and `valid`,
exit 0, and no read of `/proc`, `/sys` or a cpu. The records are the
`.json` entries directly under `validation`, and every other entry there
reads as nothing (spec FR-024, FR-026, FR-027, FR-029). The same run
stands as the CTest entry `host_record_check`. The reader's usage arms
stand in `harness_host_record_check_test`, where an unknown option, a
missing value and an extra argument each print one line on standard error
and exit 2 before any record reads (spec FR-004).

## A planted break

```sh
cp validation/*.json /tmp/broken.json
python3 - <<'PY'
import json, pathlib
p = pathlib.Path('/tmp/broken.json')
d = json.loads(p.read_text())
del d['machine']['mask']
p.write_text(json.dumps(d, separators=(',', ':')))
PY
mkdir -p /tmp/broken && mv /tmp/broken.json /tmp/broken/
./build/test/host_record_check --root . --records /tmp/broken
```

Expected: one line naming the file, the CPU type and `broken`, with the
kind `host-record` and the missing key named, exit 2 (spec FR-019,
FR-020, FR-021, FR-030).

## The record root with no record

```sh
mkdir -p /tmp/no-records && touch /tmp/no-records/README
./build/test/host_record_check --root . --records /tmp/no-records
```

Expected: one line naming the root, exit 2. The root holds no `.json`
entry, which is the empty case, and the `README` entry there reads as
nothing. A `.json` entry the reader cannot open stands a failure the same
way (spec FR-029, D-10).

## A planted stale record

```sh
mkdir -p /tmp/stale
cp validation/*.json /tmp/stale/
python3 - <<'PY'
import json, pathlib
for p in pathlib.Path('/tmp/stale').glob('*.json'):
    d = json.loads(p.read_text())
    d['evidence'].append({
        "path": "specs/031-host-validation/results/no-such-run.md",
        "command": "ctest --test-dir build -R host_record",
        "exit": 0,
        "line": "1/1 Test #1: host_record_check ... Passed"})
    p.write_text(json.dumps(d, separators=(',', ':')))
PY
./build/test/host_record_check --root . --records /tmp/stale
```

Expected: one line naming the CPU type and `stale`, with the evidence
path it could not confirm, exit 2. The committed record carries an empty
evidence list, so the scenario plants its row (spec FR-025, FR-027,
FR-041).

## A planted mapfile that names no row

```sh
mkdir -p /tmp/tree/external/pmu-events/arch/x86
printf 'family,model,stepping,match,cpu_num\n99,99,9,family,model,0\n' \
  > /tmp/tree/external/pmu-events/arch/x86/mapfile.csv
mkdir -p /tmp/tree/docs/schema /tmp/tree/validation
cp docs/schema/host-record-v1.json /tmp/tree/docs/schema/
cp validation/*.json /tmp/tree/validation/
./build/test/host_record_check --root /tmp/tree \
  --records /tmp/tree/validation
```

Expected: one line naming the CPU type and `stale`, with the identity
whose directory the planted mapfile cannot select, exit 2, and no
hardware read. The chooser answers `NO_ROW` for that identity, and the
schema stands copied into the planted tree beside the record, so the
guard answers and this run prints that one verdict (spec FR-020, FR-025,
AC-1). The matching writer arm plants its identity and its mapfile inside
`harness_host_record_test` and calls the writing entry point, which exits
2 and writes no file (spec FR-010).

## The tree that holds no mapfile

```sh
mkdir -p /tmp/no-mapfile/docs/schema /tmp/no-mapfile/validation
cp docs/schema/host-record-v1.json /tmp/no-mapfile/docs/schema/
cp validation/*.json /tmp/no-mapfile/validation/
./build/test/host_record_check --root /tmp/no-mapfile \
  --records /tmp/no-mapfile/validation
```

Expected: one refusal line naming
`/tmp/no-mapfile/external/pmu-events/arch/x86/mapfile.csv`, exit 2, and no
read of `/proc`, `/sys` or a cpu. The schema stands copied in, so the path
the line names is the mapfile. The chooser opens it at the first question that
planted record asks and answers `unreadableMapfile`, so the refusal carries
the run and no per-record line stands beside it (spec FR-004, FR-025, FR-026).

## The host the catalog cannot name

```sh
ctest --test-dir build -R 'harness_host_record_test|host_record_write' \
  --output-on-failure
```

Expected: `harness_host_record_test` drives the writing entry point with
a planted identity the catalog cannot name, a planted layout read made to
fail and an output root that refuses the write; each arm prints one line
on standard error, exits 2 and leaves no file behind, on a runner that
holds any CPU (spec FR-010, FR-011, SC-004). The same unit runs the
writer's usage arms through `hostRecordWriteMain`: an unknown option, a
missing value and an extra argument each print one line and exit 2 before
any host read. The `host_record_write` entry runs one malformed-argument
invocation and pins its refusal line with a passing pattern (spec FR-004,
FR-037).

## The narrow host

```sh
taskset -c 0,1 ctest --test-dir build -R host_record --output-on-failure
```

Expected: the same entries pass on two cpus, and each of their reads
answers for the mask the process holds. The node rows of a masked run
stand for the nodes its mask reaches, the reading `--topology` gives
(`source/harness/topology.cpp:591`), so its machine block names no cpu
outside the mask. A masked write takes an output root under the build
directory, so those machine facts never reach the committed record (spec
FR-013, FR-034, SC-009).

## The gates

```sh
cmake --preset=ci-ubuntu && cmake --build build
ctest --test-dir build -R 'example_drift|snippet_compile|example_run|report_shape' \
  --output-on-failure
```

Expected: the docs gates pass over the root-page block and the build-page
section. `README.md` measures 974 lines against the `PAGE_LINE_CAP` of
974 in `tools/docs/example_drift.py`, the 964 that `master` leaves plus
the 10 this feature adds, so the change moves that cap to the measured
length and leaves the dated note beside it, the shape of the dated notes
that stand above that row; the gate reads the moved cap (spec FR-036,
SC-010). clang-tidy and cppcheck report no finding in
the new files (spec FR-039).
