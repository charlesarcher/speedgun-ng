# The analyzer row (T057)

Host `gary`, clang-tidy from LLVM 23.1.4, cppcheck Cppcheck 2.17.1, c++ (GCC) 16.2.1 20260810. Head `99f3d650`,
preset `ci-ubuntu`: Release with `-Werror`, clang-tidy wired in as
`CMAKE_CXX_CLANG_TIDY` with `--header-filter` over the source tree, and cppcheck as
`CMAKE_CXX_CPPCHECK` with `--inline-suppr`. Logs: `/tmp/041-notes/an-conf.log`,
`an2-build.log`, `an3-build.log`.

## How the row was taken, and one attempt that was not a reading

`cmake --preset=ci-ubuntu` over a tree already built answers in 0.1 s and recompiles
nothing: the first attempt logged 540 lines, named no check, and said nothing about
anything. Both analyzers run inside a compile, so a row needs the compiles. The row below
comes from `cmake --build build --target clean` and a full rebuild, then a second build to
finish the translation units the first pass never reached.

| Pass | Exit | What it said |
|---|---|---|
| rebuild from clean, `ci-ubuntu` | 2 | one error-class finding, in a file this branch adds |
| rebuild after the rename | 0 | 0 error-class findings, 0 make errors, 38337 warning lines |

## The error-class finding, which was real

`.clang-tidy` raises one check to error level:
`WarningsAsErrors: 'readability-identifier-naming'`. The forced pass stopped the build:

```text
test/source/counters_group_probe_test.cpp:323:6: error: invalid case style for function
'sizeOneRUNS_SHORTAnswersNothingScenario' [readability-identifier-naming,-warnings-as-errors]
```

Constitution V, N-2 governs a function name: camelBack, member or not. The scenario name
carried an enumerator's spelling inside it, a naming defect in code this lane wrote, and no
earlier phase of this lane had run the analyzer to see it. The scenario is now
`sizeOneShortRunAnswersNothingScenario`, the same claim in the governed spelling, and the
pass after the rename built with no error-class finding anywhere in the tree. No other file,
lane or master, held one.

## The two cppcheck error-severity findings, both standing at the base

Read the same file out of `origin/master` with the same command and cppcheck answers the
same finding at the same construct:

| Finding | At the head | At the base |
|---|---|---|
| `syntax error [syntaxError]` | `source/harness/runner.cpp:2973` | `runner.cpp:2954` |
| `Non-local variable 'selfPath' will use pointer to local variable 'self'. [danglingLifetime]` | `test/source/harness_topdown_run_test.cpp:3420` | that file, that line |

The first is cppcheck 2.17.1 refusing a lambda that returns `std::size_t` by trailing return
type. This branch adds 19 lines above it in that file, which is the whole of the difference,
and changes nothing in it. The second stands in a file this branch never touches. Neither
stops a build under this CMake integration, and CI pins a different cppcheck, so both are
stated as host readings, each with its base reading beside it. Neither is a finding this
lane owes.

## The warning background, sized

The rebuild from clean named 14244 warning lines across 140 files before it stopped at the
error above, and the finishing pass took the tree total to 38337. The classes are the ones
`.clang-tidy` leaves at warning level on this host's clang-tidy, which is not the version CI
pins: `readability-identifier-length`, `readability-trailing-comma`,
`cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`, `misc-include-cleaner` and
`readability-simplify-boolean-expr` lead the list. Master's own headers carry the heaviest
counts of the pass: 3103 lines in `include/speedgun-ng/counters_measurement.hpp`, 1764 in
`benchmark.hpp`, 780 in `counters_provider.hpp`, and 553 in master's
`counters_linux_pmu_seam_test.cpp`.

The files this branch adds hold 195 warning lines of that same background between them, the
largest 83 in `counters_group_probe_test.cpp` and 62 in `counters_limit_fill_test.cpp`. Those
units script the kernel, so they draw `cppcoreguidelines-pro-type-vararg`,
`cppcoreguidelines-pro-bounds-pointer-arithmetic` and
`cppcoreguidelines-avoid-non-const-global-variables` on their fixture counters by the same
route every other scripted-kernel unit in the tree takes. No check was silenced, no `NOLINT`
was added, and no configuration file changed for this row.

## The pass retaken on the tree this branch carries

Host `gary`, clang-tidy from LLVM 23.1.4, c++ (GCC) 16.2.1 20260810, 2026-11-05, preset
`ci-ubuntu` configured into its own directory with `cmake --preset=ci-ubuntu -B
/tmp/041-tidy` and built whole. Logs: `/tmp/041-notes/an-conf.log` and
`/tmp/041-notes/an-build.log`.

```text
cmake --build /tmp/041-tidy -j 8
exit 0
```

The pass exits 0, so it raised no error-class finding. `readability-identifier-naming` is
the check `.clang-tidy` raises to error, and it answered nothing: the ten lines in the log
that hold that check name are a `NOLINT` comment quoted inside a source excerpt, not
findings. The disclosure unit carries 87 findings of warning class, and the common one is
`cert-err33-c` on a `std::printf` whose return the unit has no use for, the shape every
other unit in the tree holds. The count of warning-class lines over the whole tree reads
52956 at this host's clang-tidy, and CI runs a different version, so the count is a host
figure and the CI rows carry the verdict.

The pass ran in a directory of its own for a reason worth the line. A reconfigure over a
built tree recompiles nothing, so it analyses nothing, and a build of the `clean` target
leaves a tree without test binaries. Configuring into its own directory forces every
translation unit through the analyzer and keeps the release tree runnable for the suite
rows. The two cppcheck findings the earlier section accounts for stand as recorded, and the
`danglingLifetime` note at `test/source/harness_topdown_run_test.cpp:3420` reappeared here
as it does at the base.
