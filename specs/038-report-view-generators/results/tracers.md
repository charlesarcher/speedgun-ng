# The tracer reports' views on thrawn

- Reports: `reports/osnoise.json` and `reports/timerlat.json`, the
  documents the tracer runs of specs/036 wrote: `bmTouch/threads:2`,
  one repetition row and the aggregate row, schema `1.7`, the time
  unit nanoseconds. The documents stand recorded beside this record
  the way the run left them.
- Run: 2026-10-29, the lane host's standing boot, the release build
  at `build/`.
- Script: `run_views.sh`, the commands below at exit 0 each.

## The commands

```
$ build/tools/report_view specs/038-report-view-generators/results/reports/osnoise.json \
     --benchmark='bmTouch/threads:2' --metric=time --view=histogram
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/osnoise.json \
     --benchmark='bmTouch/threads:2' --metric=time --view=plot
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/osnoise.json \
     --benchmark='bmTouch/threads:2' --metric=time --view=page
exit 0

$ build/tools/report_view specs/038-report-view-generators/results/reports/osnoise.json \
     --benchmark='bmTouch/threads:2' --metric=time --view=text
exit 0
```

The four timerlat commands stand the same shape over
`reports/timerlat.json`, each at exit 0.

## The rendered views

The recorded reports stand one repetition row each, so the histogram
names the standing disclosure and the plot renders the single point
the run recorded:

```
$ cat views/osnoise-histogram.txt
renderHistogram: the histogram of the column 'time' of 'bmTouch/threads:2' stands not available: the series carries the sample count 1
```

- `views/osnoise-plot.svg`: the series of the one recorded value
  0.537718 ns, the axes naming the row and the column.
- `views/osnoise-page.html`: the page carrying the run's identity and
  the plot inline, the histogram's disclosure text standing in the
  histogram's place (FR-012, the clarify 1/Q4 ruling).
- `views/osnoise-text.txt`: the terminal text, the column line, the
  histogram part's disclosure line and the plot part's value line:

```
time
renderHistogram: the histogram of the column 'time' of 'bmTouch/threads:2' stands not available: the series carries the sample count 1
0.537718 ns
```

The timerlat views stand the same shape over its recorded value
0.528353 ns (`views/timerlat-plot.svg`, `views/timerlat-text.txt`,
`views/timerlat-page.html`, `views/timerlat-histogram.txt`). The
views render the figures the runs recorded and invent none (R-4):
a one-repetition run names no distribution shape, and the disclosure
says so.
