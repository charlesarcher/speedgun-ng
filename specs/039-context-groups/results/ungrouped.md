# The ungrouped comparison on gary

One fixture run on this branch, the same fixture run on `master`, the
console compared line by line and the two documents compared key by key
(FR-033, SC-006, FR-038). The run states no placement and names no layout
file, so it carries no partition: the shape FR-033 compares against
master.

- Fixture: `bmSharded` of `example/docs/threads.cpp`, registered with
  `.threads(4)` and no placement, so `--threads` settles the context count
  and no preset stands in effect. `--iterations=1` keeps the run to one
  iteration and skips calibration, and `--no-percentiles` leaves the
  percentile capture off. Eight contexts stand on both sides.
- Branch side, a host run: `build-w7a`, this branch's release tree, version
  `0.26.0`, schema `1.10`.
- Master side, a host run: `/tmp/sg039-master/build`, the worktree of
  `bea6af5b` built outside this checkout, version `0.25.0`, schema `1.8`.
- Driver: `specs/039-context-groups/results/run_groups.sh`, the fifth of
  the five runs it drives. The whole pass exits 0 and prints
  `run_groups: 5 runs, every reading matched`.
- Host shape: two L3 groups, `0-7,16-23` and `8-15,24-31`, 16 cores, 32
  threads, one memory node. No preset stands in effect, so the machine line
  stays out of both captures and the host's shape reaches no compared byte.
- Read on 2026-10-30, gary's standing boot.

## The branch run (host run)

The command and its exit code, as the run's transcript holds them:

```
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/ungrouped.branch.json
exit 0
the bytes stand in specs/039-context-groups/results/reports/ungrouped.branch.console.txt
$ grep -m1 '^  contexts=' specs/039-context-groups/results/reports/ungrouped.branch.console.txt
the disclosure line stands in specs/039-context-groups/results/reports/ungrouped.branch.disclosure.txt
the document carries no group key (FR-033)
```

The console bytes stand in `reports/ungrouped.branch.console.txt`:

```
speedgun-ng 0.26.0 build=Release
bmSharded/threads:8      iterations=         8  real time/iter (ns)= 1097.500  cpu time/iter (ns)=  727.500  overhead floor (ns)=   90.000  endpoint samples=16
  contexts=8  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN
```

## The master run (host run)

The far side runs the same command line from the same working directory
against its own binary:

```
$ /tmp/sg039-master/build/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/ungrouped.master.json
exit 0
the bytes stand in specs/039-context-groups/results/reports/ungrouped.master.console.txt
$ grep -m1 '^  contexts=' specs/039-context-groups/results/reports/ungrouped.master.console.txt
the disclosure line stands in specs/039-context-groups/results/reports/ungrouped.master.disclosure.txt
$ diff -u specs/039-context-groups/results/reports/ungrouped.branch.disclosure.txt specs/039-context-groups/results/reports/ungrouped.master.disclosure.txt
the two disclosure lines stand byte-identical (FR-033)
```

The driver closes this run with `ungrouped: 1 run`, the count of its five
runs; the transcript above holds both commands. The console bytes stand in
`reports/ungrouped.master.console.txt`:

```
speedgun-ng 0.25.0 build=Release
bmSharded/threads:8      iterations=         8  real time/iter (ns)=  807.500  cpu time/iter (ns)=  591.250  overhead floor (ns)=   90.000  endpoint samples=16
  contexts=8  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN
```

## The compared region

The driver cuts the context disclosure line out of each capture and diffs
the two, the region FR-033 states (SC-006). The byte reads over the two cut
lines:

```
$ cmp specs/039-context-groups/results/reports/ungrouped.branch.disclosure.txt specs/039-context-groups/results/reports/ungrouped.master.disclosure.txt
$ echo $?
0
$ wc -c specs/039-context-groups/results/reports/ungrouped.branch.disclosure.txt specs/039-context-groups/results/reports/ungrouped.master.disclosure.txt specs/039-context-groups/results/reports/ungrouped.disclosure.diff
157 specs/039-context-groups/results/reports/ungrouped.branch.disclosure.txt
157 specs/039-context-groups/results/reports/ungrouped.master.disclosure.txt
  0 specs/039-context-groups/results/reports/ungrouped.disclosure.diff
314 total
```

One line of 157 bytes answers on each side, and the diff the driver wrote
stands empty at 0 bytes: 0 differing bytes on the compared region. The
bytes are these, equal in both captures:

```
  contexts=8  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN
```

The region holds no stamp of either kind. The version sits on the header
line above it, and the schema stamp sits inside the document, so these
bytes stand equal with both numbers moved (FR-033, SC-006).

## The lines that differ, and why

The whole console block compares as three lines:

```
$ diff -u --label ungrouped.master.console.txt --label ungrouped.branch.console.txt specs/039-context-groups/results/reports/ungrouped.master.console.txt specs/039-context-groups/results/reports/ungrouped.branch.console.txt
--- ungrouped.master.console.txt
+++ ungrouped.branch.console.txt
@@ -1,3 +1,3 @@
-speedgun-ng 0.25.0 build=Release
-bmSharded/threads:8      iterations=         8  real time/iter (ns)=  807.500  cpu time/iter (ns)=  591.250  overhead floor (ns)=   90.000  endpoint samples=16
+speedgun-ng 0.26.0 build=Release
+bmSharded/threads:8      iterations=         8  real time/iter (ns)= 1097.500  cpu time/iter (ns)=  727.500  overhead floor (ns)=   90.000  endpoint samples=16
   contexts=8  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN
```

- Line 1 differs at the version: `0.25.0` on master over `0.26.0` on this
  branch, the number FR-032 lands.
- Line 2 differs at the two measured figures, `real time/iter (ns)` and
  `cpu time/iter (ns)`. Each run measures its own, and the row's shape
  stands equal: the name field, the `iterations=` field at 8 and the
  `endpoint samples=` field at 16 match byte for byte.
- Line 3, the disclosure line, holds no difference. No group segment stands
  on it, and each context keeps its standing `SPIN/NO_PIN` field (FR-033).

Mask the header's version and the row's three `(ns)=` figures, and the two
captures answer the same bytes:

```
$ diff -u --label master.masked --label branch.masked <(sed -E 's#^speedgun-ng [0-9.]+#speedgun-ng VERSION#; s#\(ns\)= *[0-9.]+#(ns)= FIGURE#g' specs/039-context-groups/results/reports/ungrouped.master.console.txt) <(sed -E 's#^speedgun-ng [0-9.]+#speedgun-ng VERSION#; s#\(ns\)= *[0-9.]+#(ns)= FIGURE#g' specs/039-context-groups/results/reports/ungrouped.branch.console.txt)
$ echo $?
0
```

## The document read

Neither document names a group, and `grep -c` answers status 1 with a count
of 0 on each file (FR-029, FR-033):

```
$ grep -c '"group"' specs/039-context-groups/results/reports/ungrouped.branch.json specs/039-context-groups/results/reports/ungrouped.master.json
specs/039-context-groups/results/reports/ungrouped.branch.json:0
specs/039-context-groups/results/reports/ungrouped.master.json:0
$ echo $?
1
```

Both documents hold the same key sets, and each of the eight context
records holds the three standing fields: `waitMode`, `placement` and
`applied`. The context array itself answers the same bytes on both sides:

```
$ cmp <(grep -o '"contexts":[^]]*\]' specs/039-context-groups/results/reports/ungrouped.branch.json) <(grep -o '"contexts":[^]]*\]' specs/039-context-groups/results/reports/ungrouped.master.json)
$ echo $?
0
```

Each of the eight records holds these bytes, and none carries a fourth
field:

```
{"waitMode":"SPIN","placement":"NO_PIN","applied":false}
```

The sorted comparison names four differing leaves over the whole document:

```
$ diff -u --label ungrouped.master.json.sorted --label ungrouped.branch.json.sorted <(python3 -m json.tool --sort-keys specs/039-context-groups/results/reports/ungrouped.master.json) <(python3 -m json.tool --sort-keys specs/039-context-groups/results/reports/ungrouped.branch.json)
--- ungrouped.master.json.sorted
+++ ungrouped.branch.json.sorted
@@ -64,7 +64,7 @@
                     "chunkIterations": 0,
                     "counterDisclosure": null,
                     "counters": [],
-                    "cpuTimePerIterationNs": 591.25,
+                    "cpuTimePerIterationNs": 727.5,
                     "endpointSamples": 16,
                     "fit": null,
                     "iterations": 8,
@@ -76,7 +76,7 @@
                     "pauseSamplesFolded": false,
                     "percentiles": [],
                     "timeAvailability": "COUNTABLE",
-                    "timePerIterationNs": 807.5,
+                    "timePerIterationNs": 1097.5,
                     "timeStats": null
                 }
             ],
@@ -86,9 +86,9 @@
             "timeUnit": "NANOSECONDS"
         }
     ],
-    "schemaVersion": "1.8",
+    "schemaVersion": "1.10",
     "speedgun": {
         "buildType": "Release",
-        "version": "0.25.0"
+        "version": "0.26.0"
     }
 }
```

Two leaves are the stamps FR-032 lands, `schemaVersion` at `1.10` over
`1.8` and `speedgun.version` at `0.26.0` over `0.25.0` (FR-030, FR-032).
Two leaves are the measured figures of the one row. No leaf stands on one
side and misses on the other, so an ungrouped run writes the document
master writes (FR-029, FR-033).

## The seam run

FR-038 names a seam run as the `ctest` invocation whose test installs a
shape no host on the lane states. This one is
`ctest --test-dir build-w7a -R '^harness_group_report_test$' --verbose`,
exit 0. Its two ungrouped scenarios build the results inside the scenario,
print them through `sg::detail::printResult`, and compare against master's
bytes held as literals: bytes a run on this branch cannot produce for
itself.

```
93: harness_group_report_test ungroupedBytes: pass
93: harness_group_report_test documentNoKey: pass
93: harness_group_report_test: 12 scenarios, 0 fail
1/1 Test #93: harness_group_report_test ........   Passed    0.00 sec
```

The two checks read as the test spells them:

```
an ungrouped run's line keeps the bytes master prints for each of the three shapes (FR-033)
an ungrouped document carries no group key (FR-029, FR-033)
```

`ungroupedBytes` prints three shapes that carry no group: a pin that
landed beside a pin that did not, no pin at all, and a pinned pair. Each
printed line compares against the bytes master answers, held in the test as
literals. `documentNoKey` writes the document of a run that carries no
partition, checks that its record keeps the bytes it wrote before the key,
and hands the document to the schema (FR-030, FR-031, D-12).

## The artifacts

The driver writes eight files for this run. Its transcript stands
untracked, and the seven files below stand tracked beside this record:

| File | Bytes it holds |
| --- | --- |
| `ungrouped.branch.console.txt` | the branch console block |
| `ungrouped.branch.disclosure.txt` | the branch disclosure line |
| `ungrouped.branch.json` | the branch document |
| `ungrouped.master.console.txt` | the master console block |
| `ungrouped.master.disclosure.txt` | the master disclosure line |
| `ungrouped.master.json` | the master document |
| `ungrouped.disclosure.diff` | the region diff, empty at 0 bytes |

The transcript `ungrouped.log` stands untracked: `.gitignore` names
`*.log`, and the driver rewrites it on every pass.

## The re-run

Both trees, then the driver. The master worktree stands outside this
checkout so no gate reads it as a side build directory of the lane, and a
fresh worktree needs its submodules init:

```
git worktree add /tmp/sg039-master bea6af5b
cd /tmp/sg039-master
git -c protocol.file.allow=always submodule update --init external/hwloc external/simdjson external/yaml-cpp external/hdrhistogram_c external/zlib external/quill external/pmu-events
cmake --preset=ci-rocky -B build && cmake --build build -j 10
cd /home/archerc/code/speedgun-ng
cmake --preset=ci-rocky -B build-w7a && cmake --build build-w7a
bash specs/039-context-groups/results/run_groups.sh --build build-w7a --master-build /tmp/sg039-master/build
```

The driver rewrites every artifact named above, so the quoted bytes come
back with each run's own figures on the measurement row.
