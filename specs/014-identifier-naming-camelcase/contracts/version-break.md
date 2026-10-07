# Contract: version break

The rename breaks the API and the ABI on the 0.x line. FR-008 chooses
0.5.0, shared-object version 2, and `SameMinorVersion` kept.

## Fields the closing commit edits

| Field | Site | After |
| --- | --- | --- |
| Project version | `CMakeLists.txt:7` | `0.5.0` |
| Shared-object version | `CMakeLists.txt:47` | `2` |
| Header version note | `include/speedgun-ng/counters_measurement.hpp:25` and `:31` | records 0.5.0 and shared-object version 2 |

`cmake/install-rules.cmake:39` stays `COMPATIBILITY SameMinorVersion`.

Derived consumers of `PROJECT_VERSION` need no separate edit:
`CMakeLists.txt:40`, `cmake/install-rules.cmake:3`,
`cmake/dbc-gate.cmake:26`, and `docs/Doxyfile.in:7`.

No other header carries `0.4.1`. No changelog file exists. The version
list entry for 0.5.0 is recorded in this feature's `plan.md`. Closed
plans at `specs/012-counters-defect-resolution/plan.md:170` and
`specs/013-counters-defect-followup/plan.md:126` stay unedited (FR-016).

## Package request

A package request for version 0.4 rejects a 0.5 package, because
`SameMinorVersion` compares the minor component. No in-tree consumer
passes a version to `find_package` today. The generated
`ConfigVersion.cmake` is the check. `quickstart.md` names the configure
that shows the rejection.

## Shared builds

`cmake/variables.cmake:9` defaults `BUILD_SHARED_LIBS` to `OFF`. It does
not force the parent library off. `CMakeLists.txt:137`, `:232`, `:384`,
and `:558` force the option off only around vendored subdirectories.
`.github/workflows/ci.yml:251` configures a shared parent. `SOVERSION`
is live in that job. The bump from 1 to 2 is the ABI record that job
links.

## What stays

The feature adds no deprecated alias. It does not rename a file, a
CMake target, a CMake option, a preset, or the package.
