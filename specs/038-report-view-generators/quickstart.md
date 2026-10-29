# Report views: the evidence quickstart

The lane's evidence stands one record per run, the way
specs/036-osnoise-timerlat-tracers recorded its tracer runs. The
script that drove every run stands at
[results/run_views.sh](./results/run_views.sh); each record names the
commands, the exit codes, the report rendered and the views
produced.

| The question | The record | The verdict |
| --- | --- | --- |
| The placement report of specs/035 renders its figures | [results/placement.md](./results/placement.md) | every command exits 0 |
| The tracer reports of specs/036 render their figures | [results/tracers.md](./results/tracers.md) | every command exits 0, the one-repetition reports naming the disclosure |

The rendered views stand under
[results/views/](./results/views/), the report documents under
[results/reports/](./results/reports/), so a reader re-renders every
figure with the driver and compares the bytes.

## Re-rendering

```
$ cmake --preset=ci-rocky && cmake --build build
$ specs/038-report-view-generators/results/run_views.sh
```

The script prints one exit status per command; every status stands 0
on the lane host's recorded run.

## The feature docs

- The main page's section: `README.md`, `Report views`.
- The public surface: `include/speedgun-ng/views.hpp`.
- The contracts: [contracts/views-api.md](./contracts/views-api.md).
