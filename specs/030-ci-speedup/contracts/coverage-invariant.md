# Contract: Coverage Invariant

**Surface**: the `coverage` job's capture and gate path, the trace it
produces, and the two exits of `tools/dbc/coverage_gate.sh`.
**Closes**: FR-040 to FR-044, FR-049.

## What may not move

| Item | Base reading | Rule |
| --- | --- | --- |
| capture command | `cmake/coverage.cmake:57` to `:66`, `--ignore-errors mismatch,negative,inconsistent` | stays exactly as master has it (FR-040) |
| extract filter | `--filter exception,branch,brace` | stays (FR-040) |
| ignore classes on extract, summary, `genhtml` | none | none is added (FR-041) |
| `LCOV_EXCL` markers | master's set | no new marker (FR-040) |
| compiler pins, toolchain sources, gcov matching | none | none is added (FR-042) |
| bar | 100% lines and 100% branches | unchanged; the branch reproduces the base reading of 99.4% lines and 97.5% branches and adds no shortfall (SC-003) |

## Lane parallelism

The `gcda` merge carries no lock, so the instrumented suite runs at a
parallelism the merge tolerates (FR-044). The starting form and the
measured alternative:

| Form | `ctest` line | Chosen when |
| --- | --- | --- |
| serialized | `ctest -j 1 --output-on-failure --no-tests=error` at `ci.yml:71` | first, the default of P-11 |
| locked | `-j "$(nproc)"` with `RESOURCE_LOCK` over the entries that fork exec children | the serialized step costs more than the matrix split returns |

The trace of that lane must carry the exec children's counts. The proof is
the record equality of SC-003, read from the artifact, and the step time
beside the base 334 s.

## Gate exits

`tools/dbc/coverage_gate.sh` reads `lcov --branch-coverage --summary` and
answers:

| Exit | Meaning | Reached by |
| --- | --- | --- |
| 0 | every gated measure reads 100.0%, or no branch record exists after the filters | a full trace |
| 1 | a shortfall: a measure below 100.0% | a real gap |
| 2 | usage fault, or no `lcov` found | a bad call |
| 3 | tool failure: `lcov` exited nonzero and its summary shows every measure at 100.0%, or shows no measure at all | a refused trace (FR-043) |

The script stops swallowing the summary status, which today's `|| true`
does, and the refusal message names `lcov` as the failing tool. A reader of
exit 3 looks at the tool, and a reader of exit 1 looks at the lines.

## Finding-list equality across the analyzer split

Both analyzers read every translation unit they read today, under the same
configuration, flags and header filter (FR-049). The unit set is fixed by
`CMakeLists.txt:145` to `:146` and `:663` to `:664`, which clear both
launchers for a vendored tree, and by `.clang-tidy:21` for the failing
check. `static-analysis` configures `ci-ubuntu`, the same preset `dbc-gate`
and `prose-lint` configure, so the flags match by construction (D-13).

## Verify

```sh
cmake --preset=ci-coverage
cmake --build build/coverage
cd build/coverage && ctest -j 1 --output-on-failure --no-tests=error
cd ../.. && cmake --build build/coverage -t coverage
```

The `coverage` target runs `lcov`, the extract step, the gate
(`cmake/coverage.cmake:96` to `:97`) and `genhtml`, and writes
`build/coverage/coverage.info`. Then, on a CI run of the branch, download
the `coverage-info` artifact and compare it against the base tree's
artifact:

```sh
diff <(sort base.info) <(sort branch.info) | head
```

The two files agree on the `SF` file list, and every `DA` and `BRDA` record
matches, which is SC-003. A run with `CCACHE_DISABLE=1` and a warm-cache
run of the same commit produce that same pair of traces (FR-010). The exit
3 path is reached with a fixture trace that makes `lcov` refuse, and the
fixture is a real trace file, with a fabricated pass standing nowhere in
the suite.
