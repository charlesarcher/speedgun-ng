# Quickstart: JSON Reports

Runnable validation for the feature. Every command runs from the
repository root. The `dev` presets are machine-local and absent from a
fresh clone, so this guide drives `ci-rocky`, the checked-in release
developer build.

## Prerequisites

- Linux x86-64, GCC or Clang at the versions the CI matrix pins
- Submodules initialized: `git submodule update --init --recursive`
- `python3` for the docs, prose and JSON inspection commands below

## Configure and build

```bash
cmake --preset=ci-rocky
cmake --build build
```

## The feature's own tests

```bash
ctest --test-dir build --output-on-failure -R 'harness_json_report|harness_json_load|harness_json_schema|harness_cli|harness_capture'
```

Expected: every test passes. `harness_json_report_test` covers the key
order, the value of every field against the same run's `BenchmarkResult`,
the `null` form of an unanswered percentile and the three outcome
spellings. `harness_json_load_test` covers the ten error forms of
`contracts/report-loader.md` and the byte-identical round trip.
`harness_json_schema_test` covers the schema checker over a written
document and over five planted disagreements.

## The written file, by hand

```bash
./build/example/docs_json_reports --json=/tmp/report.json
python3 -c 'import json; d=json.load(open("/tmp/report.json")); print(list(d))'
```

Expected: the run prints its console rows exactly as it does with no
option, and the second command prints

```text
['schemaVersion', 'speedgun', 'benchmarks']
```

The document holds one object, `schemaVersion` is `1.0`, and no key names
a date, a time, a host or a pid (FR-005, FR-021).

## The raw chunk columns

```bash
./build/example/docs_json_reports --percentiles=50,90 --json-raw=chunks --json=/tmp/raw.json
./build/example/docs_json_reports --no-percentiles --json-raw=chunks --json=/tmp/raw2.json
python3 -c 'import json; d=json.load(open("/tmp/raw2.json")); r=d["benchmarks"][0]["rows"][0]; print(r["chunkCount"], len(r["raw"]["columns"][0]))'
```

Expected: the first run prints percentile lines and writes the `raw` key;
the second prints no percentile line and still writes the `raw` key,
because the raw setting turns capture on by itself (FR-020). The last
command prints two equal numbers: the chunk count and the length of the
first retained column (FR-018).

## The error forms

```bash
./build/example/docs_json_reports --json=/no/such/dir/r.json; echo "status=$?"
./build/example/docs_json_reports --json-raw=bogus; echo "status=$?"
./build/example/docs_json_reports --json=; echo "status=$?"
```

Expected: the first runs the benchmark, prints one line naming the path,
and reports `status=2` because the report the user asked for does not
exist (FR-030). The second and third print one line naming the option and
the offending text and report `status=2` with no benchmark run (FR-029).

## The stable diff

```bash
./build/example/docs_json_reports --json=/tmp/a.json
./build/example/docs_json_reports --json=/tmp/b.json
python3 -c 'import json; a=json.load(open("/tmp/a.json")); b=json.load(open("/tmp/b.json")); k=lambda o: [list(x) for x in (o, *o["benchmarks"], *o["benchmarks"][0]["rows"])]; print(k(a) == k(b))'
```

Expected: `True`. Two runs of the same build write the same keys in the
same order; only the measured numbers differ (FR-011, SC-009).

## The gates

```bash
ctest --test-dir build --output-on-failure          # the full suite
python3 tools/docs/example_drift.py .               # the page and its regions
python3 tools/prose/prose_gate.py --check all       # prose and commits
bash test/time_source_gate.sh                       # counters-only
bash test/loop_shape.sh build                       # the timed loop shape
bash test/cli_shape.sh                              # the option scanner shape
ctest --test-dir build -R 'simdjson_purity_scan|simdjson_nm_proof'
```

Expected: no findings from any of them. The loop gate reads the same
per-iteration cycle it read before the feature, because the writer runs
once after the batch and the raw setting changes only the plan choice
(FR-032, D-14). The two vendored proofs stay green over the new link line
(FR-048).

## The sanitizer and coverage runs

```bash
cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize
ctest --test-dir build/sanitize --output-on-failure
cmake --preset=ci-tsan && cmake --build --preset=ci-tsan
ctest --test-dir build/tsan --output-on-failure
cmake --preset=ci-coverage && cmake --build build/coverage
ctest --test-dir build/coverage
cmake --build build/coverage -t coverage
```

Expected: clean under all three presets, and the coverage gate reports
100% line and 100% branch over `source/harness/json_report.cpp` with no
exclusion marker (FR-045).

## What each story proves on its own

| Story | Command | Proof |
|---|---|---|
| US1 the written file | `harness_json_report_test`, the file above | one document, every field equal to the run's own value |
| US2 the load and the round trip | `harness_json_load_test` | a loaded value written again gives the same bytes |
| US3 the raw columns | the `raw2.json` command above | the columns survive a run that printed no percentile line |
| US4 the error forms | `harness_cli_test`, the three commands above | the invalid forms exit 2, a failed write beats a failed run |
| US5 the stable diff | the `a.json` and `b.json` command above | the key sequence agrees across two runs |
| US6 the page | `example_drift.py`, `run_docs_json_reports` | the section, the region and the two rows agree |
