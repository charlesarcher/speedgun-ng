# Merge notes: feature 029, hunter core

Read this before the rebase and before the squash that lands the branch. It
carries the four values this branch reserves, each with its reason and its
rebase rule, and it audits the shared-list ledger of `plan.md` row by row
against the working tree.

The audit ran on `029-hunter-core` with the merge base to `master` at
`bd10d76174a402ff694883291458bed470002459`. Two heads were read while it ran,
`ba015f0` first and `9121226` after a co-running node committed its tasks
checkmarks, and the four reserved values stood unchanged across both reads.
Most of the feature sits in the working tree beside those commits: the second
`git status --porcelain` read counted 18 modified paths and 5 untracked ones.
So every citation below names a line read from a file or a line of command
output captured in this session, and `bd10d76` is the edge the diff commands
were run against.

## Reserved values

T117 names these four. Each one is a reservation: the number is chosen for a
tree where the lanes ahead of this one have still to land, so a rebase is
expected to retake it.

| Value | Where it is set | Why it is reserved | At rebase, when master moved |
|---|---|---|---|
| `0.19.0` | `CMakeLists.txt:14`, the `project()` `VERSION` | `0.17.0` and `0.18.0` are the minors specs/027 and specs/028 reserve on the master line | Read `VERSION` on the new master and take the next free minor above it |
| `SOVERSION 4` | `CMakeLists.txt:62`, the `speedgun-ng` archive | The exported `FakeProvider` gains the scripted group-leaf refusal of T013, so an exported layout moves | Read the `SOVERSION` in that same properties block on the new master and add one |
| `SOVERSION 11` | `CMakeLists.txt:274`, the `harness` archive | `speedgun-ng/hunt.hpp` joins the public surface and both `BenchmarkResult` and `ReportData` gain a hunt record | Read the `SOVERSION` in that same properties block on the new master and add one |
| `"1.4"` | `source/harness/json_report.cpp:34`, `kSchemaVersion` | The root `hunt` object and the per-benchmark `hunt` object are keys added inside major 1, the move `contracts/report-schema-key.md` states | Take the next free minor and carry the keys of the lanes that merged first |

### The project version, 0.19.0

The branch sets it at `CMakeLists.txt:14`, quoted `    VERSION 0.19.0`. At
`bd10d76` the same declaration sits at line 9 with `    VERSION 0.16.0`, and
the branch diff shows the move:

```
-    VERSION 0.16.0
+    VERSION 0.19.0
```

The reservation is spelled out in the comment the branch added at lines 9 to
13: `0.17.0` is the minor specs/027 reserves, `0.18.0` the minor specs/028
reserves, so this feature takes `0.19.0` (FR-090, PD-20).

At rebase, read `VERSION` in `project()` on the new master and take the next
free minor above whatever it carries. If master took `0.19.0` in the
meantime, this branch becomes `0.20.0`. Keep the comment with the new number.
Nothing else in the file moves with it: the archive properties at lines 47 and
247 and the `SG_PROJECT_VERSION` definition at line 240 all expand
`${PROJECT_VERSION}`.

### The speedgun-ng archive SOVERSION, 4

The branch sets it at `CMakeLists.txt:62`, quoted `    SOVERSION 4`, where
`bd10d76` has `    SOVERSION 3` at line 53. The reason stands in the comment
at lines 58 to 61: the exported `FakeProvider` gains the scripted group-leaf
refusal of T013, whose scripted member moves the layout of the exported final
class (FR-090, hard rule 12).

At rebase, read the `SOVERSION` inside the `set_target_properties` block of
`speedgun-ng_speedgun-ng` on the new master and take one above it, then append
a comment in the shape lines 58 to 61 already use. The two archive numbers
move on their own evidence, so a bump on one side says nothing about the
other.

### The harness archive SOVERSION, 11

The branch sets it at `CMakeLists.txt:274`, quoted `    SOVERSION 11`, where
`bd10d76` has `    SOVERSION 10` at line 261. The comment at lines 270 to 273
names the layouts that moved: `speedgun-ng/hunt.hpp` enters the public
surface, `BenchmarkResult` gains its hunt record, `ReportData` gains its hunt
record (FR-090, hard rule 12).

At rebase, apply the same rule as the counters archive: read that block on the
new master, add one, and append the reason.

### kSchemaVersion, "1.4"

The branch sets it at `source/harness/json_report.cpp:34`, quoted
`constexpr std::string_view kSchemaVersion = "1.4";`, and the writer emits it
at line 740 through `builder.append(kSchemaVersion);`. At `bd10d76` the same
declaration reads `constexpr std::string_view kSchemaVersion = "1.1";` at
line 31. The comment at lines 29 to 33 gives the reason and the rebase rule:
the root `hunt` object and the per-benchmark `hunt` object are keys added
inside major 1, which moves the minor, and a rebase onto a master that already
took `1.4` takes the next free minor.

At rebase, take the next free minor above what the new master writes and carry
the keys of the lanes that merged first (FR-053). Then move
`docs/schema/report-v1.json` with it and recheck the three statements outside
the writer that name a written version: `docs/pages/harness.md:1067`,
`test/source/harness_json_load_test.cpp:113` and
`test/source/harness_json_load_test.cpp:529`. Two of them are stale at the
time of this audit, as the findings below record, so a rebase onto `1.5` has
to settle them anyway.

### Re-taken against master, 2026-10-23

`git fetch`, then the three numbers on `origin/master` at `bd10d76`:
`VERSION 0.16.0` at line 9, `SOVERSION 3` at line 53 and `SOVERSION 10`
at line 261. `git merge-base --is-ancestor origin/master HEAD` answers
yes, so this branch already carries master, and the rebase T063 names as
its base is a no-op on this host.

Master has taken neither of the minors `0.17.0` and `0.18.0` reserve, so
the four values above stand as set: `0.19.0`, `SOVERSION 4`,
`SOVERSION 11` and `"1.4"`. No number in `CMakeLists.txt` moves for this
measurement. The page cap of 809 lines that `6e1d28b` recorded is
already the value the page carries on a tree that holds master.

## Shared-list ledger

`plan.md:445-457` names eleven shared surfaces, and `plan.md:48` names one
more, the vendored pmu events tables. Twelve rows follow. Each states what
this branch adds and whether the working tree shows it, with the line that was
read.

| Surface | What this branch adds | Line read | In the tree |
|---|---|---|---|
| `CMakeLists.txt` | The three reserved values with a comment block each | `14:    VERSION 0.19.0`, `62:    SOVERSION 4`, `274:    SOVERSION 11` | Yes |
| `test/CMakeLists.txt` | Seven `add_executable` blocks with their `add_test` lines, `SG_SCHEMA_PATH` on the hunt JSON test, `SG_CLI_SOURCE` on the table test, one `report_shape_hunt_rank_sample` entry | `1103`, `1111`, `1119`, `1127`, `1135`, `1145` for the six `harness_hunt_*` targets, `1159` for `harness_cli_table_test`, `1151` and `1165` for the two defines, `1293` for `NAME report_shape_hunt_rank_sample` | Yes |
| `example/CMakeLists.txt` | One docs example registration | `75:add_docs_example(hunt_rank)` | Yes |
| `README.md` | A `### Hunter mode` section, eight option rows, seven quick-reference name rows | `621:### Regression gate` then `646:### Hunter mode`, the section running to `671:## Quick reference`; name rows `769` to `775`, option rows `799` to `806`; `git diff --numstat` gives `40 0 README.md` | Yes |
| `docs/pages/harness.md` | The Hunter mode page: option detail, refusal lines, printed lines, the two hunt objects, the library surface | `1342:## Hunter mode` with H3 sections at `1356`, `1383`, `1404`, `1429`, `1463`, `1490`, `1501`, `1513:### The two hunt objects`, `1542`; `git diff --numstat` gives `212 0 docs/pages/harness.md` | Yes, with one stale claim, finding 1 |
| `docs/schema/report-v1.json` | `hunt` in both `x-order` lists and both `properties` objects, with both `required` lists untouched | `7:  "x-order": ["schemaVersion", "speedgun", "benchmarks", "hunt"]`, `50:          "hunt"` in the entry `x-order`, `467:          "hunt": {` as the entry property, `541:    "hunt": {` as the root property; the root `required` at `8` and the entry `required` spanning `52` to `71` hold no `hunt`; `git diff --numstat` gives `149 3 docs/schema/report-v1.json` | Yes |
| `tools/docs/example_drift.py` | `"Hunter mode"` in `SECTIONS` and in `SECTION_OWNERS`, and a `PAGE_LINE_CAP` raise with its measured note | `32:SECTIONS = (` with `58:    "Hunter mode",` inside it, `64:SECTION_OWNERS = {` with `90:    "Hunter mode": ("hunt_rank.cpp",),`, `168:PAGE_LINE_CAP = 813` with the note at `157` to `167`; `wc -l README.md` answers `813` | Yes |
| `external/pmu-events`, `cmake/EmbedPmuEvents.cmake`, `cmake/embed_pmu_blob.cmake` | Nothing, the tables are consumed as they stand | `git diff --quiet bd10d76 -- external/pmu-events cmake/EmbedPmuEvents.cmake` exits `0` and `git status --porcelain` on those paths prints no line; the same quiet check on `cmake/embed_pmu_blob.cmake` exits `0`; the embed entry stays `cmake/EmbedPmuEvents.cmake:15:function(speedgun_embed_pmu_events target vendored_root)` | Yes, as a no-edit row |
| `test/source/harness_json_load_test.cpp` | The plan asks for the version literals at `113` and `529` to sit beside the bumped version | `113:  sg::test::writeText("harness_json_load_current.json", withVersion("\"1.1\""));` and `529:    {"\"schemaVersion\":\"1.1\"", "\"schemaVersion\":1", "/schemaVersion"},`; the file appears in no row of `git diff --stat bd10d76` for the shared surfaces | No, finding 2 |
| `test/source/harness_compare_cli_test.cpp` | Nothing, exactly as the plan row requires | The file appears in no row of the same `git diff --stat`; `515:  check(containsAll(lines[0], {"speedgun 0.14.0", "schema 1.0"})` and `516:            && containsAll(lines[1], {"speedgun 0.16.0", "schema 1.1"}),` hold the frozen fixture's own version fields | Yes, as a no-edit row |
| `test/source/harness_cli_table_test.cpp` | The whole file, created here | `git cat-file -e bd10d76:test/source/harness_cli_table_test.cpp` exits `128`, so master holds no such file, and `git ls-files --error-unmatch` on it exits `0`; `git diff --numstat` gives `610 0` for it; `test/CMakeLists.txt:1159` registers it | Yes |
| `test/source/harness_pass_report_test.cpp` | Nothing, its expectation stands | The file appears in no row of the same `git diff --stat`; the covering check is `test/CMakeLists.txt:1089:add_test(NAME harness_pass_report_test_identity`, `1090:         COMMAND harness_pass_report_test byte-identity`, `1091:                 ${CMAKE_CURRENT_SOURCE_DIR}/source/detail/pass_identity.expected)` | Yes, as a no-edit row |

Three rows need a word beside the table.

The seven registrations the plan asks for land as six `harness_hunt_*` targets
plus `harness_cli_table_test`, which is the seventh and sits under its own
comment at `test/CMakeLists.txt:1155`. `grep -c 'add_executable(harness_hunt'`
answers `6`, so a reader who counts only the `harness_hunt_` prefix comes up
short of the plan number.

The pmu events tables hold no row of the plan ledger table: `plan.md:48` names
them under the dependencies, and the task that asks for this audit names them
as a shared surface. They are recorded here as a no-edit row so that the
absence is a read fact.

`git diff --stat bd10d76 -- test/CMakeLists.txt` reports `93 +++++++++++` and
`example/CMakeLists.txt` reports one line added against one removed: the new
registration took the blank line after `add_docs_example(regression_gate)` and
landed at line 75, where the plan cited "after line 74".

## Main-page impact check (IV.1)

The `Hunter mode` section of `README.md` teaches the capability with one
purpose sentence, one marked snippet, one result sentence and the
quick-reference rows, inside the 25-line budget of IV.1's per-section rule.

- The snippet is the `hunt-rank` region of `example/docs/hunt_rank.cpp`, a
  file the `docs_hunt_rank` target builds; `cmake --build <lane> --target
  docs_hunt_rank` answers 0 on this host.
- `example_run` executes that program: `test/example_run.sh` walks
  `example/docs/*.cpp` and fails a run whose registered benchmark prints no
  row, so the block the page quotes is a block that printed. It passed in
  67.76 s on this host.
- The page block and the region are one fact: `tools/docs/example_drift.py`
  answers 0 with `PAGE_LINE_CAP` at the measured 809 lines.
- `report_shape_hunt_rank_sample` compares the console the same binary
  prints with `example/docs/hunt_rank.expected`, so the rows the result
  sentence names are rows the run printed.

## Findings this audit leaves open

Every finding below is closed, each by the commit named under it. The text
stands as the audit read it.

1. `docs/pages/harness.md:1067` still states the version a run writes as
   `1.1`. That line was true at `bd10d76`, where `kSchemaVersion` stood at
   `1.1`, and the writer moved to `1.4` while the sentence stayed. The plan row
   for the page asks this sentence to move with the schema minor.

   ```
   1067	names the version the document carries. A run writes `1.1`, the shape that
   1068	added the pass fields; a `1.0` document, which carries none of them, loads
   ```

   Nothing in the page states `1.4`: the only `1.4` reads over the file land
   inside printed sample values.

   Closed by `c89d56e`, which moves the sentence to `1.4`.
2. `test/source/harness_json_load_test.cpp` carries no edit at all, while its
   plan row at `plan.md:452` places lines 113 and 529 beside the bumped
   version. The check at line 114 is named "the version this build writes
   loads (FR-010)" and the fixture it loads claims `1.1` while the build now
   writes `1.4`, so the name no longer describes the fixture. A `1.1` document
   loads under the version rule either way, and the mutation seed at line 529
   has to match the committed fixture text it mutates. No test was run here to
   settle either case.

   Closed by `ec3559f`: both rows read `1.4`, the seed at line 529 matches the
   fixture text it mutates, and `harness_json_load_test` passes on this host.
3. Landing needs the untracked artifacts added. The second status read lists
   `example/docs/hunt_rank.cpp`, `example/docs/hunt_rank.expected`,
   `test/data/hunt_no_hunt_console.txt`, `test/data/hunt_no_hunt_document.json`
   and `specs/029-hunter-core/results/`, the last holding
   `corsairone-hunt-all.md` and `gary-hunt-all.md`, the AC-10 hardware records.
   A squash that runs `git commit -a` would drop all of them. The first status
   read also found droppings at the repository root (`hunt_cli_blank.txt`,
   `hunt_cli_dup.txt`, `hunt_cli_list.txt`, `hunt_cli_raw.txt`,
   `harness_hunt_cli_report.json` and a directory named `local:/`) and a later
   read in the same session found them gone.

   Closed: the artifacts are tracked (`9338ca4` the example and its recording,
   `ec3559f` the two report fixtures, `6f874fe` the two hardware records) and
   no droppings stand at the repository root.
