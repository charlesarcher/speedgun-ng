# The placement report's views on thrawn

- Report: `reports/placement.json`, the document the placement run of
  specs/035 wrote: `bmSharded/threads:2`, five repetitions and the
  aggregate row, schema `1.8`, the time unit nanoseconds.
- Run: 2026-10-29, the lane host's standing boot, the release build
  at `build/`.
- Script: `run_views.sh`, the commands below at exit 0 each.

## The commands

```
$ ./build/example/docs_numa_placement --placement=node --repetitions=5 \
     --json=specs/038-report-view-generators/results/reports/placement.json
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/placement.json \
     --benchmark='bmSharded/threads:2' --metric=time --view=histogram
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/placement.json \
     --benchmark='bmSharded/threads:2' --metric=time --view=plot
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/placement.json \
     --benchmark='bmSharded/threads:2' --metric=time --view=page
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/placement.json \
     --benchmark='bmSharded/threads:2' --metric=time --view=text
exit 0
```

The run's console block:

```
speedgun-ng 0.25.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSharded/threads:2      iterations=3742811070  real time/iter (ns)=    0.243  cpu time/iter (ns)=    0.192  overhead floor (ns)=  100.000  endpoint samples=4
bmSharded/threads:2       AGGREGATE  primary time/iter (ns) mean=    0.189 median=    0.188 sd=    0.002 cv=  0.0103 min=    0.187 max=    0.192 n=5
  contexts=2  ctx0=SPIN/0@0@0/applied  ctx1=SPIN/1@0@0/applied
```

The console prints the primary time in the unit the report records;
the values the views render stand the stored nanosecond scale, the
five repetition values 0.243095 to 0.285961 ns.

## The rendered views

- `views/placement-histogram.svg`: the histogram of the primary time
  over the five repetitions, twenty bins covering the recorded range
  0.243095 to 0.285961 ns with the last bin closed on the maximum,
  the counts 1, 1 and 2 in the occupied bins and 2 in the last.
- `views/placement-plot.svg`: the series of the five recorded values
  in document order, the axes naming the row and the column.
- `views/placement-page.html`: the page carrying the run's identity
  (the `speedgun` version `0.25.0`, the `schemaVersion` `1.8`, the
  benchmark and the column) and the two figures inline.
- `views/placement-text.txt`: the terminal text of both figures, the
  header line naming the column, the occupied bins and the five
  value lines:

```
time
0.243095-0.245239 ns: 1 ##########
0.253812-0.255955 ns: 1 ##########
0.281674-0.283817 ns: 2 ####################
0.283817-0.285961 ns: 1 ##########
0.283611 ns
0.283262 ns
0.285961 ns
0.253841 ns
0.243095 ns
```

The empty bins stand their zero-count lines in the recorded view;
the excerpt above names the occupied ones. The recorded values stand
the report's own: the views invent no figure the run did not measure
(R-4).
