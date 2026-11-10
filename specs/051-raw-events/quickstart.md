# Quickstart: Raw Event-Code Support (G-31)

The reviewer's read set at the lane head. Every read is a command with a
binary verdict. Build once, then walk the list.

## Prerequisites

```bash
cmake --preset=ci-rocky
cmake --build build
```

The host reads used by the acceptance pair: `cpu family 26`, `model 68`,
`vendor_id AuthenticAMD` (`/proc/cpuinfo`);
`/sys/bus/event_source/devices/cpu/type` reads `4`;
`/sys/bus/event_source/devices/cpu/events/instructions` reads `event=0xc0`.

## 1. Scripted units (host independent)

```bash
ctest --test-dir build --output-on-failure   -R 'counters_raw_event_test|harness_raw_events_test'
```

`counters_raw_event_test` pins: the accept with the recorded attribute
naming PMU type 4 and config word `0xc0` (SC-001's scripted pin, FR-001);
the attribute equality with a catalog row (FR-003); one probe verdict per
errno class and the scope relabel (FR-004); the absent-device refusal with
zero scripted opens (FR-002); the never-probed wide leaf (FR-005); the one
probe pair per address (FR-008); the typed route to unit `none` and the
events^1 dimension (FR-007); the formula event name (FR-007); the scripted
plan that compiles and measures a raw leaf through the raw-entry table
(FR-011); the unchanged leaf count, catalog shape, and near-miss lists
after a raw resolve (FR-015).

`harness_raw_events_test` pins: the four refusal lines with status 2 and no
measured row (SC-002); the `listed as scope-refused (FR-023)` arm; the
duplicate address standing as two columns over one scripted open (FR-008);
the four disclosure carriers (FR-006, SC-003); the byte-identical
`counter limit:` line with the raw column seated (FR-009).

## 2. The host acceptance pair (SC-001)

```bash
./build/example/docs_command_line --iterations=1 --counter=cpu/instructions --counter=cpu/rc0
```

Expect: two report columns, labels `cpu/instructions` and `cpu/rc0`, both
printing positive figures for config word `0xc0` (the host alias text is
`event=0xc0`, read above). Add `--json=out.json` and read both figures back
from `metrics[]`, with labels in `metricLabels` and in each row's
`metrics[].label`. The results record names family 26, model 68, the alias,
and the words (FR-013).

## 3. Refusal lines (US2, SC-002)

```bash
./build/example/docs_command_line --counter=nosuch/r4000; echo "status $?"
./build/example/docs_command_line --counter=cpu/r12345678901234567; echo "status $?"
./build/example/docs_command_line --counter=cpu/r4000 --list; echo "status $?"
```

Expect, in order: the line
`--counter: nosuch/r4000: no object at path 'nosuch'` at status 2; the line naming
`raw event code exceeds 16 hex digits` at status 2; the listing still printing at status 0 with no
address line (the standing order pinned at
`test/source/harness_cli_test.cpp:769-775`). The two kernel-refusal states
and the scoped relabel print through the scripted units in step 1, because
this host's cpumask devices publish not-encodable codes (spec edge list).
The width line wins when a wide leaf also names an unpublished device
(`--counter=nosuch/r12345678901234567` answers the width line).

## 4. Disclosure greps (FR-006, SC-003)

```bash
printf 'counter=cpu/rc0\n' > /tmp/sg051_opts.txt
out=$(./build/example/docs_command_line --iterations=1 \
  --options=/tmp/sg051_opts.txt --json=/tmp/sg051.json 2>&1)
grep -F 'options from a file or the environment: --counter=cpu/rc0' <<<"$out"
grep -F 'cpu/rc0=' <<<"$out"
python3 - <<'PY'
import json
doc = json.load(open('/tmp/sg051.json'))
assert doc['schemaVersion'] == '1.16'
listed = [label for b in doc['benchmarks'] for label in b['metricLabels']]
rows = [m['label'] for b in doc['benchmarks'] for r in b['rows'] for m in r['metrics']]
assert 'cpu/rc0' in listed and 'cpu/rc0' in rows
print('carriers ok:', listed.count('cpu/rc0'), 'labels,', rows.count('cpu/rc0'), 'row entries')
PY
```

Expect: the console row carries the `cpu/rc0=` column, the disclosure line
names the address the option file held, and the python read answers the
carriers with `1.16`. No key appears that
`docs/schema/report-v1.json` does not list: the schema-check unit
(`harness_json_schema_test`) passes against the written document, and

```bash
git diff --name-only 0fce6651..HEAD -- docs/schema/ include/
```

prints nothing: the schema file and every public header stand untouched.

## 5. Standing lines stay standing (SC-004, FR-015)

```bash
ctest --test-dir build -R 'harness_cli_test|harness_hunt_cli_test|\
counters_pmu_provider_test|counters_linux_pmu_seam_test' --output-on-failure
```

Expect all green with no changed expectation in any existing scenario: the
pinned `--counter` refusals (`harness_cli_test.cpp:729-755`), the pinned
hunter line
`--hunt: 'r4a03' is a raw event code; --hunt takes catalog addresses`
(`harness_hunt_cli_test.cpp:394-398`,
`specs/029-hunter-core/contracts/hunt-cli.md:83`), and the scope arms of
the seam unit (`counters_linux_pmu_seam_test.cpp:4110-4148`,
`4195-4222`, `4309-4329`). The `--catalog` and near-miss reads answer as at
base, pinned by the FR-015 scenario in step 1 and by the untouched catalog
scenarios here.

## 6. Gates (FR-013, SC-005, SC-006)

```bash
ctest --test-dir build --output-on-failure                 # ci-rocky suite
cmake --preset=ci-ubuntu && cmake --build build            # clang-tidy + cppcheck
CXX=clang++ cmake --preset=ci-sanitize && cmake --build build/sanitize && ctest --test-dir build/sanitize
CXX=clang++ cmake --preset=ci-tsan && cmake --build build/tsan && ctest --test-dir build/tsan
cmake --preset=ci-coverage && cmake --build build/coverage
find build/coverage -name '*.gcda' -delete
ctest --test-dir build/coverage -j 1 --output-on-failure
cmake --build build/coverage -t coverage
git grep -c LCOV_EXCL -- source include | awk -F: '{s+=$2} END {print s}'
```

Expect: a green suite on both release presets; no new analyzer finding; no
sanitizer or thread-sanitizer report; the coverage gate at 100.0 percent
lines, a branch shortfall of at most the recorded allowance of 21
(`tools/dbc/coverage_gate.sh:108`), and at most 84 markers with no new one
(base read measured at lane start: 84). The lcov capture keeps the
`mismatch,negative` allowances only (`cmake/coverage.cmake:73`), and the
`inconsistent` class stays absent from every read (that file, 47-59).
The prose, spell, format, and time-source reads exit 0, and
`python3 tools/docs/example_drift.py` exits 0. No `.github/` file is in the
diff.

Numbers the head answers (FR-012, SC-006, D-6):

```bash
grep -n 'VERSION 0.35.0' CMakeLists.txt            # the reserved minor
grep -n 'SOVERSION 8' CMakeLists.txt               # counters: no public header moved
grep -n 'SOVERSION 22' CMakeLists.txt              # harness: unchanged
grep -n 'kSchemaVersion = "1.16"' source/harness/json_report.cpp
```

## 7. Performance reads (VII, research R-15)

The scripted units count the resolve-path opens per raw address: one probe
pair and one table insert, and zero opens for a repeat of one address and
for a wide leaf. The resolve path joins no sysfs read, because the device
`type` and `cpumask` reads stand at device load. The report-path read
`git diff --name-only 0fce6651..HEAD` lists no change under
`source/harness/report.cpp`,
`source/harness/json_report.cpp`, `source/harness/catalog.cpp`, or
`example/`, and the step-2 run prints both columns' figures from the same
fold machinery. The recorded exemption: the new work is resolve-time only,
bounded per address, inside the untimed checkpoint
(`source/harness/cli.cpp:960-970`); no hot-loop code changes.
