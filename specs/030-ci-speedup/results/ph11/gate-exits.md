# Phase 11 Coverage Gate Exits

**Surface**: the exit codes of `tools/dbc/coverage_gate.sh` and the line it
prints when the tool, and not the suite, is what failed.
**Head sha**: 4e71b82 with the exit-3 block of this task in the working tree
**Run id**: local
**Command**: the four lines under The exits, run from the repository root
**Numbers**: the exit status and the message beside each case

The call to `lcov --branch-coverage --summary` carried `|| true`, so the status
of the tool never reached the script. A broken `lcov` printed its own failure,
the gate found no `100.0%` line in that output and exited 1 with `line coverage
is not 100%`, naming a ratio the tool never measured. This repository has that
history: a red coverage job that turned out to be an `lcov` parse abort. The
captured status now splits the two cases (FR-043, P-12).

| Case | Input | Message | Exit |
| --- | --- | --- | --- |
| no argument | `bash tools/dbc/coverage_gate.sh` | `usage: coverage_gate.sh <coverage.info>` | 2 |
| no tool | `LCOV= PATH=/usr/bin:/bin bash tools/dbc/coverage_gate.sh <info>` | `lcov is required` | 2 |
| tool refusal, real | `/tmp/covg/short.info` through `/home/archerc/.local/bin/lcov` | `coverage-gate: lcov failed to read /tmp/covg/short.info (exit 1)` | 3 |
| tool refusal, real | `/tmp/covg/broken.info` through the same `lcov` | `coverage-gate: lcov failed to read /tmp/covg/broken.info (exit 1)` | 3 |
| measured shortfall | `/tmp/covg/short.info` through a substitute that prints the measure lines | `coverage-gate: line coverage is not 100%` | 1 |
| tool exits 2, no measure | a substitute that prints nothing and exits 2 | `coverage-gate: lcov failed to read /tmp/covg/short.info (exit 2)` | 3 |

The two refusal rows are the tool refusing for real, on this host, on two
different traces: `lcov` here is a Perl install whose `XString` module was built
against a different Perl API, so it aborts before reading anything.

```text
Perl API version v5.42.0 of XString.c does not match v5.44.0
coverage-gate: lcov failed to read /tmp/covg/short.info (exit 1)
```

That is the exact shape the new exit exists for: a nonzero tool with no
shortfall line in its own report. Before the change the same input exited 1 with
a coverage claim.

## What is not shown here

The exit-0 path has no local fixture. The task forbids a fixture that
fabricates a passing trace, and this host has no working `lcov`: the second
install, `/tmp/lcov-2.3/bin/lcov`, aborts on a missing `Capture::Tiny`. The
shortfall row above therefore runs a substitute `lcov` that prints a real
measure line for the short trace, which is the shape the gate reads; the pass
path and the real-tool shortfall path come from the coverage lane of CI, where
`lcov` runs inside the container, and T055 and T056 carry those readings.

`bash -n tools/dbc/coverage_gate.sh` exits 0 and the file carries no Principle
XI finding: `python3 tools/prose/prose_gate.py --check prose --mode tree
--paths tools/dbc/coverage_gate.sh` reads 0 findings.
