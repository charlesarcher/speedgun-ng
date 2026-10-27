# Research: Fix Merged Features

**Branch**: `032-fix-merged-features` | **Date**: 2026-10-27

Phase 0 output. The spec carries no `NEEDS CLARIFICATION` marker: the
request fixes every open decision (the spec's Principle X.1 note), and
`/speckit.clarify` asked no question. The research tasks here re-measure
the spec's baseline claims against the current tree, so each fix lands
on evidence and not on a stale reference. Every measurement below was
taken 2026-10-27 on master `500704b`, the hash the branch cuts from.

## R-01: the report schema carries two top-level `x-order` keys

**Decision**: keep the key the writer order names, delete the other.

**Rationale**: `docs/schema/report-v1.json:7` and `:8` carry two
`x-order` keys, measured today:

- `:7`: `["schemaVersion", "speedgun", "run", "jitter", "benchmarks"]`
- `:8`: `["schemaVersion", "speedgun", "run", "benchmarks", "hunt"]`

`source/harness/json_report.cpp:590` emits `jitter` before `benchmarks`
and `:676` emits `hunt` after it, so the merged order
`["schemaVersion", "speedgun", "run", "jitter", "benchmarks", "hunt"]`
of FR-017 matches the writer. The schema version stands at `1.5`
(`json_report.cpp:36`, `kSchemaVersion = "1.5"`), which confirms PC-2.

**Alternatives considered**: dropping `:7` instead of `:8` (rejected:
it omits `hunt` and disagrees with the writer order the spec fixes);
leaving both keys (rejected: a duplicate key is the defect FR-017
closes, and the test it adds must fail on exactly that shape).

## R-02: the README cap stands

**Decision**: no cap change; the trim is confirmed (FR-008).

**Rationale**: `README.md` measures 835 lines, inside the 835 cap, and
`PAGE_LINE_CAP = 835` stands once at `tools/docs/example_drift.py:192`
(plus its two uses at `:607` and `:608`). PC-4 holds.

## R-03: the online-set seam stands, and the fallback is the fix target

**Decision**: rewrite the fallback body of `onlineCpuSet()` to answer
the process affinity mask (FR-021, D-02).

**Rationale**: `source/harness/topology.cpp:327` holds
`installTestOnlineCpus` and `onlineCpuSet()` stands beside it; the
unreadable-line fallback at `topology.cpp:358` to `:365` fills the set
by index from `onlineCpuCount()`. Both readers confirm:
`execution_context.cpp:655` and `jitter_probe.cpp:711` read
`onlineCpuSet()`. The fallback body, the reserve, the loop and the push
are the four `--omit-lines` patterns `cmake/coverage.cmake:94` to `:98`
keep alive today, which is why FR-021 is the fix that lets FR-026
re-count the keep-set.

**Alternatives considered**: filling the set from a hardware probe
(rejected: D-02 names the affinity mask, and a probe is a second source
of truth); keeping the count fallback with a test (rejected: a count is
forbidden as the fallback source, D-02).

## R-04: the master defect texts stand as the spec measures them

**Decision**: each defect fixes at the measured site.

**Rationale**: re-measured today:

- `source/harness/cli.cpp:776` to `:777`: `batchProbed` reads
  `options.jitterProbe.has_value() && familyProbes.empty()`, the arm
  FR-027 covers or deletes.
- `source/harness/compare.cpp:1906` to `:1907`: the compare row prints
  `entry.name` and `entry.ruleText` raw, the site FR-030 quotes with
  `detail::quotedText`.
- `source/harness/cli.cpp:805` to `:818`: the idle preset prints one
  line and returns 2 when no cpu is free, the FR-017 fact check of
  SC-007.

**Alternatives considered**: none; the sites are the spec's own
measurements, confirmed.

## R-05: the coverage keep-set and the named comments stand

**Decision**: keep the keep-set in force; rewrite the two comments
(FR-025); re-count after FR-021 lands (FR-026).

**Rationale**: `cmake/coverage.cmake:92` to `:98` hold the five
`--omit-lines` patterns and `tools/dbc/coverage_gate.sh:60` holds
`allowance=19`, as PC-8 measures. The comments at `cmake/coverage.cmake:57`
and `:85` to `:86` carry a person's name and a decision record
(`Charles`, `D-16 in specs/022-metric-formulas/plan.md`), the targets
FR-025 rewrites. `:82` names the `brace` rule as a known gcc record,
the shape D-13's proof works in.

**Alternatives considered**: removing the stale patterns now
(rejected: D-12 says a pattern moves only after the pull-request
comment carries the approval); rewriting the comments together with a
filter change (rejected: FR-025 says no other change rides the commit).

## R-06: the `SkippedKind` enum and the ABI lane

**Decision**: add a value only when an item needs one, then move the
version to `0.21.0` and the harness `SOVERSION` to `13` (FR-001,
FR-036).

**Rationale**: `include/speedgun-ng/hunt.hpp:158` holds the enum with
`PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`, `GAP`
(measured today). The request fixes the conditional: `0.20.1` with no
public-header edit, `0.21.0` with the value. The archive `SOVERSION`
stays `4` at `CMakeLists.txt:80` and the harness `SOVERSION` stands at
`:298` (`12` today).

**Alternatives considered**: adding the value unconditionally
(rejected: FR-003 adds no row and D-09 adds a value only on need); a
patch-level bump with the header edit (rejected: FR-001 fixes the
minor bump for the ABI change).

## R-07: the scans, and one re-measurement that disagrees

**Decision**: treat the `online count` comment matches as part of
FR-021's commit; no scan item reopens.

**Rationale**: the `setenv|GCOV_PREFIX|sgcov` scan over `test/source`
returns nothing today, which confirms FR-012. The scan
`online (cpu )?count`, case-insensitive, over `specs/027-jitter-probe`,
`include/` and `source/harness` returns nothing in Baseline B. The
re-measurement finds three comment matches: `internal.hpp:742`,
`internal.hpp:755` and `topology.cpp:343`. All three describe the count
seam beside the fallback that FR-021 rewrites, so the fix commit that
changes the fallback body updates the comments that describe it, and
FR-014's scan returns nothing after the fix. No extra item opens.

**Alternatives considered**: a separate comment-rewrite item
(rejected: the comments describe the code the fix changes, and
leaving them contradicts the new behavior in the same commit).

## R-08: the preconditions hold on master

**Decision**: group B closes with recorded evidence, no code moves.

**Rationale**: each hash the spec names resolves on master today with
the subject the spec quotes: `7d8dfdd`, `658af81`, `1c47e66`,
`d7e22c2`, `cd2f2f1`, `f507b8f`, `502d42b`, `42ba692`, `e781eda`
(the debug print FR-022 removes), `4f701ae` (the PR 47 merge). The
twelve reverts of PC-6 resolve by `git log --grep` on their reverted
hashes at implementation time, each named in the record (FR-011).

**Alternatives considered**: none.

## R-09: the hunt samples name an older build

**Decision**: re-record the two samples from the current build
(FR-018).

**Rationale**: `docs/pages/harness.md:1520` and
`example/docs/hunt_rank.expected:2` carry `speedgun-ng 0.19.0
build=Release`, and the project version is `0.20.0`
(`CMakeLists.txt:15`), so the samples teach a version the reader does
not run. The sweep of FR-018 then checks every option, key and exit
status of the three features' docs against the code.

**Alternatives considered**: re-recording every sample in the page
(rejected: FR-018 says every other sample stays unchanged).

## R-10: the coverage compiler

**Decision**: name the compiler the coverage job uses from the run of
the pushed head, then re-take the zero list from it (FR-031).

**Rationale**: the coverage job runs `cmake --preset=ci-coverage` on
`ubuntu-26.04` and overrides no compiler, so it builds with the
runner's default g++; the preset carries no `CMAKE_CXX_COMPILER` and
`coverage.cmake` names gcc in its `brace` tool-rule comment. The job
records the exact version in the run (`.ccache-cxx-version`), and
FR-031 names it in the spec at implementation time, before the zero
list is taken. The `_GLIBCXX_ASSERTIONS` note in the preset
(`ci-coverage` drops the flag because it adds standard-library branches
no project test can reach) is the standing shape D-13's proof works in:
a side the compiler proves is exception-cleanup only.

**Alternatives considered**: pinning the compiler in the preset
(rejected: a CI file change needs approval, FR-059, and the request
asks for none).

## R-11: the CI-run reading needs a token first

**Decision**: request the token, read the full run `40438223916`,
then start the work (FR-064).

**Rationale**: `gh` carries no token in this session (PC-11), so the
job states stand on the request's record until the run is read. Each
check item confirms against the run before it closes.

**Alternatives considered**: skipping the read (rejected: FR-064
binds the read before the work starts).

## R-12: the precedence fix rides the existing parse points

**Decision**: implement D-01 at the parse points the option table
already has; no new row, no new value (FR-003).

**Rationale**: spec 027's contract fixes the parse points: the
command-line value parses in `speedgunMain` before the expansion, the
option-file value parses where it settles, and the family call carries
its own probe. The defect is the precedence combination; the
parse stands present, so the fix is the comparison order plus one test per source
(FR-028). `specs/027-jitter-probe/contracts/cli-options.md` documents
the standing grammar the fix keeps.

**Alternatives considered**: a new normalized source enum in the
options struct (rejected: over-engineering, the precedence is one
comparison at the settlement site).
