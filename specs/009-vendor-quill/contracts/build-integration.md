# Contract: quill Build Integration

**Feature**: `009-vendor-quill` | **Date**: 2026-09-30
**Implements**: FR-001 through FR-015a; research R-001 through R-009, R-013, R-015.
**Companion**: [privacy-contract.md](privacy-contract.md) for the non-exposure surfaces.

## 1. Interface

The ingestion is one bracket and two lines in the root `CMakeLists.txt`.
No new build-system module, no new preset, no cache option of this
project's own.

```text
function(import_quill)
  # 1. abort when the submodule is uninitialized          FR-004
  # 2. define all upstream options at upstream defaults  FR-013a  R-003
  # 3. CMAKE_POLICY_DEFAULT_CMP0077 NEW                 FR-013    R-003
  # 4. neutralize the global build-type mutation         FR-012    R-004
  # 5. add_subdirectory(external/quill ... EXCLUDE_FROM_ALL)      R-001
  # 6. promote the interface include to a system include FR-010a   R-006
  # 7. assert the build type is unchanged                FR-012    R-004
endfunction()

import_quill()

target_sources(speedgun-ng_speedgun-ng PRIVATE source/quill/quill_gate.cpp)
target_link_libraries(
    speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:quill::quill>
)
```

Bracket placement: after `import_yaml_cpp()`, so the global `install()`
override defined inside `import_hdrhistogram()` still guards it.

## 2. What the bracket must not contain

| Absent | Reason | Requirement |
|--------|--------|-------------|
| Any flag-clearing block | quill compiles nothing, so nothing in the bracket would be affected | FR-010b, R-006 |
| Any archive-merge registration | quill produces no archive to merge | FR-008, R-007 |
| Any `find_package` or `pkg_check_modules` naming quill | the submodule is the only source | FR-005, R-001 |
| Any install-tree reference to `external/quill` | the vendored headers are never installed | FR-016 |
| Any `QUILL_*` option left user-settable | a command-line flag could pull in upstream tests, examples, docs, or sanitizers | FR-013a, R-003 |
| Any symbol reference to quill in a shipped unit | measured cost at `-O3 -DNDEBUG` is 563,104 bytes against 3,744 bytes for the version constants alone, a factor of about 150 | FR-008, SC-009a, R-008 |

## 3. Upstream options pinned by the bracket

All at their upstream defaults. Listed so a reviewer can check the count
against upstream. The bracket is not the evidence.

`QUILL_NO_EXCEPTIONS`, `QUILL_NO_THREAD_NAME_SUPPORT`,
`QUILL_USE_SEQUENTIAL_THREAD_ID`, `QUILL_DISABLE_NON_PREFIXED_MACROS`,
`QUILL_DISABLE_FUNCTION_NAME`, `QUILL_DETAILED_FUNCTION_NAME`,
`QUILL_DISABLE_FILE_INFO`, `QUILL_ENABLE_ASSERTIONS`,
`QUILL_BUILD_EXAMPLES`, `QUILL_BUILD_MODULE`,
`QUILL_BUILD_EXAMPLE_PROMETHEUS`, `QUILL_BUILD_TESTS`,
`QUILL_ENABLE_EXTENSIVE_TESTS`, `QUILL_BUILD_BENCHMARKS`,
`QUILL_SANITIZE_ADDRESS`, `QUILL_SANITIZE_THREAD`, `QUILL_BUILD_FUZZING`,
`QUILL_CODE_COVERAGE`, `QUILL_USE_VALGRIND`, `QUILL_ENABLE_INSTALL`,
`QUILL_DOCS_GEN`, `QUILL_ENABLE_TIME_TRACE`

`QUILL_MASTER_PROJECT` is set upstream with `FORCE` to false when the tree
is consumed as a subdirectory and is left alone.

## 4. Wrapper unit obligations

`source/quill/quill_gate.cpp` holds the version tripwire and nothing else.

```text
#include <quill/Backend.h>          # the one quill header this unit sees

static_assert(quill::VersionMajor == 13);      FR-002, FR-003, R-002
static_assert(quill::VersionMinor == 0);
static_assert(quill::VersionPatch == 0);
static_assert(quill::Version == 130000);
static_assert(<the versioned inline namespace resolves>);   R-002
```

No symbol reference. No call. No quill macro defined before the include.
Zero executable lines, matching the shape of the four existing wrapper units
that the coverage gate already passes.

## 5. Runnable dependency check obligations

Per FR-008 and FR-008a. Registered as a CTest entry with a name distinct
from the `*_nm_proof` family (R-012).

```text
link:      speedgun-ng library + quill::quill
obtain:    a counter value from the speedgun-ng counter interface
emit:      that value through quill's logging entry point
assert:    the value appears in what reached the log
exit:      0
excluded:  the installed artifact, the exported target set, the shipped archive
```

## 6. Invariants

| Invariant | Enforcement |
|-----------|-------------|
| The submodule commit is the pinned one | `git submodule status`; FR-001 |
| No quill option reaches this project's configuration | FR-013a, R-003 |
| No system quill is ever discovered | FR-005, FR-016, FR-018; audits A2, A3, A4, A7 |
| The configured build type is unchanged by the ingestion | FR-012, SC-007; R-004 |
| The submodule worktree stays pristine | FR-015, SC-007 |
| The shipped archive gains under 64 KB | FR-008, SC-009a; R-008 |
| Diagnostics from vendored headers never reach this project's gate | FR-010a, FR-010b; R-006 |
| No quill line reaches the coverage trace | FR-010; R-013, verified during implementation |

## 7. Failure modes

| Trigger | Expected behavior | Requirement |
|---------|-------------------|-------------|
| `external/quill` empty | configure aborts naming `git submodule update --init external/quill` | FR-004 |
| Submodule at a revision declaring a version other than 13.0.0 | compile fails naming expected and found | FR-002, SC-006 |
| Upstream's constants move or are renamed | compile fails; the tripwire is the canary | FR-002, R-002 |
| A `QUILL_*` option passed on the command line | ignored; upstream's declaration is inert | FR-013a, R-003 |
| An upstream option added after the pin | defaults to off with no bracket change | FR-013a, R-003 |
| The global build-type mutation moves the configured build type | configure aborts naming both values | FR-012, R-004 |
| A vendored header emits a diagnostic | suppressed by the system include marking | FR-010a, R-006 |
| The example's link manifest gains an entry | that audit fails; it is left unwidened and the cause investigated | R-011 |

## 8. Re-pinning

Follow the `## Re-pinning quill` section of `README.md`: check out the new
tag, commit the submodule pointer, bump the version constants in
`source/quill/quill_gate.cpp`. Configure and compile both stay green on a
supported bump; the tripwire fires if the pointer moves and the constants do
not (FR-007, SC-006).

If a future upstream release changes the option set, step 3 of the bracket
grows and the README section stays as written: a new option defaults to off
without a code change (R-003).
