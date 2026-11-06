# Quickstart and Validation: Lifecycle of the Programmable Limit Table

Feature: 050-limit-table-lifecycle. Spec: [spec.md](spec.md). Plan:
[plan.md](plan.md). Contract:
[contracts/limit-table-ownership.md](contracts/limit-table-ownership.md). This
page is a run guide. Figures come from commands run on the lane host
kennykiller, read at head `fba814d2`. The branch now carries the change, so a
base figure stands apart from a head figure, and each reading below names the
sha it measured. The page states no implementation code and no test body.
A figure that names no sha is a head figure. `plan.md`, Source (this
feature), names where the change lands: `source/counters/detail/core.hpp`,
`source/counters/system.cpp`, `include/speedgun-ng/counters_system.hpp`, and the
five cases in `test/source/counters_limit_lifecycle_test.cpp`.

## The rule this feature ships

The limit table maps an object's canonical path to the count of programmable
counters that object's event source published (`spec.md`, Key Entities). At the
base one function-local static holds it (base `858c55d9`,
`source/counters/system.cpp:187-191`), so every `System` in a process reads
that one table, an entry a first `System`
learned stays readable to a second, and nothing clears it (`spec.md`,
Preconditions). The change moves the table into `System::Impl`, gives every tree
node the address of the `System` that owns it, and reads the table and the two
walk reads through that owner (FR-001, FR-004, FR-005, FR-007), so a table ends
with its own `System` (`plan.md`, Logical view).

The sentence to repeat: one `System`, one limit table, and that table ends with
that `System`. The contract carries that form at
`contracts/limit-table-ownership.md:93`: "The table ends with its `System`." The
rule in full stands at `:25-27`, where the table is state of the `System`'s
private `System::Impl` and ends when that `System` ends.

## Prerequisites

The lane host is kennykiller, the acceptance host D-10 names. Two gate tools
need a pin, the rule SC-003 states. No gate, preset, or tool file of the
repository changes.

| Need | Check | This host |
| --- | --- | --- |
| Linux x86-64, the only supported platform | `uname -r`; `nproc`; `hostname` | `7.3.1-1-cachyos`, 20 cpus, `kennykiller` |
| Host cpu | `grep -m1 'model name' /proc/cpuinfo` | Intel(R) Core(TM) Ultra 7 265K |
| Toolchain | `c++ --version`; `cmake --version` | GCC 16.2.1; `cmake 4.4.5` |
| Build preset | `ci-rocky` (`CMakePresets.json:168`) | configured, writes `./build` (`CMakePresets.json:141-145`) |
| clang-format 18.1.8, the version CI installs by pip | `clang-format --version` at the pin | `18.1.8`; the host `/usr/bin` carries `23.1.4` (`.github/workflows/ci.yml:33-34`) |
| codespell, the spell tool CI installs | `codespell --version` at the pin | `2.4.3`; no `codespell` on this host's `PATH` |
| doxygen, for the dbc pair gate | `doxygen --version` | `1.18.0`, at `/usr/bin/doxygen` |
| An interpreter with PyYAML, for the same gate | `/usr/bin/python3 -c 'import yaml'` | PyYAML 6.0.3 on Python 3.14.9 |
| `Capture::Tiny` for the host `lcov` | `perl -e 'require Capture::Tiny'` | the module stands at `/usr/share/perl5/vendor_perl/Capture/Tiny.pm`; `lcov` reads `2.6-0` |

CI installs the two pins by pip (`.github/workflows/ci.yml:33-34`), and this
host carries both at `/tmp/050-tools-venv/bin`, outside the repository
(`/tmp/050-design-notes.md:121-124`). The host `/usr/bin/clang-format` is
23.1.4. SC-003 measures both faults: a run with that formatter flags 140 files
this lane never touches, and the spell gate stops at a speller that is absent.
The lane points `FORMAT_COMMAND` and `SPELL_COMMAND` at the pins (`plan.md`,
Gate commands). `cmake/dbc-gate.cmake:9-12` answers a fatal error when doxygen
stands off `PATH`, and the target stops at exit 2 without PyYAML
(`tools/dbc/dbc_gate_common.py:63-70`), which is why `plan.md`, Gate commands,
names `Python3_EXECUTABLE=/usr/bin/python3` for `build/dev`.

## Build and run the new test

```bash
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build -R counters_limit_lifecycle_test --output-on-failure
```

`ci-rocky` is the release developer-mode build, and its `binaryDir` is `build`
(`CMakePresets.json:141-145`, `:168-173`). Once per feature, build `ci-ubuntu`
(`CMakePresets.json:157`) the same way: it carries clang-tidy and cppcheck, and
SC-004 keeps the two `ctest` runs on the `ci-rocky` tree. The new entry stands
beside the limit entry, which shows the shape of one executable and one
`add_test` (`test/CMakeLists.txt:715-720`, `plan.md`, Technical Context).

A passing entry answers one console block and one summary:

```text
    Start 64: counters_limit_lifecycle_test
1/1 Test #64: counters_limit_lifecycle_test ....   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1
```

The index follows the position of the new `add_test` line in
`test/CMakeLists.txt`, and the base tree holds 181 entries (`plan.md`, Technical
Context). A pass keeps the program's own output in the log, because
`--output-on-failure` prints output for a failing entry. This is the log block a
green entry leaves, read here at the base:

```text
counters_objects_limit_test: ok
<end of output>
Test time =   0.00 sec
```

The new binary prints `counters_limit_lifecycle_test: ok` on its last line, the
convention of that file (`test/source/counters_objects_limit_test.cpp:124`), and
a failed check prints its own banner to stderr and exits 1 (`:28-39`). CTest
writes that block into `build/Testing/Temporary/LastTest.log` and closes the
entry with `Test Passed.`.

The base is not green, here or in CI. On this host 180 of 181 entries pass.
`harness_hunt_cli_test` fails on its standing check, and three entries report
Skipped: `counters_pmu_test`, `counters_overhead` and `harness_topdown_run_test`
(`/tmp/050-design-notes.md:111-116`, `plan.md`, the Risks row "The host suite
is not clean at the base"). In CI run 41946446748 three jobs are red:
`test-rocky` on three entries (`prose_gate_fixtures` at
`test/CMakeLists.txt:262`, `harness_hunt_cli_test` and
`harness_hunt_topdown_test`), `sanitize` on one entry, and `coverage`, where
`lcov` answers `(inconsistent)` for `source/harness/hunt_topdown.cpp:155`. The
`docs` job stands skipped, and eleven jobs pass. The obligation of the lane is a
comparison, not a repair. Every `ctest` entry that passes at the base must pass
at the head, on this host and in CI. No CI job that passes at the base may turn
red. The lane fixes none of these failures, because none of them reads the
counters code. SC-001 still asks the entry of this lane to read Passed in the
full run and under `taskset -c 0,1`, and `plan.md`, Gate commands, lists both
runs. The gate block below gives the job counts: `-j 20` on the full run, and
`-j 2` under `taskset -c 0,1`. The two base figures come from exactly those two
lines: 94.3 s and 367.4 s in that order. For the CI leg, the owner pushes the
branch and opens a pull request against `master`. That request is what starts
the run the lane must read, because `.github/workflows/ci.yml:7-14` triggers
`push` on `master` alone and `pull_request` against `master`. A push of a
feature branch alone starts no run, so the lane opens the request before it
reads any CI verdict. The head of that request is the single squashed commit of
D-10, the commit the merge takes. The run checks out the merge of that head
into `master`, a second commit with a sha of its own. That sha stands apart
from the sha `gh pr view --json headRefOid` answers, the sha SC-005 names.
D-11's rebase makes that difference harmless, because it keeps the tree of the
merge equal to the tree of the head. The lane reads that run and records its
verdict with its run number. The verdict stands on the pull request, because a
commit can carry no reading taken after it exists. The verdict and its run
number join the record SC-005 names, and the body of the pull request carries
both.

## The two-System read

The first three cases of SC-001 read this shape. One `System` scripts 4 for
`cpu`; a second declares the same path with one leaf and scripts nothing; the
program prints both answers. Every name below is a shipped name: `addObject` at
`include/speedgun-ng/counters_fake.hpp:135-137`, `addCounter` at `:215-221`,
`setProgrammableCounters` at `:169-170`, `registerProvider` at
`include/speedgun-ng/counters_system.hpp:302-303`, `object()` at `:315-316`, and
`programmableCounters()` at `:92-93`. The read of the handle takes the same
shape as the file beside the new one, a dereference of the expected
(`test/source/counters_objects_limit_test.cpp:74`).

```cpp
#include <cstddef>
#include <cstdio>
#include <memory>

#include "speedgun-ng/counters.hpp"

using sg::counters::FakeProvider;
using sg::counters::System;

// One provider over one object: `cpu` with one leaf counter, plus the
// limit the caller names. A count of zero scripts nothing, the fact that
// US-1 scenario 3 of spec.md states.
auto script(System& system, std::size_t limit) -> void
{
  auto provider = std::make_unique<FakeProvider>();
  provider->addObject("cpu", "cpu", "one scripted cluster");
  provider->addCounter("cpu", "cycles", "ops", "core cycles elapsed");
  if (limit > 0) {
    provider->setProgrammableCounters("cpu", limit);
  }
  if (!system.registerProvider(std::move(provider)).has_value()) {
    std::printf("register failed\n");
  }
}

auto print(const char* tag, System& system) -> void
{
  const auto cpu = *system.object("cpu");
  const auto limit = cpu.programmableCounters();
  if (limit.has_value()) {
    std::printf("%s: %zu\n", tag, *limit);
  } else {
    std::printf("%s: no value\n", tag);
  }
}

auto main() -> int
{
  System first;
  script(first, 4);
  System second;
  script(second, 0);
  print("first cpu", first);
  print("second cpu", second);
  return 0;
}
```

Compile it the way both probe rounds compiled this shape: the project flags
`-std=c++23 -Iinclude -isystem build/export -DSPEEDGUN_NG_STATIC_DEFINE`, the
archive `build/libspeedgun-ng.a`, and the vendor archives
`build/test/CMakeFiles/counters_objects_limit_test.dir/link.txt` names
(`/tmp/050-clarify-round1.md:6-10`). The file
`/tmp/sg050r2/probe_unscripted.cpp` holds that shape at the base, and it adds
the one macro line the next section names.

After the change the program answers:

```text
first cpu: 4
second cpu: no value
```

The first `System` answers its own scripted 4, the answer FR-002 keeps. The
second answers no value, because its own table holds no record for `cpu`
(FR-006). At the base the same shape answers:

```text
first cpu: 4
second cpu: 4
```

The second read reaches the one process-wide table and finds the 4 the first
`System` wrote, at line 304 of the base `source/counters/system.cpp`
(`858c55d9`). Round 2 measured that row and the rows beside it
(`/tmp/050-clarify-round2.md:96-100`), and the probe
binary re-ran here on 2026-11-05 with the same two answers. The plan carries
those figures as the base readings of the cases (`plan.md`, the FR coverage
map). Two further base figures carry the other cases: a second `System` that
scripts 2 for the same path leaves both reading 2, so the story's "the answer
stays 4" fails (`/tmp/050-clarify-round1.md:60-62`), and a fresh `System` over a
path whose `System` that scripted 8 has ended reads 8
(`/tmp/050-clarify-round1.md:63-65`).

## The defect at the base

At the base the listing stops at the compiler: `'sg::counters::System::System()'
is private within this context`, with `note: declared private here` at
`include/speedgun-ng/counters_system.hpp:293` at the base `858c55d9`, and the
same pair for the destructor at its line 294
(`/tmp/050-clarify-round1.md:21-24`). Round 1 tried seven
routes to a second `System` while a public header stands unchanged, and every
one fails: deriving reaches nothing, a hand-built handle reaches nothing, the
private header names `System::Impl` inside a private section, and no exported
factory produces a `System` (`/tmp/050-clarify-round1.md:14-53`). One mechanical
route works: `#define private public` before the include, `#undef private`
after, and both probe rounds ran it to measure the base figures above
(`/tmp/050-clarify-round1.md:55-58`, `/tmp/050-clarify-round2.md:11-16`). It
redefines a C++ keyword, the behaviour stands undefined, and Principle X rejects
it as a test device, so no test of this lane carries it (D-5). The five cases
build their `System` objects through the constructor FR-008 makes public, and
the red of that case is the compiler line above (`plan.md`, Modules and
dependency order).

## The gates

Run them in this order, the order `plan.md`, Gate commands, writes:

```bash
cmake --preset=ci-rocky && cmake --build build
ctest --test-dir build -j 20 --output-on-failure
taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure
cmake -D FORMAT_COMMAND=<pinned-clang-format-18.1.8> -P cmake/lint.cmake
cmake -D SPELL_COMMAND=<pinned-codespell> -P cmake/spell.cmake
python3 tools/docs/example_drift.py .
python3 tools/prose/prose_gate.py --check all --mode range --base origin/master
cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build/dev
cmake --build build/dev --target dbc-gate
cmake --preset=ci-sanitize && cmake --build build/sanitize -j "$(nproc)"
ctest --test-dir build/sanitize -j 1 --output-on-failure --no-tests=error
cmake --preset=ci-tsan && cmake --build build/tsan -j "$(nproc)"
ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error
cmake --preset=ci-coverage
cmake --build build/coverage -j "$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage --target coverage
bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
```

`ci-sanitize` answers the address and undefined-behavior sanitizers and
`ci-tsan` the thread sanitizer, and the two presets stand apart because the
compilers reject the pairing (`CMakePresets.json:120-140`). The coverage target
runs the gate script as its summary command (`cmake/coverage.cmake:113-121`,
`:142`), so the last line re-reads that trace. That command pins `LCOV` to the
`lcov` the configure step found (`cmake/coverage.cmake:115`), so a wrapper
outside the repository reaches no step of the target. The trace is the host
trace `build/coverage/coverage.info`, because the CI `coverage` job writes no
trace at the base. The gate wants a line rate of `100.0%`
(`tools/dbc/coverage_gate.sh:45-48`) and a branch shortfall of 21 rows or fewer
(`:102`, `:114-118`). The 21 rows are the tree-wide allowance of the D-14
ledger, and SC-002 keeps them. SC-002 also bars any new `--ignore-errors` flag,
and it bars the flag from every command that stands in the tree: a CMake file, a
script, a preset, a gate. Reading the broken base trace on this host needs
`--ignore-errors inconsistent`, so the flag rides only a host wrapper outside
the repository, `/tmp/lcov-soft`. Both commands ran here on 2026-11-05 and
answer differently. The bare run exits 3, and its message names lcov's own exit
code of 1. The target reaches the gate through that pin, so it answers the same
exit 3 on this trace. With `LCOV=/tmp/lcov-soft` the read finishes, the gate
prints `lines.......: 97.5% (17350 of 17786 lines)`, and it exits 1 on the line
rate. The line check at `:45-48` stands ahead of the branch section at
`:102-118`, so this host reads no branch figure from the gate. The wrapped read
names three inconsistent sites and counts them: `source/harness/runner.cpp:707`,
`source/harness/runner.cpp:820`, `source/harness/hunt_topdown.cpp:155`, and it
prints `inconsistent: 3`. In CI the refusal names
`source/harness/hunt_topdown.cpp:155`, and this lane touches no line of that
file. `plan.md`, Gate commands, exports `PERL5LIB` for `Capture::Tiny`, and
the block above leaves that line out. `Capture::Tiny` stands in
`/usr/share/perl5/vendor_perl`, which perl reads already, so the export answers
nothing here.

The CI column names the job of `.github/workflows/ci.yml`, and the line ranges
stand in that file.

| Gate | Exit the lane expects | The CI job that runs it |
| --- | --- | --- |
| `ctest`, full, and under `taskset -c 0,1` | 0 for every entry of this lane | `test` (`:561-563`) and `test-rocky` (`:637-649`); the pinned run is lane-only |
| lint, through the pinned formatter | 0 | `lint` (`:36-37`) |
| spell | 0 | `lint` (`:44-46`) |
| example drift | 0 | `lint` (`:30-31`) |
| prose, over the range and over each commit message | 0 | `prose-lint` (`:1377-1382`), which runs the prose gate through `cmake/prose-lint.cmake:21-35` |
| the checked build, clang-tidy and cppcheck | 0, with no new finding | `static-analysis`, which configures `ci-ubuntu` (`:373-374`) and fails on a gate check (`:429-440`) |
| the dbc pair gate, through its target | 0 | `dbc-gate` (`:1283-1290`) |
| the two sanitizer suites | no new failing entry: `sanitize` fails one `ctest` entry at the base and `tsan` passes | `sanitize` (`:210-221`) and `tsan` (`:296-300`) |
| coverage, through the target and the gate | exit 3 from the target, exit 1 from the wrapped read; the bar is 100 percent of the lines of each changed source file, read from the trace rows, and no new missing branch row | `coverage` (`:104-121`) |

Three of those jobs stand red at the base, as measured above: `test-rocky`,
`sanitize` and `coverage`. For those three the lane reads its own entries as
passing and names no new failure, because the base faults sit outside the
counters code. The coverage bar is the narrower one, because the gate reads one
whole trace summary, and its exit says nothing about any one file. That rule
counts the lines of the summary, and this host trace reads 97.5 percent there,
so the gate exits 1. The trace holds the figures file by file, and the lane
reads its three changed sources there. `source/counters/system.cpp` reads 282 of
282 lines with 3 missing branch rows, and `source/counters/detail/core.hpp`
reads 79 of 79 with 3 more. The public header
`include/speedgun-ng/counters_system.hpp` carries its own entry: 20 of 20 lines
and 6 of 6 branch rows, and no dark row. The bar is 100 percent of the lines of
each changed source file, read from those rows. No edit may add a missing branch
row beyond the 6 the two files with dark rows already hold.

Three readings stand read at head `b19e257f` on this host: the prose gate over
the range exits 0 with `0 findings` over the four sources of that range, the
drift gate exits 0, and `cmake -P cmake/spell.cmake` exits 1 for want of a host
`codespell` (`/tmp/050-clarify-round2.md:145-149`, `:201-203`). The bare
`python3 tools/dbc/dbc_pair_gate.py` exits 2 for want of its arguments, and the
target exits 0 with `doc-gate: 347 interfaces, 0 gaps` and `pair-gate: 347
interfaces, 0 gaps` (SC-003, `/tmp/050-dbc-notes.md:143-150`). The newly public
pair joins that matrix with one row, `System::~System`: the doc gate exempts a
member whose name matches a friend name of the project, and `Object` names
`System` (FR-010, `tools/dbc/dbc_doc_gate.py:356-362`, `:424-431`). The house
form `\pre none` and `\post none` adds no gap on either row (FR-010,
`/tmp/050-dbc-notes.md:55-69`).

The gate over this page alone carries the same verdict, read here on
2026-11-05:

```bash
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/050-limit-table-lifecycle/quickstart.md
```

`prose-lint: 0 findings, 0 skipped`, exit 0. The prose gate prints a source
count and a unit count beside that verdict. Both move with the branch, so this
page keeps the verdict and drops both counts.

Before each push the lane rebases on `master`, reads both sides of every
conflict, and runs every command above again on the rebased head (`plan.md`,
Gate commands, and D-11).

## Troubleshooting

| Text you see | What it means |
| --- | --- |
| `'sg::counters::System::System()' is private within this context` | The listing ran at the base, where the pair stands private (`858c55d9`: `include/speedgun-ng/counters_system.hpp:293-294`). FR-008 moves it. |
| `no object at path 'cpu'` | The reading `System` holds no node there, so the limit read never runs (`source/counters/system.cpp:305-313`). Declare the path there with one leaf, as the listing does (SC-001, Clarification 1). |
| `Run again with FIX=YES` | The host clang-format 23.1.4 answered. Point `FORMAT_COMMAND` at the 18.1.8 pin (`plan.md`, Gate commands). |
| `Spell checker returned with no such file or directory` | No `codespell` on this `PATH`. Point `SPELL_COMMAND` at the pin (`/tmp/050-clarify-round2.md:201-203`). |
| `dbc-gate requires doxygen on PATH`, or `dbc gates require PyYAML (install python3-yaml)` | The target needs both (`cmake/dbc-gate.cmake:9-12`, `tools/dbc/dbc_gate_common.py:63-70`). Reconfigure as `plan.md`, Gate commands, does. |
| `documented-not-enforced` | A documented `\pre` or `\post` with no matching macro in that body. The house form is `none` where no fact stands checkable at that boundary (`/tmp/050-dbc-notes.md:55-69`). |
| `coverage-gate: lcov failed to read build/coverage/coverage.info (exit 1)` | lcov exits 1, and the gate prints that code in its message before it exits 3. The tool failed and the suite went unjudged (`tools/dbc/coverage_gate.sh:40-43`). |

## What to read next

- [spec.md](spec.md): the requirements, the D-n decisions, and the five cases.
  SC-001 states them in one paragraph.
- [contracts/limit-table-ownership.md](contracts/limit-table-ownership.md): the
  shipped rule (`contracts/limit-table-ownership.md:25-27`) and the case that
  carries each fact (`:165-173`).
- [docs/pages/harness.md](../../docs/pages/harness.md): the counters passage of
  the multi-pass section names `Object::programmableCounters` at
  `docs/pages/harness.md:1617-1620`, and D-7 puts the ownership words there, as
  FR-011 states. FR-012 moves nine version lines across five files, and the
  captured line at `docs/pages/harness.md:2586` stands among them. The history
  line `CMakeLists.txt:61` stays where it stands, because it records which
  feature took 0.31.0.

## Deviations

No decision of the spec or the plan moves. One host reading of the plan's gate
block does not hold here. `plan.md`, Gate commands, exports
`PERL5LIB="$HOME/perl5/lib/perl5"` for `Capture::Tiny`, and that directory
stands absent on kennykiller while the module stands at
`/usr/share/perl5/vendor_perl/Capture/Tiny.pm`. Read here on 2026-11-05, the
command `perl -M Capture::Tiny -e 'print $INC{"Capture/Tiny.pm"}'` answers
that vendor path, and the default `@INC` lists that directory. Run with that
export set, the same command answers the same path. The export changes nothing
on this host, so the page leaves that line out of its gate block.

The page cites `spec.md` and `plan.md` by requirement, decision, clarification,
or section, and never by line, because the lane edits both while it plans.
`data-model.md` and `contracts/limit-table-ownership.md` carry the same rule.
Every other `path:line` cite here names code, a test, a CI file, a tool, or a
note under `/tmp/`.
