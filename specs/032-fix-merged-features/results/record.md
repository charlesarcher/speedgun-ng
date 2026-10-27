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
