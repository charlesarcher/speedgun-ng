# Raw event-code counter columns (specs/051-raw-events)

`--counter=device/rNNN` now names a perf event code directly: a leaf whose
name is a lower-case `r` followed by one to sixteen hex digits resolves
through the PMU device of its path, encodes `config=<word>`, and lands as
its own column in text and JSON output. Refusals carry the standing
counters wording, the identity checkpoint prints one line per bad address
and stops the run at status 2, and a device without a PMU answers the
standing no-object line. The listing routes keep their shapes: `--list`
prints no address line for such an entry, and the hunter route refuses
raw spellings as it did before this lane.

## Host acceptance (AMD, read at run time)

Host: AMD Ryzen 9 9950X3D, family 26, model 68, vendor AuthenticAMD,
`/sys/devices/cpu/type` 4, and the standing alias file reads
`event=0xc0` for instructions.

```
--iterations=1 --counter=cpu/instructions --counter=cpu/rc0
cpu/instructions=  773.000     cpu/rc0=  773.000
```

Both columns carry positive figures, and both measure config word `0xc0`
on this host, the word `rc0` spells (SC-001). The JSON read answers schema
`1.16` with `cpu/rc0` in `metricLabels` and in the row labels. The four
refusal reads answer status 2 with the no-object line, the 16-digit width
line, the width line again ahead of the device lookup, and `--list` exits
0 with no address line. The option-file run carries the disclosure line
and the `cpu/rc0=  657.000` column. The `--catalog` listing answers 467
lines, equal to the base figure. The full log stands in
`specs/051-raw-events/results/acceptance-host.md`; the summary stands in
`results/host-acceptance.md`.

## Gates at the lane head (one row per leg, copied from results/gates.md)

| leg | command shape | verdict at the lane head |
| --- | --- | --- |
| suite (ci-rocky) | configure, build, `ctest` | exit 0: 192 registered, 191 executed, 1 skipped by the topdown host guard |
| address and undefined sanitizers (clang++) | `CXX=clang++`, own tree | exit 0: 187 registered, 186 executed, 1 skipped, zero sanitizer lines; an earlier head held a recorded load-contention read of `counters_overhead` with five green isolation runs, superseded by this green reading |
| thread sanitizer (clang++) | `CXX=clang++`, own tree | exit 0: 188 registered, 187 executed, 1 skipped, zero ThreadSanitizer lines; an earlier head held a recorded contention stall of `harness_execution_context_test` at load average 20.6 with isolation reads green, superseded by this green reading |
| coverage | own tree, `ctest -j 1`, gate | lines 100.0% (18930 of 18930 host; the CI job instruments 18945); branches 9949 of 9971 on the host, a 22-row shortfall against the recorded allowance of 21 with zero lane rows in it; the CI union answers 25 at this head against 26 at the branch base, one row better through the lane; marker count 84, at the ceiling, none added |
| static analysis | `ci-ubuntu` preset, separate tree, 326 units | naming check clean under `WarningsAsErrors`; three pre-existing error-severity findings reproduce verbatim, all outside the lane path set |
| format, spell, prose range, prose tree, drift, time-source gate | the five fast legs | exit 0 each, zero findings |

The coverage branch figure deserves its arithmetic. The host read at
the lane head is 100.0% lines with a 22-row branch shortfall against
the recorded allowance of 21, and the census attributes every row: the
lane contributes none. The `coverage` job of this pull request reads its own
union, where every entry runs to its end; the recorded union at the
base of this branch answers 26 rows (9874 of 9900, run 42753235724, with the
topdown entry identity-skipped on that runner too), which stands above
the same allowance and answers red on master as well. An earlier
projection in these records expected the CI union at 15 to 18 and
green; the recorded base run disproves that premise, and the record
carries the measured base figure as the premise. The
lane adds no dark rows at either reading, its union count answers one
row better at the CI read, and the marker count stands at
84 with none added, and the allowance constant against the current
tree is an owner ruling outside this lane, named with its census in
`results/gates.md`.

## Versions (SC-006)

Version 0.35.0, report schema 1.16, harness SOVERSION 22, counters
SOVERSION 8 kept. This lane moves no public header, so the counters
number stands where the previous lane left it; the version rows also
moved through the two test carriers that pin them, named as carriers in
`results/commits.md`.

## Stacking

This branch stands on `0fce6651`, the head of the open pull request for
specs/049. The diff of this pull request therefore shows both changes
until that one lands; the arrangement the spec records as an assumption.

## Records

`specs/051-raw-events/` carries the spec, plan, tasks, the red-unit
record with its flip addendum, base readings, the clarify and analyze
passes, numbers, commits and built heads, gates with all six re-reads
and the census, and the two acceptance files.
