# The pre-push run

`tools/ci/prepush.sh` with no option runs the five gates, the build, the
quick tier of 132 entries twice and the naming check, and
`tools/ci/prepush.sh --full` adds the slow tier of 2 entries, the
release build of the installed package and the tsan configure and link.

## Quick start

The `prepush` configure preset names the build tree and the
`prepush-quick` and `prepush-full` test presets run the two tiers in it:

<!-- sg-example: example/quickstart.sh#tiers -->
```sh
cmake --preset=prepush
cmake --build build/prepush -j "$(nproc)"
ctest --preset prepush-quick -j "$(nproc)"
taskset -c 0,1 ctest --preset prepush-quick -j 2
ctest --preset prepush-full -j "$(nproc)"
```

## The tiers

The quick tier is what a fast local run may stop at, and the slow tier
is what `--full` adds; the audit entry `test_tier_labels` reads the two
lists the presets filter on and fails the run when an entry lands in
both, in neither, or in a tier its budget does not fit. The `taskset`
line is step 9, the quick tier pinned to two cpus the way a four-vCPU
runner has it.

CI is authoritative: the script mirrors its checks on your machine, and
a green run here never closes a run CI has not made.
