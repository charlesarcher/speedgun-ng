# Phase 2 Collision Audit

**Surface**: the `ctest` registrations in `test/CMakeLists.txt`, the files
the test programs write, and the scratch paths the shell and Python
scripts under `test/` and `tools/` use.
**Head sha**: b6c11cd7b4c6009d043221a9d6467edc4b60ffdf
**Run id**: local
**Command**: the three blocks under Method, each run from the repository
root
**Numbers**: the table under Counted readings, one row per reading

Every registered entry runs with its working directory set to the `test`
directory of the build tree, so one directory holds every scratch file a
program opens under a relative name. That single directory is what makes
the audit worth running at `-j $(nproc)`: two entries that pick the same
relative name are two writers in one directory.

`example/CMakeLists.txt` holds no `add_test` at this head (79 lines), so
every entry named below is registered in `test/CMakeLists.txt`. The plan
says two entries live in `example/CMakeLists.txt`; that reading does not
match this tree, and the budget loop of T012 needs the same correction.

## Method

The registrations, read out of the tree already configured in `build`.
This audit configures nothing and changes no file outside its own record:

```text
cd /home/archerc/code/speedgun-ng
ctest --test-dir build -N --show-only=json-v1 > /tmp/roster.json
```

Then, for every entry, open the source the entry runs and list the names
it opens for writing. The roster, the entry to source map, the name list
and the inversion that finds a name held by two entries all come out of
one command:

```text
python3 - <<'PY'
import json, collections, os, re
repo = "/home/archerc/code/speedgun-ng"
roster = json.load(open("/tmp/roster.json"))["tests"]
prop = collections.Counter()
workdirs = set()
program = {}
for t in roster:
    for p in t.get("properties", []):
        prop[p["name"]] += 1
    workdirs.update(p["value"] for p in t["properties"] if p["name"] == "WORKING_DIRECTORY")
    cmd = t["command"]
    prog = cmd[1] if cmd[0].endswith(("/bash", "/python3")) else cmd[0]
    program.setdefault(prog, []).append(t["name"])
lit = re.compile(r'"([A-Za-z][A-Za-z0-9_.\-]*\.(?:txt|json|csv|log|out|tmp|dat|bin|md|yaml|yml|rules|opts|input))"')
owner = collections.defaultdict(set)
for t in roster:
    cmd = t["command"]
    prog = cmd[1] if cmd[0].endswith(("/bash", "/python3")) else cmd[0]
    stem = os.path.splitext(os.path.basename(prog))[0]
    src = os.path.join(repo, "test", "source", stem + ".cpp")
    if not os.path.exists(src):
        continue
    for line in open(src, errors="ignore"):
        for m in lit.finditer(line):
            if "/" not in m.group(1):
                owner[m.group(1)].add(t["name"])
twice = {p: n for p, n in program.items() if len(n) > 1}
print("entries", len(roster))
print("workdirs", sorted(workdirs))
print("props", dict(prop))
print("literals", len(owner))
shared = {n: sorted(v) for n, v in owner.items() if len(v) > 1}
print("shared", len(shared))
for n, v in sorted(shared.items()):
    print("  ", n, v)
print("programs twice", len(twice))
for p, n in sorted(twice.items()):
    print("  ", os.path.basename(p), n)
PY
```

The script entries were read one by one, the same way. The pattern below
locates the scratch paths, and each matched script was then read at the
lines it printed:

```text
rg -n '/tmp/|mktemp|>\s*[A-Za-z_./]|rm -rf|mkdir -p|os\.remove|unlink|TemporaryDirectory|NamedTemporary|mkdtemp' \
  test tools --glob '*.sh' --glob '*.py'
```

Thirty-four script files stand behind the 41 script entries:
`tools/docs/report_shape.py` carries six of them,
`test/compile-fail/run.sh` carries two, and `pause_bound_removed` holds
its command inline in place of a file. Seventeen of the thirty-four write,
and every one of the seventeen takes its root from `mktemp -d`, from
`tempfile.TemporaryDirectory`, from the `--out` argument the registration
passes it, or from a name carrying the process id. The other seventeen read
and print.

## Counted readings

| Reading | Value | Verdict |
| --- | --- | --- |
| entries registered | 134 | one working directory for all of them |
| entries by driver | 93 run a built program, 41 run a script (39 through `bash` or `python3`, 2 with the script as the command) | 34 script files and one inline command, both families audited |
| entries with `RESOURCE_LOCK` | 2, holding one lock string | master already locks one pair |
| entries with `PROCESSORS` | 1, value 2 | `harness_cli_test` |
| entries with `TIMEOUT` | 9 | 8 at 120 s, `example_run` at 300 s |
| entries with `SKIP_RETURN_CODE` | 8, value 2 | the privilege-gated PMU family |
| relative file-name literals in the program sources | 191 | the write surface of the suite |
| names that reach two entries | 7 | 6 belong to the `harness_hunt_json_test` pair |
| names two entries write at the same time | 1 | one new lock |
| programs registered by two or more entries | 5 | 4 write nothing shared |
| scripts that write outside a private root | 0 | no lock asked for |

## Shared resources and their fixes

| Resource | Entries that touch it | Fix |
| --- | --- | --- |
| `/tmp/counters_trap_checked_<pid>.txt`, the scratch file the trap checker redirects each fixture run into | `counters_trap_checked`, `counters_trap_checked_rejects_unknown_mode` | keep the master lock `counters_trap_checked_out` (`test/CMakeLists.txt:310` and `:329`) |
| `harness_pass_report_out.txt` in the entry working directory | `harness_pass_report_test`, `harness_pass_report_test_identity` | add the lock `harness_pass_report_out` to both, the one new lock this audit asks for |
| `harness_cli_column_note_<pid>.txt`, `harness_cli_out_<pid>.txt`, `harness_cli_counter_opts.txt`, `harness_cli_metric_opts.txt` | `harness_cli_test` | no lock: the note and the report carry the process id, the two option files are named by this one program; keep `PROCESSORS 2` (`test/CMakeLists.txt:674`) |
| `/tmp/sg-dbc-matrix.XXXXXX`, the build root of one matrix cell | `dbc_semantics_matrix` today, the five `dbc_matrix_*` rows once T006 splits it, `dbc_gate_fixtures` when it drives the same script | no lock: `tools/dbc/semantics_matrix.sh:76` takes the root from `mktemp -d`; keep `PROCESSORS` equal to `SPEEDGUN_TEST_JOBS` on the matrix rows |
| `/tmp/cf_$$.o`, the object file of the compile-fail runner | `dbc_compile_fail`, `counters_compile_fail` | no lock: `test/compile-fail/run.sh:111` puts the shell pid of that entry into the name |
| `/tmp/harness_hunt_json_<pid>_document.json`, `..._capture.json`, `..._capture_console.txt` | `harness_hunt_json_test`, `harness_hunt_json_test_no_hunt` | no lock: `adoptScratch` builds the prefix from the pid at `test/source/harness_hunt_json_test.cpp:1559` |
| `/tmp/<entry>_<pid><kind>`, the capture files of the CLI and hunt tests, plus `/tmp/dbc_trap_checked_<pid>.txt` | `harness_cli_interleave_test`, `harness_cli_sources_test`, `harness_cli_table_test`, `harness_cli_threads_test`, `harness_hunt_cli_test`, `harness_hunt_collect_test`, `harness_hunt_report_test`, `dbc_trap_checked` | no lock: each `capturePath` puts the pid in the name, the first at `test/source/harness_cli_interleave_test.cpp:431` |
| `build/test/counters_pmu_seam_tables`, the synthetic event-table root | `counters_linux_pmu_seam_test` | no lock while one entry writes it; the root is fixed (`test/CMakeLists.txt:429`), so a second entry over the same root needs its own directory or a lock named for it |
| the three fixture output roots under the build directory | `dbc_gate_fixtures` to `build/dbc-gate-fixtures`, `prose_gate_fixtures` to `build/prose-gate-fixtures`, `pmu_events_check` to `build/pmu-events-gate-fixtures` | no lock: each registration passes its own `--out`, and the three paths differ |
| the cpu PMU, opened through `perf_event_open` | the eight entries that carry `SKIP_RETURN_CODE 2`: `counters_pmu_test`, `counters_metric_formula_test`, `counters_metric_lookup_test`, `counters_metric_trees_test`, `counters_metric_threadless_test`, `counters_overhead`, `counters_counter_limits_test`, `counters_objects_limit_test` | no lock: the kernel arbitrates the counters and multiplexes concurrent groups, the checks hold once the ratio falls below 1 (`test/source/counters_pmu_test.cpp:811`, and scenario 6 asks for that shape on purpose), and a runner that refuses the open makes every one of the eight skip |
| cpu cores held by spinning worker threads | `harness_cli_test` at `PROCESSORS 2` today; `counters_recorder_test` (8 threads, `test/source/counters_recorder_test.cpp:361`), `harness_threads_test` (4, `:100`), `harness_jitter_preset_test` (4, `:124`), `harness_pass_collect_test` (2, `:114`), `harness_hunt_collect_test` (2, `:108`), and the loop spawners `counters_clock_raw_test`, `harness_context_barrier_test`, `harness_execution_context_test` | `PROCESSORS` set to the count each entry starts (FR-003), no lock; the three entries that narrow their own affinity mask (`harness_context_report_test.cpp:646`, `harness_topology_test.cpp:273`, `harness_jitter_cli_test.cpp:543`) pin only their own process and restore the mask, so they share nothing to lock |
| the example binaries, run from the entry working directory | the six `report_shape*` entries, `example_run`, `base_cli_matrix`, `counters_standalone_example`, `counters_giraffe_example` | no lock: no program under `example/` opens a file for writing, and `base_cli_matrix` sends its document to `/tmp/sg028_console_<pid>.json` through `reroute` in `test/base_cli/run_matrix.py` |

## The three sites the task named

The checker's scratch file is confirmed as a pair behind the master lock,
and corrected on one point: the name carries the process id at
`test/source/counters_trap_checked_test.cpp:89`, so the two entries do not
share the file today. The lock stays. It costs one scheduling edge between
two short entries, and it keeps the pair correct if the pid ever leaves the
name.

`harness_cli_test` and `harness_cli_column_note.txt` are corrected. The
file never exists under that name: `scratchName` appends the process id at
`test/source/harness_cli_test.cpp:253`, and the column note takes it at
`:515`. The registration comment at `test/CMakeLists.txt:673` already
states the invariant. FR-004 asks for a lock where two tests share a file,
so no lock belongs here, and T009's request to add one to
`harness_cli_test` is not carried. Where the sources disagree, this record
follows spec.md FR-004 and the contract's rule that a lock marks a real
shared resource, in that order, ahead of the task line.

The `tools/dbc` paths under `/tmp` are corrected.
`tools/dbc/semantics_matrix.sh:76` takes its build root from
`mktemp -d /tmp/sg-dbc-matrix.XXXXXX`, `tools/dbc/asm_smoke.sh:40` takes
its work directory from `mktemp -d`, and `test/compile-fail/run.sh:111`
names its object after the shell pid. No entry shares a `tools/dbc` scratch
path, so the row in the contract that offered "a lock, or the path made
unique per entry" is settled in favor of the second, which the scripts
already implement.

One collision the three known sites did not cover: `harness_pass_report_test`
captures its report through the fixed relative name
`harness_pass_report_out.txt` at `test/source/harness_pass_report_test.cpp:71`
and reads it back at `:472`. Two entries run that binary, the second one
selecting the byte-identity scenario through `argv`, so at `-j $(nproc)`
either of them can read the other's bytes and fail on a report it did not
write.

## Names that share a basename across directories

`CMakeLists.txt`, `README.md`, `harness.md` and `mapfile.csv` each appear
in an entry source and in a fixture script. Every write lands inside a
`tempfile.TemporaryDirectory` (`test/example-drift-fixture/run_example_drift_fixtures.py:187`,
`test/example-drift-fixture/run_report_shape_fixtures.py:262`) or inside
the staged trees under the runner's own `--out` root, while the entry
source reads the repository copy and never writes it.
`gate_ok_base.json`, `hunt_no_hunt_document.json` and
`hunt_no_hunt_console.txt` are read from `test/data` by the two
`harness_hunt_json_test` entries through `dataPath`. No lock for any of
them.

## Entries that need no lock

The remaining entries in the suite write nothing that a second entry
reaches. Four families cover them. The `harness_*_out.txt` family (about
40 names: `harness_capture_out.txt`, `harness_json_report.json`,
`harness_jitter_window_free.json` and their neighbors) holds one fixed
name per entry, each built inside the entry that names it, so two entries
never pick the same one. The capture family writes under `/tmp` behind a
pid. The shape and gate scripts (`barrier_shape`, `loop_shape`,
`cli_shape`, `snippet_compile`, `time_source_gate`,
`counters_header_purity`, `counters_tsc_read_shape`,
`simulation_mark_shape`, `example_run_fixtures`) build their probe trees
under `mktemp -d` and read the built binaries in the tree without writing
there. The scan and proof scripts (`hwloc_*`, `simdjson_*`,
`hdrhistogram_*`, `zlib_*`, `yaml_*`, `quill_*`,
`counters_push_atomic_scan`, `dbc_dependency_scan`, `example_drift`,
`report_shape`) read the repository and the archive and write nothing.

## What the next node adds

One lock string, named after the resource the way
`counters_trap_checked_out` names its scratch file:

```cmake
set_tests_properties(
  harness_pass_report_test harness_pass_report_test_identity
  PROPERTIES RESOURCE_LOCK harness_pass_report_out
)
```

Nothing else is added. `counters_trap_checked_out` and
`PROCESSORS 2` on `harness_cli_test` stay where master sets them, the
matrix rows keep `PROCESSORS` at `SPEEDGUN_TEST_JOBS`, and the multi-thread
entries above take their `PROCESSORS` value from FR-003.

PHASE2 COLLISIONS: 12 shared resources named, 1 new lock asked for
