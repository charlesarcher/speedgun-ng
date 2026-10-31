# Evidence plan: Host Validation

What the lane runs, in what order, and what each run has to show. The
files beside this one hold the results: `quickstart.md` every quickstart
scenario, `taskset.md` the narrow-host runs, `ci-run.md` the pushed-head
verdict.

## The lane host

| Fact | Value |
| --- | --- |
| Vendor, family, model, stepping | `GenuineIntel`, 6, 198, 2 |
| Model key, what `mapfileModelKey` renders | `GenuineIntel-6-C6` |
| Record key, the CPU type, what `mapfileKey` renders | `GenuineIntel-6-C6-2` |
| Record file, the record key with the `.json` suffix | `validation/GenuineIntel-6-C6-2.json` |
| Catalog directory | `arch/x86/arrowlake/` |
| Online cpus | 20 |
| `perf_event_paranoid` | 2, recorded in the cited run output alone |
| Toolchain | the `ci-rocky` preset, GCC release developer mode |

The record key carries the model key plus the one hex digit of the
stepping, and a host that answers no leaf 1 leaves that digit out, so its
record key equals its model key (FR-007, FR-009). Every table below names
a CPU type by its record key.

## The runs

| # | Command | Shows |
| --- | --- | --- |
| 1 | `cmake --preset=ci-rocky && cmake --build build` | the build, the two programs, the five CTest entries |
| 2 | `ctest --test-dir build -R host_record --output-on-failure` | every wave green |
| 3 | `cmake --build build --target validate` | one record, the record line, one line per committed record, exit 0 |
| 4 | `cmake --build build --target validate` again | the refresh keeps the evidence list standing |
| 5 | `./build/test/host_record_check --root . --records validation` | the committed set green, exit 0 |
| 6 | the planted break, stale, empty-root, planted-mapfile and no-mapfile scenarios of `quickstart.md` | each verdict, each refusal line, each exit |
| 7 | `taskset -c 0,1 ctest --test-dir build -R host_record` | the narrow host, with any write aimed at an output root under the build directory |
| 8 | the pushed CI run | every hard gate, the keep-set reading of FR-038 |

## The record set

| CPU type | Record | Evidence |
| --- | --- | --- |
| `GenuineIntel-6-C6-2` | committed by run 3 as `validation/GenuineIntel-6-C6-2.json` | none; the list stands empty (FR-041) |
| the Zen 5 types | one record, or two when the two idents differ; both wait for their hosts | the `--hunt=all` row cited from `specs/029-hunter-core/results/gary-hunt-all.md`, the placement rows cited from `specs/035-numa-placement/results/` and the pre-check row cited from `specs/034-host-noise-precheck/results/quickstart.md` (FR-016, FR-017) |
| the Intel desktop type of `specs/029-hunter-core/results/corsairone-hunt-all.md` | waits for its host | the `--hunt=all` row and its top 20, cited from `specs/029-hunter-core/results/corsairone-hunt-all.md` and `specs/029-hunter-core/results/corsairone-hunt-all-top20.txt` (FR-016) |

Whether the desktop Zen 5 host and the Threadripper Zen 5 host render one
key or two stays open: no record holds their idents, and rows `:46` and
`:47` of `external/pmu-events/arch/x86/mapfile.csv` spread family 26 over
two directories, row `:46` matching the models it names and row `:47`
sending the rest to `amdzen6`. Two idents that differ keep two records;
two that match share one record, and a shared record carries both hosts'
rows with the machine block of the host that wrote last (D-7, FR-006,
FR-016).

## Gaps

FR-041 names a record that waits for its host, and states in one line a
CPU type that owns no acceptance row.

| CPU type | Shape it waits for | Run it waits on |
| --- | --- | --- |
| the desktop Zen 5 type | a host that renders its key | the record
write; its `--hunt=all` row stands recorded at
`specs/029-hunter-core/results/gary-hunt-all.md` |
| the Threadripper Zen 5 type | a host that renders its key | the record
write; its placement and pre-check rows stand recorded, and it ran no
`--hunt=all`, so it carries no hunter row |
| the Intel desktop type | a host that renders its key | the record
write; its `--hunt=all` row and top 20 stand recorded at
`specs/029-hunter-core/results/corsairone-hunt-all.md` |

`GenuineIntel-6-C6-2` owns no acceptance row, so its record carries none,
and that stands stated here in one line: the gap table names rows waiting
on a run, and no run of FR-016 or FR-017 waits on this host (FR-016,
FR-017).

A row binds when its record is written, so the lane commits its own
record with an empty list. A run of `--hunt=all` here fills nothing: FR-016
and FR-017 carry the rows the earlier prompts named, a third CPU type's
hunt closes none of them, and the host's `perf_event_paranoid` value of 2
narrows what such a run measures (the Clarifications note of 2026-10-29).
Authoring a row for a host that never ran would make the record a claim no
host made.

## The acceptance rows the lane cannot carry

The table names each waiting evidence entry: the CPU type whose host ran
it, the run, the command, the exit status the record states, the place that
carries it, and the record the entry waits on. A row binds when its record
is written, so every entry waits on the record its own host's first write
produces, and this lane commits its record with an empty list (FR-006,
FR-015, FR-016, FR-041). `specs/031-host-validation/spec.md:67-70` names the
three CPU shapes these runs stand at. The record set table above names the
record each type waits for, and this table names the rows.

| CPU type | Row | Command | Exit | Cited | Record it waits on |
| --- | --- | --- | --- | --- | --- |
| the desktop Zen 5 type | hunter | `--hunt=all` with `--hunt-top=20`, `--min-time=0.2` and one instance named by `--filter` | 0 | `specs/029-hunter-core/results/gary-hunt-all.md:30` for the invocation, `:34` for the exit status, `:173-200` for the 20 ranked rows | its own `validation/<key>.json`, the key unread until that host writes |
| the Intel desktop type | hunter | `--hunt=all` with `--json` on the quickstart benchmark, the capture on the fixed tree | 0 | `specs/029-hunter-core/results/corsairone-hunt-all.md:368` for the invocation, `:372` for the exit status, `:387` for the rank rows, and `specs/029-hunter-core/results/corsairone-hunt-all-top20.txt:1-20` for the attached 20 | its own `validation/<key>.json`, and that host stands down since 2026-10-26 (`specs/031-host-validation/spec.md:69-70`) |
| the Threadripper Zen 5 type | placement, `NPS1` | `./build/example/docs_topology_placement --topology` | 0 | `specs/035-numa-placement/results/nps1.md:10` for the command, `:17` for the exit status | its own `validation/<key>.json`, the key unread until that host writes |
| the Threadripper Zen 5 type | placement, `NPS1` | `./build/example/docs_numa_placement --placement=node` | 0 | `specs/035-numa-placement/results/nps1.md:19` for the command, `:24` for the exit status | its own `validation/<key>.json`, the key unread until that host writes |
| the Threadripper Zen 5 type | pre-check, `NPS1` | `./build/example/docs_numa_placement --placement=node` with `--jitter-probe=idle,mode=precheck,precheck=50` | 0 | `specs/035-numa-placement/results/nps1.md:30` for the command, `:31` for the exit status | its own `validation/<key>.json`, the key unread until that host writes |
| the Threadripper Zen 5 type | placement, `NPS2` | the command set of `specs/035-numa-placement/results/plan.md:36-39` | none | `specs/035-numa-placement/results/nps2.md:11` for the runs standing none, `:18` for the boot as the gap | the `NPS2` boot, then its own `validation/<key>.json` |
| the Threadripper Zen 5 type | placement, `NPS4` | the command set of `specs/035-numa-placement/results/plan.md:36-39` | none | `specs/035-numa-placement/results/nps4.md:11` for the runs standing none, `:18` for the boot as the gap | the `NPS4` boot, then its own `validation/<key>.json` |
| the 24-core, 4-L3-group type of `specs/034-host-noise-precheck/results/quickstart.md` | pre-check | `ctest --test-dir build -R harness_precheck`, the command that record's row takes from its row above | 0 | `specs/034-host-noise-precheck/results/quickstart.md:5` for the command and its exit status, `:6` for the measured pre-check | its own `validation/<key>.json`, the key unread until that host writes |

The command column names the options and points at the lines that spell the
whole invocation: the output-file argument of the two hunter runs carries a
host name, and FR-042 keeps a host name out of this file, so each row states
the coordinates a reader opens for the exact bytes.

Two readings travel with the Intel row. Its first attempt at `--hunt=all`
aborted at exit 134 on an earlier commit, at
`specs/029-hunter-core/results/corsairone-hunt-all.md:25`, and its top-20
section states
`Status: defect observed, not acceptance evidence` at `:165`, so the row
cites the capture on the fixed tree, which opens at `:356`. That host also
discloses `perf_event_paranoid` 2 at `:390`, so the 489 rank rows of that
run each read `instr=0.000 cv=0.00 mark=unrepeated` (`:387`), and the
attached 20 rows carry no rank number
(`specs/029-hunter-core/results/corsairone-hunt-all-top20.txt:1`). The row
states the run as that host answered it, and a ranked figure needs a host
whose PMU answers user-space reads.

Three readings travel with the placement rows and the pre-check row. The
`NPS2` and `NPS4` states stand recorded with no run: each names the
memory-map reboot as the reach it stands outside, at
`specs/035-numa-placement/results/nps2.md:18` and
`specs/035-numa-placement/results/nps4.md:18`, so those rows wait on a boot
before they wait on a record. The pre-check record names its host by shape,
24 cores and 4 L3 groups
(`specs/034-host-noise-precheck/results/quickstart.md:1`), and its pinning
line numbers a cpu 24 (`:6`), the shape
`specs/031-host-validation/spec.md:68` gives for the Threadripper type, so
that row binds on the machine shape the record carries, and no ident stands
in that record to bind it further. The placement records carry a suite row
each, and those state test status, so no evidence entry stands for them
here.

A CPU type that owns no acceptance run carries no row: the Threadripper type
ran no `--hunt=all`, so it carries no hunter row, and `GenuineIntel-6-C6-2`
owns no acceptance run at all (FR-016, FR-017, SC-011).

Every path above opened and every line above read as its row states it,
checked on 2026-10-29 against the tree at `3acae46`. The reader's second
and third staleness rules ask a path, then a line inside that file
(`specs/031-host-validation/contracts/record-check.md:114-115`), so each row
carries both coordinates and the next row authored here meets them on its
first read (FR-015, FR-025).

## The name scan

Task T057, then T065, FR-042. This run reads the landing tree at `d2c95fa`,
2026-10-30. The scan takes every file this feature adds, whole, and every
line the branch adds to a file it edits, so the shipped surface and the text
this branch wrote into the standing pages both stand covered. The lane
record `validation/GenuineIntel-6-C6-2.json`, which `3d7dd2f` landed the
same day, reads with them.

Scanned whole, 28 files: `cmake/validate.cmake`, the schema
`docs/schema/host-record-v1.json`, `source/harness/host_record.cpp`,
`source/harness/host_record_check.cpp`,
`source/harness/detail/host_record.hpp`,
`source/harness/detail/host_record_check.hpp`, `tools/hostcheck/mapfile.hpp`,
`tools/hostcheck/host_record_write.cpp`,
`tools/hostcheck/host_record_check.cpp`, the three test units
`test/source/harness_host_record_test.cpp`,
`test/source/harness_host_record_check_test.cpp` and
`test/source/harness_host_record_schema_test.cpp`, the record
`validation/GenuineIntel-6-C6-2.json`, and the 15 files under
`specs/031-host-validation/`. Scanned as added lines, 324 lines over the 16
files the branch edits: `README.md`, `docs/pages/building.md`,
`example/quickstart.sh`, `cmake/dev-mode.cmake`, `cmake/dbc-gate.cmake`,
`cmake/coverage.cmake`, `test/CMakeLists.txt`, `tools/docs/example_drift.py`,
`tools/dbc/dbc_pair_gate.py`, `tools/dbc/coverage_gate.sh`,
`source/harness/json_load.cpp`, `source/harness/detail/internal.hpp`,
`.codespellrc`, `.github/workflows/ci.yml`, `.gitignore` and
`.specify/feature.json`.

Twelve patterns, each case-insensitive where a name can shift case, run
over each file with Python's `re`. Every one of them comes off the machine,
so the scan reads what a lane record would leak. Four name the identity of
this host: the account `git config user.name` and `$USER` answer, the same
token as the suffix of the git remote owner, the owner token `git remote
get-url origin` carries and which holds a given name, and the hostname
`uname -n` answers, which also stands in the domain part of `git config
user.email`. Two name the hosts the standing hunter records carry, the two
file stems of `specs/029-hunter-core/results/`. Six are shape patterns that
catch an identity without naming it: an email address, an `@` followed by a
host, a home path of the form `/home/`, `/Users/`, `/run/user/`, `/mnt/` or
`/media/` followed by a name, a Windows user path, a machine id, and a
`desktop-` or `laptop-` machine string. This record names every pattern by
its source command and leaves the token out, because FR-042 keeps a host
name and a person name out of every file this feature adds, and a lane
record is one of those files. The five commands quoted above reproduce the
four identity patterns on the lane host.

The shipped surface answers nothing. Neither program under
`tools/hostcheck/`, neither header, neither library source, the schema, the
three test units, `cmake/validate.cmake`, `cmake/dev-mode.cmake`,
`cmake/coverage.cmake`, `README.md`, `docs/pages/building.md` nor
`example/quickstart.sh` carries a pattern, no added line of any edited file
answers one, and the lane record answers none: its 315 bytes hold the cpu
key, the catalog directory and the machine shape, which is the identity
FR-042 asks for.

Twenty lines answer, every one of them inside a spec artifact of this
feature. Seventeen cite a hunter evidence file under
`specs/029-hunter-core/results/`, spelled the way the repository spells it:
twelve lines of `results/plan.md`, three of `tasks.md` and two of
`results/taskset.md`. Two more name those records by file name alone, at
`specs/031-host-validation/spec.md:120-121`. Each of those paths and each of
those four file names stands in the tree under that spelling, checked file
by file. FR-042 asks for exactly that: a recorded run keeps the name it was
written under, so a reader can open it, while the cpu type and the machine
shape carry the identity of a record.

One line answers the machine-id shape and names no machine. The record
digest at `specs/031-host-validation/results/taskset.md:337` is 32
hexadecimal characters that hash record bytes.

The finding the first run carried is gone. `specs/031-host-validation/tasks.md`
stated the repository root as an absolute path under a home directory, and
`b855388` replaced that line with `$PWD`, the substitution this record set
uses for every absolute path. At this head the account pattern, the
remote-owner pattern, the hostname pattern, the email pattern, the `@`-host
pattern, the home-path pattern, the Windows-user pattern and the
machine-string pattern answer nothing in either scope, and the two host
patterns answer only in the cited paths above.
