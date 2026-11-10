# Clarify pass 7: Raw Event-Code Support (G-31)

Artifacts read: `specs/051-raw-events/spec.md`, `plan.md`, `research.md`,
`data-model.md`, `quickstart.md`, `checklists/requirements.md`, plus
`.specify/memory/constitution.md` (2.20.0). Every citation names the
committed text at `6de0d7db` on `051-raw-events`; the working tree moved
during the pass, and that move has its own section at the end. No file
changed except this report.

## Verdict

No material clarification remains. Every candidate question resolved against
the code the artifacts cite: scope, the pinned line text, the file set, the
version and schema and SOVERSION numbers, and the test strategy each answer
one way. Three wording corrections to derived artifacts are recorded at the
end; none changes what the lane builds.

## Anchor audit

Every `path:lines` citation in the six artifacts was resolved to a tracked
file and checked against that file's length: 185 citations, zero missing
files, zero ranges past an end of file. The load-bearing citations were then
read in full. The readings that carry the material categories:

| Artifact claim | Verified at | Reading |
|---|---|---|
| FR-001 shape "the exact shape the pinned hunter contract recognizes" | `source/harness/detail/hunt_selection.hpp:116-131`, `specs/029-hunter-core/contracts/hunt-cli.md:59` | The predicate is uncapped: `if (text.size() < 2 || text.front() != 'r')` then every later character must be `0-9`, `a-f` or `A-F`. The contract row states `r[0-9a-fA-F]+`. Matches FR-001 and keeps FR-005's too-wide leaf raw-shaped. |
| FR-007 and D-1 route | `include/speedgun-ng/counters_measurement.hpp:607-608`, `include/speedgun-ng/counters_system.hpp:147`, `source/counters/linux_pmu/metric_formula.cpp:1290`, `source/counters/system.cpp:573-590` | One definition, three callers; the typed route and the formula route reach it today. |
| FR-005 width line shape and status | `source/harness/runner.cpp:1021` (`const std::string origin = item.origin.empty() ? "--counter" : item.origin;`), `:1023` (`"%s: %s: %s\n",`), `:1027` (`return 2;`) | The three-field shape and status 2 exist; only the message text is new. |
| R-02's "the wrong arm per FR-005" | `source/harness/runner.cpp:995-997` | The device lookup precedes every leaf test today, so the width arm must be inserted ahead of it, as plan.md step 2 states. |
| FR-004 listed-as line and state words | `source/harness/runner.cpp:1012-1013`, `source/harness/report.cpp:32-39` | `:1012` reads `message = std::string("listed as ") + availabilityName(found->avail)` and `:1013` closes it: `+ " (FR-023)";`. The name table holds `permission-blocked`, `not-encodable` and `scope-refused`. |
| FR-004 errno table and scope relabel | `source/counters/linux_pmu/provider.cpp:722-729`, `:537-557` | `EINVAL`, `EOPNOTSUPP`, `ENOENT` answer `NOT_ENCODABLE`; the default answers `PERMISSION_BLOCKED`; `deviceScoped && onCpu == Availability::PERMISSION_BLOCKED` settles `SCOPE_REFUSED`, and an encoding refusal keeps its own state. |
| FR-003 attribute bits | `source/counters/linux_pmu/group_io.cpp:200-225` | `fillAttr` sets `attr.exclude_kernel = 1`, `attr.exclude_hv = 1` at `:221-222` and the group read pair at `:223-224`: `attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED` then `| PERF_FORMAT_TOTAL_TIME_RUNNING;`. Every leaf of every device gets them. |
| FR-008 duplicate and `r0043` distinctness | `include/speedgun-ng/counters_measurement.hpp:132-141` | `LeafSet::addLeaf` dedups on `leaves[i].address == leaf.address`. |
| FR-007 unit `none`, events^1 | `include/speedgun-ng/counters_core.hpp:177-179`, `:205-206` | `"none"` maps to `Unit::NONE`, and `case Unit::NONE: return Dimension {.time = 0, .events = 1};`. |
| FR-015 compiled plan and gate | `source/counters/plan.cpp:445-466`, `:474`, `:505-536` | The tree scan is the place a raw leaf would answer `"leaf '" + leaf.address + "' is not in the system tree (FR-017)"`; the availability gate and the provider grouping keyed on `record->providerIndex` stand after it. |
| FR-006 carriers | `source/harness/cli.cpp:106-128`, `source/harness/report.cpp:727-736`, `source/harness/runner.cpp:1437-1443` and `:1586-1597`, `source/harness/json_report.cpp:532-538` and `:873-880` and `:1118-1121` | The console disclosure line prints `"options from a file or the environment: "` then `--name=value` joined by `"; "`; the console row prints `"  %s=%9.3f ratio=%.3f scaled=%d gap=%d"`; the column label is the address verbatim; the JSON writes `metricLabels`, each row's `metrics[].label`, and the option uses. |
| FR-009 limit read and line | `source/harness/runner.cpp:1610-1623`, `:1636-1639` | The planner splits each leaf address at the last slash, reads `object->programmableCounters()`, and prints `"counter limit: device '%s' count %zu source probe\n"`. |
| D-7 hunter surface untouched | `source/harness/hunt_cli.cpp:640-645`, `source/harness/hunt.cpp:729-733`, `:421`, `:1049` | Both refusal sites read `detail::hasRawCodeShape`; a hunt run hands only catalog addresses to the runner. |
| SC-004 pins | `test/source/harness_cli_test.cpp:729-755`, `:769-775`, `test/source/harness_hunt_cli_test.cpp:394-398`, `docs/pages/harness.md:2577-2579`, `:2621` | The pinned texts match the contract row at `hunt-cli.md:83` byte for byte. |
| SC-001 host pair | `/proc/cpuinfo`, `/sys/bus/event_source/devices/cpu/type`, `.../cpu/events/instructions` | `vendor_id AuthenticAMD`, `cpu family 26`, `model 68`, type `4`, `event=0xc0`. spec.md:306-307 states `cpu/instructions` beside `cpu/rc0`. |

## Numbers read at the head

- `CMakeLists.txt:82`: `VERSION 0.34.0`; `:172`: `SOVERSION 8`; `:456`:
  `SOVERSION 22`; the specs/050 SOVERSION entry stands at `:167-171`, and
  the specs/048 kept-number precedent at `:453-455`. FR-012 and D-6 read
  correctly.
- `0.35.0` is claimed by no artifact other than this feature, so the
  reservation rule of specs/029 FR-090 leaves it free.
- `source/harness/json_report.cpp:58`: `constexpr std::string_view kSchemaVersion = "1.16";`
- `git grep -c LCOV_EXCL -- source include` sums 84;
  `tools/dbc/coverage_gate.sh:108` reads `allowance=21`;
  `cmake/coverage.cmake:73` runs `--ignore-errors mismatch,negative`, and
  the `inconsistent` comment stands at `:47-59`.
- `tools/docs/example_drift.py:256` reads `PAGE_LINE_CAP = 1004`; the
  README measures 1003 lines. The counter row is `README.md:970`, the guide
  row is `docs/pages/harness.md:675`, and the option text sits at
  `source/harness/options.cpp:564`: `"Adds one catalog counter to the report columns",`.
  No test pins that text anywhere in the tree, so FR-010's three
  carriers are the whole set.
- Presets: `ci-sanitize` at `CMakePresets.json:121`, `ci-tsan` at `:130`,
  `ci-ubuntu` at `:157`, `ci-rocky` at `:168`, and `ci-coverage` at `:113`
  with the binary directory `build/coverage` at `:101`.
- Registration patterns: `test/CMakeLists.txt:553-559` (counters unit) and
  `:1837-1841` (harness unit with the source-harness include). The seam
  family links the same plain way at `:487-498`, so a counters unit that
  includes a private counters header needs nothing beyond that pattern.
- The quick start commands reach real binaries: `add_docs_example(command_line)`
  at `example/CMakeLists.txt:67` builds `docs_command_line`.
- `--list` prints and returns 0 at `source/harness/cli.cpp:937-942`, ahead
  of the address checkpoint at `:966`, which is quickstart.md step 3's third
  expectation.

## Candidate questions, and what closed each one

1. Does the 16-digit refusal contradict an uncapped shape test? Closed. The
   harness predicate carries no width test, and `hunt-cli.md:59` states the
   same uncapped class FR-001 names, so a 17-digit leaf is raw-shaped and
   answers the width line. The two readings differ only in where the width
   arm sits, and plan.md step 2 and R-02 fix that ahead of `runner.cpp:995`.
2. What is the exact text of the unknown-device line? Closed by code.
   `source/counters/system.cpp:311` builds `"no object at path '" +
   std::string(path) + "'"`, and `runner.cpp:1025` passes the address through
   `quotedText`, which escapes control bytes only (`source/harness/options.cpp:82-108`).
   `test/source/harness_cli_test.cpp:731` pins
   `"--counter: /nosuch/leaf: no object at path '/nosuch'"`. quickstart.md:62
   matches; the correction sits in data-model.md, listed below.
3. Can the raw-entry table race a read? Closed by the plan's own rule. The
   table section states "Written once, by the raw probe step, before any
   read." Every write comes from a resolve, and the harness resolves at the
   untimed checkpoint (`source/harness/cli.cpp:960-970`) before any window
   opens; `source/counters/linux_pmu/group_io.cpp:178-187` only reads. The
   lifecycle sentence in plan.md and data-model.md cites the probed-kinds
   ordering, which does not apply, and that is listed as a correction below.
   No file set or test changes: the lane runs `ci-tsan` under FR-013.
4. What does the counters lookup answer for a too-wide leaf on the typed
   route? Closed on behavior. FR-005 keeps it off the catalog-name route and
   FR-015 reserves the no-such-leaf line for names that are not raw-shaped,
   and the plan's `wideLeafNeverProbes` pins "refused, never truncated" with
   zero scripted opens. The library `Error` text is unpinned because the
   library prints nothing itself.
5. Which device counts a raw event name inside a metric formula? Closed by
   FR-007's wording: the formula loop calls `resolveLeafCore` per pmu object
   (`source/counters/linux_pmu/metric_formula.cpp:1286-1291`) and takes the
   first hit. The feature asks for the same lookup and nothing else; the
   covering unit is scripted.
6. Can two typed spellings of one raw address reach two table rows? Closed.
   `System::Impl::canonicalize` (`source/counters/detail/core.hpp:237-244`)
   rewrites only platform aliases, and every real provider seeds
   `.alias = {}` (`source/counters/linux_pmu/provider.cpp:856`,
   `source/counters/clock_provider.cpp:305`, `source/counters/push_provider.cpp:95`);
   aliases come from the scripted FakeProvider only, which owns no PMU
   device. For a raw leaf the typed device part is the tree path.
7. Do the superseded commit hashes in research.md:3-4 break the reviewer
   reads? Closed by measurement. `22dca333` and `1e24ba35` are not ancestors
   of `6de0d7db`; their trees differ from the current `a7f46392` and
   `0fce6651` only in `specs/049-names-old-records/` files, and
   `git diff --name-only 1e24ba35..HEAD` names no file under `source`,
   `include`, `test`, `example`, `docs`, `cmake` or `tools`. Every anchor
   read at that head therefore still stands, and the two quickstart reads at
   lines 98 and 157 answer as the artifact states.
8. Immaterial leftovers: FR-015 sits between FR-007 and FR-008 in the
   requirement list; plan.md:121 counts "three read arms" while plan.md:346
   counts "five read arms"; the code comment at
   `source/harness/detail/hunt_selection.hpp:82` names
   "`splitAddress` of `runner.cpp:41-49`" while the split stands at
   `runner.cpp:81-89`. None touches scope, line text, files, numbers, or
   tests, and the last one sits in code this lane leaves alone.

## Working tree moved during the pass

`git status` at the end of the pass shows four of the six artifacts modified
and uncommitted: `data-model.md`, `plan.md`, `research.md`, `quickstart.md`.
The pass answered the committed text, as the task states, and the uncommitted
text already carries corrections 1 and 3 below. Its new sentences were read
against the code and hold:

- `source/counters/linux_pmu/provider.cpp:767` is `auto device = detail::loadDevice(dir);`
  inside the constructor's device loop at `:766-792`, so the `type` and
  `cpumask` facts stand once per device at load. The zero-sysfs-read pin in
  R-05 and R-15 is right.
- `include/speedgun-ng/counters_measurement.hpp:99` is
  `std::string address;  // "<object path>/<name>", canonical spelling`, and
  `:103` is `Availability avail = Availability::COUNTABLE;`, which is how the
  state reaches the harness in the reworded plan step 2.
- `include/speedgun-ng/counters_core.hpp:162-206` spans `unitFromToken` at
  `:162` through the `case Unit::NONE:` return at `:205-206`.

One line in that uncommitted text lags the design its own research now
states. The physical view at plan.md:206-211 still reads, for
`source/counters/linux_pmu/provider.cpp`: "raw synthesis: sysfs type and
cpumask reads on the loadDevice pattern (:325-342)". R-05 and R-15 pin zero
sysfs reads per raw address. The line should name the loaded device the
constructor reads at `provider.cpp:767` in place of the per-address read
pair. Wording only: no scope, number, carrier, or test moves.

## Corrections to carry into the change (no re-specification)

1. Committed data-model.md:136 shows `<origin>: "nosuch/r4000": no object at
   path 'nosuch'`. The address carries no double quotes in the printed line,
   as quickstart.md:62 and `harness_cli_test.cpp:731` already show. The
   uncommitted edit already drops them and cites the pin.
2. plan.md:44-46 and data-model.md:68-69 (committed: 66-67) cite
   `source/counters/system.cpp:170-172` as the lifecycle the raw-entry table
   copies. Those lines state "The table is filled while a provider registers
   and read only after the catalog opens", which is the probed-kinds
   ordering. Raw addresses arrive after the catalog freezes, which R-06
   already records. Name the raw rule in its own words: one write per address
   in the untimed resolve stage, reads after. This item stays open in the
   committed and the uncommitted text alike.
3. Committed research.md:3-4 names `22dca333` and `1e24ba35` as the branch
   head and parent. Those two objects are no longer on the branch, and their
   trees differ from the current commits only in `specs/049-names-old-records/`
   files. The uncommitted edit already records `0fce6651` as the anchor.


## One gate the docs prose must clear

`tools/docs/example_drift.py:491-503` names the forbidden classes, and the
scan reaches the main page at `:784` and the two User-guide pages at
`:785-788`. The classes include `FR-` and `SC-`
identifiers, `051-raw-events` style spec identifiers, and any hex run of 7
to 40 characters that holds a letter. The counter prose FR-010 adds can name
short examples such as `cpu/rc0` and `cpu/r4000` safely, and it must keep
requirement identifiers and long raw hex runs out of those two pages.
FR-010's "the drift reads stay clean" already binds this; it is recorded so
the implement step does not discover it late.
