# Narrow-mask record: Host Validation

Task T058, FR-031, FR-034, FR-035, SC-009. The suite runs under a mask of
two cpus, which is how a host of 20 reaches the shape SC-009 names: this
lane host holds one package, one L3 group and one node, and its committed
record carries the mask `0-19`. A run held to `0,1` leaves it two cpus of
one node, the width the scenario asks for. The kernel of this host answers
`perf_event_paranoid` 2.

## The command and the two readings

```sh
taskset -c 0,1 ctest --test-dir build-val -j 2 --output-on-failure
```

Reading 1 ran at `80c6b08` over 149 entries and closed in 619.08 s: 145
passed, `counters_pmu_test` and `counters_overhead` stood skipped, and
`counters_linux_pmu_seam_test` and `harness_cli_test` failed. A sibling
landed two more CTest entries for the reader over the next twenty minutes,
so reading 2 ran the same command at `fa68c1d` over 151 entries and answers
the same verdict on the same two entries:

```text
99% tests passed, 2 tests failed out of 151

Label Time Summary:
quick    = 353.46 sec*proc (144 tests)
slow     = 855.30 sec*proc (7 tests)

Total Test time (real) = 585.40 sec

The following tests did not run:
	 47 - counters_pmu_test (Skipped)
	 53 - counters_overhead (Skipped)

The following tests FAILED:
	 48 - counters_linux_pmu_seam_test (Failed)             quick
	 70 - harness_cli_test (Timeout)                        slow
Errors while running CTest
```

Two lanes shared the machine across both readings, with the one-minute load
at 1.86 when reading 2 started and 1.99 when it closed, over 20 cpus. The
mask is what the run measures, and the load stands recorded beside it.

## The entries the run covers

`ctest --test-dir build-val -N` lists 151 entries at `fa68c1d`, every one
selected by the run above. All of them ran but the two skipped entries
named below.

```text
1 speedgun-ng_test
2 dbc_test
3 dbc_literal_fixture
4 dbc_trap_checked
5 dbc_compile_fail
6 dbc_standalone_test
7 dbc_dependency_scan
8 hwloc_purity_scan
9 hwloc_nm_proof
10 simdjson_purity_scan
11 simdjson_nm_proof
12 hdrhistogram_purity_scan
13 hdrhistogram_nm_proof
14 zlib_purity_scan
15 zlib_nm_proof
16 yaml_purity_scan
17 yaml_nm_proof
18 quill_dependency_check
19 quill_purity_scan
20 dbc_asm_smoke
21 dbc_matrix_off_o0
22 dbc_matrix_off_o2
23 dbc_matrix_on_o0
24 dbc_matrix_on_o2
25 dbc_matrix_observe_off_o0
26 counters_tsc_read_shape
27 simulation_mark_shape
28 test_tier_labels
29 dbc_gate_fixtures
30 prose_gate_fixtures
31 counters_tsc_test
32 counters_core_test
33 counters_fake_test
34 counters_compile_fail
35 counters_header_purity
36 counters_push_atomic_scan
37 counters_recorder_test
38 counters_noalloc_test
39 counters_trap_checked
40 counters_trap_checked_rejects_unknown_mode
41 counters_objects_test
42 counters_registration_order_test_clock_first
43 counters_registration_order_test_pmu_first
44 counters_clock_push_test
45 counters_provider_ext_test
46 pmu_events_check
47 counters_pmu_test
48 counters_linux_pmu_seam_test
49 counters_metric_formula_test
50 counters_metric_lookup_test
51 counters_metric_trees_test
52 counters_metric_threadless_test
53 counters_overhead
54 counters_standalone_example
55 counters_giraffe_example
56 counters_counter_limits_test
57 counters_objects_limit_test
58 counters_clock_raw_test
59 simulation_test
60 time_source_gate
61 harness_capture_test
62 harness_calibration_test
63 harness_growth_test
64 harness_percentile_test
65 harness_json_report_test
66 harness_json_schema_test
67 harness_json_load_test
68 harness_compare_test
69 harness_compare_cli_test
70 harness_cli_test
71 harness_cli_table_test
72 harness_cli_sources_test
73 harness_cli_threads_test
74 harness_cli_interleave_test
75 base_cli_matrix
76 harness_catalog_test
77 harness_gap_test
78 harness_jitter_probe_test
79 harness_jitter_probe_alloc_test
80 harness_jitter_window_test
81 harness_precheck_test
82 harness_jitter_cli_test
83 harness_noise_test
84 harness_noise_report_test
85 harness_jitter_preset_test
86 harness_topology_test
87 harness_placement_preset_test
88 harness_numa_placement_test
89 harness_cli_placement_test
90 harness_placement_report_test
91 barrier_shape
92 loop_shape
93 cli_shape
94 harness_statistics_test
95 harness_registry_test
96 harness_family_test
97 harness_capture_macro_test
98 harness_template_test
99 harness_instance_name_test
100 harness_fixture_test
101 harness_callback_test
102 harness_argument_test
103 harness_disabled_test
104 harness_time_columns_test
105 harness_cpu_leaf_test
106 harness_real_time_test
107 harness_manual_time_test
108 harness_pause_test
109 harness_execution_context_test
110 harness_context_barrier_test
111 harness_threads_test
112 harness_context_report_test
113 pause_bound_removed
114 harness_user_counters_test
115 harness_throughput_test
116 harness_label_test
117 harness_custom_stats_test
118 harness_aggregates_only_test
119 harness_complexity_test
120 harness_time_unit_test
121 harness_pass_plan_test
122 harness_pass_collect_test
123 harness_pass_report_test
124 harness_pass_report_test_identity
125 harness_hunt_plan_test
126 harness_hunt_rank_test
127 harness_hunt_rank_trap_test
128 harness_hunt_collect_test
129 harness_hunt_cli_test
130 harness_hunt_report_test
131 harness_hunt_json_test
132 harness_hunt_json_test_no_hunt
133 harness_host_record_schema_test
134 harness_host_record_test
135 harness_host_record_check_test
136 host_record_check
137 host_record_check_bogus_option
138 host_record_write
139 example_drift
140 example_drift_fixtures
141 report_shape_fixtures
142 snippet_compile
143 report_shape
144 report_shape_guide_sample
145 report_shape_percentiles_sample
146 report_shape_json_reports_sample
147 report_shape_jitter_sample
148 report_shape_numa_sample
149 report_shape_hunt_rank_sample
150 example_run
151 example_run_fixtures
```

## The two entries that stand skipped

`counters_pmu_test` and `counters_overhead` answer status 2 on this host and
carry `SKIP_RETURN_CODE 2` (`test/CMakeLists.txt:468`, `:546`), so ctest
lists them under "did not run". Their own lines name the reason: `SKIP: no
provider seeded a cpu object at this perf_event_paranoid, so no hardware
event is reachable to exercise (FR-039, SC-004)` and `SKIP: no provider
seeded a cpu object, so the counters overhead has no regime to compare
against the clock`. Both are counters entries outside this feature, they
stand skipped at the full mask too, and no mask moves them.

## The two entries that fail, and the pair the repository already fails

The same two entries failed under the mask. Each one failed the same way at
the full mask, run together over `-j 2` at the same head, which is the
control that separates the mask from the pair:

```text
0% tests passed, 2 tests failed out of 2

Label Time Summary:
quick    =   0.10 sec*proc (1 test)
slow     = 600.20 sec*proc (1 test)

Total Test time (real) = 300.20 sec

The following tests FAILED:
	 48 - counters_linux_pmu_seam_test (Failed)             quick
	 70 - harness_cli_test (Timeout)                        slow
Errors while running CTest
```

`counters_linux_pmu_seam_test` fails in 0.10 s on the claim that a vendored
row lands on the one device its Unit scope names and on no other (FR-019),
after printing the placement of 1166 rows over the ten Unit scopes this
host publishes under `/sys/bus/event_source/devices`. It reads the identity
of the host, so a mask over two cpus changes nothing it sees, and the same
entry stands recorded unreproduced on the Rocky lane job of
`specs/033-hunter-budgets/results/quickstart.md`.

`harness_cli_test` reaches the 300 s ceiling the time budget of
`test/CMakeLists.txt` gives every entry that names no ceiling of its own,
and its registration names `PROCESSORS 2` beside the comment that two
windows of 1,000 in-process runs hold one core for the whole entry, measured
at 52 minutes on a host granted the PMU.
`specs/029-hunter-core/results/corsairone-hunt-cpuset.md` records the same
entry needing 3126.26 s on a lane host and reaching the deadline ctest
applies for that reason alone.

The ceiling, and not the mask, is what the suite printed. Run outside ctest
from a scratch directory, with a wrapper deadline of 1800 s, the entry
printed `harness_cli_test: ok` and exit 0 in 606 s at the full mask, and the
same two under the mask `0,1` in 338 s. The narrow mask is the quicker of the
two readings, so an entry that needs minutes more than its ceiling on this
host fails the same way whichever mask it holds, the reading
`specs/029-hunter-core/results/corsairone-hunt-cpuset.md` reached when its
entry needed 3126.26 s against the deadline ctest applied there.


## The entries of this feature under the mask

Six entries carry this feature, and all six ran under the mask in both
readings: `harness_host_record_schema_test`, `harness_host_record_test`,
`harness_host_record_check_test`, `host_record_check`,
`host_record_check_bogus_option` and `host_record_write`. None of them
skipped, none of them failed, and the narrow run of the guide's own pattern
stands recorded beside the scenarios in
`specs/031-host-validation/results/quickstart.md`. SC-009 holds for the
feature: no entry of it sleeps, reads a clock or names a fixed cpu number,
which the greps under the last two headings show over its sources.

## Each read resolves under the root it is given

FR-031 asks every `/proc` and `/sys` read of the feature to resolve under the
standing file root of `proc_root.hpp`, and FR-035 asks the write path and the
reader to run against planted records through the injected roots. The feature
makes no such read. Over `source/harness/host_record.cpp`,
`source/harness/host_record_check.cpp`, the two headers under
`source/harness/detail/`, the two programs and `mapfile.hpp` under
`tools/hostcheck/`, and the three test units, every line carrying the text
`/proc` or `/sys` is a comment, and no line calls `procRootPath`. The library
states the rule as an invariant at
`source/harness/detail/host_record_check.hpp:23-24`. What the feature does
read stands under a root the caller names: the tree root of `--root`, which
holds `docs/schema` and the mapfile, the record root of `--records`, and the
output root of `--out`.

The planted case ran under the mask at `7615d87` over a directory the tree
does not hold, written below as `<planted>`, holding the schema, the mapfile
with its one `arch/x86/arrowlake/` directory, and the committed record:

```text
$ taskset -c 0,1 ./build-val/test/host_record_check --root <planted> --records <planted>/validation
<planted>/validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid
exit 0

$ taskset -c 0,1 ./build-val/test/host_record_write --root <planted> --out <planted>/out
host-record: GenuineIntel-6-C6-2  <planted>/out/GenuineIntel-6-C6-2.json
exit 0

$ taskset -c 0,1 ./build-val/test/host_record_check --root <planted> --records <planted>/out
<planted>/out/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid
exit 0
```

The same root with its mapfile removed answers the two refusals, each exit 2,
and the lines name the planted path, which is how the run shows where the read
went: `error: the record run cannot read
'<planted>/external/pmu-events/arch/x86/mapfile.csv'` from the reader, and
`error: the mapfile '<planted>/external/pmu-events/arch/x86/mapfile.csv'
cannot be read` from the write path. The installed-layout arm of the same rule
stands in the units, which install a layout for the machine read through the
standing seam `installTestLayout` (`source/harness/detail/internal.hpp:790`,
used at `test/source/harness_host_record_test.cpp:183` and `:200`), and those
units passed under the mask in both readings.

## A masked run takes its output root under the build directory

FR-034 asks a narrow-mask run to aim its output root under the build
directory, so the machine facts a mask yields cannot reach the committed
record. The masked target run did that at `7615d87`:

```text
$ taskset -c 0,1 cmake --build build-val --target validate
[100%] Writing and checking the host record
host-record: GenuineIntel-6-C6-2  $PWD/build-val/validation/GenuineIntel-6-C6-2.json
$PWD/build-val/validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid
[100%] Built target validate
exit 0
```

The committed record carried the digest `e9b4627ad02c79ad22328a6894587a34`
and the modified time `2026-10-30 02:08:11` before that run and the same two
after it, while `build-val/validation` held the record the run wrote. A
masked write also shows what those machine facts would have been. Aimed at
`$PWD/build-val/validation-masked`, it wrote one record of 310 bytes:

```text
host-record: GenuineIntel-6-C6-2  $PWD/build-val/validation-masked/GenuineIntel-6-C6-2.json
exit 0
{"recordVersion":"1.0","cpu":{...},"machine":{"packages":1,"l3Groups":1,"cores":2,
"threads":2,"mask":"0-1","l3":[{"ordinal":0,"cpus":"0-1"}],
"nodes":[{"number":0,"cpus":"0-1"}]},"evidence":[]}
```

Two cores, two threads and the mask `0-1`, five bytes fewer than the 315 of
the committed record with its 20 cores and its mask `0-19`. The committed
record kept its digest through that run too, which is the clause FR-034 asks
for: the root under the build directory is what keeps a masked run out of it.

## No file-root seam joins the tree, and no cpu number stands in the sources

`source/harness/detail/proc_root.hpp` stands alone as the definition of the
file root, and the four files that read it, `source/harness/jitter_probe.cpp`,
`source/harness/noise_tracer.cpp`, `source/harness/detail/noise_tracer.hpp`
and `test/source/harness_precheck_test.cpp`, all predate this branch. No file
this feature adds names `procRootPath`, so the second clause of FR-031 holds:
no new seam joined the tree.

The same shape of grep carries SC-009 and FR-034. Over the three test units no
line carries `sleep_for`, `sleep_until`, `steady_clock`, `system_clock`,
`::now()` or a cpu number, and the same holds over the two programs, the two
library sources, the two headers and `cmake/validate.cmake`. The entries of
this feature therefore answer the same way on two cpus and on 20, which is
what the scenario asks of them.

## What stands open

The 2-cpu host of the second arm of SC-009 stands out of reach for this lane,
the way the `NPS2` and `NPS4` boots stand open in
`specs/035-numa-placement/results/quickstart.md`. The `taskset -c 0,1` mask is
the substitute the spec names for it (`specs/031-host-validation/spec.md:537`,
`:850`), and it is the reading this record carries. The pair above stands
outside this feature: no commit of this branch touches the counters seam the
first entry reads, and no commit of it moves the ceiling the second entry
reaches.
