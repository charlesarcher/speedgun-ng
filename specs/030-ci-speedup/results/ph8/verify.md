# The Phase 8 Verification Wave (030 FR-047, FR-048, SC-008, T048)

**Surface**: the six claims of the phase 8 wave, taken over the two
configured trees `build/prepush` and `build/tiers-on`, the two
registration files, `.github/workflows/ci.yml`, `tools/ci/prepush.sh`,
and the four records of `specs/030-ci-speedup/results/ph8`.
**Head sha**: b66d927934926b48db670d464c1ff6c59e6b9d78 of branch
`030-ci-speedup`, the head every printed command ran against. The
branch moved under this wave: the first reading of the claim 4 greps ran
at `a977c08`, and every line number below was re-read at `b66d927`, the
head the record stands at.
**Run id**: GitHub Actions run 40239619604 at head `49c12ec` and run
40242340926 at head `e77b713` are the cited runs, both `completed`.
Runs 40241027505 at `37e068d` and 40242327268 at `43dd811` come back
`completed` from `gh run list` with a stop conclusion, so no timing
below comes from either one.
**Command**: each command printed under the six claims, plus one log
read per lane job under claim 3, all run from the repository root.
**Numbers**: the two read lines of claim 1, the four counts and the two
histograms of claim 2, the eighteen seconds readings of claim 3, the
four grep results of claim 4, the forty re-derived table rows of claim
5, and the three gate lines of claim 6.

## Claim 1: every entry carries a budget or a row

`PASS`. Both trees answer with a read line and exit 0.

```text
$ bash test/tier_labels.sh build/prepush ctest; echo exit=$?
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither
registration breaks FR-048
exit=0

$ bash test/tier_labels.sh build/tiers-on ctest; echo exit=$?
read: 139 entries, 132 quick, 7 slow
ok: every entry carries one label and a budget, and neither
registration breaks FR-048
exit=0
```

The two lines above are wrapped here; the tool printed each one on a
single line. The budget side was read a second way, straight from the
json listing of each tree, so the reading does not rest on the script's
own arithmetic. Every entry of both trees holds the `TIMEOUT` property
on its own, and none is left to the exception row path.

```text
$ ctest --test-dir <tree> -N --show-only=json-v1
prepush:  134 of 134 entries carry TIMEOUT, 0 without
tiers-on: 139 of 139 entries carry TIMEOUT, 0 without
value histogram, prepush:  120 s on 8 entries, 300 s on 126
value histogram, tiers-on: 120 s on 8 entries, 300 s on 131
```

## Claim 2: the registered entry passes and the lists partition

`PASS`. Both trees run the registered entry green.

```text
$ ctest --test-dir build/prepush -R test_tier_labels --output-on-failure
1/1 Test #23: test_tier_labels .................   Passed    0.15 sec
100% tests passed out of 1
exit=0

$ ctest --test-dir build/tiers-on -R test_tier_labels --output-on-failure
1/1 Test #28: test_tier_labels .................   Passed    0.21 sec
100% tests passed out of 1
exit=0
```

The partition was recomputed from the three listings themselves, with
the entry names as the keys.

```text
$ ctest --test-dir <tree> -N --show-only=json-v1   # registered
$ ctest --test-dir <tree> -N -L quick              # quick list
$ ctest --test-dir <tree> -N -L slow               # slow list

prepush:  registered 134, quick 132, slow 2, sum 134, equal
tiers-on: registered 139, quick 132, slow 7, sum 139, equal
overlap of the two lists: empty in both trees
entries in neither list: empty in both trees
duplicate entry names: none in either tree
```

## Claim 3: the rows carry a reason and a per-lane number

`PASS`. Nine rows sit in `test/CMakeLists.txt`, each opening with the
marker and closing with the seconds and the lane, and each of the nine
numbers is on a log line of the lane the row names.

```text
$ grep -n 'budget exception' test/CMakeLists.txt
1666:# The time budget exceptions (030 FR-032, D-7, SC-008): the entries that
1680:# budget exception harness_calibration_test keeps the 120 s master
1683:# budget exception base_cli_matrix keeps the 120 s master sets for the
1686:# budget exception report_shape keeps the 120 s master sets for the
1689:# budget exception report_shape_guide_sample keeps the 120 s master
1692:# budget exception report_shape_percentiles_sample keeps the 120 s
1695:# budget exception report_shape_json_reports_sample keeps the 120 s
1698:# budget exception report_shape_jitter_sample keeps the 120 s master
1701:# budget exception report_shape_hunt_rank_sample keeps the 120 s
1704:# budget exception example_run keeps the 300 s master sets for the
exit=0
```

The example directory holds no row, so its list is empty and the claim
about it rests on the header alone.

```text
$ grep -c 'budget exception [a-z]' example/CMakeLists.txt
0
exit=1

$ grep -n 'PROPERTIES TIMEOUT' example/CMakeLists.txt
88:    set_tests_properties(${speedgun_example} PROPERTIES TIMEOUT 300)
exit=0
```

The header of that list is at `example/CMakeLists.txt:92`, the budget
loop line at `:88`, and `add_docs_example` is the only registration
call in the file, so no entry can reach the list.

Each of the nine rows was checked against the five lane logs of run
40239619604, read one job at a time. The seconds of the row matched the
line, and the lane the row names is the slowest of the five for that
entry. The last column is the same reading for the run 40242340926
column of `exceptions.md`.

| entry | row line | row seconds | lane | log line, run 40239619604 | 40242340926 |
| --- | --- | --- | --- | --- | --- |
| `harness_calibration_test` | 1680 | 0.85 | `coverage` | `Passed 0.85 sec` | 0.73 |
| `base_cli_matrix` | 1683 | 26.54 | `sanitize` | `Passed 26.54 sec` | 26.39 |
| `report_shape` | 1686 | 1.09 | `coverage` | `Passed 1.09 sec` | 1.47 |
| `report_shape_guide_sample` | 1689 | 9.85 | `tsan` | `Passed 9.85 sec` | 8.98 |
| `report_shape_percentiles_sample` | 1692 | 0.22 | `coverage` | `Passed 0.22 sec` | 0.18 |
| `report_shape_json_reports_sample` | 1695 | 0.18 | `coverage` | `Passed 0.18 sec` | 0.15 |
| `report_shape_jitter_sample` | 1698 | 1.17 | `test` | `Passed 1.17 sec` | 1.10 |
| `report_shape_hunt_rank_sample` | 1701 | 2.21 | `tsan` | `Passed 2.21 sec` | 2.72 |
| `example_run` | 1704 | 7.78 | `coverage` | `Passed 7.78 sec` | 7.60 |

The read that produced the table, one per job id:

```text
$ gh run view --job <job id> --log > /tmp/ph8verify/<lane>.log
exit=0 for all five jobs of both runs

$ sed -E 's/^[^Z]*Z //' <lane>.log | grep -E 'Test +#[0-9]+: '
```

The five job ids of run 40239619604 are 121308544920 for `test`,
121308544974 for `test-rocky`, 121308544923 for `coverage`,
121308545010 for `sanitize` and 121308544961 for `tsan`. The five of
run 40242340926 are 121316717757, 121316717877, 121316717683,
121316717684 and 121316717698 for the same five lanes. All ten belong
to the run named beside them, read from the jobs projection of the
run. One line per entry, the one that settles `base_cli_matrix`:

```text
116/134 Test  #70: base_cli_matrix ............   Passed   26.54 sec
```

## Claim 4: no timeout flag and no raised job timeout

`PASS`. All four searches were re-run at head `b66d927`.

```text
$ grep -nE 'ctest.*--timeout' .github/workflows/ci.yml tools/ci/prepush.sh
exit=1

$ grep -n -- '--timeout' .github/workflows/ci.yml tools/ci/prepush.sh
exit=1

$ grep -c 'timeout-minutes' .github/workflows/ci.yml
0
exit=1

$ git show origin/master:.github/workflows/ci.yml | grep -c 'timeout-minutes'
0
exit=1

$ git rev-parse origin/master
f507b8fe5e18f1aa4749dc5412ef6b072fcae9b1
```

The workflow file is the only one in `.github/workflows`, and a
recursive search for the key over that directory also exits 1, so the
thirteen jobs of the file each carry the runner default. The job keys
read from the `jobs` block are `lint`, `coverage`, `sanitize`, `tsan`,
`static-analysis`, `test`, `test-rocky`, `shared-audit`,
`downstream-consumer`, `consumer-release`, `dbc-gate`, `prose-lint` and
`docs`.

```text
$ grep -n 'ctest' .github/workflows/ci.yml tools/ci/prepush.sh | wc -l
12
exit=0
```

Twelve lines, the same twelve `exceptions.md` lists, and none of them
carries the timeout flag. The two ctest presets that the pre-push
script reaches, `prepush-quick` at `CMakePresets.json:214` and
`prepush-full` at `:229`, carry a label filter, `outputOnFailure` and
`noTestsAction`, and no timeout of their own.

## Claim 5: the two records name their four fields

`PASS` on the wording of the claim, with the citation defects named
below. Both records carry all four fields in the header block.

```text
$ grep -n 'Head sha\|Run id\|^\*\*Command\|^\*\*Numbers' \
    specs/030-ci-speedup/results/ph8/audit.md \
    specs/030-ci-speedup/results/ph8/exceptions.md
audit.md:8:**Head sha**: 7a7dc1f26612f20b8bcb4dd7fcc16fb9498d278b. ...
audit.md:14:**Run id**: GitHub Actions run 40242340926 of branch ...
audit.md:24:**Command**: the log read under Readings and the four greps ...
audit.md:26:**Numbers**: the five tables under Lanes, the tightest row ...
exceptions.md:8:**Head sha**: ffcd00d07cfa99dff2c2b5b8043bbf750cce1a2c. ...
exceptions.md:15:**Run id**: GitHub Actions run 40239619604 of branch ...
exceptions.md:21:**Command**: the greps under The rows in the tree ...
exceptions.md:24:**Numbers**: the nine rows under The exceptions table ...
exit=0
```

The two canceled ids appear once in each record, inside the sentence
that says they carry no timing, and no table of either record cites
them.

```text
$ grep -n '40241027505\|40242327268' specs/030-ci-speedup/results/ph8/*.md
audit.md:22:for the same five lanes. Runs 40241027505 and 40242327268 were
exceptions.md:20:40241027505 and 40242327268 were canceled, so they supply no timing.
exit=0
```

Every number was re-derived from the cached logs, one table at a
time. All forty rows of the five lane tables of the cited run, and all
twenty of the
rows of the five lane tables of the cited run, and all twenty of the
cross-check table, came back the same when the top four passing entries
of each cached lane log were sorted by the seconds field: 0
mismatches across 40 rows. The four partial readings the records set
aside were read on their own lines and matched: 18.20 s for
`harness_jitter_cli_test` on `coverage` of run 40239619604, 11.40 s for
the same entry on `coverage` of run 40242340926, 1.07 s for
`harness_context_barrier_test` on `tsan` of run 40239619604, and 0.52 s
for `harness_calibration_test` on `sanitize` of run 40242340926. No
entry of either run ends in a timeout on any lane: the word appears in
neither the duration lines nor the suite summary of all ten logs.

## Claim 6: the three source gates

`PASS`. All three exit 0.

```text
$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
exit=0

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit=0
```

The prose gate needs a throwaway index, because tree mode reads the
tracked listing and the whole `ph8` directory is untracked at head. The
plain run reports the empty source list, and the run in an index under
`/tmp` with the four records staged reports four.

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree \
    --paths specs/030-ci-speedup/results/ph8
prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped
exit=0

$ cd /tmp/ph8prose && python3 \
    /home/archerc/code/speedgun-ng/tools/prose/prose_gate.py \
    --check prose --mode tree --paths specs/030-ci-speedup/results/ph8
prose-lint: 4 sources, 973 units examined, 0 findings, 0 skipped
exit=0
```

With this record staged beside the four, the same run reports five
sources, 0 findings and exit 0.

## Defects this wave found

The six claims hold as written. Four citation defects sit behind them
and each needs a fix before the phase closes.

1. `specs/030-ci-speedup/results/ph8/exceptions.md:151` and `:152` put
   the eleven coreutils wrapper lines at `.github/workflows/ci.yml:1086`
   and lines 1110 to 1129. At head `b66d927` the first is at 1090 and
   the run is 1114 to 1133, and at `ffcd00d`, the head the record
   names, the same eleven lines sit at 924 and 948 to 967. The record
   states the line numbers are the working copy's, so the fix is the
   pair 1090 and 1114 to 1133.
2. `specs/030-ci-speedup/results/ph8/exceptions.md:127` puts the
   wrapped ctest invocation at `.github/workflows/ci.yml:1187` with the
   failure flag at 1188. At head the invocation is at 1191 and the flag
   at 1192.
3. `specs/030-ci-speedup/results/ph8/exceptions.md:33` and
   `test/CMakeLists.txt:1674` say run 40239619604 holds one red. Its
   `tsan` lane holds two: the base red at 25.37 s and
   `harness_context_barrier_test` at 1.07 s, which `audit.md:285`
   records. The conclusion each sentence draws still holds, since none
   of the nine exception entries is red on any lane of either run.
4. `audit.md:8` names head `7a7dc1f2` and `exceptions.md:8` names head
   `ffcd00d0`, and no ref contains either commit: the rebase that
   produced `fec828b` superseded that line. A reader at head cannot
   check either record at the head it names.

One note about the trees, which is not a defect. `build/prepush` was
configured at 12:35 and `build/tiers-on` at 18:05, while
`test/CMakeLists.txt` was last written at 18:47, so both trees carry an
older copy of the registration. The uncommitted diff of the two
registration files touches no `TIMEOUT` row and no line of the slow
list, only comment lines, so the label and budget readings of claims 1
and 2 are the ones a fresh configure would give.

VERDICT: PASS
