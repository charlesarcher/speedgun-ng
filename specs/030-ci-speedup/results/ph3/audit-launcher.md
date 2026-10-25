# Phase 3 Launcher for the Audit Lanes

**Surface**: the cache variables of `ci-linux-audit` and `ci-linux-ignore` in
`CMakePresets.json`, the two presets the verification of phase 3 found without
a launcher (FR-006).
**Head sha**: 6767c20
**Run id**: local
**Command**: the two configure lines and the cache grep under Reading, run from
the repository root
**Numbers**: the exit status and the resolved launcher value of each preset

`ph3/verify.md` observation 1 found three building lanes whose cache steps
restore and save a directory that no compile writes into:
`shared-audit` and `downstream-consumer` configure `ci-linux-audit` and
`consumer-release` configures `ci-linux-ignore`, and the ancestry of neither
preset passes `ci-cache`, so `CMAKE_CXX_COMPILER_LAUNCHER` stayed unset there.
FR-006 asks the CI presets to pass `ccache` as the launcher, so both presets
now name the two launcher cache variables themselves.

## Reading

```text
cmake --preset=ci-linux-audit -B /tmp/la-ci-linux-audit   -> exit 0
cmake --preset=ci-linux-ignore -B /tmp/la-ci-linux-ignore -> exit 0
grep -c 'COMPILER_LAUNCHER:UNINITIALIZED=ccache' /tmp/la2/CMakeCache.txt
2
```

| Preset | `inherits` | Launcher variables resolved | Contract semantic |
| --- | --- | --- | --- |
| `ci-linux-audit` | `ci-linux` | `CMAKE_C_COMPILER_LAUNCHER`, `CMAKE_CXX_COMPILER_LAUNCHER`, both `ccache` | `enforce` |
| `ci-linux-ignore` | `ci-linux` | the same two, both `ccache` | `ignore` |

The two cache lines the configure writes:

```text
CMAKE_C_COMPILER_LAUNCHER:UNINITIALIZED=ccache
CMAKE_CXX_COMPILER_LAUNCHER:UNINITIALIZED=ccache
```

`ci-linux-ignore` keeps `speedgun-ng_CONTRACTS:STRING=ignore` and
`ci-linux-audit` keeps the `enforce` default, so the audit premise each job
checks is untouched: a launcher fronts a compile and changes no flag.

## Why the variables and no inheritance

Adding `ci-cache` to the `inherits` of these two presets would have been one
word per preset, and it would have dragged `dev-mode` in with it: `ci-cache`
inherits `dev-mode`, so the audit lanes would have picked up the development
flags of that preset, and both lanes exist to audit a release-shaped tree. The
audit of the package surface and the ignore-contract archive are what those
jobs test, so the flag set is the thing under test there. Naming the two
variables copies exactly what `ci-cache` carries and nothing else, which is the
same pair `contracts/presets.md` lists for the cache preset.

The three jobs already install `ccache` for their cache steps, and
`ph3/verify.md` check 2 confirmed the install step of each precedes the first
`ccache` command, so the launcher resolves to a real binary in every lane that
now names it.

## Effect on the phase 3 readings

`ph3/cache-steps.md` and `ph3/verify.md` stay true at their own head shas. The
hit-rate records T017 to T019 can now expect a nonzero hit rate in all ten
building lanes: the three that previously printed zeros in `ccache -s` have a
launcher in front of their compiles.
