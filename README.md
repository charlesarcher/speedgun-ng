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
# macOS: BSD patch ships with the OS
brew install autoconf automake libtool
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

## Quality gates

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
