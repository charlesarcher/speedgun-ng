# speedgun-ng

<p align="center">
  <img src="docs/images/sg.jpg" alt="speedgun-ng" width="280">
</p>

A C++23 benchmarking framework in the spirit of Google Benchmark.

The name and the idea come from Speedgun, a tool I worked on at Akuna.
I wanted something like it for personal projects, so this is a
ground-up rethink: Speedgun Next Generation. It shares no code with
that project. Everything here is written by the author and a local AI
army.

## Build

speedgun-ng builds on Linux. Linux is the supported platform, on the
distribution families the CI jobs cover: Ubuntu for the main jobs and Rocky
Linux for the container job. macOS and Windows are unsupported, and no build
is offered for either. A future specification may add a platform.

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

Install:

```sh
cmake --install build
```

Use from CMake:

```cmake
find_package(speedgun-ng REQUIRED)
target_link_libraries(your_target PRIVATE speedgun-ng::speedgun-ng)
```

## Contracts

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

## Re-pinning hwloc

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

## Re-pinning simdjson

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

## Re-pinning HdrHistogram_c

HdrHistogram_c is a vendored git submodule at `external/hdrhistogram_c`,
pinned by commit. To update it:

1. Check out the new tag: `git -C external/hdrhistogram_c checkout <tag>`
2. Commit the submodule pointer.
3. Bump the version assertion in
   `source/hdrhistogram/hdrhistogram_gate.cpp`.

The build fails until the assertion matches. That is the point: the
compile-time check keeps the pinned sources and the recorded version
in lockstep. HdrHistogram_c is a native CMake library, so no autotools
bootstrap tools are needed for it. Its logging support uses the
vendored zlib: the redirect in the root CMakeLists.txt resolves that
lookup to `external/zlib`, so no system HdrHistogram_c or zlib package
is consulted during the build.

## Re-pinning zlib

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

## Re-pinning yaml-cpp

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

## Re-pinning quill

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

## Re-pinning pmu-events

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

The coverage gate runs as its own build target, and the `coverage-linux`
preset makes lcov and genhtml hard requirements: configuration fails when
either is absent. genhtml loads the `GD.pm` perl module when it runs,
which no configure-time check can see, so the GD perl package of the
distribution has to be installed as well. That package name varies
(`perl-gd` on Arch-family, where the lcov package does not pull it in).
Without the module, `cmake --build build/coverage -t coverage` exits
non-zero after the coverage summary has printed its verdict. The exit
code then reports the missing module, and the gate keeps its own
verdict.
