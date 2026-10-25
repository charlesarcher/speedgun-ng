# Phase 2 The Capture Entry Joins the Slow Tier

**Surface**: one row added to `SPEEDGUN_SLOW_TESTS` in `test/CMakeLists.txt`,
and the tier partition and budgets that follow from it (T011, T012, P-05,
P-07, FR-025, FR-027, FR-031).
**Head sha**: 4a8047a
**Run id**: local
**Command**: the base reproduction under Reading, then the audit line and the
two label listings, run from the repository root
**Numbers**: the partition line, the timeout line of the base run and the exit
status of every command

`harness_cli_test` was the entry that broke the local quick tier at the 300 s
budget: `***Timeout 300.10 sec`. Its window is two runs of 1,000 captured
iterations in one process, which holds a core for the whole entry, and on a host
granted the PMU the slow region of the entry executes. A full-affinity run of
this entry on this host completed in 3110.37 s, a reading kept in the lane
notes. The row that moves it out of the quick tier now sits in the slow list
with its reason.

## The base reading

A worktree of the base commit `f507b8f` was configured with `ci-rocky` and run
from `/tmp/basecheck`, with the submodules initialized at that commit.

```text
ctest --test-dir build -R '^(harness_json_load_test|harness_cli_table_test)$'   -> exit 8
ctest --test-dir build -R '^harness_cli_test$'                                  -> exit 8
1/1 Test #65: harness_cli_test .................***Timeout 1500.11 sec
```

The first command names the two entries that fail at the base commit on this
host, before any 030 change. The second is the capture entry under the ceiling
master left it with: the CTest default of 1500 s, reached at 15:44:03 from a
start at 15:19:03. The 300 s budget of T012 shortens that local failure; it
does not cause it.

## The row

```cmake
  # two windows of 1,000 in-process runs, 52 min on a host granted the PMU
  harness_cli_test
```

The list is the one T011 wrote, so the label loop gives this entry `slow` and
`ctest --preset prepush-quick` leaves it out of both quick steps of
`tools/ci/prepush.sh`. Nothing else about the entry changed: its registration
keeps `PROCESSORS 2` and no `TIMEOUT`, so the budget block still gives it 300 s
and the audit still finds a budget on it.

## Reading

```text
bash test/tier_labels.sh build/cli ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
[exit status: 0]

ctest --test-dir build/cli -R '^test_tier_labels$'
1/1 Test #23: test_tier_labels .................   Passed    0.20 sec
100% tests passed out of 1

ctest --test-dir build/cli -N -L quick | grep -c harness_cli_test   -> 0
ctest --test-dir build/cli -N -L slow  | grep harness_cli_test      -> the slow listing holds it
SPEEDGUN_DBC_MATRIX=ON bash test/tier_labels.sh build/cli ctest     -> the same partition line
```

The partition moved from 133 quick and 1 slow, the reading of
`ph2/tier-list.md`, to 132 quick and 2 slow. The matrix rows stay unregistered
until phase 6 lands, so the matrix reading matches the plain one.

## Scope of CI and the limit of this row

`contracts/prepush-script.md` and the list's own comment hold the rule that
keeps this safe for CI: no ctest line in the workflow passes a label filter, so
the labels move local speed and never CI scope (FR-027). The capture entry
still runs in every CI lane under its 300 s budget, and the region that needs
1500 s and more needs a granted PMU, which a runner has none of.

The limit sits in `--full`: step 11 runs the slow tier with the same 300 s
budget, so a PMU host sees this entry time out there, where master saw it time
out at 1500 s. The exception row of FR-032 and the CI-measured value are T044's
work, which is where a number for this entry is named from a run id.
