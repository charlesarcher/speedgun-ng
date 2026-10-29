# The tracer evidence plan

The acceptance runs stand on thrawn, the Threadripper 9960X lane
host, one record per engine: `osnoise.md` and `timerlat.md`. The
tracer writes need the rights a root shell carries, so the runs
stand behind `sudo`; the feature itself never asks for them and a
run without them stands the `not available` disclosure (FR-020, D-7).

## The CCD names

The `--topology` listing the same host prints names the L3 groups
the tracer cpus stand on; on this host the four L3 groups map one to
one onto the four CCDs of the 9960X:

```
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
node 0: 0-47
```

A record names the CCD beside each figure as `CCD<n>` with the L3
group the listing printed, `CCD0` standing `l3 0: 0-5,24-29`.

## The command set per engine

The commands run from the repository root on the built tree; the
record carries each command, its exit code and the pre-check block
it printed. The tracer writes run behind `sudo`:

```
./build/example/benchmark_example --topology
sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=osnoise --json=/tmp/osnoise.json
ctest --test-dir build -R harness_noise_test
```

The timerlat record runs the same set with the engine word and the
document path changed:

```
sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=timerlat --json=/tmp/timerlat.json
```

One probe cpu per L3 group the plans touch stands the tracer's
candidate set, the default the option carries (FR-008).

## The count-file re-confirmation

The re-confirmation of the per-cpu count-file name and line format
rides the run: the record states the tree the run read beside the
figures, and a mismatch between the tree and the format the parser
pins stands a question for the pull request (R-03, research.md R-3).

## The record format

One file per engine, named for it, beside this plan:

- the engine, the run date, and the `uname -r` line;
- each command, its exit code, and the pre-check block it printed;
- the per-CCD figure table, the CCD named beside each figure;
- the count-file re-confirmation note, the confirmed facts beside
  the figures, and the question a mismatch stands as.
