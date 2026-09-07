# Quickstart / Validation: Design By Contract Facility

**Feature**: 001-dbc-facility | **Date**: 2026-09-06

Runnable scenarios that prove the facility works end-to-end. Prerequisite: a
configured dev build (`cmake --preset=dev`). See [plan.md](plan.md) for the
design, [contracts/api-contracts.md](contracts/api-contracts.md) for the public
API, and [data-model.md](data-model.md) for the entities.

## 1. Build with contracts on (default dev semantic = `enforce`)

```sh
cmake --preset=dev
cmake --build --preset=dev -j
```

**Expected**: clean build, no warnings. Contract checks are active
(`enforce`).

## 2. Run the full suite with contracts enabled

```sh
ctest --preset=dev --output-on-failure
```

**Expected**: all tests pass with contracts in `enforce` (SC-005). No test may
require disabling contracts to pass.

## 3. Prove a contract fires (fail side)

Install a test observer, trigger a known violation, and assert the record:

```sh
ctest --preset=dev -R dbc_test --output-on-failure
```

**Expected**: the observer receives a **Violation record** with the correct
kind, file, line, message, and predicate text (SC-003, US3). In a terminating
configuration the same violation terminates the process with a structured
diagnostic naming the kind, file, line, and message.

## 4. Prove a release build is contract-free (SC-002)

Configure a consumer-style release build (developer mode off, `ignore`):

```sh
cmake -S . -B build/release-consumer -D CMAKE_BUILD_TYPE=Release
cmake --build build/release-consumer -j
```

**Expected**:
- The **semantic-gated** trap site (a contract that aborts in a checked build)
  runs silently.
- The **always-on** trap site (`SG_*_ALWAYS`) in the same fixture **still
  fires** — it is the deliberate exception (FR-036).
- Symbol inspection of the artifact shows no semantic-gated contract-machinery
  code (no `sg::dbc::check_*` code for gated sites; always-on dispatch present
  only because the fixture uses it).

This is exactly what the dedicated consumer-release CI job performs (research
R-006/R-011).

## 5. Prove the Phase 0 gate catches drift (SC-006)

Seed the tree with (a) a public function missing a `\pre` or `\post` doc
section, and (b) a documented contract with no matching enforcement, then run
the gate:

```sh
# the dbc-gate CI job does this automatically; locally:
cmake -P cmake/dbc-gate.cmake   # doxygen XML + pairing check
```

**Expected**: the gate fails, naming the offending interface and the
missing / drifted kind. A fully documented-and-enforced interface — including
an explicit `none` marker for a genuinely empty contract set — passes.

## 6. Verify the facility is self-conformant (SC-001)

Run the gate over the facility's own public headers and the conformed
pre-existing interface.

**Expected**: the DBC matrix reports zero gaps and zero drift on the facility
itself and on `exported_class` (conformed in 001, research R-009).

## 7. Prove the header is standalone (FR-037)

Compile a minimal TU that includes only `dbc.hpp` with **no project include
paths**:

```sh
g++-13 -std=c++20 -Wall -Wextra -Wpedantic -c \
  -x c++ - -o /dev/null <<'EOF'
#include "dbc.hpp"   // resolved via -I to include/speedgun-ng only
EOF
```

**Expected**: compiles clean with no project headers in scope (primary design:
standard headers only).

## Out of scope here

The AST-level hard gate (feature 002), dynamic execution / vacuity coverage,
and native C++26 integration are not exercised by this quickstart.
