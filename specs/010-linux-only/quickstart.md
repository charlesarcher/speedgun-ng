# Quickstart: Verifying the Linux-only Platform Policy

**Feature**: `010-linux-only` | **Date**: 2026-10-02
**Spec**: [spec.md](spec.md) | **Contract**: [platform policy](contracts/platform-policy.md)

Runnable checks, keyed to the requirements and criteria they prove. Every
command runs on Linux and needs no network. Section 8 is the one that can
fail on a machine whose local preset file still names a deleted preset; run
it first, because its failure makes every later command fail for an unrelated
reason.

## Prerequisites

```sh
git submodule update --init
```

Host tools: the same set the Linux CI jobs install. For the full gate,
`cmake`, a C++23 compiler, `git`, `python3`, `doxygen`, `lcov`, `codespell`,
and `clang-format` 18.1.8. The formatter version is pinned because the
repository's `lint` job installs that version and a newer one disagrees with
it in both directions.

## 1. The local preset file resolves (FR-018a)

```sh
cmake --list-presets
```

Expected: exit 0, and the surviving Linux presets listed with no
`Invalid configure preset` line. CMake validates every preset in the
resolved set before it resolves any single one, so a machine-local preset
inheriting a deleted preset fails this command and takes
`cmake --preset=dev` with it. If it fails, delete the named preset from
`CMakeUserPresets.json`. That file is gitignored, so the fix carries no
tracked diff.

## 2. The gate set names Linux alone (SC-003, FR-001 through FR-003)

```sh
sed -n '/### VIII. CI Quality Gates/,/### IX\./p' \
  .specify/memory/constitution.md | head -12
```

Expected: the hard gate list names Linux and enumerates no other platform,
and the sentence reinstating the Windows gate by an upstream port is gone.

```sh
grep -nE 'ci-macos|ci-windows|AppleClang|MSVC' .specify/memory/constitution.md
```

Expected: only hits inside a `Prior report` block, which records what an
earlier amendment decided and stays. A hit in the body of a principle is a
failure.

```sh
tail -3 .specify/memory/constitution.md
grep -n '| 2.11.0 |' .specify/memory/constitution.md
```

Expected: the version footer reads 2.11.0 with `Last Amended` 2026-10-02, and
the lineage table carries a 2.11.0 row with its rationale.

## 3. No live support claim survives (SC-002, FR-015)

The full token set, over the full path list, each hit classified into one of
the contract's four buckets:

```sh
grep -rniE 'macos|osx|apple|darwin|xcode|windows|win32|msvc|appleclang|mingw|msys|homebrew|brew install' \
  README.md AGENTS.md CMakeLists.txt CMakePresets.json .codespellrc \
  .github/workflows cmake tools test example docs include source \
  --exclude-dir=external
```

Expected: every hit lands in one of three classes, and none lands in the
live-claim class.

- **Preserved seam.** A branch selecting platform behaviour, such as
  `if(WIN32)` in `cmake/ImportAutotoolsSubmodule.cmake` or `#if defined(_WIN32)`
  in `source/counters/clock_provider.cpp`. Stays.
- **False positive.** The word "windows" meaning a measurement window, in
  `include/speedgun-ng/counters_measurement.hpp`, `test/CMakeLists.txt`,
  `test/source/counters_clock_push_test.cpp`, and
  `test/source/counters_recorder_test.cpp`. Stays.
- **Historical.** Any hit inside `specs/`, and the `.codespellrc` comment
  naming the macOS Big Sur codename, which explains a spelling exemption.
  Stays.

The narrow pattern this replaces caught none of the three runtime
diagnostics the feature exists to fix, so the width is the point
(research.md R-004). Record each hit with its path, its line, and its bucket
in the pull request.

## 4. The preserved seams did not move (SC-006, FR-016)

```sh
git diff <base>..HEAD -- cmake/ImportAutotoolsSubmodule.cmake cmake/variables.cmake
git diff <base>..HEAD -- CMakeLists.txt
```

Expected: the `ImportAutotoolsSubmodule.cmake` diff touches only diagnostic
strings and the header comment FR-013a names. No hunk in any of the three
files touches an `if(WIN32)`, `if(APPLE)`, `if(MSVC)`, or `if(UNIX)` line,
and none touches the `MSVC`, `WIN32`, or `CMAKE_HOST_WIN32` shims.

```sh
git diff <base>..HEAD -- source include test example
```

Expected: empty. No C++ file changes at all.

```sh
git diff --name-only <base>..HEAD
```

Expected: exactly five files, `.specify/memory/constitution.md`,
`CMakePresets.json`, `README.md`, `AGENTS.md`, and
`cmake/ImportAutotoolsSubmodule.cmake`.

## 5. The preset set is Linux-only and every survivor resolves (FR-009, FR-010, SC-004)

```sh
python3 - <<'EOF'
import json, re
d = json.loads(re.sub(r'//.*', '', open('CMakePresets.json').read()))
names = {p['name'] for p in d['configurePresets']}
gone = {'flags-appleclang', 'flags-msvc', 'ci-darwin', 'ci-win64',
        'ci-macos', 'ci-windows', 'ci-multi-config'}
print('configure presets:', len(names))
print('survivors:', len(names - gone))
bad = [(p['name'], i) for p in d['configurePresets']
       for i in ([p['inherits']] if isinstance(p.get('inherits'), str)
                 else p.get('inherits', [])) if i in gone]
print('dangling inherits:', bad or 'none')
EOF
```

Expected: 15 survivors, and no dangling `inherits`. The seven deleted presets
form a closed cluster under `inherits`, so no survivor is left with a deleted
parent (research.md R-001).

```sh
cmake --preset=dev
cmake --preset=ci-ubuntu
cmake --preset=ci-coverage
```

Expected: all three exit 0. These three cover every inheritance shape the
survivors use: a single parent (`ci-linux-ignore`), a two-element list
(`ci-linux`), and a five-element list (`ci-ubuntu`). A fresh binary directory
needs `doxygen` on `PATH`, because `cmake/dbc-gate.cmake` requires it; the
configured trees in `build/` already satisfy that.

## 6. The CI matrix is untouched (FR-011, SC-005)

```sh
grep -cE '^  [a-z-]+:$' .github/workflows/ci.yml
grep -nE 'runs-on:' .github/workflows/ci.yml | grep -v ubuntu-26.04
```

Expected: 12 top-level keys, of which 11 are jobs and one is the `push`
trigger, and every `runs-on` line reads `ubuntu-26.04`. The `test-rocky` job
additionally names a `rockylinux:10` container, which is a second Linux
distribution. A second operating system is a different thing.

```sh
git diff <base>..HEAD -- .github/
```

Expected: empty. No job is added, removed, or changed (research.md R-007).

## 7. The gates pass on this head (SC-007)

```sh
cmake --build --preset=dev -j 8
ctest --preset=dev
cmake --preset=ci-ubuntu && cmake --build build -j 8
cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake
cmake --build build/dev -t prose-lint
cmake --build build/dev -t spell-check
cmake --build build/dev -t dbc-gate
```

Expected: every one exits 0, and `ctest` reports 42 of 42. Pass
`FORMAT_COMMAND=clang-format-18`: the repository's `lint` job installs that
version, and a newer local formatter names files the pinned one calls clean.

## 8. The merged record is intact and superseded by name (SC-008, FR-017)

```sh
git diff <base>..HEAD -- specs/003-vendor-hwloc specs/004-vendor-simdjson \
  specs/005-vendor-hdrhistogram specs/006-vendor-yaml-cpp \
  specs/009-vendor-quill
```

Expected: empty. The five merged vendor specs keep their original platform
text, each stating that Linux is the enforced gate, that macOS must keep
building for developers, and that Windows stays possible by design.

```sh
grep -nE 'specs/00(3|4|5|6|9)-' .specify/memory/constitution.md
```

Expected: at least one hit in the amendment's Sync Impact Report, naming
every superseded spec. A reader who finds the old platform text in a merged
spec learns from the constitution that it no longer holds.

## 9. The one open item this feature does not close

`specs/009-vendor-quill` T038 asked for a `ci-macos` build result. It retires
by reference when the amendment closes the constitution's macOS deferral
entry, which is S6. No file under `specs/` is edited to achieve that, so
T038 remains marked open in its own feature's task file and is closed by the
supersession the amendment records.