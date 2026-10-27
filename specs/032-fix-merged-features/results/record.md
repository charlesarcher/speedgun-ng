# Results Record: Fix Merged Features

The evidence the check items and the fixes record. Each figure comes
from the CI run of the last pushed commit (FR-061) or from the local
measurement named beside it.

## T001: the CI run read (FR-064, PC-11)

Run `40438223916` read 2026-10-27 with a supplied token: status
completed, conclusion success, head `500704bb757c1bb14a1457b792608deecc505266`
(master, the hash the branch cuts from). Every job reports success:
lint, sanitize, coverage, tsan, consumer-release, downstream-consumer,
prose-lint, shared-audit, dbc-gate, test, static-analysis (tests,
library, examples), test-rocky, docs.

## T010: the lint job's spell step (FR-010)

The lint job's steps read 2026-10-27: the spell step ran and succeeded
("Spell check", success) beside the drift, lint and time-source steps,
so the spell gate needs no CI change.

## T003: the option-lane confirmation (FR-003)

Measured 2026-10-27 on master `500704b`: the option table at
`source/harness/detail/options.hpp` adds no row and moves no value for
this feature. The lanes hold: `260` to `269` the jitter-probe family,
`270` to `279` the hunter family, `290` to `319` the command-line
family.

## T004: the SC-007 fact checks

- `SPEEDGUN_JSON` matches no environment row: the derivation at
  `options.hpp:109` to `:115` builds the name from a flagged row and
  no row is named `json`, so the unmatched-name rule at
  `options.cpp:1310` to `:1325` leaves it unread. Confirmed by read,
  2026-10-27.
- The idle preset exits 2 with one line when no cpu is free:
  `cli.cpp:805` to `:818` prints one line naming the preset word and
  returns 2. Confirmed by read, 2026-10-27.

## Check items of Phase 2

Each check below was measured 2026-10-27 on master `500704b`, the
hash the branch cuts from; the local measurements stand in
`research.md` R-01 to R-12 and carry the same evidence:

- T005 (FR-005): `7d8dfdd` "Harness: Rename hunt retry pass parameter"
  resolves on master; the preset build runs at the branch head.
- T006 (FR-006): `658af81` "test: Anchor the mutation at schema
  version 1.5" resolves; `kSchemaVersion = "1.5"` stands at
  `json_report.cpp:36`.
- T007 (FR-007): `1c47e66` and `d7e22c2` resolve; the CLI table test
  passes at the branch head; the format check holds.
- T008 (FR-008): `cd2f2f1` and `f507b8f` resolve; `README.md` measures
  835 lines; `PAGE_LINE_CAP = 835` stands once at
  `tools/docs/example_drift.py:192`.
- T009 (FR-009): `502d42b` resolves; the owning test runs under
  `taskset -c 0,1`, `taskset -c 0-3` and the full mask at the branch
  head.
- T010 (FR-010): stands on the T001 read when the token stands.
- T011 (FR-011): each of the twelve reverted hashes of PC-6 resolves
  by `git log --grep` at the branch head.
- T012 (FR-012): the `setenv|GCOV_PREFIX|sgcov` scan over
  `test/source` returns nothing (measured 2026-10-27, research R-07).
- T013 (FR-013): `42ba692` resolves; the helper stands at
  `hunt_plan.cpp:97`; the macro scan runs at the branch head.
- T014 (FR-014): the seam stands (`topology.cpp:327`), both bounds
  read it (`execution_context.cpp:655`, `jitter_probe.cpp:711`), and
  the gap scenario stands: the execution-context bound reads the set
  `{0, 1, 2, 3, 8, 9, 10, 11}` beside the mask `{0, 1, 8, 9}`
  (`harness_execution_context_test.cpp`, `scenarioOnlineSetBound`),
  and the option-value bound refuses a cpu the installed set lacks
  (`harness_jitter_cli_test.cpp`). The old count-rule text scan
  returns nothing after the FR-021 fix, which rewrites the three
  comments research R-07 names.
- T015 (FR-015): the three records stand (spec 027 row, grammar row,
  error-table row).
- T016 (FR-016): no hunt row carries `fromEnvironment`.

## Fixes landed on the branch

- FR-028, D-01 (T017, T020): the family-probe build reads every
  family; a family's own probe wins over the command line's and the
  option file's alike, and the batch's stands only where no family
  carries one. The `jitterProbeFromOptionFile` gate is removed from
  `source/harness/cli.cpp`.
- FR-021, D-02 (T019, T021): the unreadable-line fallback of
  `onlineCpuSet()` answers the process affinity mask; a count is no
  source of the fallback, so `installTestOnlineCount` and
  `onlineCpuCount` are deleted with the count slot. The line seam
  `installTestOnlineLine` stands in for the read, the way
  `installTestLayout` does, and the empty installed line stands for
  the unreadable case. The three comments research R-07 names carry
  the mask wording.


## Fixes landed on the branch (continued)

- FR-030 (T047, T048): the compare row quotes the name and rule-text
  cells once with `detail::quotedText` at the print site
  (`compare.cpp:1905` to `:1912`), the reason cell unquoted; the
  fixture pair `test/data/gate_quoted_base.json` and
  `gate_quoted_contender.json` carries a name with a backslash and a
  newline, and the row stands one line under the escaped spelling.
- FR-017 (T049 to T051): the report schema carries one top-level
  `x-order` key, the merged writer order; `checkReport` refuses a
  schema or a document copy that carries one key twice, reading the
  pairs the parser's tape carries, and names `/x-order` beside it.
- FR-022, removal half: the `e781eda` segment print left the family
  fold check. The intermittent failure's root cause stands owed to the
  CI observation FR-061 records once the branch pushes.
- T001, T010: run `40438223916` read with a supplied token; all 15
  jobs succeeded on the base head, and the lint job's spell step ran
  and succeeded.

- FR-034, FR-045, D-07 (T027, T029): the ambient-value refusals stand
  on master and carry their tests: `SPEEDGUN_COUNTER`, `SPEEDGUN_METRIC`
  and `SPEEDGUN_PLACEMENT` each stop the hunt with the one line naming
  the option, from the environment and from the option file beside the
  command line's (`scenarioAmbientCombinationRefusals`).
- FR-035, D-08 (T028, T030): the hunt and the probe run together, and
  the fix landed: the deferred print of a probed run read no run
  record, so the console dropped the hunt block the document carried;
  the deferred pass now reads the record each entry answered, and the
  scenario runs the hunt beside the probe on a host that scripts the
  probe's leaves (`scenarioHuntWithProbe`).

## The hunt review items (T031 to T044, T046)

- T031 (FR-037, D-10): the hunt's first pass carries no retry on master;
  the collect test pins the pass count the plan runs, and the rank
  fixture's record keeps `EntryHunt::passCount` equal to the plan's
  passes, the passes a retry appended included, of which none stands.
- T032 (FR-038, D-11): the `FakeProvider` open-failure seam stays
  public in the counters archive, and the archive `SOVERSION` stays `4`
  (`CMakeLists.txt:80`).
- T033 (FR-039, FR-040): the flag parameters ride the options
  structures (`RunOptions`, `HeldRow`), and the contract-only helper at
  `execution_context.cpp:656` carries `[[maybe_unused]]`.
- T034 (FR-041): the hunter acceptance rows move to the host-validation
  feature (spec 031); this feature closes the item with the move.
- T035 (FR-042): the PR 47 items P-10 to P-18 carried their detail in
  the fixes-1 prompt, which is absent from this host; each is confirmed
  against master through the standing tests of the hunt suites, and an
  item the tree does not settle goes to the pull request as a question.
- T036 (FR-036, D-09): no item needed a new `SkippedKind` value, so no
  value joins and the version lane stays `0.20.1` (FR-001); the enum
  stands at `hunt.hpp:158` with five values.
- T037 (FR-043): the hunt parsers' process exit stands: every refusal
  row of the hunt suites exits 2 with one line (`checkRefusals`).
- T038 (FR-044): `--hunt=none` behaves exactly like a run with no hunt
  (`hunt-none-like-no-hunt`), and the source order rule stands
  (`option-order-settles`).
- T039 (FR-045): the refused combination stands from every source
  (`scenarioAmbientCombinationRefusals`).
- T040 (FR-046): the hunt beside `--threads` and `--interleave` stands
  with tests (`hunt-with-threads-and-interleave`), and beside
  `--jitter-probe` with `hunt-with-probe`.
- T041 (FR-047): the raw-code rule with globs stands: the trailing glob
  strips before the shape read (`cpu/r4a03*`, and the bare `r4*` row).
- T042 (FR-048): the `ABSENT` relabel abort stands with its trap
  fixture: `harness_hunt_rank_trap_fixture` feeds `rankEvents` an
  address whose every window carries no anchor and no refusal kind, and
  `harness_hunt_rank_trap_test` asserts the abort before the marker in
  a checked build and the survival with the marker in an ignore build.
- T043 (FR-049): this host carries no real PMU, so the real-PMU test
  item routes to the pull request as a named gap, the catalog the host
  carries being the fixture's own.
- T044 (FR-050): the `--hunt-noise` grammar stands with its tests (the
  hunt CLI suite carries fifteen `hunt-noise` cases).
- T046 (FR-032, FR-033, D-05, D-06): the counter-address refusal stands
  from every source with its tests in `harness_cli_test.cpp`: the
  command line, the environment (`SPEEDGUN_COUNTER`), and the option
  file beside it, each one line and status 2; a valid PMU path this
  host lacks routes to FR-049's named gap.

## US6: the requirement language (T056 to T058)

- T056: the five named targets stand rewritten: the ruling sentence of
  `specs/028-full-command-line/spec.md:1127` states the two statuses and
  how they read together; the Decisions intro of
  `specs/029-hunter-core/spec.md` names no person and no date; the three
  lines of `plan-review-2.md` state the rule the finding holds.
- T057: the scan over the 130 files `git diff --name-only fea69c5
  4f701ae` names carries no person's name, no host name and no
  acceptance wording in prose. The owner placeholder of
  `.github/workflows/ci.yml` and the `HOMEPAGE_URL` of `CMakeLists.txt`
  are functional values the file boundary keeps: `LICENSE`,
  `CODEOWNERS` and `CMakeLists.txt` stay untouched (FR-024), and the CI
  file boundary of FR-059 keeps the workflow.
- T058: the two comments of `cmake/coverage.cmake` state what the code
  does: the keep-set patterns name the lines of the replaced count arm,
  match no line of code today, and stand until the approval that
  removes them; the `brace` rule names the gcc record and no person, no
  decision record and no host.

## US5: the docs and the merge residue (T052 to T055)

- T052 (FR-018): the two hunt samples re-record from the current build
  and name `0.20.1`; every other sample stays unchanged, and the drift
  gates run green on the stamped page region and the expected file.
- T053 (FR-018, SC-008): the sweep stands scripted and clean: every
  `--`-spelled option token of `docs/pages/harness.md` and of the three
  features' contract files exists in the option table of
  `options.cpp` (36 rows), and the one `--scope` mention is the
  disclaimer that names `--placement` as the branch's own spelling;
  every exit-status claim in those files reads 0, 1 or 2. Zero
  disagreements.
- T054 (FR-020): this host carries GCC 16.2.1 and no GCC 14, so the
  Rocky-toolchain reproduction cannot run here. The gate passes on GCC
  16.2.1 and on the CI test job's GCC 15.2.0 (run `40438223916`, job
  `test`, success). The reproduction stands owed to the Rocky lane and
  routes to the pull request as a question if it reproduces there.
- T055 (FR-023): the reconcile commit `4f701ae` touched nine files; its
  diff is the conflict surface the rebase left. Read against master:
  each duplicated scan case stands exactly once (`kInterleaveOptionValue`,
  `kHelpOptionValue`, `kVersionOptionValue`, `kHuntOption`, one each in
  `options.cpp`), the braces the rebase split stand (`json_load.cpp`,
  `report.cpp`; the build compiles and the json and report suites run
  green), and the standing suites pin the behavior of every file the
  surface carries. The 38-block count and the eight unrepaired files
  stand on the request's record, the fixes-1 prompt being absent from
  this host, and route to the pull request with the surface named.

## US2 leftovers and the process audits (T023 to T026, T065 to T069)

- T023 (FR-019, M-7): the scan step and the settle step parse through
  the one function `parseJitterProbeValue`: the command line's text
  parses at `cli.cpp:566` to `:576`, the option file's row parses where
  it settles at `options.cpp:1770`, and both call the one reader, so
  one code path owns the value's grammar; the behavior stays, and the
  grammar's tests pin it.
- T024 (FR-022, M-12): the `e781eda` segment print left the family
  fold check in the fix commit. The intermittent failure's root cause
  stands owed to the CI observation the push run records: the failure
  was observed twice on the 2026-10-24 runs and carries no document,
  so no local reproduction stands. Routes to the pull request as a
  question beside the run's logs.
- T025 (FR-027, PR 46 P-3): the batch-probe arm stands covered: the
  batch probe's own scenarios cover the arm with `familyProbes` empty,
  and the family scenarios (`family-probe`, `family-vs-cli`,
  `family-vs-file`) cover the arm beside a family's own probe; no
  unreachable code stands at `cli.cpp:776` to `:777`.
- T026 (FR-026, D-12): the keep-set stands. After FR-021 the four
  `--omit-lines` patterns of the count arm and the `cpus.empty()` line
  match no line of code, and the comment of `cmake/coverage.cmake`
  states it; the allowance of 19 stands re-counted on the coverage run
  of the pushed head, and the pull-request comment asks before any
  removal.
- T065, T066 (FR-053, FR-054): all 13 commits carry the footer
  `Refs: specs/032-fix-merged-features` then `Approved-by: OmO`, one
  each, and no subject passes 50 characters and no body line passes
  72.
- T067 (FR-055): each fix ships with its test in the same commit
  (`b5889bc`, `a153910`, `d70bf0d`, `c792e94`, `c957677`, `6b7930a`),
  and the red run of the precedence fix stands on the PC-10
  measurement the option-file override carried before it.
- T068 (FR-056 to FR-058): the `setenv|GCOV_PREFIX|sgcov` scan over
  `test/source` returns nothing; no lambda stands inside an `SG_*`
  macro in `source/` or `include/`; the quoting test proves text from
  a user makes no contract fail.
- T069 (FR-059, SC-006): the branch touches `cmake/coverage.cmake`
  and `CMakeLists.txt` only as the request mandates (the comment
  rewrite of FR-025 and the version lane of FR-001), and neither
  changes a gate's behavior; no workflow, preset, lint, spell, dbc,
  prose or drift file moved.
