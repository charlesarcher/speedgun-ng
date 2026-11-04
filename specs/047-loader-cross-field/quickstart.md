# Quickstart and Validation: Loader Cross-Field Checks for the Pass Fields

Feature: 047-loader-cross-field. Spec: [spec.md](spec.md). Plan: [plan.md](plan.md).
Contract: [contracts/loader-pass-pairs.md](contracts/loader-pass-pairs.md).
This page is a run guide. Figures come from commands run on the lane host chilly
on 2026-11-03 at `e63f7a87`, the base `b1dd7670` plus two spec commits. The page
states no implementation code and no test body: the change lands in
`source/harness/json_load.cpp`, and the tests land in the two files the plan
names.

## Prerequisites

The lane host is chilly (AMD Zen 4, Ryzen 9 7950X3D), the host the spec names
(Assumptions). Two host tools need a pin, and D-20 records both. No gate,
preset, or tool file of the repository changes for either.

| Need | Check | This host |
| --- | --- | --- |
| Linux x86-64, the only supported platform | `uname -r` | `7.3.1-1-cachyos`, 32 cpus |
| The lane host | `grep -m1 'model name' /proc/cpuinfo` | `AMD Ryzen 9 7950X3D`, family 25, model 97 |
| Toolchain | `c++ --version`; `cmake --version` | GCC 16.2.1; `cmake 4.4.4` |
| Build preset | `ci-rocky` (`CMakePresets.json:168`) | configured, writes `./build` |
| clang-format 18.1.8, the version CI installs by pip | `~/.local/bin/clang-format --version` | `18.1.8`, the pin of `.github/workflows/ci.yml:33-34` |
| `Capture::Tiny` for the host `lcov` | `PERL5LIB=$HOME/perl5/lib/perl5` | the module stands at that path |
| doxygen, for the dbc pair gate | `~/.local/bin/doxygen --version` | `1.18.0` |

`uv tool install clang-format==18.1.8` lands the pinned formatter, and
`~/.local/bin` precedes `/usr/bin`. The host `/usr/bin/clang-format` is 23.1.4:
it names a long list of untouched files as badly formatted and answers exit 1
(`build/logs/notes/lint.log`), so the lane runs the gate through `FORMAT_COMMAND`.
Without `PERL5LIB` the host `lcov` dies at `lcovutil.pm line 16`, and the
capture, the extract and the gate exit 2, 2 and 3. `cmake/dbc-gate.cmake:11`
answers a fatal error when doxygen stands off `PATH`.

## Build and test

```bash
cmake --preset=ci-rocky && cmake --build build
ctest --test-dir build --output-on-failure
taskset -c 0,1 ctest --test-dir build --output-on-failure
```

Both runs answer exit 0 at acceptance on chilly (FR-019, SC-003). Neither ran at
research time (`build/logs/notes/clarify-gates.md`, Q5), so the two counts stand as
the acceptance run's own figures. The pinned run is the record for the
load-sensitive FWQ rows of `harness_jitter_window_test`. A new case is one
function plus one call in its file's `main`: the targets and the CTest entries of
`harness_json_load_test` and `harness_compare_cli_test` already stand at
`test/CMakeLists.txt:775-783` and `:795-801`. Once per feature, build `ci-ubuntu`
(`CMakePresets.json:157`) the same way: it carries clang-tidy and cppcheck.

## The five gates

```bash
cmake -D FORMAT_COMMAND="$HOME/.local/bin/clang-format" -P cmake/lint.cmake
cmake -P cmake/spell.cmake
python3 tools/docs/example_drift.py .
# the range gate, a local check over the lane's own commit bodies
python3 tools/prose/prose_gate.py --check all --mode range --base origin/master
# the pair gate, one gate over two lines: configure once, then the target
cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build/dev --target dbc-gate
```

Each command is the gate's own command line (FR-019), and each answers exit 0 on
the host of the table above. The last column carries the exit read at
`5f910845`.

| # | Gate | Expected exit | Read at the head |
| --- | --- | --- | --- |
| 1 | lint, through the pinned formatter | 0 | 0; the host 23.1.4 answers 1 |
| 2 | spell | 0 | 0, silent |
| 3 | example drift | 0 | 0, silent |
| 4 | prose, over the range | 0 | 0, `14 sources, 2022 units, 0 findings` |
| 5 | the dbc pair gate, through its target | 0 | 0, `doc-gate: 345 interfaces, 0 gaps` and `pair-gate: 345 interfaces, 0 gaps` |

The prose gate answers 0 clean, 1 findings and 2 usage. The range carried one
`CM.BODY-WRAP` finding at research time and one `XI2.CONTRASTIVE` finding at the
analyze round, and the two commit bodies closed them. The gate over this feature directory alone is the last command of the
plan's list:

```bash
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/047-loader-cross-field
```

The pair gate runs through its CMake target: the bare `tools/dbc/dbc_pair_gate.py`
needs four arguments. The target runs the gate through the interpreter CMake
resolves, so the configure pins one that carries PyYAML, the package CI installs
(`.github/workflows/ci.yml:88`).

## Sanitizers and the checked build

```bash
cmake --preset=ci-sanitize && cmake --build build/sanitize -j "$(nproc)"
ctest --test-dir build/sanitize -j 1 --output-on-failure --no-tests=error
cmake --preset=ci-tsan && cmake --build build/tsan -j "$(nproc)"
ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error
cmake --build build/dev
```

`ci-sanitize` answers ASan and USan and `ci-tsan` the thread sanitizer, and the
two presets stand apart because the compilers reject the pairing (Principle
VIII). `cmake --build build/dev` carries the clang-tidy and cppcheck reports.
The Clang compile, the sanitizer jobs and the coverage union stand in CI
(`.github/workflows/ci.yml:313`, `:147`, `:230`), and the lane reads their verdicts into
the acceptance record.

## Coverage

```bash
export PERL5LIB="$HOME/perl5/lib/perl5"
cmake --preset=ci-coverage
cmake --build build/coverage -j "$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage --target coverage
bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
```

The measurement point is the coverage job's union of a full suite, the reading
the gate takes (`tools/dbc/coverage_gate.sh:86-88`). The union is the gate: the
branch shortfall it computes must stand at 21 rows or fewer (`:102`, `:111-118`),
and the line rate must read `100.0%` (`:45-48`). The union reads 0 rows short
today (`specs/043-fwq-ftq/plan.md:122`), so all 21 rows stand free.

A developer host reads higher by design, because an entry there can stop early
and leave the arms past its stop dark (`:90-101`). The host reading is reported,
and it judges nothing. The host reading at the base is 99.5 percent lines (17484
of 17565), 97.3 percent functions and 98.8 percent branches (8593 of 8701),
because two coverage entries fail on this host today:
`harness_jitter_window_test` and `harness_topdown_run_test`.

The change adds 26 branch records, plus 4 for the two guards of D-14, and every
arm has a test, so it adds no dark arm (D-19). No exclusion construct joins the
tree: no `LCOV_EXCL`, no `--omit-lines`, no `--ignore-errors`, no `--remove`
(D-12, FR-018).

## Acceptance

SC-001 to SC-005 are the success criteria. Local acceptance (G-20) runs on
chilly, and the pull request body names the host, the kernel, the GCC version,
the head sha, the two `ctest` counts and the coverage figures (Constraints). A CI
run on a GitHub runner answers no acceptance. Every figure below is recorded in
the pull request body: the case names, the two counts, the five verdicts, and the
coverage figures beside the G-20 numbers.

| Criterion | How it is read | Where the number is recorded |
| --- | --- | --- |
| SC-001, one test per refusal and per accepted form | `grep -c` over the `main` call list of the load test (`:2207-2234`): 27 today plus nine new, one function and one call each | the body, as the count and the `ok` line (FR-016) |
| SC-002, lines at 100 percent, a shortfall of 21 rows or fewer | the gate line over the union, plus the host reading | the body, beside the G-20 figures (FR-018) |
| SC-003, both `ctest` runs pass | the two counts, full and under `taskset -c 0,1` | the body, as the two counts (FR-019) |
| SC-004, the five gates pass | the five exits of the gate table | the body, as the five verdicts (FR-019) |
| SC-005, every written document loads after the change | the round-trip cases of FR-010 to FR-012, named in the source and green in the run | the body, as the case count and the `ok` line (FR-010 to FR-012) |

The reading of `source/harness/json_load.cpp` at the head joins the SC-002
figures (FR-018). The version move carries no figure of its own: `0.30.1` stands
at `CMakeLists.txt:59`, and the two pinned sites at `views_test.cpp:279` and
`harness_cli_table_test.cpp:1453` move with it (FR-015). The version commit
rebuilds the harness library whole and relinks every harness test, because
`SG_PROJECT_VERSION` is a `PRIVATE` define of that target
(`CMakeLists.txt:323-327`).

## The specs/046 rebase

Both lanes touch `source/harness/json_load.cpp` and
`test/source/harness_json_load_test.cpp`, in different readers of that file
(D-13). The second lane to merge rebases. The steps, in order:

1. `git fetch origin`, then read both sides of the two files before the rebase.
2. `git rebase origin/master`, and keep both sides: the specs/046 hunt reader and
   this lane's three checks.
3. Re-anchor every `path:line` this page and the contract cite, because the
   rebase moves lines in the file both lanes touch.
4. `cmake --build build`, then run every gate, both `ctest` runs and coverage.
5. `git push --force-with-lease` against the last pushed sha. `git ls-remote
   origin 047-loader-cross-field` then equals `git rev-parse HEAD`. A failed
   lease stops the lane and reports.

No file of specs/045 stands in this lane's scope, and the lane never merges
(Constraints, Scope).

## Troubleshooting

| Text you see | What it means |
| --- | --- |
| `Can't locate Capture/Tiny.pm in @INC` | `PERL5LIB` stands unset. Export it, then rerun. |
| `coverage-gate: lcov failed to read ... (exit 3)` | The same cause: exit 3 names a tool failure (`coverage_gate.sh:40-43`). |
| `Run again with FIX=YES` from the lint gate | The host clang-format 23.1.4 answered. Use the pinned 18.1.8. |
| `CM.BODY-WRAP` | A commit body wraps past 72 columns, the limit of `prose_rules.yaml`. |
| `dbc-gate requires doxygen on PATH` | `cmake/dbc-gate.cmake:11`. Put doxygen on `PATH`. |
| `carries schema version '2.0', which this build cannot read` | The standing version gate, ahead of the entry read (`json_load.cpp:3129-3135`). |
| `XI2.CONTRASTIVE family=XI.2` | The prose gate names a contrastive phrase. State what the thing is. |
