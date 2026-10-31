# CI record: Host Validation

Task T052, T053 and T054, FR-037, FR-038, FR-039, SC-008, SC-012. Two readings
stand recorded here: the coverage gate over the measured roots, and the suite
under each of the two sanitizer trees. The narrow-mask suite stands in `taskset.md`, the guide
scenarios in `quickstart.md` and the plan of the round in `plan.md`. Every run
below went on the lane host (`GenuineIntel-6-C6-2`, 20 cpus, one L3 group, one
node, `perf_event_paranoid` 2).

## The coverage measure, 2026-10-30, at `062eaa0` with `4552ac0`

### What shapes the measure

`cmake/coverage.cmake` spells the measure in two steps, and both ran as the
file spells them, with no flag added.

| Step | What it names | What it left |
| --- | --- | --- |
| Capture (`lcov -c -q --branch-coverage --no-external --ignore-errors mismatch,negative,inconsistent`) | every instrumented object under `build/coverage` | 326 source files in `coverage.raw.info` |
| Extract (`--filter exception,branch,brace`, two `--omit-lines`) | the roots `include/speedgun-ng/*`, `source/*` and `tools/hostcheck/*` | 64 source files |
| Omit (`--remove`) | `tools/hostcheck/host_record_check.cpp` and `tools/hostcheck/host_record_write.cpp` | 62 measured source files |

The two `--omit-lines` patterns are `SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?`
and `if \(cpus\.empty\(\)\) \{`. Both match code today: the first reaches
contract calls in 42 files under `source/` and `include/`, the second names `source/harness/topology.cpp:395`,
`source/harness/topology.cpp:591` and `source/harness/host_record.cpp:454`. The
three filters fired as well, and the extract step reports their work: 513 exception
instances over 2884 coverpoints, 806 `omit_lines` instances over 1503
coverpoints, 389 brace instances and 2672 branch instances over 7213
coverpoints (`/tmp/sgng-cov3.log`). `tools/hostcheck/mapfile.hpp` stays inside
the measure, because the extract root names the directory and the omit names
only the two `main` units.

### The two units the omit removes

Read off the trace after the extract step and before the omit
(`/tmp/sgng-t053-noomit.info`):

| Unit | Lines | Branch positions |
| --- | --- | --- |
| `tools/hostcheck/host_record_check.cpp` | 3 of 3 | none emitted |
| `tools/hostcheck/host_record_write.cpp` | 2 of 25 | 4 positions, none counted |

Neither unit can carry a measure here, and that is the reason both stand
omitted. `tools/hostcheck/host_record_check.cpp` is one call and one exit: its
`main` parses nothing and hands the parse, the schema read, the chooser and the
answer lines to `sg::detail::hostRecordCheckMain` in
`source/harness/host_record_check.cpp`, which the gate measures, with the chooser
builder `sg::tools::makeDirectoryChooser` from `tools/hostcheck/mapfile.hpp`,
which the gate measures too. Its three instrumented lines are the ones a parse
refusal reaches. `tools/hostcheck/host_record_write.cpp` is the seam its header
names: past the parse its `main` answers the identity through `pmuIdentCurrent`,
the key through `mapfileKey`, the directory through `mapfileDirectory` and
`mapfileText`, and the layout through the standing topology read, then hands all
of it to the library entry point, which reads no host of its own (R-04).
FR-031 and FR-035 bar a test of this repository from CPUID, the installed layout
and the mapfile, so the suite drives no line past the parse and the 100.0% line
bar has no run to stand on for code no test reaches. What such a run owes the
reader stands pinned instead by the CTest entries `host_record_check`,
`host_record_check_bogus_option` and `host_record_write`: one malformed
argument, the line the parse prints, status 2, all of it before any host read
(`FR-010`, `FR-037`). All three pass in every tree of this record, and each is
the reason the two units still enter the trace.

### The run and the gate reading

```sh
cmake --preset=ci-coverage                      # fresh configure into build/coverage
find build/coverage -name '*.gcda' -delete      # counters zeroed
ctest --test-dir build/coverage -j 1            # the full suite
./build/coverage/test/harness_cli_test          # the supplement, one completed run
cmake --build build/coverage --target coverage  # capture, extract, omit, gate
```

The suite answers 146 entries, 2 skipped (`counters_pmu_test`,
`counters_overhead`), 2 failed and 142 passed in 1155.29 s
(`/tmp/sgng-cov3.log`). The supplement run of `harness_cli_test` ends `ok` with
exit 0, so the trace holds that entry's counts, which a 300 s stop would lose.
1155.29 s is what ctest reports for the run above. The two timestamp files of
the earlier session, `/tmp/sgng-cov-ctest-start.txt` and
`/tmp/sgng-cov-ctest-end.txt`, carry 02:56:08 to 03:17:14, the wall of an earlier
run whose CLI entry was killed at its slot.

`tools/dbc/coverage_gate.sh` over `build/coverage/coverage.info` exits 1:

```text
  source files: 62
  lines.......: 100.0% (14160 of 14160 lines)
  branches....: 99.7% (6864 of 6888 branches)
coverage-gate: branch shortfall 24 rows exceeds the  D-14 recorded allowance of 21
```

The line metric, the strictest one, is met. Of the 62 measured files, 55 read
100.0% lines and 100.0% branches. Seven carry the 24 short positions, and their
lines read 100.0% everywhere:

| File | Short positions | Branch positions |
| --- | --- | --- |
| `source/harness/cli.cpp` | 12 | 418 |
| `source/harness/json_load.cpp` | 3 | 1004 |
| `source/counters/linux_pmu/provider.cpp` | 2 | 117 |
| `source/harness/execution_context.cpp` | 2 | 68 |
| `source/harness/host_record_check.cpp` | 2 | 78 |
| `source/harness/options.cpp` | 2 | 496 |
| `source/counters/linux_pmu/fast_read.cpp` | 1 | 18 |

SC-008 asks the four files this feature adds under `source/` for 100.0% lines
and 100.0% branches. `source/harness/host_record.cpp` answers 100.0% lines over
253 and 100.0% branches over 82, and `source/harness/host_record_check.cpp`
answers 100.0% lines over 235 and 97.4% branches over 78, short only at the two
positions of line 184 that `tools/dbc/coverage_gate.sh` records.
`tools/hostcheck/mapfile.hpp` answers 100.0% lines over 38 and 100.0% branches
over 8. The two private headers `source/harness/detail/host_record.hpp` and
`source/harness/detail/host_record_check.hpp` declare constants and signatures
without a body, so gcov emits no line for them and the trace holds no row for
either. Eight files enter with no branch position at all and read `-`:
`barrier.hpp`, `counters_provider.hpp`, `dbc.hpp`, `speedgun-ng.hpp`,
`harness/detail/hunt_plan.hpp`, `harness/detail/internal.hpp`,
`simulation/marker.cpp` and `speedgun-ng.cpp`. No `LCOV_EXCL` marker joined this
change (`source/harness/host_record.cpp`, `source/harness/host_record_check.cpp`
and `tools/hostcheck/mapfile.hpp` hold none), no `--ignore-errors` class joined,
no keep-set pattern left, and no allowance row left the gate.

## The reconciliation the report owes FR-038

The allowance lives in `tools/dbc/coverage_gate.sh` as `allowance=21` with two
recorded rows. Row one carries the 19 positions of the 2026-10-26 round (D-14).
Row two carries 2 positions, `source/harness/host_record_check.cpp` line 184,
`branch 22 -> 23` and the same line's `branch 22 -> 24`, the unwind tail of the
`EvidenceRow` construction there; `8b09d9b` landed them and `4552ac0` wrote the
sizing paragraph. The sizing counts the union of the CI coverage job, where
every entry runs to its end.

That union is downloadable: run 40983302766 at `bea6af5b`, artifact
`coverage-info`. It reads 100.0% lines over 13910 and 16 branch rows:
`source/harness/cli.cpp` at 255 four times, 943, 1101, 1138, 1249 twice, 1331
twice and 1426; `source/harness/execution_context.cpp` at 469 and 470;
`source/harness/options.cpp` at 1427 twice. This host's 24 hold those 16
position for position, and add 8: the two at
`source/harness/host_record_check.cpp:184` that row two carries, and 6 that the
runner covers, `source/counters/linux_pmu/fast_read.cpp:197`,
`source/counters/linux_pmu/provider.cpp:567` and `:595`, and
`source/harness/json_load.cpp:130` twice and `:1356`. Those 6 stand past the
first stop of the two entries that run to their end in CI and do not here. Both
were run alone in the coverage tree and the trace was re-captured, and all 6
stayed at zero, so the early stop is the whole of the difference. The allowance
gains no row for them: a row would excuse on the runner that covers those 6 what
only this host leaves dark. For CI the arithmetic closes at 16 rows at master, 18
with this feature's two, inside the 21 the file holds. A host run that reports
the 6 reports the host, and this run does.

`make coverage` fails at this gate on this host. That is the gate working as
designed: it reads a host where two entries stop early, and it reports the
shortfall they leave.
The line gate, the strictest one, passes here at 100.0% over 14160 and passes in
CI at 100.0% over 13910.

FR-038 also asks which keep-set patterns and which allowance rows no longer match
code. Every pattern in `cmake/coverage.cmake` still matches: three roots that all
return files, two `--omit-lines` patterns that match 42 files and three lines
respectively, and three `--filter` kinds that all report work. Of row one's 19
positions the CI union reports 16, so 3 recorded positions no longer appear in a
current trace. This record does not name the 3, because row one lists its 19 in
the round report of 2026-10-26 and the naming belongs to the owner of that
record. Nothing has been removed from the allowance, and FR-038 asks before any
pattern or row is removed, so both questions stand open here.

## The sanitizer trees, 2026-10-30

`cmake --preset=ci-sanitize` configures `build/sanitize` with
`CMAKE_BUILD_TYPE=Sanitize` and
`-U_FORTIFY_SOURCE -O2 -g -fsanitize=address,undefined -fno-omit-frame-pointer
-fno-common`. `cmake --preset=ci-tsan` configures `build/tsan` with
`-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -O2 -g -DNDEBUG -fsanitize=thread
-fno-omit-frame-pointer -fno-common` on the Release configuration,
`-fsanitize=thread` on the Release link flags, and `SPEEDGUN_DBC_MATRIX=ON`. Both
CI jobs pin `CXX: clang++` (`.github/workflows/ci.yml:153`, `:236`), so this host
used clang 23.1.4 for both trees. The environment is CI's, quoted at the run:
`ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1`
and `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`
(`.github/workflows/ci.yml:213`, `:219`), and
`TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1`
(`.github/workflows/ci.yml:299`).

The two trees do not register the same CTest set, so each reading names its
count: 146 entries under the address sanitizer and 147 under the thread
sanitizer. Four allocation-proof entries stand outside the thread tree, guarded
on the Release flags at `test/CMakeLists.txt:329`, `:645`, `:860` and `:1103`;
five `dbc_matrix_*` entries join it through `SPEEDGUN_DBC_MATRIX=ON`; 142 entries
stand in both.

Suite readings at `fcaf65d`, `-j 16`, both trees rebuilt first: 146 entries in
301.89 s and 147 entries in 303.16 s, each with 2 failures, and both failures are
the two host entries of this record. The six entries of this feature passed in
both rebuilt trees at `4552ac0`, and had passed there at `fcaf65d` too:
`harness_host_record_schema_test`, `harness_host_record_test`,
`harness_host_record_check_test`, `host_record_check`,
`host_record_check_bogus_option` and `host_record_write`, at `#128` to `#133` in
the address tree and `#129` to `#134` in the thread tree.

`harness_cli_test` cannot reach its end inside CTest under either sanitizer.
`test/CMakeLists.txt:1891` registers `TIMEOUT 300` for it, and a registered
property beats `ctest --timeout`, so `ctest -R '^harness_cli_test$' --timeout
3600` stopped at 300.11 s with exit 8. The binary run directly, in the same
environment and with `timeout 3600` as the only guard, reaches its end:

| Tree | Binary built at | Wall | Exit | Reading |
| --- | --- | --- | --- | --- |
| `build/sanitize/test` | 05:57:36 | 693.2 s | 0 | `harness_cli_test: ok`, no report line |
| `build/tsan/test` | 05:57:45 | 705.4 s | 0 | `harness_cli_test: ok`, no report line |

The first attempt at this pair wasted one run: a relative `cd` sent the second
run back to the address tree, so it timed the address binary twice, and its log
(`/tmp/sgng-t053/direct2-tsan.log`) holds no thread-sanitizer evidence. The
address reading above came from the first run of that attempt, taken alone, and
the thread reading above is the re-run.

The same pair ran at 03:41 on binaries of 03:04:46 and 03:28:30, before the write
path was fixed, and closed in 590.1 s and 610.8 s with exit 0. The table above is
the pair at the head this record measures.

No report of any family appears in any log of this round: no `AddressSanitizer`,
no `LeakSanitizer`, no `ThreadSanitizer`, no `runtime error:` from the undefined
behavior sanitizer, no `data race`, no `lock-order-inversion`. One stop stays
unattributed: `final-tsan.log:327` records `145/147 Test #68: harness_cli_test
... Subprocess terminated***Exception: 113.50 sec` in a suite that otherwise ran
all 147 entries in 131.79 s. No driver killed it, the journal holds no
out-of-memory record since 02:00, and no coredump names the process. The entry
passes in every other reading of that tree, including the direct run above.

## The two host entries and the toolchain

`counters_linux_pmu_seam_test` stops at its FR-019 assertion:

```text
 43/146 Test  #43: counters_linux_pmu_seam_test ...................***Failed    0.36 sec
COUNTERS LINUX PMU SEAM TEST FAIL: a vendored row lands on the one device its Unit scope names and on no other (FR-019)
seam placement: device 'cpu' holds 0 merged rows
```

`harness_cli_test` stops at its CTest slot, two lines into its output:

```text
 65/146 Test  #65: harness_cli_test ...............................***Timeout 300.10 sec
no benchmark matches the filter '^nothingHere$'
SKIP the metric columns: no metric row of this host's table resolves to a leaf its catalog publishes (FR-025, FR-046)
```

The seam entry reaches its assertion and fails there in every tree of this
record. The CLI entry reaches its end only outside CTest: once as the coverage
supplement above, once in each sanitizer tree.

Toolchain of the coverage measure: lcov 2.6-0, gcov (GCC) 16.2.1 20260810,
`c++` (GCC) 16.2.1 20260810, CMake and CTest 4.4.5, Linux 7.3.0-4-cachyos. The
two sanitizer trees used clang 23.1.4.

## The static-analyzer reading, 2026-10-30, at `7c5fbc8`

Task T062, beside T055, per FR-039 and SC-012. Two readings stand here. The first sweeps the
whole tree the `ci-ubuntu` preset configures. The second re-analyses the seven translation units
this feature adds at the tip this record names. Both ran on the lane host
(`GenuineIntel-6-C6-2`, 20 cpus, one L3 group, one node).

### What the analyzer job runs over

`cmake --preset=ci-ubuntu` wires both tools into the compile. Read off the cache of the tree used
here, `build-tidy`, configured with `-B` because the preset declares no `binaryDir`:

| Cache row | Value |
| --- | --- |
| `CMAKE_BUILD_TYPE` | `Release` |
| `CMAKE_CXX_CLANG_TIDY` | `clang-tidy;--header-filter=^<repo>/;--exclude-header-filter=^<repo>/external/` |
| `CMAKE_CXX_CPPCHECK` | `cppcheck;--inline-suppr` |
| `CMAKE_CXX_COMPILER_LAUNCHER` | `ccache` |

`.clang-tidy` sets `Checks: "*"` at line 9, then lists nine exclusions and re-enables
`fuchsia-multiple-inheritance` inside that block, sets `HeaderFilterRegex:
'/include/speedgun-ng/.*|/source/.*'` at line 20, and `WarningsAsErrors:
'readability-identifier-naming'` at line 21, followed by the naming option rows the house spelling
rests on. clang-tidy therefore reads every translation unit the build compiles, together with the
project headers under `include/speedgun-ng/` and `source/` that reach it. `external/` stands
excluded by name. cppcheck runs over the same units under its own rule set, and `--inline-suppr` is
the only flag the preset adds.

CI names its scopes as three shards of that one preset (`.github/workflows/ci.yml:313`). `library`
builds `speedgun-ng_speedgun-ng` and `speedgun-ng_harness` (`:382`), `tests` builds `build/test`
(`:389`), and `examples` builds `build/example` (`:395`). Each shard collects its findings, uploads
them, and ends with a step that reads the `WarningsAsErrors` line out of `.clang-tidy`, counts the
findings whose check stands on that list, and fails when the count rises above zero
(`:429`). A finding on any other check prints a warning, and the master files print thousands of
those already. That list is the gate this record measures against.

### Versions run on this host

clang-tidy from LLVM 23.1.4, cppcheck 2.22.0, CMake and CTest 4.4.5, and `g++` (GCC) 16.2.1
20260810 as the compiler both analyzers drive. The job installs clang-tidy and cppcheck from apt on
`ubuntu-26.04` and pins no version (`.github/workflows/ci.yml:357`), so the counts below belong to
this host the way the coverage counts belong to lcov 2.6-0 and gcov 16.2.1.

### The sweep of the whole tree, at `ccc664d`

`cmake --preset=ci-ubuntu -B build-tidy`, then `cmake --build build-tidy -j 12`, then a second pass
with `-- -k` so every target reaches its analysis after the first stop. The sweep emitted 37,566
diagnostics of the form `file:line:col: warning:` across 232 files, and handed 356 translation units
to cppcheck. Three findings stood at error severity:

| File | Line | Finding | Where it stands |
| --- | --- | --- | --- |
| `test/source/harness_host_record_test.cpp` | 331 | `readability-identifier-naming` on the constexpr `key` | cleared here by `373f7e7` |
| `test/source/harness_host_record_check_test.cpp` | 130 | `readability-identifier-naming` on the constexpr `suffix` | cleared by `80c6b08` |
| `test/source/dbc_test.cpp` | 232 | cppcheck `throwInEntryPoint` | open, a file this feature leaves alone |

The two naming findings stopped the compile, because `WarningsAsErrors` names that check, and the
keep-going pass showed them as the only build-breaking rows in the tree. cppcheck reported nothing
else across its 356 units. The 37,566 belong to `Checks: "*"`, and the master headers hold the bulk
of them: 7,098 in `include/speedgun-ng/counters_measurement.hpp`, 5,569 in `benchmark.hpp`, 1,580 in
`counters_provider.hpp`, 960 in `test/source/detail/json_fixture.hpp`, 846 in `hunt.hpp` and 790 in
`counters_core.hpp`.

### The reading over the units this feature adds

Seven translation units re-analysed at `7c5fbc8`: the two library units, the two `main` units and the
three test units. No finding stood at error severity, so the gate check reads clear across the
feature, and cppcheck read all seven units and reported nothing.

| File | Findings | Largest families |
| --- | --- | --- |
| `source/harness/host_record.cpp` | 53 | `cppcoreguidelines-pro-type-vararg` 12, `modernize-use-std-print` 12, `cert-err33-c` 11, `misc-include-cleaner` 4, `performance-string-view-conversions` 4 |
| `source/harness/detail/host_record.hpp` | 0 | none |
| `tools/hostcheck/host_record_write.cpp` | 1 | `misc-include-cleaner` 1 |
| `source/harness/host_record_check.cpp` | 60 | `misc-include-cleaner` 13, `cppcoreguidelines-pro-type-vararg` 8, `modernize-use-std-print` 8, `cert-err33-c` 6 |
| `source/harness/detail/host_record_check.hpp` | 6 | `performance-enum-size` 6 |
| `tools/hostcheck/host_record_check.cpp` | 0 | none |
| `tools/hostcheck/mapfile.hpp` | 4 | `readability-identifier-length` 2, `misc-const-correctness` 2 |
| `test/source/harness_host_record_test.cpp` | 192 | `cppcoreguidelines-pro-type-vararg` 31, `modernize-use-std-print` 30, `readability-container-contains` 26 |
| `test/source/harness_host_record_check_test.cpp` | 149 | `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` 63, `cppcoreguidelines-pro-type-vararg` 18 |
| `test/source/harness_host_record_schema_test.cpp` | 79 | `modernize-use-designated-initializers` 37, `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` 11 |

The two headers under `source/harness/detail/` declare, so the analyzers read them through their
including units. `tools/hostcheck/mapfile.hpp` belongs to both lanes: the chooser lives there and so
does the helper that reads the mapfile directory.

### The cleared count, and the families that stay

Three findings went away during T055, and none of them went away behind a suppression:

| File | Finding | Commit |
| --- | --- | --- |
| `test/source/harness_host_record_test.cpp` | the constexpr `key` against `readability-identifier-naming` | `373f7e7` |
| `test/source/harness_host_record_test.cpp` | a lambda capture of a tree its arguments already carry | `373f7e7` |
| `tools/hostcheck/host_record_write.cpp` | `std::move` reached through another header, `<utility>` now named | `f7aeb00` |

T062 asks for every finding in the six files T055 names to be cleared. 120 advisory findings stand in
those six files, 54 in this lane's three and 66 in the reader lane's three, and they stay. The owner
ruled on 2026-10-30 that the advisory families stay where master leaves them. The reason is the gate
above: it holds one check, CI stands green on that basis today, and the master files carry these same
families by the thousand.

Three families make up 35 of the 53 findings in this lane's library unit, and all three land on
`std::fprintf`: `cppcoreguidelines-pro-type-vararg`, `modernize-use-std-print` and `cert-err33-c`.
That call is how every refusal of this feature speaks and how `source/harness/` speaks elsewhere.
Silencing them would take a marker at each site, which would change no verdict and leave the file
harder to read than it stands. No `NOLINT` marker joined any file of this feature: the count across
the ten files above stands at zero.

One finding stays open and on the record by the same ruling. `misc-include-cleaner` asks that
`source/harness/detail/internal.hpp` stand named in `tools/hostcheck/host_record_write.cpp` beside the
one symbol `sg::detail::layout` that unit reads from it.

## The commit template over the branch, 2026-10-30, at `e497673`

`git log --format='%B%n@@END@@' 1049cce..HEAD` collected every commit message between `1049cce`,
the master base the branch stood on at that head, and that head, and a scan of that text answers
the five checks of T061. The range holds 218 commits.

| Check | Reading |
| --- | --- |
| `<Section>: <one-line imperative>` | 218 subjects open with a section word and a colon, and 0 run past 50 characters |
| a body wrapped at 72 columns | 0 lines stand past 72 columns |
| `Refs: specs/031-host-validation` | 0 messages lack the row |
| `Approved-by:` | 0 messages lack the footer |
| `Fixes #n` | 0 messages carry the row |

The section words follow master's spelling: `Docs` opens 133 subjects, `test` 34, `Harness` 33,
`CMake` 11, `Meta` 4, `CI` 2 and `dbc` 1. Master writes `test:` in 184 of its own subjects and
`dbc:` in 14, so those two lanes keep the spelling the base already carries.

`gh issue list --state open --search 031 --json number,title` answered no open issue, so the
`Fixes` row of the template stands unasked across the range.

`python3 tools/prose/prose_gate.py --check commit --mode range --base 1049cce --head HEAD` reads
the same 218 messages through the kernel of Principle XI and answers `0 findings, 0 skipped` at
exit 0, and `--check all` over the same range answers the same over the 254 sources the branch
touches.

The branch rebased three more times, and the scan re-ran above each new base: 229 messages over
`bea6af5..736e0a2`, 234 over `f9bd6f6..fb0f209`, and 243 over `9ff389f..4795218`. Each run answered
as the first did. The branch now stands on `94c84ff`, and the run over `94c84ff..30a2dfb` covers
252 messages with the same answers: 0 subjects past 50 characters, 0 body lines past 72 columns, 0
messages without the `Refs` row and 0 without the `Approved-by:` footer. `python3
tools/prose/prose_gate.py --check commit --mode range --base 94c84ff --head 30a2dfb` answers `0
findings, 0 skipped` at exit 0.

## The docs build, 2026-10-30, run by `align-3` at `7976b15`

The `docs` job of `.github/workflows/ci.yml` reports nothing on a pull request, because its `if:`
row names `github.ref == 'refs/heads/master'` together with a push event and a repository owner.
Its one build step is the cmake script below, so the records lane ran that step on this host to
read what it answers for this branch. The tree stood at `7976b15`, and the binary directory
`build-docs-rec` stood absent before the run and is held out of git by the `build-*/` line of
`.gitignore`, so no lane's tree was touched and no build cache was reused.

| Reading | Value |
| --- | --- |
| Command | `cmake "-DPROJECT_SOURCE_DIR=$PWD" "-DPROJECT_BINARY_DIR=$PWD/build-docs-rec" -P cmake/docs-ci.cmake` |
| Exit | 0, in 2 s |
| cmake | 4.4.5 |
| Doxygen | 1.18.0 at `/usr/bin/doxygen` |
| Python | 3.14.8 at `/usr/bin/python3`, with jinja2 3.1.6 and Pygments 2.21.0 |
| m.css | came over the network: `build-docs-rec/docs/.ci` stood absent before the run and present after, so the pinned archive at `4a1324c2` downloaded and matched the `EXPECTED_MD5` the script states |
| Output | `build-docs-rec/docs/html`, 152 files, 2.8 MB |

An earlier run of the same step on this host, in `build-docs` with a Python 3.12.13 venv, left the
same 152 files in 2.8 MB and the same 141 warnings, so the reading holds across the two toolchains.

### The 141 warnings the passing step leaves

The step passes and prints 141 Doxygen warnings. One is a tool-version note: the
`M_SHOW_UNDOCUMENTED` line of `docs/Doxyfile.in` sets a tag Doxygen 1.18.0 reports as unsupported
and ignores. The other 140 are unresolved references, and they cover every anchor link in the two
pages the build renders: 107 in `README.md` over 27 targets, and 33 in `docs/pages/harness.md` over
`hunter-mode`, `jitter-probe` and `the-noise-tracer`. The quick reference of the root page writes
its Teaches column as `[registration](#registration)`, while no `{#registration}` header stands
anywhere and m.css names its section anchors `#autotoc_mdNN`. Doxygen drops the link, and
`index.html` ships 5 cells reading `registration` with 0 links behind them.

The renderer pass adds 70 warnings of its own: 29 runs of inline code fall back to code blocks, 16
listings report an unrecognized language `.text`, 15 program listings carry no filename attribute,
9 files of language Markdown are skipped, and 1 image lookup misses `docs/images/sg.jpg`.

### The anchor defect stands upstream, and no gate reads it

The evidence puts the fault below this branch. `git show 94c84ffd:README.md` counts the same 107
links over the same 27 targets at the base, and `git diff 94c84ffd..HEAD -- docs/pages/harness.md`
names no change, so that page stands untouched between the base and here. `git blame` on sampled
rows of the root page names `c289d4ba`, `cc840f20` and `612c9ce6`, and each one is an ancestor of
that base. Nothing in the docs family reads a reference: `cmake/docs-ci.cmake` builds and states
nothing about links, `tools/docs/example_drift.py` weighs page lines against `PAGE_LINE_CAP`, and
the `docs` job runs a build step and a deploy step. That is why no gate reports the 140 today.

This branch leaves those links alone. The record carries the finding, its counts, its blame, and
the gate that misses it, so a later change can own the fix.

## The Actions run over `30a2dfb`, read 2026-10-30 (T060, T063)

Run `41127479202` belongs to PR #60, head `30a2dfb`, and closed at conclusion `failure`. It holds
fifteen jobs over thirteen check names, three of them shards of `static-analysis`. Every check
reported except `docs`, which the run holds as skipped. Ten shard-level checks are green, four are
broken: `test-rocky`, `coverage`, `consumer-release` and `dbc-gate`.This lane pulled the four failing job logs with `gh api
repos/charlesarcher/speedgun-ng/actions/jobs/<id>/logs`, and re-ran the pair gate on this host over
an extracted checkout of the base `94c84ffd` whose `CMakeLists.txt`, `source/harness/cli.cpp` and
`include/speedgun-ng/benchmark.hpp` hash-match that commit. Job windows run from the API's own
`startedAt` to `completedAt`.| Check | Verdict | Reading |
| --- | --- | --- |
| `lint` | green | 39 s, the docs drift gate among its steps |
| `prose-lint` | green | 47 s |
| `test` | green | 8 min 12 s, the whole CTest suite on the Ubuntu runner |
| `sanitize` | green | 10 min 59 s |
| `tsan` | green | 15 min 7 s |
| `shared-audit` | green | 2 min 25 s |
| `downstream-consumer` | green | 2 min 51 s |
| `static-analysis` | green | three shards green: library 10 min 50 s, examples 12 min 3 s, tests 42 min 57 s from 20:59:47Z to 21:42:44Z |
| `coverage` | broken | branch shortfall of 29 rows against the recorded allowance of 21 |
| `dbc-gate` | broken | `pair-gate: 1 of 319 interfaces drifted` |
| `consumer-release` | broken | 14 errors under `-Werror` with GNU 15.2.0 |
| `test-rocky` | broken | `99% tests passed, 2 tests failed out of 158` |
| `docs` | skipped | its `if:` row names a push to `master`, so a pull-request run never builds docs, and its own step stands recorded above |

### `coverage`: the gate's arithmetic and where its rows come from

The line metric reads 100.0% over 15035 of 15035 lines, so the strict line check passed. The branch
metric reads 99.6% over 7795 of 7824, and the shortfall is that dark count: 7824 minus 7795 is 29,
against `allowance=21` in `tools/dbc/coverage_gate.sh`. The script prints every dark branch
position after that line and the log holds all 29. The gate exits 1 on a measured shortfall, and
that is the path this run took: the build shows `Error 1` at
`CMakeFiles/coverage.dir/build.make:75`. The exit 2 the job reports belongs to `make`, whose rule
failed, while the exit 2 the script reserves for a usage fault never happened.

```
lines.......: 100.0% (15035 of 15035 lines)
branches....: 99.6% (7795 of 7824 branches)
coverage-gate: branch shortfall 29 rows exceeds the  D-14 recorded allowance of 21
```

The suite read `100% tests passed out of 153` in this job, with `counters_linux_pmu_seam_test`
passing in 0.54 s and `harness_cli_test` in 12.45 s, so both entries that stop early on this host
ran to their end there. Only `counters_pmu_test` and `counters_overhead` skipped, as they do
wherever no PMU answers. None of the six rows the CI union alone exercises stands among the 29:
`source/counters/linux_pmu/fast_read.cpp:197`, `source/counters/linux_pmu/provider.cpp:567` and
`:595`, and `source/harness/json_load.cpp:130` twice and `:1356` appear nowhere in the log.

| Rows | Position | Blamed at `30a2dfb` | Standing |
| --- | --- | --- | --- |
| 12 | `source/harness/cli.cpp` 256, 985, 1143, 1180, 1294, 1376, 1471 | `9125c9a2`, `2d0d43e2`, `34b3f9e9`, `85937014`, `1c236982` | base |
| 2 | `source/harness/hunt_ranks.cpp` 128, 201 | `8820158b`, `f9bd6f69` | base |
| 2 | `source/harness/execution_context.cpp` 469, 470 | `9be7d4a6`, `8af5f394` | base |
| 2 | `source/harness/options.cpp` 1464 | `14e65836` | base |
| 2 | `source/harness/statistics.cpp` 84 | `0216a05a` | base |
| 7 | `source/harness/hunt_ranks.cpp` 307, 312, 318 | `30a2dfb Harness: Order the rank keys without float equal` | this branch |
| 2 | `source/harness/host_record_check.cpp` 184 branches 8 and 9 | `f74e077f Harness: Read the record one file holds` | this feature, ledgered |

So 20 rows stand on lines the base already carried, 7 rows stand on the three lines the rank-order
fix rewrote, where each `std::partial_ordering` comparison adds arms no entry takes, and 2 rows are
the pair this feature recorded. The allowance of 21 is that pair beside the 19 positions the first
ledger row carries, so the run stands 8 rows above the allowance and the pair this feature owns
stands inside it.

The seven rows blamed on `30a2dfb Harness: Order the rank keys without float equal` stand on no
line today. The fifth rebase onto `e3368112` met
`da5b3e39 Harness: Settle the wave's gate findings`, which orders the same keys through plain `>`
and `<` pairs where our commit had written `std::partial_ordering`, and the replay kept master's
shape and dropped our commit at the conflict. The 20 rows on base lines and the 2 ledgered rows
carry across the replay unchanged, and the seven return to whatever arms the rebased branch leaves
dark.

### `dbc-gate`: the pair gate names `sg::speedgunMain`

The step built the target over a tree configured with `cmake -S . -B build/dev --preset=ci-ubuntu`
, and this is its whole report:

```
Running DBC documentation and pairing gates
doc-gate: 319 interfaces, 0 gaps (/home/runner/work/speedgun-ng/speedgun-ng/build/dev/dbc-gate/doc-matrix.json)
sg::speedgunMain: enforced-not-documented assertion
pair-gate: 1 of 319 interfaces drifted (documented-not-enforced / enforced-not-documented)
gmake[3]: *** [CMakeFiles/dbc-gate.dir/build.make:74: CMakeFiles/dbc-gate] Error 1
gmake: *** [Makefile:239: dbc-gate] Error 2
```

`doc-gate` passes with 319 interfaces and 0 gaps. The pair gate finds one drift, the interface
`sg::speedgunMain` , the kind `assertion` , the direction `enforced-not-documented` . The
enforcement it reads is `SG_ASSERT(loaded.has_value(), ...)` at `source/harness/cli.cpp:608` ,
inside the body of `speedgunMain` , and the document block of that interface in
`include/speedgun-ng/benchmark.hpp` carries `\pre none` and one `\post` and names no assertion.

`git blame` at `30a2dfb` gives `a5a872ad Harness: Run the ranks through the CLI` for that line, an
ancestor of the base `94c84ffd` , and
`git diff 94c84ffd..30a2dfb -- source/harness/cli.cpp include/speedgun-ng/benchmark.hpp` names no
file, so the branch touched neither side of the pair. The check that settles it ran here over a
checkout of the base:

```
python3 tools/dbc/dbc_pair_gate.py --src include --src2 source \
  --registry tools/dbc/macros.yaml --doc-matrix doc-matrix.json --out pair-matrix.json
```

It printed the same two lines and exited 1, and the matrix it wrote holds one drift row for
`sg::speedgunMain` with enforced kinds `postcondition` and `assertion` against documented kinds
`precondition` and `postcondition` . The base doc-matrix reports 319 interfaces and 0 gaps, the
same answer the job gave at `30a2dfb` . The drift stands at the base.

### `consumer-release`: the release build with the contract macros off

The job configured with
`cmake --preset=ci-linux-ignore -B build/consumer-release -D CMAKE_BUILD_TYPE=Release -D speedgun-ng_DEVELOPER_MODE=OFF`,
a following step read the cache and found `speedgun-ng_CONTRACTS:STRING=ignore`, and the compiler
identified itself as GNU 15.2.0. With the contracts off, `SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT`
and `SG_ASSERT` expand to `((void)0)` in `include/speedgun-ng/dbc.hpp`.
`cmake --build build/consumer-release -j "$(nproc)"` printed 14 errors and stopped:

```
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:60:30: error: unused parameter ‘object’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:60:55: error: unused parameter ‘key’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:67:28: error: unused parameter ‘object’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:67:53: error: unused parameter ‘key’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:74:30: error: unused parameter ‘value’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:81:30: error: unused parameter ‘object’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:81:55: error: unused parameter ‘key’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:88:29: error: unused parameter ‘value’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:95:29: error: unused parameter ‘object’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:95:54: error: unused parameter ‘key’ [-Werror=unused-parameter]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/host_record_check.cpp:336:8: error: variable ‘parsed’ set but not used [-Werror=unused-but-set-variable]
/usr/include/c++/15/bits/unique_ptr.h:192:67: error: null pointer dereference [-Werror=null-dereference]
/usr/include/c++/15/bits/unique_ptr.h:192:67: error: null pointer dereference [-Werror=null-dereference]
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/hunt_ranks.cpp:189:16: error: variable ‘placed’ set but not used [-Werror=unused-but-set-variable]
```

Thirteen of the fourteen stand in `source/harness/host_record_check.cpp`, the file this feature
adds, and they share one cause. `objectOf`, `textOf`, `numberOf` and `arrayOf` read their
parameters inside an `SG_ASSERT` and nowhere else, and `readHostRecord` uses `parsed` there, so
with the macros expanded away those parameters and that variable are unused. The two
`null pointer dereference` errors are reported inside `/usr/include/c++/15/bits/unique_ptr.h:192`
on paths the compiler reached through simdjson `element::is_null()` inlined from `nullableOf`, once
through `identFrom` into `recordFrom` and once through `nodeRowsOf` and `machineFrom` into the same
function. The fourteenth, `placed` at `source/harness/hunt_ranks.cpp:189`, carries `git blame`
`8820158b Harness: Stamp the family and rank figures`, an ancestor of the base, which is the check
that puts that single error below this branch.

### `test-rocky`: the sealed-path arms on a runner that reads as root

`ctest --output-on-failure --no-tests=error -C Release` read the line below and the step exited 8.
Both failing entries belong to this feature, and the two entries that stop early on this host
passed there: `counters_linux_pmu_seam_test` in 0.28 s and `harness_cli_test` in 1.79 s.

```
The following tests FAILED:
  141 - harness_host_record_test (Failed)                 quick
  142 - harness_host_record_check_test (Failed)           quick
  ##[error]Process completed with exit code 8.
```

21 checks failed: 8 in `harness_host_record_test` and 13 in `harness_host_record_check_test`. Every
one of the 21 seals a path before it asks. In the first entry those are the sealed output root
`readonly_root` and the sealed standing record `unreadable_standing`, both sealed with
`std::filesystem::permissions(..., perms::none)`, and the eight names carry arms such as
`a write the output root refuses answers status 2 (FR-011, SC-004)` and
`a standing record no read reaches answers status 2 (FR-011, SC-004)`. In the second entry the
seals are the planted entry file, the planted mapfile and the planted guard answer, planted by the
helper that calls `::chmod(file.c_str(), 0)`, and the thirteen names carry arms such as
`an entry the reader cannot open answers status 2 (FR-004, FR-029)` and
`the chooser of a mapfile it cannot open answers the run refusal (FR-004, FR-025)`.

The log shows what the seals failed to bind:

```
  unreadable standing record: status=0 lines=0
an entry the reader cannot open expects  error: the record run cannot read 'harness_host_record_check_root/pid13340-sealed-entry-line-31/validation/GenuineIntel-6-C6-2.json'
printed  harness_host_record_check_root/pid13340-sealed-entry-line-31/validation/GenuineIntel-6-C6-2.json  GenuineIntel-6-C6-2  valid
```

The job runs inside a container, and a container on this runner reads its user as root, which mode
bits do not bind. The log carries `Starting job container`, `container=oci` and the Rocky Linux 10
base image, and its package steps install with `dnf` and no unprivileged user. So the write into
the sealed root answers status 0, and the read of a file planted at mode 0 returns bytes and
answers `valid`. Every arm that seals nothing answered as its fence spells, and the failure list
names none of them: `a tree with no schema refuses before its factory stands asked`,
`the tie-break leaves the mapfile question unasked`,
`a record root that stands absent answers no refusal`,
`a root that holds no .json entry answers no refusal` and
`a standing set of records answers no refusal` all stand outside the 21.

### What the run leaves standing

`git diff 94c84ffd..30a2dfb -- CMakeLists.txt` changes no line, so the `VERSION` row and the two
`SOVERSION` rows stand unmoved, and `kSchemaVersion` reads `1.10` in
`source/harness/json_report.cpp` at both heads. The diff over `README.md` and `docs/pages` adds no
table row, so no option row and no `Name` row joined a table, which is the reading FR-040 and
SC-012 ask the record to hold.

`coverage` fails on 29 dark rows: 20 stand on base lines, 7 on the three lines this branch's
rank-order fix rewrote, and 2 on the pair this feature recorded. `consumer-release` fails on 13
errors in `source/harness/host_record_check.cpp` beside the one error the base carries.
`test-rocky` fails on 21 checks whose seal binds nothing on a runner that reads as root. `dbc-gate`
fails on a drift that stands at the base and names neither side of this branch. This section
carries the readings, and the changes that answer them belong to their own commits.

## The run over `12d735ab`, 2026-10-30, dead at the format step

Run `41141094040` tested `12d735ab` and closed at conclusion `failure` in 36 s. Its `lint` job ran
32 s and failed, and the other twelve checks report `skipped`, so the run holds no verdict for any
check beside `lint`. The step that failed is
`cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, over a runner where an earlier step
pinned `clang-format==18.1.8`, and its report fills the fence:

```
The following files are badly formatted:

source/harness/host_record_check.cpp
test/source/harness_host_record_check_test.cpp

CMake Error at cmake/lint.cmake:51 (message):
Run again with FIX=YES to fix these files.
```

`52af6826 Harness: Format the record reads and their unit` reformatted both files, and the unit did
not settle there. `32dffcbb test: Keep the planted pairs in named locals` moved the two long
planted documents into named locals, and that commit states the reason: each pass of the pinned
formatter moved the continuation lines of the longest pair again, so the file never reached a
layout the gate would accept. T060 and T063 stay open through this run, which reports nothing to
read.

## The run over `32dffcbb`, read 2026-10-30 (T060, T063)

Run `41143779478` belongs to PR #60, head `32dffcbb`, and closed at conclusion `failure`. It holds
fifteen jobs over thirteen check names, opened 23:19:34Z and closed 23:46:07Z, which is 26 min 33
s. Ten shard-level checks are green, four are broken: `test`, `sanitize`, `tsan` and `coverage`.
`docs` reports `skipped`, because its `if:` row names `github.ref == 'refs/heads/master'` together
with a push event and a repository owner, so a pull-request run never builds docs.

This lane pulled the four failing job logs with
`gh api repos/charlesarcher/speedgun-ng/actions/jobs/<id>/logs`. Job windows run from the API's own
`startedAt` to `completedAt`.

| Check | Verdict | Reading |
| --- | --- | --- |
| `lint` | green | 39 s, the format step clean over the two files that broke the last run |
| `prose-lint` | green | 54 s |
| `test` | broken | `99% tests passed, 1 tests failed out of 158`, 9 min 31 s |
| `test-rocky` | green | `100% tests passed, 0 tests failed out of 158`, 10 min 27 s |
| `sanitize` | broken | `99% tests passed, 1 tests failed out of 153`, 9 min 42 s |
| `tsan` | broken | `99% tests passed, 1 tests failed out of 154`, 9 min 53 s |
| `coverage` | broken | the suite step exited 8 after 17 min 15 s, and the gate printed no arithmetic |
| `dbc-gate` | green | 1 min 16 s, with the pair the last run found absent from its report |
| `shared-audit` | green | 2 min 20 s |
| `downstream-consumer` | green | 1 min 49 s |
| `consumer-release` | green | 3 min with the contract macros off |
| `static-analysis` | green | three shards green: library 10 min 53 s, examples 20 min 10 s, tests 25 min 49 s |
| `docs` | skipped | its `if:` row names a push to `master`, and the step's own reading stands in the docs-build section above |

### `test`, `sanitize` and `tsan`: one entry, the ceiling arm

Three checks fail on one entry, `harness_host_record_test`, and on one arm of it: the
standing-record arm that plants a ceiling on the descriptors its own child may open. The steps read
`ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"` in `test` and
`ctest --output-on-failure --no-tests=error -j "$(nproc)"` in `sanitize` and `tsan`, and each
exited 8.

```
99% tests passed, 1 tests failed out of 158
99% tests passed, 1 tests failed out of 153
99% tests passed, 1 tests failed out of 154
128/158 Test #141: harness_host_record_test .......................***Failed    0.10 sec
The following tests FAILED:
141 - harness_host_record_test (Failed)                 quick
```

The three summary lines above are `test`, `sanitize` and `tsan` in that order, by their totals. The
entry line quoted is the `test` job's, and the same entry reads `#136` in `sanitize` at 0.20 s and
`#137` in `tsan` at 0.18 s. Six checks fail, and the log prints the same six words in each of the
three jobs:

```
FAILED: a standing record no read reaches answers status 2 (FR-011, SC-004)
FAILED: the failed standing read prints one line (FR-011)
FAILED: that line names the record the read failed on (FR-011)
FAILED: that line speaks the read the ceiling refused (FR-011)
FAILED: the standing bytes stand kept behind the failed read (FR-011)
FAILED: the standing record no read reaches ended clean (FR-011, SC-004)
```

The arm prints what its plant answered. In `test` that reads `standing ceiling: status=0 lines=0`
beside the root `harness_host_record_roots/standing_ceiling_pid17918/GenuineIntel-6-C6-2.json`: the
ceiling left the read of the standing record nothing to refuse, so the run answered status 0 with
no line and all six checks went.

`test-rocky` runs the same commit and reports `100% tests passed, 0 tests failed out of 158`, with
`harness_host_record_test` passing in 0.07 s and `harness_host_record_check_test` in 0.04 s. The
plants that `52af6826` and `32dffcbb` set bind on the Rocky image, and the ceiling does not bind on
the Ubuntu runners. This section states the difference, and the change that answers it belongs to
its own commit.

### `coverage`: the suite step failed and the gate never spoke

The steps read `cmake --preset=ci-coverage`, then `cmake --build build/coverage -j "$(nproc)"`,
then `ctest -j 1 --output-on-failure --no-tests=error`. The build passed and the suite step exited
8 on the same entry, reading `99% tests passed, 1 tests failed out of 153` with
`136/153 Test #136: harness_host_record_test .......................***Failed    0.34 sec`, and
only `counters_pmu_test` and `counters_overhead` skipped, as they do wherever no PMU answers.

The job then uploaded its artifact and ended, so its log holds no `lines.......` line, no
`branches....` line and no `coverage-gate` line. No shortfall reading exists at this head, and the
29 dark rows of the run over `30a2dfb` stay the last measured set.

### What this run reads for FR-040 and SC-012

`git diff e3368112..origin/031-host-validation -- CMakeLists.txt` changes no line, so the `VERSION`
row and the two `SOVERSION` rows stand unmoved, and `kSchemaVersion` reads `1.10` in
`source/harness/json_report.cpp` at this head. The diff over `README.md` and `docs/pages` adds no
table row, so no option row and no `Name` row joined a table.

Four checks report `failure`, so T060 and T063 stay open beside this section. `consumer-release`
and `dbc-gate` pass here, which leaves the fourteen errors and the one drifted pair as readings of
the runs that reported them. `12d735ab Harness: Keep the bar count with contracts off` marks the
bar count `[[maybe_unused]]` in `source/views/histogram.cpp`, the count a contract-free build
leaves set and never reads, and this run reads that step as passing.
