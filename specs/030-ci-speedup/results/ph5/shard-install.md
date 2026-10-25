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

## The second gap, read from run 40236978814

**Head sha**: 3cf4d639a3ae85af42d2dd087a5aa83094eb7943
**Run id**: 40236978814
**Command**: the two commands under The two readings, the first for the
job table, the second for the shard log
**Numbers**: 3 shards failed, 1 failing step each named Configure, 8
packages installed before this second fix, 10 after it

The bootstrap tools got the shard past the hwloc probe and into the
project's own gate module, where a second required program was missing.

### The two readings

```bash
gh run view 40236978814 --json jobs
gh run view --log --job 121300655678 | awk -F'\t' '$2=="Configure"'
```

The second command prints the Configure step of the library shard, whose
tail reads:

```text
CMake Error at cmake/dbc-gate.cmake:11 (message):
  dbc-gate requires doxygen on PATH
Call Stack (most recent call first):
  CMakeLists.txt:945 (include)
```

`cmake/dbc-gate.cmake:10` runs `find_program(DOXYGEN_EXECUTABLE NAMES
doxygen)` and stops with a fatal error at `cmake/dbc-gate.cmake:11` when
the search comes back empty, and `CMakeLists.txt:945` includes that module
on every developer-mode configure, which is what each shard runs. Every
other lane of the workflow installs doxygen for that reason, as the eight
install steps of `ci.yml` show.

A grep for the fatal stops over `cmake/` and `CMakeLists.txt` names the
whole configure-time program set for this path: doxygen at
`cmake/dbc-gate.cmake:11`, the autotools probes of
`cmake/ImportAutotoolsSubmodule.cmake:164` to `:172`, and nothing else
until `cmake/coverage.cmake`, which only the coverage preset reads. The
install step now carries clang-tidy, cppcheck, ccache, doxygen,
python3-yaml, autoconf, automake, libtool and patch, matching the set the
`test` lane installs plus the two analyzers.

Verdict: the install set of the first fix was one package short of the
configure path, read from a run that failed at the same step for a
different reason.

## The third gap, read from run 40239619604

**Head sha**: 49c12ecee775ed6fafa66e18d341c081edd194b9
**Run id**: 40239619604
**Command**: the library shard's own log, at the command under The third
stop
**Numbers**: 3 shards failed at Configure again, 10 packages installed
before this fix, 11 after it, 3 fatal stops the modules can raise

Doxygen moved the shard past `cmake/dbc-gate.cmake` and one include
further, into the coverage module. That module is included by
`cmake/dev-mode.cmake:17`, so it runs on every developer-mode configure,
and it stops when no lcov stands on PATH:

```text
CMake Error at cmake/coverage.cmake:7 (message):
  lcov is required for developer builds (constitution VI: 100% line and
  branch coverage).  Install lcov and reconfigure.  Coverage is not disabled
  when the tool is absent.
Call Stack (most recent call first):
  cmake/dev-mode.cmake:17 (include)
  CMakeLists.txt:946 (include)
```

### The third reading

```bash
gh api --allow-escape-sequences \
  repos/charlesarcher/speedgun-ng/actions/jobs/121308544924/logs
```

The endpoint needs the escape flag because the runner log carries terminal
escape sequences, and `gh api` refuses such a body without it.

A search for every program the configure looks for, over the cmake
modules and the two registration files, names the whole set: the six
autotools probes of `cmake/ImportAutotoolsSubmodule.cmake:164` to
`:172`, lcov and genhtml at `cmake/coverage.cmake:5` and `:15`,
doxygen at `cmake/dbc-gate.cmake:9`, and the interpreter at
`cmake/docs-ci.cmake:40`. Three of those searches can raise a fatal
stop: doxygen, lcov with genhtml, and the interpreter, the last only on
the docs path. The Debian lcov package carries genhtml, so one package
answers the second stop.

The install step now holds eleven packages and covers all three stops,
so the shard configure has no program left to ask for.

Each of the three gaps cost one CI cycle to find, because no local
reading shows the shard's environment: this host carries every one of
these tools. The exhaustive search above is the reading that would have
found all three at once, and it is the reading this section keeps.

Verdict: the shard install set is complete against the configure path.
The modules name every program that path asks for, and the install step
carries a package for each of them.
