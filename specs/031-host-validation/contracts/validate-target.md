# Contract: the `validate` target

Phase 1 output. The developer entry point (spec FR-001, FR-002, FR-005,
FR-006). It builds nothing new by itself: it depends on the two programs,
runs the write, then runs the check, and maps a nonzero status to a
failed target, so a failing step fails the target (FR-001).

## Shape

```cmake
set(VALIDATE_OUT "${CMAKE_SOURCE_DIR}/validation" CACHE PATH
    "Where the host record is written and where the reader looks")

add_custom_target(validate
  DEPENDS host_record_write host_record_check
  COMMAND ${CMAKE_COMMAND}
          -D VALIDATE_ROOT=${CMAKE_SOURCE_DIR}
          -D VALIDATE_OUT=${VALIDATE_OUT}
          -D VALIDATE_BIN=$<TARGET_FILE_DIR:host_record_write>
          -P ${CMAKE_SOURCE_DIR}/cmake/validate.cmake)
```

`VALIDATE_OUT` stands as a cache variable, so a configure-time
`-D VALIDATE_OUT=<dir>` reaches both programs through the `COMMAND` line
(FR-002, FR-035).

The target stands in `cmake/dev-mode.cmake` beside the `format-check`,
`spell-check` and `prose-lint` includes, outside `ALL`, and
`cmake/folders.cmake` files it under `UtilityTargets` (FR-002). The script lives
at `cmake/validate.cmake` and maps statuses the way `cmake/prose-lint.cmake:40`
does.

## Knobs

| Knob | Default | Meaning |
| --- | --- | --- |
| `VALIDATE_ROOT` | `${CMAKE_SOURCE_DIR}` | the tree whose `external/pmu-events` mapfile the staleness rules read |
| `VALIDATE_OUT` | the cache variable, `${CMAKE_SOURCE_DIR}/validation` | where the record is written and where the reader looks, a directory the script stands |
| `VALIDATE_BIN` | the built programs' directory | where the two executables stand |

`VALIDATE_OUT` lets a reviewer point the write at the build tree, so the
default check over the committed records stays untouched (FR-003,
FR-035). The script stands whatever directory that knob names, so a
reviewer pointing it under the build tree needs no `mkdir` (FR-002). Both
programs take the tree root as their first argument and a second root
beside it: the writer takes the output root, the reader the record root
(FR-002, FR-028). A build artifact of the two programs stands under the
build directory, where the developer build puts every target, and the
record alone stands under the output root (FR-003).

## Run order

```text
cmake --build build --target validate
  ├─ file(MAKE_DIRECTORY ${VALIDATE_OUT})
  │    the output root stands before the write, the way dbc-gate.cmake:21
  │    stands its directory
  ├─ host_record_write --root <VALIDATE_ROOT> --out <VALIDATE_OUT>
  │    exit 2 ─► the target stops: one line already stands on standard error
  └─ host_record_check --root <VALIDATE_ROOT> --records <VALIDATE_OUT>
       exit 2 ─► the target fails, the lines name the records
```

`cmake/validate.cmake` stands the output root before it invokes the
writer, because `validation/` stands absent at HEAD and the first
`--target validate` run has to work on that tree (FR-002). The writer
creates nothing beyond the record file it names, so a run straight against
`host_record_write` needs its output root standing first, and one it
cannot write refuses with one line and leaves no bytes behind (FR-011). The
default resolves the same way, so a writer run with no `--out` stands its
record at `<root>/validation` under the root the run carries, and the
directory that started the process names nothing (FR-002, FR-028).

Both programs print their own lines; the script prints nothing of its
own, so one run prints the record line naming the CPU type and the file,
then one line per committed record (FR-001, FR-005, FR-024). Each
program's entry point in `source/harness/detail/` parses the arguments
before any host read and answers a status the thin main maps to an exit,
so an unusable argument refuses the same way on every runner, including
one that answers no CPUID, reads no hwloc and opens no `/proc` or `/sys`
path (FR-004, FR-037). The writer's entry point takes the host reading as
a third parameter, `hostRecordWriteMain(argc, argv, facts)` with `facts` of
type `HostFactsSeam`, so an in-process arm names the root its planted facts
answer for (FR-002, FR-032, FR-037).

## Exits

| Case | Target | Program status |
| --- | --- | --- |
| every record valid | succeeds | 0 |
| the host cannot be named | fails, before the check runs | 2 |
| a machine read fails | fails, before the check runs | 2 |
| the finished record would pass 16 KiB | fails, before the check runs, the standing bytes stay | 2 |
| a standing record cannot be read | fails, the bytes stay | 2 |
| a record breaks or stands stale | fails after every line | 2 |
| the record root holds no `.json` entry | fails | 2 |

## CTest registration

`test/CMakeLists.txt` registers three entries beside the drift entries,
each named by its `add_test(NAME ...)` row:

- `host_record_check` runs the reader with two absolute arguments,
  `--root ${CMAKE_SOURCE_DIR}` and `--records ${CMAKE_SOURCE_DIR}/validation`,
  so `test`, `test-rocky`, `coverage`, `sanitize` and `tsan` validate the
  committed records from files alone, with no hardware, no token and no
  workflow edit. The arguments stay absolute because CTest starts a test in
  the build directory of the `CMakeLists.txt` that added it, `build/test`
  for these entries, and `test/CMakeLists.txt` names no `WORKING_DIRECTORY`
  in any of its 150 `add_test` rows. A relative root would resolve under
  `build/test`, where no schema and no mapfile stand, and the reader would
  refuse a tree it never meant to read. The absolute root carries the
  schema and the mapfile with it, so the entry needs no `WORKING_DIRECTORY`
  and no `--schema` option joins either CLI (FR-020, FR-024, FR-026,
  FR-037, R-09).
- `host_record_check_bogus_option` runs the reader with one option the parse
  cannot use, and its `set_tests_properties` row declares
  `PASS_REGULAR_EXPRESSION` for the line that answers it:
  `error: the option '--bogus' names no arm of the reader`. The parse stands
  ahead of the chooser factory and ahead of any record root, so the arm
  prints that line and exits 2 on a runner of any cpu and layout. The
  reader's other usage arms run in-process through `hostRecordCheckMain`,
  which parses before it asks its factory for a chooser (FR-004, FR-035,
  FR-037).
- `host_record_write` runs one malformed-argument invocation and declares
  `PASS_REGULAR_EXPRESSION` for its refusal line, the shape the
  `counters_trap_checked_rejects_unknown_mode` row already sets for an arm
  that must fail (the file holds no `WILL_FAIL` row). The argument parse
  stands before any host read, so the arm prints one line, exits 2 and
  writes no file on every runner. The pattern pins the printed line: a test
  that prints it passes even at exit 0, so the failing status rests on the
  refusal path the line proves, and T038 and T039 read that status
  in-process. Its three usage arms, the unknown option, the missing value
  and the extra argument, run in-process through `hostRecordWriteMain`, the
  writing entry point under `source/harness/detail/`, which parses before
  any host read and answers a status the test reads (FR-004, FR-037). The
  suite then holds six entries: three test executables and three program
  entries.

The write entry reads no host on purpose. The refusal arms run in
`test/source/harness_host_record_test.cpp` through the writing entry
point: an unnamed identity, an identity a planted mapfile names no row
for, a failed machine read and an unwritable output root, each with a
planted identity and planted machine facts (FR-010, FR-011, FR-032,
SC-004). An entry that wrote on some runners and refused on
others would measure the runner. The live write path runs through the
target, and the lane records it in
`specs/031-host-validation/results/quickstart.md` (FR-041).
