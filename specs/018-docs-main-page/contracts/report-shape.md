# Contract: Sample Output Shape

Normative behavior of the check that keeps the sample output block equal
in shape to the report the quick start program really prints. FR-007,
FR-034, SC-012. The check reuses the row-shape mask the harness test
suite already owns (`test/source/harness_cli_test.cpp`: a digit run folds
to one `#`, a whitespace run folds to one space, any other character
copies; widths are deliberately not compared because a sanitizer run
shifts field padding). The python check adds one fold to that rule:
whitespace directly after an `=` goes with it, because a value one column
wider eats that padding, so one host prints `= #` and another `=#` for the
same column. The `build=` field of the header joins that
exemption for the same reason: it records the configuration of the build
that ran, so the sanitize and coverage builds print another value for one
and the same page (S-2).

## S-1: The sample block

1. The main page carries exactly one block whose info string is
   `sg-sample-output=first_benchmark.expected#report` (grammar:
   `contracts/example-drift.md` R-2). The block's stored text is that
   region; this contract compares that text against the live run by
   shape, and the drift gate holds the block byte-equal to the region.
2. The block is labeled in prose as one host's run (FR-007).
3. The block holds the header line and every row line the program prints
   for its default run, in print order, no shell prompt, no command echo.

## S-2: Shape rule

Mask the block lines and the captured stdout lines with the fold above,
then compare the masked sequences line for line.

| Difference | Verdict |
| --- | --- |
| Any digit value differs | pass (the mask folds it) |
| A value is wide enough to consume the padding after `=` | pass (widths are not compared) |
| The `build=` field of the header names another build | pass (the build that ran owns it) |
| A column header missing, renamed, or reordered | fail |
| A row line missing or added | fail |
| An aggregate line appearing or vanishing | fail |

## S-3: Capture

The check runs the quick start binary itself, with no arguments, exactly
as the page's run command does, and captures stdout to end of process.
The program is one benchmark of one family, so the printed line sequence
is fixed (spec Assumptions: no library change in this feature).

## S-4: Registration and CI

```cmake
if(TARGET first_benchmark)
  add_test(NAME report_shape
           COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../tools/docs/report_shape.py
                   --binary $<TARGET_FILE:first_benchmark>
                   --readme ${CMAKE_SOURCE_DIR}/README.md)
  set_tests_properties(report_shape PROPERTIES TIMEOUT 120)
endif()
```

1. The `test` job's existing `ctest` step runs it on the release build of
   the `ci-ubuntu` preset, so one CI job both runs the harness example
   binary and shape-compares its report (FR-034).
2. With `BUILD_EXAMPLES=OFF` the test is simply not registered; the
   mandatory suite still exists for `--no-tests=error`.
3. A timeout bound of 120 seconds guards a wedged run.

## S-5: Mask fidelity fixture

`tools/docs/report_shape.py` re-implements the fold in python. Its own
runner `test/example-drift-fixture/run_report_shape_fixtures.py` carries
two case sets. The check must be able to fail on the real comparison in
addition to the fold, which is what the second set proves.

Mask golden pairs (pinned constants):

1. a digit-run change folds equal;
2. a whitespace-run change folds equal;
3. a header rename folds different;
4. a width-only change (one digit wider) folds equal;
5. a value wide enough to eat the padding after an `=` folds equal;
6. a `build=` field naming another build folds equal, a renamed build field
   folds different.

End-to-end cases against a scratch page plus a fake-binary script that
prints pinned stdout (the planted-probe discipline):

1. scratch page whose sample block matches the pinned stdout in shape:
   pass;
2. planted renamed column header in the scratch block: fail, named;
3. planted missing row line: fail, named;
4. planted extra aggregate line: fail, named;
5. a run whose header names another build: pass;
6. a run whose wider values eat the padding after an `=`: pass.

The runner imports the mask and the comparator from
`tools/docs/report_shape.py`; the import error is its TDD-red state.
Registration:

```cmake
add_test(NAME report_shape_fixtures
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/example-drift-fixture/run_report_shape_fixtures.py
                 ${CMAKE_SOURCE_DIR})
```

S-5 registers the runner unconditionally (its cases run against a fake
binary script and a scratch page, no example target needed); only
S-4's `report_shape` carries the `if(TARGET first_benchmark)` guard.

The golden pairs are machine-consumed constants checked in both
directions, the prose-gate fixture discipline (specs/002 precedent).
