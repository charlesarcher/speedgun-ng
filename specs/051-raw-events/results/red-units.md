# Red units: the counters unit and the harness unit (T006, T009)

**Mode**: direct unregistered binary runs on unmodified base sources. Each
binary is run straight from its build tree with stdout and stderr redirected
into a log file, and this record quotes that log. Neither unit carries an
`add_test` entry yet, so no ctest read of the standing suite can go red on
them: T015 registers the counters unit and T018 registers the harness unit.
No tracked file under `source/` or `include/` differs from base at this read.
The tracked change in the tree stands in `test/CMakeLists.txt`, and the two
unit sources are new untracked files.

## T006: the counters unit, binary from the shared `build` tree

The binary is `build/test/counters_raw_event_test`, built at T005 over the
shared `ci-rocky` tree `build` (Release). Log:
`/tmp/t006-counters-red.log`.

```text
$ ./build/test/counters_raw_event_test > /tmp/t006-counters-red.log 2>&1; echo exit:$?
exit:1
```

The unit prints one `case: <name>` line ahead of each scenario and one
`RAW EVENT TEST FAIL:` line per failed check, and its `main` returns 1 while
its failure counter is nonzero. The whole log, with the exit line copied
above and the final summary line its last line:

```text
case: rawAcceptRecordsTypeAndConfigWord
RAW EVENT TEST FAIL: the raw address `cpu/rc0` resolves through the core lookup (FR-001)
RAW EVENT TEST FAIL: the scripted kernel saw an attribute of type 4 and config 0xc0 (SC-001)
case: rawAttrMatchesCatalogRowAttr
RAW EVENT TEST FAIL: the typed route resolves the leaf whose window attribute is read (FR-003, FR-007)
case: rawProbeErrnoArms
RAW EVENT TEST FAIL: the raw address r1e answers a settled leaf (FR-004)
RAW EVENT TEST FAIL: the refused open of r1e settles the state of the one errno table (FR-004)
RAW EVENT TEST FAIL: the raw address r2f answers a settled leaf (FR-004)
RAW EVENT TEST FAIL: the refused open of r2f settles the state of the one errno table (FR-004)
RAW EVENT TEST FAIL: the raw address r3a answers a settled leaf (FR-004)
RAW EVENT TEST FAIL: the refused open of r3a settles the state of the one errno table (FR-004)
RAW EVENT TEST FAIL: the raw address r4b answers a settled leaf (FR-004)
RAW EVENT TEST FAIL: the refused open of r4b settles the state of the one errno table (FR-004)
case: rawProbeScopeRelabel
RAW EVENT TEST FAIL: a raw address on a cpumask device answers a settled leaf (FR-004)
RAW EVENT TEST FAIL: the device scope refuses the per-task kind, so one probe runs for the address (FR-004 edge)
case: absentDeviceRefusesWithoutProbe
case: wideLeafNeverProbes
RAW EVENT TEST FAIL: the wide leaf answers at the leaf-match stage, ahead of the catalog-name route (FR-005)
case: probeRunsOncePerAddress
RAW EVENT TEST FAIL: the raw address resolves before its probe count is read (FR-004)
RAW EVENT TEST FAIL: one address pays one probe pair over the two target kinds (FR-004)
case: typedRouteUnitNoneEventsOne
RAW EVENT TEST FAIL: the typed route resolves a raw address under events^1 (FR-007)
RAW EVENT TEST FAIL: the wrong dimension answers the standing dimension line (FR-007)
case: formulaResolvesRawEventName
RAW EVENT TEST FAIL: a metric row whose event name is raw-shaped resolves (FR-007): raw_rate: reference 'r8f' names no event the scope 'core' reaches (FR-014)
case: scriptedPlanCompilesAndMeasuresRawLeaf
RAW EVENT TEST FAIL: the typed route resolves the raw leaf the plan compiles (FR-007)
case: listingsNeverReadRawTable
RAW EVENT TEST FAIL: a raw resolve runs before these listing reads (FR-015)
case: duplicateSpellsDistinctAddresses
RAW EVENT TEST FAIL: each raw spelling resolves through the core lookup (FR-001)
counters_raw_event_test: 22 FAILED CHECKS
```

A second direct run of the same binary reproduces this log byte for byte.

Twelve scenarios run, 22 checks fail, and 11 of the twelve scenarios print a
failing line. Per scenario name:

- `rawAcceptRecordsTypeAndConfigWord`: red, 2 failed checks. The core lookup
  refuses `cpu/rc0`, and the scripted kernel records no open of type 4 with
  config word `0xc0`.
- `rawAttrMatchesCatalogRowAttr`: red, 1 failed check. The window path
  refuses the raw leaf `r412`, so the raw attribute never reaches the
  comparison against the catalog row attribute.
- `rawProbeErrnoArms`: red, 8 failed checks. Each of the four scripted errno
  rows (EINVAL, EOPNOTSUPP, ENOENT, EACCES) answers a refusal for its raw
  address, and the settled state that refused open should leave reads unset
  with it.
- `rawProbeScopeRelabel`: red, 2 failed checks. A raw address on the
  scripted cpumask device answers a refusal, and the scripted kernel records
  0 opens for word `0x500` against the one probe the scope rule asks for.
- `absentDeviceRefusesWithoutProbe`: green, described below.
- `wideLeafNeverProbes`: red, 1 failed check. The refusal of the
  seventeen-digit leaf carries the catalog-name wording
  `has no counter named`, which the check expects behind the leaf-match
  stage. Its two standing checks pass: the wide leaf stays refused, and
  its hex text reaches no probe and no truncation.
- `probeRunsOncePerAddress`: red, 2 failed checks. The resolve of `r5d3`
  answers a refusal, and the scripted kernel records 0 opens for that word
  against the probe pair the two target kinds ask for.
- `typedRouteUnitNoneEventsOne`: red, 2 failed checks. The typed route
  refuses the raw address under `events^1`, and the wrong dimension then
  answers the standing dimension line.
- `formulaResolvesRawEventName`: red, 1 failed check. The metric row
  `raw_rate` answers the refusal
  `reference 'r8f' names no event the scope 'core' reaches (FR-014)`.
- `scriptedPlanCompilesAndMeasuresRawLeaf`: red, 1 failed check. The typed
  route refuses the raw leaf the scripted plan compiles.
- `listingsNeverReadRawTable`: red on 1 failed check, described below.
- `duplicateSpellsDistinctAddresses`: red, 1 failed check. Neither the short
  nor the padded raw spelling resolves through the core lookup.

The standing pins that already answer green at base:

- `absentDeviceRefusesWithoutProbe`: green on all three of its checks, which
  print no failing line. The counters device lookup already refuses a device
  the scripted tree does not hold, and its message already carries the
  standing wording `no object at path 'nosuch'`. The scripted kernel records
  0 opens carrying word `0x4000`, so the refusal already costs no probe.
  FR-002 asks for the base answer of that lookup, so the scenario reads green
  ahead of any raw code.
- `listingsNeverReadRawTable`: its 4 listing checks pass at base, and the 1
  check ahead of them goes red. The seeded count, the catalog names, the
  near-miss candidates for `cache-mise`, and the catalog route for a name
  outside the raw shape keep their base answers, because the raw-entry table
  these reads compare against stands absent. The failing check is the raw
  resolve of `r77b` that stands ahead of those listing reads.

## T009: the harness unit, binary from the dedicated `build/raw-red` tree

**Superseded** by the T009b reading below. The driver behind this run ends at
its first failed check, so 2 of the 10 scenario names is all it can reach.

The binary is `build/raw-red/test/harness_raw_events_test`, built at T008 in
the dedicated tree `build/raw-red` (Release) while
`source/harness/runner.cpp` stands unmodified. Log:
`/tmp/t009-harness-red.log`.

```text
$ ./build/raw-red/test/harness_raw_events_test > /tmp/t009-harness-red.log 2>&1; echo exit:$?
exit:1
```

The unit prints one `case <name>` line ahead of each scenario. The whole log,
with the exit line copied above:

```text
case unknownDeviceRawLine
case wideLeafWidthLine
HARNESS RAW EVENTS TEST FAIL: a seventeen-digit leaf answers the width line in the origin, address, message shape (FR-005, US2-2, SC-002)
  answer:   '--counter: cpu/r1234567890abcdef0: no such leaf in the catalog (FR-023)'
  expected: '--counter: cpu/r1234567890abcdef0: raw event code exceeds 16 hex digits'
```

A second direct run of the same binary reproduces this log byte for byte. The
run prints no summary line, and the shape of that log is the reading of this
task. The unit's `check` and `checkText` helpers reach `fail`, which is
marked `[[noreturn]]` and calls `std::exit(1)` at
`test/source/harness_raw_events_test.cpp:94-120`, and `childOutput` weighs
the scripted child's exit status through that same helper at `:500-530`. The
first failed check ends the process, so a direct run reads the scenario list
up to that check: 2 of the 10 names.

Per scenario name of the T007 list, in the order of that list:

- `unknownDeviceRawLine`: green. Its checks print no failing line: the
  scripted run of `nosuch/r4000` answers the standing no-object line at the
  expected status 2, the scripted kernel records 0 opens carrying word
  `0x4000`, and the run marks `BENCHMARK_NO_RUN` with `BENCHMARK_RAN`
  absent. Those are base answers of the harness, so the scenario reads green
  ahead of the arms of T016 and T017.
- `wideLeafWidthLine`: red, and this failure ends the run. Its first check
  compares the refusal line of the seventeen-digit leaf
  `cpu/r1234567890abcdef0`. The check text, the answer, and the expected
  text stand verbatim in the log block above: the base answer reads
  `no such leaf in the catalog (FR-023)` where the check asks for
  `raw event code exceeds 16 hex digits`, over the same origin and address
  in both.
- `wideLeafBeatsAbsentDevice`, `refusedProbeListedAsLines`,
  `scopeRefusedRawLine`, `refusedRawEndsRunBeforeMeasurement`,
  `duplicateRawAddressTwoColumns`, `rawColumnDisclosesEverywhere`,
  `limitLineCountsRawColumn`, `rawRunConsoleAndJson`: no verdict at this
  read. The process exited at the second scenario, so these 8 names printed
  neither a `case` line nor a failing line of their own.

The T007 list carries nine names expected to read red at this base and one
expected to read green. This direct run puts a verdict on 2 names and leaves
8 names unread, and the record names them as unread.

## T009b: the harness unit rerun with the record-and-continue driver

This section is the T009 answer at this read. Tree state: head
`a65d11bfe2d8cf70618f5878f43c3d6df4ea1a8a`. The only tracked change stands in
`test/CMakeLists.txt`, which holds the two unregistered unit build blocks of
T005 and T008, and the two unit sources stand untracked. Every file under
`source/` and `include/` matches head, so `source/harness/runner.cpp` stands
unmodified. The binary `build/raw-red/test/harness_raw_events_test` was
rebuilt after the driver of `test/source/harness_raw_events_test.cpp` was
converted to record each failed check and continue, and that build log is
`/tmp/t007-rebuild.log`. Run log: `/tmp/t009b-harness-red.log`.

```text
$ ./build/raw-red/test/harness_raw_events_test > /tmp/t009b-harness-red.log 2>&1; echo exit:$?
exit:1
```

All ten `case <name>` lines print, and the run ends on its own count:

```text
harness_raw_events_test: 29 FAILED CHECKS
```

A second direct run of the same binary into
`/tmp/t009b-harness-red-rerun.log` reproduces that log byte for byte. Each
`HARNESS RAW EVENTS TEST FAIL:` line now carries its scenario name ahead of
the check text, and a status failure prints the scripted child's own merged
output ahead of the next line. Ten scenarios run, 29 checks fail, and 1
scenario reads green.

Per scenario name, with its failing-check count and its first failure:

- `unknownDeviceRawLine`: green, 0 failed checks, the expected answer. The
  scripted run of `nosuch/r4000` answers the standing no-object line at the
  expected status 2, records 0 opens carrying word `0x4000`, and marks
  `BENCHMARK_NO_RUN`. Those stay base answers of the harness.
- `wideLeafWidthLine`: red, 2 failed checks. The refusal of the
  seventeen-digit leaf answers `no such leaf in the catalog (FR-023)` where
  the width line is asked for, and a second check weighs the same
  catalog-name wording in that run text.
- `wideLeafBeatsAbsentDevice`: red, 2 failed checks. The address
  `nosuch/r1234567890abcdef0` answers `no object at path 'nosuch'` where the
  width line is asked for, and the device lookup stays behind it.
- `refusedProbeListedAsLines`: red, 2 failed checks. The scripted `EINVAL`
  and the scripted `EACCES` both answer the catalog refusal where the
  encoding and the permission listing lines are asked for.
- `scopeRefusedRawLine`: red, 1 failed check. The scripted `EACCES` on the
  `uncore` cpumask device answers the catalog refusal where
  `listed as scope-refused (FR-023)` is asked for.
- `refusedRawEndsRunBeforeMeasurement`: red, 1 failed check. One refused raw
  address among good ones answers the catalog refusal where
  `listed as not-encodable (FR-023)` is asked for.
- `duplicateRawAddressTwoColumns`: red, 5 failed checks. The `dupSingle` and
  the `dupTwice` child each answer status 2 against the expected 0, and the
  two-label and the document checks follow.
- `rawColumnDisclosesEverywhere`: red, 8 failed checks. The `file` and the
  `env` child each answer status 2 against the expected 0, and the option
  line, the console label and the document checks follow.
- `limitLineCountsRawColumn`: red, 3 failed checks. The `limitRaw` child
  answers status 2 against the expected 0, and the standing
  `counter limit:` line and the run-level record checks follow.
- `rawRunConsoleAndJson`: red, 5 failed checks. The `plain` child answers
  status 2 against the expected 0, and the benchmark, the console label, the
  document and the scripted open checks follow.

Six of the 29 lines name a scripted child at status 2 where its scenario
expects 0: modes `dupSingle`, `dupTwice`, `file`, `env`, `limitRaw` and
`plain`. Each dumped child text starts at `figure device=cpu limit=6`,
carries the catalog refusal for its raw address, marks `BENCHMARK_NO_RUN`,
and closes at `opens total=16`.

## T018: the harness unit green at registration

**Tree**: the shared `ci-rocky` tree `build` (Release), the tree T005,
T015, T016 and T017 built in, on branch `051-raw-events`. The head behind
this section, read fresh with `git rev-parse HEAD` at this write:

```text
$ git rev-parse HEAD
20ba8d77d79db43dd2fbb694d7d5938b64893683
```

T018 ran three rounds in that tree. `test/CMakeLists.txt` carries the line
the task names, inside the block T008 opened:

```text
add_test(NAME harness_raw_events_test COMMAND harness_raw_events_test)
```

**Registration reads.** Three `-N` reads stand in the three `reg` logs.
`ctest --test-dir build -N -R harness_raw_events_test`, read into
`/tmp/t018-r3-reg1.log`, answers the entry:

```text
Test project /home/archerc/code/speedgun-ng/build
  Test #155: harness_raw_events_test

Total Tests: 1
```

`ctest --test-dir build -N`, read into `/tmp/t018-r3-reg2.log`, lists every
registered entry and closes with:

```text
Total Tests: 192
```

`ctest --test-dir build -N -R raw`, read into `/tmp/t018-r3-reg3.log`,
answers the three entries whose name carries `raw`:

```text
Test project /home/archerc/code/speedgun-ng/build
  Test  #55: counters_raw_event_test
  Test  #69: counters_clock_raw_test
  Test #155: harness_raw_events_test

Total Tests: 3
```

Entry #155 is this unit. Entry #55 is the counters unit T015 registered, and
entry #69 names the standing clock unit `counters_clock_raw_test`, which
takes no part in this lane. The 192 stands two above the 190 of the base
read in `results/base-readings.md`: the counters line joined at T015 and
this line joins at T018, and the T017 full-suite read in the same tree
answered 191 between them, with entry 189 skipped
(`/tmp/t017-full-suite.log`).

**The one residual red after T016 and T017.** The W5 direct run of the
unregistered binary named 25 failing checks, byte for byte twice, in
`/tmp/t017-rawunit.log` and `/tmp/w5-rawunit.log`. The registration build
of round 1 (`/tmp/t018-build.log`) left one check, and the two reads of
round 1 agree on which one, the registered read
(`/tmp/t018-unit.log`) and the direct read of the built binary
(`/tmp/t018-direct.log`):

```text
HARNESS RAW EVENTS TEST FAIL: rawRunConsoleAndJson: the scripted column measured a positive figure (US1)
harness_raw_events_test: 1 FAILED CHECKS
```

The check that weighed it, at `test/source/harness_raw_events_test.cpp:1074`
of the file as it now stands:

```text
      check(printed > 0.0 && written > 0.0,
            "the scripted column measured a positive figure (US1)");
```

The other nine scenarios read green at that round, and every other check of
`rawRunConsoleAndJson` read green: the scripted kernel recorded opens
carrying word `c0` with the scripted device type, the child reached its
benchmark, the console row carried the label `cpu/rc0`, and the written
document carried a metric entry under that label. The figure under that
label is what missed. A replay of that scenario's child mode from a scratch
directory answers the row tail and the document entry the child produced
(`/tmp/t018-probe.log`, `/tmp/t018-probe.json`):

```text
cpu/rc0=    0.000 ratio=1.000 scaled=0 gap=0
```

```text
"value":0.0,"runningRatio":1.0,"scaled":false,"availability":"COUNTABLE"
```

**The fork it posed, and the fixture comparison that settled it.** A column
that prints 0.000 beside a ratio of 1.000 and a COUNTABLE availability has
three candidate owners: the column-build arm T017 landed, the counters
window fold W3 carries, or the scripted kernel inside the unit's own
fixture. The record here is the comparison that decided the question, the
unit's scripted read set against the green sibling's group-read reply, both
read against the production ask and fold. The production side, from
`source/counters/linux_pmu/group_io.cpp:293-315`:

```text
      const auto want = static_cast<std::size_t>(
          (kHeaderWords + group.members.size()) * sizeof(std::uint64_t));
      const auto got =
          kernelCalls().readGroup(group.leader, scratch.data(), want);
...
      const auto count = static_cast<std::size_t>(scratch[0]);
      group.enabled = scratch[1];
      group.running = scratch[2];
      for (std::size_t member = 0; member < group.members.size(); ++member) {
        group.values[member] =
            member < count ? scratch[kHeaderWords + member] : 0;
      }
```

`kHeaderWords` is 3 at `group_io.cpp:244`, the plain run seats one member in
its group, and the scratch the window reads into is sized at
`group_io.cpp:272`, so the ask for that group is four words and words
beyond the reply keep the size-initialized zero.

The unit's read at that round, quoted from the fixture as it stood in the
round, where these lines stood at `:252-270`:

```text
auto scriptedRead(int, void* buffer, std::size_t count) -> long
{
  const std::uint64_t windows[3] = {
      kScriptedCount,
      kScriptedWindow,
      static_cast<std::size_t>(gGroupSize) <= kScriptedFit ? kScriptedWindow
                                                           : 1000};
  auto* const posted = static_cast<std::uint64_t*>(buffer);
  for (std::size_t index = 0; index < 3; ++index) {
    posted[index] = windows[index];
  }
  const auto bytes = 3U * sizeof(std::uint64_t);
...
  return static_cast<long>(bytes < count ? bytes : count);
}
```

The constants of that fixture state read `kScriptedCount = 17`,
`kScriptedWindow = 4000` and `kScriptedFit = 6`.

The sibling's reply, the one entry #55 carries through the same window, at
`test/source/counters_raw_event_test.cpp:178-196`:

```text
  calls.readGroup = [](int, void* buffer, std::size_t count) -> long
  {
    const auto words = static_cast<std::uint64_t*>(buffer);
    const auto room = count / sizeof(std::uint64_t);
    if (room <= 3U) {
      return -1;
    }
    const auto members = room - 3U;
    words[0] = members;
    words[1] = gReadPair;
    words[2] = gReadPair;
    for (std::size_t member = 0; member < members; ++member) {
      ...
      words[3U + member] = gReadValue;
    }
    gReadValue += kReadStep;
    gReadPair += kReadStep;
    return static_cast<long>(count);
  };
```

Laid against the fold, the two replies differ in the header. The sibling
derives the member count from the room the window asked for, posts it in
word 0, answers words 1 and 2 with the enabled and running pair, and posts
one value per member from word 3. The unit's reply answered a single-value
triple into a four-word ask: word 0 carried the scripted count, so the fold
read 17 as a member count, words 1 and 2 carried the equal window pair and
settled enabled and running with it, and word 3, the member figure the reply
never wrote, kept the zero the scratch sizing left there. A 24-byte answer
clears `groupReadShort`: the test at `group_io.cpp:38` compares the
returned count against a header size, and the call at `group_io.cpp:297`
names that size as 24 bytes, so the read counted as a good read and no gap
was disclosed. The comparison put the fix in the fixture reply. The
column-build arm and the window fold kept the text they came in with.

**The second round, a fold of a difference.** Round 2 read the same FAIL
line at the same check: its registered read (`/tmp/t018-r2-unit.log`) and
its direct read (`/tmp/t018-r2-direct.log`, byte-identical to the round 1
direct read) each close on one failed check, and the replay from the scratch
directory answered the same zero figure beside ratio 1.000
(`/tmp/t018-r2-probe.log`). The header answer now reached production, so the
remaining fault sat in the figure. A measuring window takes the two endpoint
reads of a pass and folds their difference, so a member figure that stands
still between the two reads folds to zero at every pass, while a pair equal
inside one read keeps the ratio at one. The reply of round 2 posted the same
per-member constant, `kScriptedValue` at 17, on every read. The sibling
quote above already carries the moving shape: `gReadValue` and `gReadPair`
stand at namespace scope at `counters_raw_event_test.cpp:138-139`,
`kReadStep` is 1000 and `kPairStart` is 5000 at `:94-95`, and the reply
advances both figures once per read. The fixture took that shape, and these
are its lines as they now stand:

```text
constexpr std::uint64_t kReadStep = 1000U;
constexpr std::uint64_t kPairStart = 5000U;
constexpr std::size_t kGroupHeaderWords = 3;
...
auto scriptedRead(int, void* buffer, std::size_t count) -> long
{
  auto* const posted = static_cast<std::uint64_t*>(buffer);
  const auto room = count / sizeof(std::uint64_t);
  if (room <= kGroupHeaderWords) {
    return -1;
  }
  const auto members = room - kGroupHeaderWords;
  posted[0] = members;
  posted[1] = gReadPair;
  posted[2] = gReadPair;
  for (std::size_t member = 0; member < members; ++member) {
    ...
    posted[kGroupHeaderWords + member] = gReadValue;
  }
  gReadValue += kReadStep;
  gReadPair += kReadStep;
  ...
  return static_cast<long>(count);
}
```

`test/source/harness_raw_events_test.cpp:210-212` and `:262-281`. The pair
stays equal inside one read, so the ratio still folds to one, and the pair
and the member figure both move across reads, so the endpoint difference of
a pass carries one step of 1000.

**The ruled boundary.** The route that relaxes the check until it accepts a
printed 0.0 stood barred twice. `specs/051-raw-events/tasks.md` opens its
task list with the standing bar:

```text
No task rewrites a red unit so that it
passes, and no task edits an existing test expectation (SC-004).
```

The lane review rails put the same bar on a fixture round directly: the fix
belongs to the scripted reply, and expectation text, scenario names and
claim wording stand unchanged. No check changed its claim in either round.
The claim string of this failing check reads byte-identical in the round 1
log, in the round 2 log and in the current file at `:1075`, and all ten
scenario names of the T007 list print in the green read below. What the two
rounds changed sits inside the fixture: the words `scriptedRead` answers,
the member count it derives from the requested room, and the two
namespace-scope figures that step per read. The five pinned paths of the
T018 verify read unedited, which is the SC-004 half of the same rule
(`/tmp/t018-r3-diffpins.txt`).

**The green read.** Round 3 rebuilt the shared tree
(`/tmp/t018-r3-build.log`). That build log closes on
`Built target harness_raw_events_test` and holds no `error:` line. The
registration chain of round 3 printed:

```text
reg_exit:0
reg_all_exit:0
reg_raw_exit:0
exit:0
```

The last line is the registered read of entry #155, and its log
(`/tmp/t018-r3-unit.log`) answers:

```text
    Start 155: harness_raw_events_test
1/1 Test #155: harness_raw_events_test ..........   Passed    0.02 sec

100% tests passed out of 1
```

A green ctest read prints no child stdout, so the ten case names come from
two read-only runs. `ctest --test-dir build -R harness_raw_events_test -V`
(`/tmp/t018-r3-unit-verbose.log`) prints the ten names under the `155: `
prefix and the ok line. The built binary prints the same ten names whole
(`/tmp/t018-r3-direct.log`). That chain printed:

```text
ctest_V_exit:0
direct_exit:0
```

The direct log itself reads:

```text
case unknownDeviceRawLine
case wideLeafWidthLine
case wideLeafBeatsAbsentDevice
case refusedProbeListedAsLines
case scopeRefusedRawLine
case refusedRawEndsRunBeforeMeasurement
case duplicateRawAddressTwoColumns
case rawColumnDisclosesEverywhere
case limitLineCountsRawColumn
case rawRunConsoleAndJson
harness_raw_events_test: ok
```

Neither green read prints a FAIL line. The four neighbors read in the same
tree and all pass, that read exiting 0 (`/tmp/t018-r3-neighbors.log`):

```text
1/4 Test  #55: counters_raw_event_test ..........   Passed    0.00 sec
2/4 Test  #77: harness_json_schema_test .........   Passed    0.01 sec
3/4 Test  #82: harness_cli_test .................   Passed    0.31 sec
4/4 Test #154: harness_hunt_cli_test ............   Passed    0.67 sec

100% tests passed out of 4
```

and the third chain of round 3 printed:

```text
exit:0
diff_exit:0
pin_exit:0
```

The second line belongs to the read `git diff --name-only 0fce6651` over
the five paths the T018 verify names, and its output file stands empty.
The third line counts the new registration line: the diff of
`test/CMakeLists.txt` piped into `grep -c` on the pattern
`add_test(NAME harness_raw_events_test` answers 1
(`/tmp/t018-r3-pin.txt`).

**Log paths this section relies on.** A later converge pass can re-read
every claim in this section from these files, with no rebuild and no rerun.

- `/tmp/t018-r3-reg1.log`: entry #155 listed, `Total Tests: 1`.
- `/tmp/t018-r3-reg2.log`: the full list, `Total Tests: 192`.
- `/tmp/t018-r3-reg3.log`: entries #55, #69 and #155, `Total Tests: 3`.
- `/tmp/t018-r3-unit.log`: entry #155 passed, 100% out of 1.
- `/tmp/t018-r3-unit-verbose.log`: the same pass with the ten case names
  under the `155: ` prefix and the ok line.
- `/tmp/t018-r3-direct.log`: the ten case names and the ok line from the
  binary, no FAIL line.
- `/tmp/t018-r3-neighbors.log`: #55, #77, #82 and #154 passed.
- `/tmp/t018-r3-diffpins.txt`: empty, the five pinned paths.
- `/tmp/t018-r3-pin.txt`: the single line of the new registration count.
- `/tmp/t018-r3-build.log`: the round 3 build of the shared tree.
- `/tmp/t018-build.log`, `/tmp/t018-r2-build.log`: the round 1 and round 2
  builds of the same tree.
- `/tmp/t018-unit.log`, `/tmp/t018-r2-unit.log`: rounds 1 and 2 of the
  registered read, one failed check each.
- `/tmp/t018-direct.log`, `/tmp/t018-r2-direct.log`: the same failure from
  the binary, byte-identical, with the ten case names ahead of it.
- `/tmp/t018-probe.log`, `/tmp/t018-probe.json`, `/tmp/t018-r2-probe.log`
  and `/tmp/t018-r2-probe.json`: the replayed child mode in a scratch
  directory, its console row and its document carrying the zero figure
  beside ratio 1.000. The two round 1 files carry the version line
  `speedgun-ng 0.34.0 build=Release` and the two round 2 files carry
  `speedgun-ng 0.35.0 build=Release`, the shared tree having taken the
  version bump between the rounds.
- `/tmp/t018-bin.txt`: the two unit binary paths, the shared tree and
  `build/raw-red`.
- `/tmp/t018-cmake-diff.txt`: the `test/CMakeLists.txt` diff of round 1,
  both `add_test` lines of the lane.
- `/tmp/t018-reg1.log`, `/tmp/t018-reg2.log`, `/tmp/t018-reg3.log`,
  `/tmp/t018-r2-reg1.log`, `/tmp/t018-r2-reg2.log` and
  `/tmp/t018-r2-reg3.log`: the registration reads of rounds 1 and 2.
- `/tmp/t018-r2-neighbors.log`: the four neighbors at round 2, all passed.
- `/tmp/t017-rawunit.log`, `/tmp/w5-rawunit.log`: the W5 direct run, 25
  failed checks, byte-identical.
- `/tmp/t017-full-suite.log`: the 191-entry registered suite at T017 with
  entry 189 skipped.
- `/tmp/t017-cli.log`, `/tmp/t017-hunt.log`,
  `/tmp/t017-counters-unit.log`: the pinned neighbor reads of T017.
