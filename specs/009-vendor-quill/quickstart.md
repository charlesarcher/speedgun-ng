# Quickstart: Validating the quill Import

**Feature**: `009-vendor-quill` | **Date**: 2026-09-30
**Spec**: [spec.md](spec.md) | **Contracts**: [build integration](contracts/build-integration.md), [non-exposure](contracts/privacy-contract.md)

Runnable checks, keyed to the requirements and criteria they prove. Nothing
here duplicates implementation detail; the tasks own that.

## Prerequisites

```sh
git submodule update --init external/quill
```

Host tools: the same set the four existing vendor imports need. quill is a
native CMake project, so no autotools bootstrap is required. quill's build
requires the platform threads package, which every existing job already
installs.

## 1. The build works and the pin holds (SC-001, SC-006, SC-007)

```sh
cmake --preset=dev
cmake --build --preset=dev
```

Expected: succeeds. quill's headers compile through the wrapper unit.

```sh
git submodule status external/quill
```

Expected: the recorded commit is `eb802a37c7d585840324886a3d8648c9c2159952`,
with no `+` or `-` prefix.

```sh
grep -E '^QUILL_' build/dev/CMakeCache.txt
```

Expected: the two entries upstream sets itself, `QUILL_MASTER_PROJECT`
and `QUILL_ENABLE_GCC_HARDENING`, and nothing else. Every option this
bracket pins is inert, so none reaches this project's configuration
(FR-013a).

Read the cache file, which is where the configuration lives.
`cmake -LA build/dev | grep -i quill` does not answer the question.
The vendored tree prints a `-- QUILL_*: OFF` configure-status line for
each option the bracket pins, so that form returns eleven lines on a
correct tree, every one of them reporting a pinned default. It was
replaced here for the same reason T011's check was: a check that cannot
pass on a correct tree proves nothing. FR-013a's claim is about the
configuration, and the cache file is where the configuration lives.

A developer who passes `-DQUILL_BUILD_TESTS=ON` on the command line
still leaves a cache entry no code consumes, because the build system
records a command-line value before any ingestion runs and the pins make
every later declaration inert. FR-013a places that entry outside this
specification's requirements and its removal is the developer's action.

## 2. The strict-warning build stays clean (SC-001, SC-011)

```sh
cmake --preset=ci-ubuntu
cmake --build build -j 2 2>&1 \
  | grep -E '(warning|error|note):' \
  | grep -F 'external/quill/'
```

Expected: no output. The unoptimized development preset cannot answer
this; the release preset can (SC-001).

The pattern reads the build for diagnostics whose location names a header
under the vendored tree, which is what SC-011 asks. It replaces an
earlier `grep -i quill` that matched nothing diagnostic: every line it
matched named this feature, and no line named a finding. Among them are
the target names `quill` and `quill_dependency_check`, the object path
under `tools/quill/`, and the two `Checking ...` lines clang-tidy emits
once per translation unit. That check could not pass on a correct tree.
This one is not vacuous either: the release build's diagnostics give it
something to discriminate among, and it matches a planted line naming a
header under `external/quill/`.

A diagnostic from this project's own translation unit is expected and is
not what this check audits. `tools/quill/quill_dependency_check.cpp`
reports findings of its own under the committed `.clang-tidy`, and
`include/speedgun-ng/` reports more through it. Those are this project's
diagnostics, which the analyzer gate exists to surface (Constitution
VIII), and SC-011 scopes its claim to diagnostics originating in a
vendored header.

## 3. The dependency is real, end to end (SC-009)

```sh
ctest --test-dir build/dev -R quill --output-on-failure
```

Expected: the purity scan and the runnable dependency check both pass. The
runnable check is the authoritative proof: it links the library and quill,
drives a real counter, emits the value, and asserts the value reached the
log. A pass that only shows a compile is not a pass (FR-008a).

## 4. The shipped archive stays lean (SC-009a)

Build the same tree twice in the same configuration, once with the ingestion
and once without, and compare the archives:

```sh
ls -l <build-with-quill>/libspeedgun-ng.a <build-without-quill>/libspeedgun-ng.a
```

Expected: growth under 64 KB. The wrapper unit's object is 3,744 bytes in the
release configuration, the configuration SC-009a names, so the cap sits about
seventeen times above the measured cost. A jump into the hundreds of kilobytes
means a quill symbol reference crept into a shipped unit, which FR-008 forbids.

Compare like configurations only. The same unit is about 319 KB in a debug
build, almost entirely debug information, so comparing a debug archive against a
release archive would read as a failure when nothing is wrong.

## 5. The tripwire fires on a wrong pin (SC-006)

```sh
git -C external/quill checkout v12.2.2
cmake --build --preset=dev 2>&1 | grep -i version
```

Expected: the compile fails, naming the expected version 13.0.0 and the
version found.

```sh
git -C external/quill checkout v13.0.0
```

Expected: back to the pinned commit, and the build is green again. This is
also the re-pinning rehearsal for step 8.

## 6. The submodule worktree stays pristine (SC-007)

```sh
git status --short external/quill
```

Expected: empty. The vendored tree configures and builds out of tree, so a
full build leaves it untouched.

## 7. The privacy contract holds (SC-002 through SC-005, SC-008)

Run after `cmake --install`, each audit against its own prefix. Audit IDs
are defined in
[contracts/privacy-contract.md](contracts/privacy-contract.md).

| Audit | Command | Expected |
|-------|---------|----------|
| A1 | `find prefix/ -iname '*quill*'` | empty |
| A2 | `grep -riE 'quill' prefix/lib/cmake/speedgun-ng/*.cmake` | empty |
| A7 | discovery-call grep over the build files, `external/` and `specs/` excluded | zero hits |

For the shared-build audits, configure with the shared preset, install to a
separate prefix, then:

| Audit | Command | Expected |
|-------|---------|----------|
| A3 | `grep -riE 'quill' prefix-shared/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake` | empty |
| A4 | the allowlist membership check written out below | exit 1, no output |
| A4 | `nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so \| grep '8fmtquill'` | empty, admits no exception |
| A5 | `ldd prefix-shared/lib/libspeedgun-ng.so \| grep -iE 'quill'` | empty |

A5 may name a platform threading entry. That is a platform property. Anything
naming quill is a leak (R-011).

A4 is a set-membership check, and an empty output passes it:

```sh
cat > /tmp/quill_allowlist <<'EOF'
W _ZN5quill3v136detail13get_thread_idEv
W _ZN5quill3v136detail15get_thread_nameB5cxx11Ev
EOF
nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so \
  | awk '{print $2, $3}' | grep -E '5quill|8fmtquill' | sort -u \
  > /tmp/quill_actual
grep -vxF -f /tmp/quill_allowlist /tmp/quill_actual
```

The last command prints nothing on a correct tree and its exit status is the
verdict: 1 means every exported quill symbol is allowlisted, and an empty
`/tmp/quill_actual` passes as well. A third symbol, a change of symbol type, or
any bundled-formatter symbol fails it. An empty-output requirement would
contradict FR-018, which permits exactly the two symbols in the allowlist, and
would also fail a library that exports none.

Upstream marks both thread helpers `QUILL_ATTRIBUTE_USED`, which forces the
compiler to emit them whether or not anything calls them, and `QUILL_EXPORT`,
which on GCC and Clang resolves to default visibility regardless of this
project's settings. Both are reachable from any translation unit that includes
`quill/Backend.h`, so a shared build exports them, and no project-side change
suppresses that without editing the pinned tree.

Also unchanged and expected to stay unchanged:

```sh
readelf -d build/example/counters_standalone_example | grep NEEDED \
  | grep -vE '\[(libc|libm|libstdc\+\+|libgcc_s)\.so'
```

Expected: empty. The example compiles public headers only, and no public
header includes a quill header, so the example never reaches the dependency
(R-011). If this audit goes red, investigate; do not widen it.

## 8. Re-pinning rehearsal (FR-007, SC-006)

Follow the `## Re-pinning quill` section of `README.md` on a scratch clone:
check out a different tag, commit the submodule pointer, bump the version
constants in `source/quill/quill_gate.cpp`. Expected: the build stays green.
Then bump the pointer without the constants. Expected: the tripwire fires.

## 9. The full gate (SC-001)

```sh
cmake --preset=dev && cmake --build --preset=dev && ctest --preset=dev
cmake --preset=ci-ubuntu && cmake --build build
ctest --test-dir build --output-on-failure --no-tests=error -j 2
cmake --preset=ci-coverage && cmake --build build/coverage -t coverage
```

Expected: every job green, including the coverage gate at 100% line, 100%
branch, and 100% design-by-contract coverage. The vendored headers are
dropped from the trace by the extract allowlist already in
`cmake/coverage.cmake`, whose capture command now carries
`--ignore-errors mismatch` for the reason R-013 records. Confirm the gate
reports 100% lines and 100% branches, and that no path under any vendored
tree appears in `coverage.info`. `genhtml` may still exit non-zero locally
when the perl GD module is absent; README documents that as a local
environment gap, and it is not a coverage verdict.

## 10. The dependency classification gate (R-015)

```sh
ctest --test-dir build/dev -R dbc_dependency_scan --output-on-failure
```

Expected: passes. A new private link edge classifies as a runtime dependency
and fails this gate unless `tools/dbc/dependency_scan.sh` has a branch for
quill. This is the failure mode most easily missed.
