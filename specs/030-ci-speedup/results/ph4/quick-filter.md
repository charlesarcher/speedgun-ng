# Phase 4 Quick Preset Label Filter

**Surface**: the `prepush-quick` test preset of `CMakePresets.json`, read
through the two tier steps of `tools/ci/prepush.sh` that name it (T021,
FR-029, D-10).
**Head sha**: 3498874b1bf264fe63e352c93e6b7958079e50c8
**Run id**: local
**Command**: the four selection lines under Reading, each run from the
repository root
**Numbers**: 134, 132, 134 for the three preset shapes, 132 for the label
query, 3 added lines and 1 removed line in the fix

## Reading

The tree is `build/prepush`, configured by `cmake --preset=prepush`. The label
query is the ground truth for what the quick tier holds:

```console
$ ctest --test-dir build/prepush -N          | tail -1
Total Tests: 134
$ ctest --test-dir build/prepush -N -L quick | tail -1
Total Tests: 132
$ ctest --test-dir build/prepush -N -L slow  | tail -1
Total Tests: 2
```

The preset of the same name selected every entry until this fix:

| `filter` body in `prepush-quick` | `ctest --preset prepush-quick -N` |
| --- | --- |
| `{"labelInclude": "quick"}` | 134 |
| `{"include": {"labelInclude": "quick"}}` | 134 |
| `{"include": {"label": "quick"}}` | 132 |

Each row is the count that command printed after the row's body was written
into the preset. `prepush-full` carries no filter and printed 134 throughout,
which is what its contract asks for.

## The fault

`CMakePresets.json` declares `"version": 2`, and a version 2 test preset reads
its selection from `filter.include` and `filter.exclude`. `labelInclude` is no
field of `filter`; cmake ignores an unknown field inside `filter` and prints
nothing, so `prepush-quick` ran the whole suite while spelling itself quick.
The preset arrived in `9fb9a00 CMake: Add the test and prepush presets`.

The consequence showed up twice while this phase measured the script. A run at
`3498874` printed the slow entries inside the quick tier, and the denominator
names what the step selected:

```text
129/134 Test #126: snippet_compile ......... Passed 52.75 sec
132/134 Test  #76: harness_jitter_cli_test . Passed 73.81 sec
```

An earlier run at an older head selected 134 entries the same way and lost three
of them, one to the budget the branch gives every entry without an exception
row:

```text
133/134 Test  #65: harness_cli_test ........ ***Timeout 300.10 sec
98% tests passed, 3 tests failed out of 134
Total Test time (real) = 301.52 sec
prepush: quick tier, full affinity failed with 8
```

A second attempt of the same script stopped earlier, at step 5, on a prose
finding in a record of this phase. On master at `f507b8f` the same entry times
out at the CTest default instead, `***Timeout 1500.11 sec`, because master
registers no `TIMEOUT` for it.

## The effect

`ctest --preset prepush-quick` now selects the 132 quick entries, so steps 8
and 9 of the script are the quick tier at both affinities, which is the shape
D-10 names. `harness_cli_test` and `snippet_compile` stay in the suite and in
`prepush-full`, where the entry above still needs an exception row (T044), and
no `ctest` line of `.github/workflows/ci.yml` gained a filter, so CI scope is
untouched (FR-027). The timing record `ph4/timing.md` reads the script after
this change; a number taken before it counts the slow tier twice.
