# Phase 3 Preset Inheritance

**Surface**: the `inherits` lists of the configure presets in
`CMakePresets.json` and the compiler launcher the CI presets resolve to
(T015, FR-006, FR-028).
**Head sha**: 5d496c9, the launcher already in the tree from the preset commit
**Run id**: local
**Command**: the two lines under Reading, run from the repository root
**Numbers**: the inherit list and the cache line under Reading

`ci-cache` is the abstract preset that names the launcher; a concrete preset
that inherits it builds through `ccache`, which is what makes the cache steps of
`ph3/cache-steps.md` worth running at all. T015 added `ci-cache` to
`ci-sanitize`, `ci-tsan` and `ci-coverage`, the three instrumented lanes, so
every CI build now carries the launcher beside the local `prepush` preset that
already did.

## Reading

```text
python3 -c "import json; p=json.load(open('CMakePresets.json')); ..."
cmake --preset=ci-sanitize -B /tmp/cfgsan && grep -m1 CMAKE_CXX_COMPILER_LAUNCHER /tmp/cfgsan/CMakeCache.txt
```

| Preset | `inherits` | Own `CMAKE_CXX_COMPILER_LAUNCHER` |
| --- | --- | --- |
| `ci-cache` | `dev-mode` | `ccache` |
| `ci-coverage` | `coverage-linux`, `dev-mode`, `ci-cache` | none, inherited |
| `ci-sanitize` | `ci-linux`, `dev-mode`, `ci-cache` | none, inherited |
| `ci-tsan` | `ci-linux`, `dev-mode`, `ci-cache` | none, inherited |
| `ci-ubuntu` | `ci-build`, `ci-linux`, `clang-tidy`, `cppcheck`, `dev-mode`, `ci-cache` | none, inherited |
| `ci-rocky` | `ci-build`, `ci-linux`, `dev-mode`, `ci-cache` | none, inherited |
| `prepush` | `ci-build`, `dev-mode`, `ci-cache` | none, inherited |

The configure of `ci-sanitize` exits 0 and the generated cache carries the
resolved value:

```text
CMAKE_CXX_COMPILER_LAUNCHER:UNINITIALIZED=ccache
```

One concrete preset is enough to prove the resolution, because the six inherit
`ci-cache` through the same mechanism and none of them overrides the variable.

No preset is renamed or removed: `dev` stays unused as FR-028 requires, and the
preset count is the count `ph1/verify.md` recorded.
