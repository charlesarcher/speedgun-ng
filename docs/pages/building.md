# Building speedgun-ng

The maintainer facts behind the main page: the platform claim, the
contract-check facility and its gate, the archive versions a consumer
links against, how each vendored dependency is pinned and re-pinned, and
the quality gates a change has to clear. The main page carries the
reader-facing quick start and points here.

## Platform

speedgun-ng builds on Linux. Linux is the supported platform, on the
distribution families the CI jobs cover: Ubuntu for the main jobs and Rocky
Linux for the container job. macOS and Windows are unsupported, and no build
is offered for either. A future specification may add a platform.

## Contract checks

Public interfaces carry design-by-contract annotations: doxygen
`\pre`/`\post`/`\invariant` plus runtime enforcement through the
`SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT`, and `SG_ASSERT` macros
(`include/speedgun-ng/dbc.hpp`, spec `001-dbc-facility`). The
`speedgun-ng_CONTRACTS` cache option selects the semantic: `ignore`
(release: semantic-gated checks emit no code), `observe` (report and
continue), `enforce` (report and terminate; the dev and CI default), or
`quick_enforce` (terminate without reporting). The `SG_*_ALWAYS`
variants enforce in every configuration.

Documentation-to-enforcement pairing is a hard gate. Run it locally:

```sh
cmake --build build/dev -t dbc-gate
```

CI enforces the pairing in the `dbc-gate` job, and the
`consumer-release` job proves release builds carry no semantic-gated
contract code. The C++26 migration mapping lives in
`docs/pages/dbc-migration.md`; measured enforcement overhead:
`docs/pages/dbc-overhead.md`.

## Archive versions

Both shared archives carry a hand-kept `SOVERSION` in `CMakeLists.txt`.
Deriving the number from the major version would make every `0.x` release
claim the same ABI, so the value is stated by hand and the package
config's compatibility requirement tracks the minor version position.

Release 0.8.0, the next minor above 0.7.0, moved the harness archive from
`SOVERSION` 3 to 4 and left the counters archive at `SOVERSION` 2. Three
exported layouts changed: `State` gained the pause bookkeeping and the
manual accumulator, `ResultRow` gained `cpuTimePerIterationNs`,
`endpointSamples`, and `timeAvailability`, and `BenchmarkResult` gained
`timeUnit`. No existing signature was replaced: the report row layout
changed and the registration and timing surface kept its shape.

Release 0.9.0, the next minor above 0.8.0, moved the harness archive from
`SOVERSION` 4 to 5 and left the counters archive at `SOVERSION` 2. The
exported layout moves behind it: the row model gained the counter columns
(`Counter`, a per-row `counters` vector and a row `label`), the aggregate
statistics (`StatisticValue`) and the complexity fit (`FitRow`) joined the
row shape, and the `BigO`, `CounterFlag`, `CounterBase` and `StatUnit`
enums joined the public surface. `State` gained `setCounter`, `setLabel`
and `setComplexityN` and `BenchmarkHandle` gained `addStatistic`,
`reportAggregatesOnly` and `complexity` at the same time; member
functions alone move no layout, and the number records the data changes
above.

Release 0.10.0, the next minor above 0.9.0, moved the harness archive
from `SOVERSION` 5 to 6 and left the counters archive at `SOVERSION` 2.
The move came from data members: `State` gained the thread index and the
thread count of the context running the loop, and `BenchmarkResult`
gained the disclosure vector of the run's contexts. The execution-context
types and callbacks that joined the surface at the same time add no
layout to the exported types that already stood.

Release 0.11.0, the next minor above 0.10.0, moved the harness archive
from `SOVERSION` 6 to 7 and left the counters archive at `SOVERSION` 2.
`State` gained the chunk fields and the chunk cadence and `ResultRow`
gained the percentile columns, data members that move those layouts;
`BenchmarkHandle` gained the percentile selector at the same time, a
member function that moves no layout on its own.

## Dependency re-pinning

Every vendored dependency is a git submodule pinned by commit, and every one
carries a tripwire that fails until the recorded version matches the checked
out sources. The steps below name the pin location and the tripwire of each
dependency.

### hwloc

hwloc is a vendored git submodule at `external/hwloc`, pinned by commit.
To update it:

1. Check out the new tag: `git -C external/hwloc checkout <tag>`
2. Commit the submodule pointer.
3. Bump the version assertion in `source/hwloc/hwloc_gate.cpp`.

The build fails until the assertion matches. That is the point: the
compile-time check keeps the pinned sources and the recorded version in
lockstep.

Building the vendored autotools tree needs autoconf, automake, libtool
and patch:

```sh
# Debian-family
sudo apt-get install autoconf automake libtool patch
# RPM-family
sudo dnf install autoconf automake libtool patch
```

### simdjson

simdjson is a vendored git submodule at `external/simdjson`, pinned
by commit. To update it:

1. Check out the new tag: `git -C external/simdjson checkout <tag>`
2. Commit the submodule pointer.
3. Bump the version assertion in
   `source/simdjson/simdjson_gate.cpp`.

The build fails until the assertion matches. That is the point: the
compile-time check keeps the pinned sources and the recorded version
in lockstep. simdjson is a native CMake library, so no autotools
bootstrap tools are needed for it.

### hdrhistogram_c

HdrHistogram_c is a vendored git submodule at `external/hdrhistogram_c`,
pinned by commit. To update it:

1. Check out the new tag: `git -C external/hdrhistogram_c checkout <tag>`
2. Commit the submodule pointer.
3. Bump the version assertion in
   `source/hdrhistogram/hdrhistogram_gate.cpp`.

The build fails until the assertion matches. That is the point: the
compile-time check keeps the pinned sources and the recorded version in
lockstep. HdrHistogram_c is a native CMake library, so no autotools
bootstrap tools are needed for it. Its logging support uses the
vendored zlib: the redirect in the root CMakeLists.txt resolves that
lookup to `external/zlib`, so no system HdrHistogram_c or zlib package
is consulted during the build.

### zlib

zlib is a vendored git submodule at `external/zlib`, pinned by commit.
To update it:

1. Check out the new tag: `git -C external/zlib checkout <tag>`
2. Commit the submodule pointer.
3. Bump both version assertions in `source/zlib/zlib_gate.cpp`: the
   `ZLIB_VERSION` string and the `ZLIB_VERNUM` number. The number is
   the version digits read as hex, so 1.3.2 corresponds to `0x1320`.

The build fails until both assertions match. That is the point: the
compile-time check keeps the pinned sources and the recorded version
in lockstep. The vendored zlib carries the `Z_PREFIX` rename of its
public symbols in every configuration: the rename lives in the root
CMakeLists.txt bracket, and the gate proves the renamed archive symbol
resolves. zlib is a native CMake library, so no autotools bootstrap
tools are needed for it.

### yaml-cpp

yaml-cpp is a vendored git submodule at `external/yaml-cpp`, pinned
by commit. To update it:

1. Check out the new tag: `git -C external/yaml-cpp checkout <tag>`
2. Commit the submodule pointer.
3. Bump the expected-version constant in the `import_yaml_cpp`
   bracket of `CMakeLists.txt`.

Configure fails until the constant matches. That is the point: the
check keeps the pinned sources and the recorded version in lockstep.
Unlike the simdjson and HdrHistogram_c assertions, which the compiler
checks against a version macro, this tripwire is configure-time:
yaml-cpp 0.9.0 exposes no version macro, so the bracket reads the
version declaration from the submodule's own `project()` line and
consults no git metadata. yaml-cpp is a native CMake library, so no
autotools bootstrap tools are needed for it.

### quill

quill is a vendored git submodule at `external/quill`, pinned by commit.
To update it:

1. Check out the new tag: `git -C external/quill checkout <tag>`
2. Commit the submodule pointer.
3. Bump the version assertions in `source/quill/quill_gate.cpp`.

The build fails until the assertions match. That is the point: the
compile-time check keeps the pinned sources and the recorded version in
lockstep. Unlike the simdjson, HdrHistogram_c, hwloc, and zlib
assertions, which read a preprocessor macro, this tripwire reads compiled
constants: quill publishes its version as `quill::VersionMajor`,
`VersionMinor`, and `VersionPatch` in `quill/Backend.h` and publishes no
macro for it. Its own parsed version variable does not reach a consuming
scope either, so a configure-time read of that variable is unavailable too.

quill is header-only, so two things differ from the other vendored
dependencies and both are deliberate. No vendored archive is built, so
there is nothing to merge into `libspeedgun-ng.a`, and the gate unit holds
the version assertions and no symbol reference: taking the address of one
quill function emits its whole inline call graph, which measured about
150 times the size of the assertions alone. The dependency is proven at
run time instead, by the `quill_dependency_check` test, which links the
library and quill, drives a real counter, and asserts the counter's value
reached the log. The gate unit's include path is also marked as a system
include, so a diagnostic from a quill header never reaches this project's
warning set.

Every one of quill's own build options is pinned at its default inside the
ingestion bracket, so a command-line flag cannot switch on upstream tests,
examples, documentation, or sanitizers, and a future upstream option
defaults to off with no change here.

### pmu-events

The kernel x86 PMU event tables are vendored data under
`external/pmu-events`, pinned to a kernel tag by the `RECORD` manifest
holding the ref, fetch URL, date, and per-file sha256. Two checks
enforce the pin: the configure-time tripwire in the pmu-events bracket
of `CMakeLists.txt` fails configuration when the gate constant and the
recorded ref disagree, and

```sh
python3 tools/pmu_events/update_pmu_events.py --check
```

verifies every file's sha256 against `RECORD` and the gate constant
against the recorded ref, exiting 1 and naming each drifted file. CI
runs `--check` in the test job; the check never touches the network.
To re-pin:

```sh
python3 tools/pmu_events/update_pmu_events.py --to <kernel-ref>
```

The tool fetches the snapshot from kernel.org cgit, with the GitHub
mirror as the documented fallback, validates that every JSON file
parses and every mapfile regex compiles, and replaces the tree only
after all validation passes. It rewrites `RECORD`, bumps the gate
constant (the single bump point in the bracket), and prints a
per-architecture event-count digest. Omitting `--to` resolves the tag
from the running kernel per the source-ref policy DCR (owner directive
2026-09-26): distro and localversion suffixes are stripped, so
`7.2.4-1-cachyos` resolves to `linux-7.2.4`. The fixture suite runs
through `ctest -R pmu_events_check`.

## Quality gates

The style gate is judged by `clang-format` 18.1.8, the version the
`lint` CI job installs. A newer local formatter disagrees with it in
both directions: it reports findings on files the pinned one calls
clean, and a tree it reformats is one the pinned formatter rejects.

```sh
cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake
cmake --build build/dev -t format-check
```

Pass `FORMAT_COMMAND` when the `clang-format` on your `PATH` is not
18.1.8. `format-fix` rewrites the files in place.

Two static analyzers ride the compile of each translation unit.
`CMAKE_CXX_CLANG_TIDY` carries clang-tidy and `CMAKE_CXX_CPPCHECK` carries
cppcheck, each set by the configure preset of the same name
(`CMakePresets.json:33-43`). The `ci-ubuntu` preset inherits both, so it is
the analyzer build; `ci-rocky` carries neither. No target runs either
analyzer on its own: an analyzer speaks where a compiler speaks, and a name
such as `analyzer-gate` stands for no target in `cmake/` or
`CMakeLists.txt`. CI runs both in the `test` job, which installs clang-tidy
and cppcheck and builds the `ci-ubuntu` preset
(`.github/workflows/ci.yml:165-174`).

The `ci-ubuntu` and `ci-rocky` presets share the `build` directory, and the
Unix Makefiles generator judges a file by its timestamp, so a `build` tree
another preset already warmed recompiles nothing, and a recompile-free build
prints no analyzer output at exit 0. An analyzer pass needs a tree no
preset has built:

```sh
rm -rf build
cmake --preset=ci-ubuntu
cmake --build build
```

One check carries the hard part of the verdict. `.clang-tidy` sets
`Checks: "*"` with nine negations (`.clang-tidy:9-19`), so an analyzer build
prints a warning-rich log, and `WarningsAsErrors` names
`readability-identifier-naming` (`.clang-tidy:21`) as the check that ends
the build. cppcheck runs with `--inline-suppr` and no exit-code flag, so its
findings print and the build continues.

One command runs the prose and commit-message gate over the range from
the merge base with `origin/master` to `HEAD`, the same verdict CI
produces:

```sh
cmake -P cmake/prose-lint.cmake
```

Direct and build-target equivalents:

```sh
python3 tools/prose/prose_gate.py --check all
cmake --build build/dev -t prose-lint
cmake --build build/dev -t prose-lint-fixtures
```

Exit 0 is clean, 1 reports findings, and 2 signals a usage error or an
unusable environment. The vocabulary and thresholds live in
`tools/prose/prose_rules.yaml`, a mechanical projection of constitution
Principle XI and the Pull Request Quality template. The gate's own
fixtures run through `ctest -R prose_gate_fixtures`.

Two further gates watch the main page. `example_drift` compares every
code block on the page with the example region it names, checks the
feature-section list, the quick-reference rows against the public headers,
the option rows against the command-line table, and the substance list the
main page must stay free of:

```sh
python3 tools/docs/example_drift.py .
```

`report_shape` runs a built quick start binary and compares its printed
report with the sample block on the page by shape, so a number difference
passes and a renamed or missing column fails:

```sh
python3 tools/docs/report_shape.py --binary build/example/first_benchmark \
  --readme README.md
```

`snippet_compile` emits each C++ block of the page and syntax-compiles it
over the canonical includes, after rejecting one planted broken probe:

```sh
bash test/snippet-compile/run.sh .
```

The coverage gate runs as its own build target, and every developer-mode
preset makes lcov and genhtml hard requirements: configuration fails when
either is absent, because `cmake/coverage.cmake` runs from the
developer-mode block of `CMakeLists.txt`. The `coverage-linux` preset
adds the instrumented build that produces the trace the gate reads.
genhtml loads the `GD.pm` perl module when it runs,
which no configure-time check can see, so the GD perl package of the
distribution has to be installed as well. That package name varies
(`perl-gd` on Arch-family, where the lcov package does not pull it in).
Without the module, `cmake --build build/coverage -t coverage` exits
non-zero after the coverage summary has printed its verdict. The exit
code then reports the missing module, and the gate keeps its own
verdict.

### Validating the host record

The `validate` target of a developer build writes the record of the running
host and reads the set back, and the reader answers over any record root:

```sh
cmake --build build --target validate
./build/test/host_record_check --root . --records validation
```

One record stands per CPU type at `validation/<key>.json`, named by the key
the mapfile spells for that identity, and it carries the CPUID identity, the
catalog directory that mapfile selects, the machine layout, and the evidence
rows naming the file, command, exit and line each claim was read from.

The pre-push run carries one step for the record set: `tools/ci/prepush.sh`
builds the `validate` target at its step 11, and a failing target stops the
run with the status the target answers. CI carries the committed-record
check of the same set: the `test` job runs the reader alone over the records
under `validation/` against the schema under `docs/schema`, through
`tools/ci/host_record_gate.sh`. The check writes no record and reads no
host, so it stands free of the runner's CPU, and a record that does not
validate fails the check with the one line that quotes the file and the
reason.

The reader guards each file against `docs/schema/host-record-v1.json` and
asks four staleness rules in order: a record version behind the version that
schema names, an evidence path absent under the tree, a cited line absent in
that file, and a mapfile that answers no row for that identity.

Each program answers 0 or 2. The writer answers 0 once its file stands
written, and the reader 0 with one line per record carrying the word
`valid`. Either answers 2 with the one line that names its case: an unusable
argument, an absent schema, an unreadable mapfile, a root with no `.json`
entry, or a reading that reads `broken` or `stale`.
