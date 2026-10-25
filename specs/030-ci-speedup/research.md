# Research: CI Speed-Up

**Date**: 2026-10-25 · **Spec**: [spec.md](./spec.md)

The spec numbers its decisions D-1 to D-13. This file numbers the plan's
decisions P-01 to P-13, so a bare `D-n` in any artifact of this feature
names the spec's decision and `P-nn` names this file's entry. The spec
left the mechanism to the plan in seven places: the lane switch for the
matrix (D-5), the launcher placement (D-9), the cell argument (D-4), the
tier list shape (D-6), the budget loop (D-7), the shard lists (D-1), and
the docs landing (FR-050). Each is settled below with its rejected
alternatives. Every anchor here was re-read at `f507b8f` on 2026-10-25.

## P-01: The matrix lanes are chosen by a cache variable

**Decision:** the configure presets `ci-ubuntu-test`, `ci-rocky` and
`ci-tsan` set the cache variable `SPEEDGUN_DBC_MATRIX` to `ON`, and
`ci-coverage` and `ci-sanitize` leave it unset.
`test/CMakeLists.txt` registers the five matrix entries inside its
existing Linux block (the block opens at `:131`, the single entry sits at
`:139`) guarded by that variable.

**Rationale:** the five lanes that run the suite already select distinct
presets, so the truth about which lane runs the matrix lives in one place
beside the flags that define the lane. FR-023 asks for a CMake value that
decides registration and records the compiler per registering lane, and a
preset cache variable is that value with no new plumbing.

**Alternatives:** a `-D` flag on each configure command in `ci.yml`
splits the truth between the workflow and the registration file, so a
preset edited alone can silently change which lanes run the matrix;
rejected. A detector on `CMAKE_CXX_COMPILER_ID` cannot express D-5,
because `coverage` runs the same GCC 15.2.0 as `test` and would register
the matrix a second time in the same family; rejected. An environment
variable read at configure time would make the configure step depend on
the job's `env:` block, which a local developer does not reproduce;
rejected.

## P-02: One abstract `ci-cache` preset carries the launcher

**Decision:** a new configure preset `ci-cache` holds only
`CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER` set to
`ccache`. `ci-ubuntu`, `ci-rocky`, `ci-ubuntu-test`, `ci-sanitize`,
`ci-tsan`, `ci-coverage` and `prepush` inherit it, each keeping its own
inheritance list.

**Rationale:** the file already carries abstract presets that hold flags
and no `binaryDir` (`dev-mode`, `ci-linux`, `clang-tidy`, `cppcheck`),
which is the shape `ci-cache` follows. One site names the launcher, so a
lane cannot drift into an uncached build, which is FR-006's whole duty.
`static-analysis` keeps its bite on a warm cache, because CMake runs both
analyzers before the compile of every unit even when the compiler is a
cache hit (FR-017).

**Alternatives:** the launchers inside `ci-build` reach `ci-ubuntu` and
`ci-rocky` but miss `ci-sanitize`, `ci-tsan` and `ci-coverage`, which do
not inherit `ci-build`; rejected. Seven repeated `cacheVariables` blocks
invite a lane to drift; rejected. A tool like
`hendrikmuhs/ccache-action` adds a third-party action and sets
`CCACHE_SLOPPINESS` and a basedir of its own, which FR-011 forbids;
rejected.

## P-03: The matrix script runs one cell per invocation

**Decision:** `tools/dbc/semantics_matrix.sh` keeps its one-argument
form and gains a second, optional argument naming the cell:
`semantics_matrix.sh <source-dir> [cell]`. With a cell name it configures
that one cell through the existing `run_cell` function (`:154`), skips
the other cells, and prints the independence table for that cell alone.
Without the argument it runs all five, so a developer's local call keeps
its behavior.

**Rationale:** FR-019 wants one `ctest` entry per cell, and the entry
must name the cell that broke. Keeping `run_cell` intact preserves every
check of the cell (FR-022), and the optional argument leaves the
developer-facing entry point unchanged.

**Alternatives:** five scripts duplicate the configure and probe logic
about 200 lines deep; rejected. One script that registers CTest tests at
run time cannot work, because registration happens at configure time;
rejected.

## P-04: Entry names, `JOBS` and `PROCESSORS`

**Decision:** the five entries are `dbc_matrix_off_o0`,
`dbc_matrix_off_o2`, `dbc_matrix_on_o0`, `dbc_matrix_on_o2` and
`dbc_matrix_observe_off_o0`, each passing its script cell name
(`NDEBUG=off_-O0` and the rest, `:323` to `:326` and `:330`) so the log
carries both spellings. A new cache variable `SPEEDGUN_TEST_JOBS`
defaults to `2`; each entry passes it to the script in place of the
`nproc` read at `:60`, and each entry sets `PROCESSORS` to the same
value.

**Rationale:** the script builds with `-j "$JOBS"` (`:228`), so a bounded
`JOBS` is what keeps five concurrent entries from oversubscribing a
4-vCPU runner. `PROCESSORS` equal to `JOBS` is what FR-003 asks for, and
two is the value the runner can hold while four entries run side by side.
The script already isolates its build tree through `mktemp -d` (`:76`),
so one entry cannot collide with another.

**Alternatives:** `JOBS` from `nproc` divided by the entry count at run
time needs the entry count inside the script, which the split removes;
rejected. `PROCESSORS 5` on each entry serializes the matrix to one
runner-wide slot and costs the saving the split exists for; rejected.

## P-05: One slow list, and the rest are quick

**Decision:** `test/CMakeLists.txt` holds one list named
`SPEEDGUN_SLOW_TESTS`. Each row is one entry name with a comment naming
its reason, and today's rows are the five matrix entries and
`snippet_compile`. A block at the end of the file reads the directory
`TESTS` property and applies `LABELS slow` to a listed entry and
`LABELS quick` to every other entry. `example/CMakeLists.txt` holds no
list of its own, because its two entries fall under the same trailing
mechanism once the property read runs in that directory too, so each file
carries its own block.

**Rationale:** FR-025 wants exactly one label per entry from one list,
with a reason beside each `slow` row, and D-6 wants the list to be the
single source. A trailing block cannot miss an entry that a later
`add_test` adds, which is how a hand-labeled file would drift.

**Alternatives:** `LABELS quick` at 133 `add_test` sites is 133 chances
to miss one; rejected. A `quick` list with `slow` as the fallback inverts
FR-025's default; rejected. CTest fixture dependencies cannot express a
tier; rejected.

## P-06: The pre-push presets build outside the CI tree

**Decision:** the configure preset `prepush` inherits `ci-build`,
`dev-mode` and `ci-cache`, and overrides `binaryDir` to
`${sourceDir}/build/prepush`. The test presets `prepush-quick` and
`prepush-full` configure against `prepush`; `prepush-quick` filters on
the label `quick` and `prepush-full` carries no filter. Both set
`noTestsAction: error`, matching `ci-tsan` and `ci-sanitize` (`:1` of
their blocks). The name `dev` stays unused.

**Rationale:** a lane host that runs the pre-push check beside a
`ci-rocky` build must not write into `build`, and `CMakeUserPresets.json`
is git-ignored at `.gitignore:13` and carries a `dev` preset on lane
hosts, where a duplicate name is a CMake error (D-6). FR-028 asks that
the pre-push preset carry the launcher, so it inherits `ci-cache`.

**Alternatives:** reusing `ci-rocky` as the local preset would put the
dev-mode flags of `ci-rocky` against a Rocky container's toolchain path;
rejected. A machine-local `CMakeUserPresets.json` preset cannot be the
thing `tools/ci/prepush.sh` calls, because a fresh clone has no such file;
rejected.

## P-07: The budget loop reads the `TESTS` property

**Decision:** the trailing block of `test/CMakeLists.txt` and
`example/CMakeLists.txt`, after every `add_test`, reads the directory
`TESTS` property and, for each entry whose `TIMEOUT` property is unset,
sets `TIMEOUT 300`. The eight 120 s values (`test/CMakeLists.txt:580`,
`:742`, `:1392`, `:1408`, `:1425`, `:1440`, `:1456`, `:1473`) and the
300 s `example_run` value (`:1487`) stay untouched. The exceptions block
sits beside the loop as a comment list naming each entry, its reason and
the measured per-lane number read from a CI run.

**Rationale:** FR-031 names both files and the `TESTS` property, and
D-7 requires that no lower value move. Reading the property after the
registrations means an entry added tomorrow arrives with a budget
instead of the 1500 s CTest default.

**Alternatives:** a `TIMEOUT` line at each of 133 registrations misses
every future entry; rejected. A `--timeout` flag on the `ctest` command
is refused by FR-034, and it would also lift the eight 120 s budgets.

## P-08: `static-analysis` shards by build scope

**Decision:** a job `static-analysis` with a three-value matrix named
`library`, `tests`, `examples`. All three configure `ci-ubuntu`. The
`library` shard builds `speedgun-ng_speedgun-ng` and
`speedgun-ng_harness`, the `tests` shard runs `cmake --build build/test`,
and the `examples` shard runs `cmake --build
build/example`. Each shard prints the count of translation units it
compiled. The job carries `needs: [lint]` beside the ten jobs that
already do, and each shard uploads its sorted `file:line:check` finding
list as an artifact.

**Rationale:** the three scopes are the three target directories the
project already owns, so the union of their units is the whole tree by
construction, and no hand-kept list of about 95 target names exists.
Three shards answer FR-018's two-or-three range with the widest split,
and the unit count beside FR-049's finding-list equality is the guard
that no unit was dropped to meet 15 min (D-1).

**Alternatives:** `run-clang-tidy` over `compile_commands.json` is refused
by D-13, because that list also names the vendored units and its cppcheck
configuration differs from the preset launcher's. A result cache is
refused by hard rule 5 (D-1). Two shards would put the 90 test units with
the library and leave the largest scope unsplitted; rejected.

## P-09: The README block lands in `Quick start`

**Decision:** a plain fenced `sh` block inside `## Quick start` names the
two tiers and `tools/ci/prepush.sh` in 25 lines or fewer, with one
sentence each side. `PAGE_LINE_CAP` in `tools/docs/example_drift.py:192`
moves from 835 to the measured page length, with the file's own comment
form recording the move, as 023, 024, 025 and 026 each did.

**Rationale:** `Quick start` is where the page tells a reader to
configure and build, and the page holds 834 lines against a cap of 835,
so the cap must move with the block. The page already holds one fenced
block with no `sg-example` marker, so the gate does not require a marker.
No H3 title is added, so the 27-entry `SECTIONS` audit of `How you do it`
keeps its input.

**Alternatives:** a new marked region in `example/quickstart.sh` would
put pre-push commands into a script that a consumer copies, and it would
drag `docs/pages` with it; rejected. A new H2 section would compete with
`Quick start` for the same reader; rejected. Trimming other lines to free
the room the block needs touches prose this feature does not own;
rejected.

## P-10: The cache key and the directory

**Decision:** each building job sets `CCACHE_DIR` to
`${{ github.workspace }}/.ccache`, restores `ccache-*` through
`actions/cache`, and stops at a pull request without saving (FR-008). The
key joins, in D-9's order: the prefix `ccache`, the job name, the preset
name, a hash of the compiler's `--version` output,
`hashFiles('CMakePresets.json', 'external/**')`, and `github.run_id`. The
restore keys are that string with the run id dropped. `CCACHE_BASEDIR`
and `CCACHE_SLOPPINESS` stay unset, and `CCACHE_MAXSIZE` is set per lane
with the sizes summed in the pull request against the 10 GB budget.

**Rationale:** the run id in the key means a key is written once, so
`actions/cache` never fails on an existing entry, and dropping it in the
restore keys still finds the newest prior save. `CCACHE_DIR` inside the
workspace covers the rocky container, whose home differs. The key names
the compiler and the preset, so a runner image bump is a miss rather than
a wrong object.

**Alternatives:** a key on the commit sha restores nothing across the
commits of one pull request, which is the common case; rejected.
`/usr/local/etc/ccache.conf` for the size and basedir settings hides the
configuration from the workflow and from a local reader; rejected.

## P-11: The coverage lane starts serialized

**Decision:** the coverage job's `ctest` line (`ci.yml:71`) goes from
`-j 2` to `-j 1` at first, and its step time is recorded beside the base
334 s. If that costs more than the matrix saving of D-4 returns, the lane
moves to `-j $(nproc)` with a `RESOURCE_LOCK` over the entries that fork
exec children, named from the P-13 audit. Either form satisfies FR-044.

**Rationale:** the profile counters merge into one `gcda` per object with
no lock, the defect the reverted `00e5848` measured on 2026-10-24. With
the reverted relocation gone, a child writes into the build tree its
parent's counters live in, so separation is the mechanism and AC-3's cold
against warm trace equality is the measurement that settles the choice.

**Alternatives:** `-j 4` on the instrumented suite is refused, because a
lost counter reads as a coverage gap and would push the lane toward an
ignore class or an `LCOV_EXCL` marker, both forbidden (FR-040, FR-041).
`RUN_SERIAL` on every entry is refused by FR-005, because it serializes
entries that share nothing. `GCOV_PREFIX` redirection into per-entry
trees, then one merged capture, is the shape the reverted pair
`5fe94fb` and `872086c` carried; it stays out until a run shows the
serialized form costs more than the split returns.

## P-12: Two exits for a refused trace

**Decision:** `tools/dbc/coverage_gate.sh` keeps `exit 2` for usage and a
missing `lcov`, `exit 0` for a pass and `exit 1` for a shortfall, and adds
`exit 3` for a tool failure: `lcov` returned nonzero and the summary lines
it printed show every gated measure at 100.00 percent, or show no measure
at all. The script stops swallowing the status, which today's `|| true`
on the summary call does.

**Rationale:** FR-043 asks that a refusal not read as a shortfall. The
current shape greps the printed summary for `100.0%`, so a refusal that
prints nothing falls into the shortfall message and sends a reader to
count lines that are already covered.

**Alternatives:** an ignore class for the refusing record is refused by
FR-041. Retrying `lcov` is refused by FR-048's determinism rule. Parsing
the `.info` file instead of lcov's output would duplicate the measure that
the gate is about to read; rejected.

## P-13: The collision audit names its resources

**Decision:** before `-j $(nproc)` reaches `ctest`, the change lists every
entry that shares a resource, and each pair gets a `RESOURCE_LOCK` named
for the resource. The list is read from two sources: the registrations
that already set `PROCESSORS` or `RESOURCE_LOCK`
(`test/CMakeLists.txt:297`, `:660`), and the test sources that write a
fixed path. The known sites today are `counters_trap_checked` on its
scratch file, `harness_cli_test` on `harness_cli_column_note.txt` in its
working directory, and the `tools/dbc` scripts that write under `/tmp`.

**Rationale:** FR-004 asks for the lock where a resource is shared, and
the audit is what keeps a raised `-j` from turning a deterministic suite
into a flaky one. Naming the resource in the lock name is the precedent
`counters_trap_checked_out` sets.

**Alternatives:** `RUN_SERIAL` on the affected entries is refused by
FR-005, because it separates entries that share nothing. A retry or a
sleep is refused by FR-005 and FR-048. Giving every entry its own working
directory would fix the cwd case alone and leave the scratch files and
device nodes untouched; it stays available as the per-entry fix where a
lock would cost more than a directory.
