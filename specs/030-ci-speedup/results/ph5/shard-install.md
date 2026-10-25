# Phase 5 Shard Install Set

**Surface**: the `Install analyzers` step of the `static-analysis` job of
`.github/workflows/ci.yml`, read against the configure path that job calls.
**Head sha**: 5d43840e25ae42f75885411a188e7747b9d83a91. The step edited here
is a working-tree change above that commit; the run below tested `3498874`,
two commits back.
**Run id**: 40235334432
**Command**: the two `gh` readings under Reading and the parse under Check
**Numbers**: 3 failed shards, 1 failing step name each, 3 packages before the
fix, 8 packages after it

## Reading

The pull-request run of `3498874` carried thirteen jobs. Ten reached their
verdict; the three shards of the analyzer job went red on the same step:

```console
$ gh run view 40235334432 --json jobs \
    --jq '.jobs[] | select(.name|startswith("static-analysis")) \
    | .name + " :: " + ([.steps[] | select(.conclusion=="failure") | .name] | join(", "))'
static-analysis (tests) :: Configure
static-analysis (examples) :: Configure
static-analysis (library) :: Configure
```

`lint`, `prose-lint`, `dbc-gate` and `downstream-consumer` completed with
success in the same run, so the toolchain, the checkout and the cache steps
were not the fault: the shard died at `cmake --preset=ci-ubuntu`, the first
step that needs more than the three packages its install step named.

The step body of a run that is still open is withheld by the Logs API, so the
error text below is quoted from the source the step reaches, which is the same
text the FATAL_ERROR raises.

## Cause

`CMakeLists.txt:93` imports the vendored hwloc through the autotools helper,
and that helper probes the bootstrap toolchain at configure time:

```cmake
import_autotools_submodule(
    NAME hwloc_vendor
    SUBMODULE_DIR external/hwloc
    BOOTSTRAP autogen.sh
```

```cmake
    if(IAS_BOOTSTRAP STREQUAL "autogen.sh")
      find_program(IAS_PROG_AUTOCONF autoconf)
      find_program(IAS_PROG_AUTOMAKE automake)
      find_program(IAS_PROG_PATCH patch)
      find_program(IAS_PROG_LIBTOOL
          NAMES libtoolize glibtoolize libtool glibtool)
    endif()
```

A program the probe cannot find is appended to `_ias_missing`, and a non-empty
list raises `message(FATAL_ERROR ...)` at
`cmake/ImportAutotoolsSubmodule.cmake:197`. The shard's install step named
`clang-tidy cppcheck ccache`, so four programs of that probe were absent on the
runner and the configure stopped before a single translation unit was counted.

Every other building lane lists the autoreconf family in its install step,
which is why the lane this feature added was the only one to go red.

## Fix

The step now names what its own configure calls for:

```yaml
    - name: Install analyzers
      run: sudo apt-get update -q
        && sudo apt-get install clang-tidy cppcheck ccache autoconf automake libtool patch -q -y
```

Doxygen belongs to the `docs` lane, `python3-yaml` to the tier audit and lcov
to the coverage lane. A shard compiles and analyzes, so those three stay out of
the list and the shard's install step stays the shortest of the building lanes.

## Check

Nothing else in the workflow moved. The seven checks of `ph5/verify.md` were
re-run against this working tree and hold as they did there: thirteen jobs,
eleven of them building, the cache invariant in each of those, three distinct
shard keys, an analyzer-free `test` lane on `ci-ubuntu-test`, no analyzer
argument outside the presets, and both source gates clean. The caps in the file
still read eleven at 8448 MB, the three shards sharing one `env` block, and the
cache a runner charges for stays 8960 MB as `analyzer-job.md` reads it.

A GitHub reading of the fixed step is owed by the next run: T029 needs the
finding artifacts of a shard that reaches its compile.
