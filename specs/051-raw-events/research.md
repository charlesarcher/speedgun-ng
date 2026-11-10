# Research: Raw Event-Code Support (G-31)

All readings are from `051-raw-events` at the specs/049 squashed head
`0fce6651`, the parent of this lane's documents on the branch. The
scouting notes at `/tmp/part2-scout-notes.md` mapped the surface on the
specs/049 branch before that squash, at `dd400fd7`; every anchor below was
re-read at `0fce6651` and the current line number stands.

## R-01. The shape recognizer, one contract in two places

Decision: the counters core lookup carries its own raw-shape predicate in
the private counters detail; the hunter keeps `hasRawCodeShape`
(`source/harness/detail/hunt_selection.hpp:106-131`) byte-identical; the
harness counter-route arms reuse that harness-side predicate.

Rationale: FR-001 names the shape as "the exact shape the pinned hunter
contract recognizes": lower-case `r`, then one or more hex digits of
either case. The contract line stands at
`specs/029-hunter-core/contracts/hunt-cli.md:59` and the test pin at
`test/source/harness_hunt_cli_test.cpp:394-398`; the refusal sites
`source/harness/hunt_cli.cpp:640-645` and `source/harness/hunt.cpp:729-733`
stay untouched (D-7). The harness header comment at
`hunt_selection.hpp:110-113` already states that each route reads the entry
the shape test names in its own way, and the repository already carries the
same-split-in-two-places pattern (`splitAddress` at `source/harness/runner.cpp:81-89`
against `devicePartOf` at `hunt_selection.hpp:86-93`, cross-noted there at
line 82). The counters layer cannot include a harness header, and exporting a
counters predicate would move a public header, which R-12 prices.

Alternatives considered: one shared exported helper, rejected because an
export moves the counters SOVERSION (R-12); a copied predicate inside the
harness, rejected because the harness already owns the identical predicate
in `hunt_selection.hpp`.

## R-02. Sixteen hex digits, checked ahead of everything

Decision: `kMaxRawHexDigits = 16`. The width check stands in the harness
leaf-match stage ahead of the device lookup and ahead of any probe, and a
wide leaf answers the FR-005 line inside the standing three-field shape.
The counters raw arm applies the same width gate, so a wide leaf reaches
neither the probe nor the near-miss candidate list; the counters refusal
text stays unpinned, because FR-005 and FR-011 place the printed line in
the harness.

Rationale: D-2 fixes the width at the 64-bit config word, and FR-005
orders the arms explicitly: "The shape and width check stands ahead of the
device lookup: when a leaf is too wide and names an unpublished device,
the width line answers." The line shape is the existing
`origin: address: message` print at `source/harness/runner.cpp:1022-1027`,
with the origin rule at `runner.cpp:1021`; the message text is fixed by
FR-005 (`raw event code exceeds 16 hex digits`). The status 2 exit already
stands there (`runner.cpp:1027`), and the checkpoint runs after the
listings and before any measurement (`source/harness/cli.cpp:960-970`,
the order pinned at `test/source/harness_cli_test.cpp:769-775`). The hex
parse uses `std::from_chars` on the digit run; a run longer than sixteen
digits fails the width gate before any parse, so truncation cannot occur
(the parse-range discipline `source/counters/linux_pmu/encode.cpp` follows
for format widths is the model).

Alternatives considered: clamping or masking a wide word, forbidden by D-2
("the hex text is never truncated"); refusing inside the counters lookup
alone, which would let `nosuch/r<17 digits>` answer the device line first
at `runner.cpp:995`, the wrong arm per FR-005.

## R-03. One lookup carries raw recognition for every route

Decision: raw recognition joins the definition of `resolveLeafCore`
(declared `include/speedgun-ng/counters_measurement.hpp:607-608`, defined
`source/counters/system.cpp:573-590`).

Rationale: FR-007 requires the typed library route and the metric-formula
route to reach the same lookup. The typed route runs through it
(`include/speedgun-ng/counters_system.hpp:147`), the harness column build
then asks for a dimension through it
(`source/harness/runner.cpp:1487`/`1494`), and the formula route already
calls it per pmu object (`source/counters/linux_pmu/metric_formula.cpp:1290`,
after `objects("pmu")` at `metric_formula.cpp:1277`). Recognition inside
the existing exported definition adds no symbol and moves no declaration,
so the public header stays untouched (R-12). The unit the raw leaf carries
is the token `none` (`counters_system` path maps it through `unitFromToken`
and `dimensionOf`, `include/speedgun-ng/counters_core.hpp:205-206`: events
to the first power), which is what FR-007 names.

The miss arm at `system.cpp:582-589` (the `has no counter named` error with
near-miss candidates) stays the catalog-name route: the raw arm returns
before it, so near-miss candidates keep coming from seeded leaves only
(`system.cpp:582-585`), which keeps FR-015's byte-identical listings.

Alternatives considered: a separate exported raw lookup function, rejected
for the public-move cost; teaching the harness to recognize raw and bypass
the lookup, rejected because FR-007 requires the same lookup on every
route.

## R-04. The raw-entry table follows the probed-kinds precedent

Decision: one address-keyed static table beside the frozen tree, declared
in `source/counters/detail/pmu.hpp` and stored where the probed-kinds
table stands (`source/counters/system.cpp:165-177`), with the readers FR-015
names and no listing reader.

Rationale: D-5 says the raw entries sit in one table "keyed by address the
way the probed-kinds records already are". The precedent is measured: the
comment at `system.cpp:165-172` states the fill-then-read lifecycle, the
store at `system.cpp:173-177`, the writer called from the provider
enumerate (`provider.cpp:838-841`, key `device.path + "/" + entry.name`),
and the reader declarations at `pmu.hpp:278-292`. Readers for raw: the core
lookup, `compileCore`'s record read (`source/counters/plan.cpp:445-466`),
and `group_io` resolve (`source/counters/linux_pmu/group_io.cpp:178-187`).
Non-readers: `PmuProvider::enumerate` (`provider.cpp:814-842`), which
builds the only `CatalogSeed` stream the tree sees, and the two near-miss
candidate loops (`system.cpp:306-312`, `system.cpp:582-585`). The tree
itself gains no leaf, so the `--catalog` line
(`source/harness/catalog.cpp:43-60`) and the leaf listings answer as at
base. The record value takes the `LeafRecord` shape (`source/counters/detail/core.hpp:30-39`:
core, ratio pair, provider index) so the plan grouping
(`plan.cpp:505-536`, keyed on `record->providerIndex`) and the slot layer
run unchanged.

Alternatives considered: a second leaf kind inside `TreeNode`, rejected
because it moves the published listings and re-opens the frozen-tree rule;
per-provider instance state, rejected because three translation units in
two directories read it, and the address-keyed free table is the pattern
the tree already ships.

## R-05. Availability comes from the standing probe, once per address

Decision: the raw probe opens through `pmuProbe`
(`source/counters/linux_pmu/provider.cpp:667-730`) per kind on the
`probeDevice` pattern (`provider.cpp:258-285`), maps errno through the one
table (`provider.cpp:722-729`), and settles a scoped device through
`scopeSettledState` (`provider.cpp:537-557`); the verdict is stored with
the entry and never recomputed.

Rationale: FR-004 names exactly these parts. The per-kind probe pair is the
existing construction: per-task probe except on a cpumask device
(`provider.cpp:259-261`), cpu probe always (`provider.cpp:262-263`), mask
merge (`provider.cpp:269`), scope relabel only where no kind settled
(`provider.cpp:274-284`). The errno arm at `provider.cpp:722-729` is the
one table in the tree, and the comment at `provider.cpp:710-721` states
that no second table stands anywhere else; the probe runs through the
kernel seam (`provider.cpp:704-705`), so the scripted-kernel units drive
every arm. The device facts come from the same sysfs reads the loader uses
for every device: the `type` file (`provider.cpp:325-332`) and the
`cpumask` presence (`provider.cpp:341-342`), under the seam's
`devicesRoot` (`source/counters/linux_pmu/kernel_calls.hpp:80-81`, real
value `/sys/bus/event_source/devices` at
`source/counters/linux_pmu/kernel_calls.cpp:121`). That loader runs once,
in the provider constructor (`provider.cpp:767`), so a raw resolve reads
the settled device facts and opens no sysfs file of its own. FR-002's
wording "read from the kernel device tree" names that read. The state words the
line prints are the harness table `source/harness/report.cpp:32-39`
(`not-encodable`, `permission-blocked`, `scope-refused`), which the
standing line composes (`source/harness/runner.cpp:1012-1013`).

Memoization follows FR-008 and the scripted-open counts: one address
resolves to one probe pair and one table row, and one address written
twice in one run reaches one measured leaf through the spine dedup
(`include/speedgun-ng/counters_measurement.hpp:132-141`). Two spellings of
one config word, `r0043` and `r43`, stay two addresses, two table rows,
and two leaves (edge list).

Alternatives considered: probing at window open only, rejected because the
identity echo at `cli.cpp:966` runs before any open and FR-004 requires
the state in that line; host-side capability guesses, forbidden by D-3.

## R-06. Synthesis lives in provider.cpp, off-Linux stubbed, no interface move

Decision: the raw synthesis function is defined in
`source/counters/linux_pmu/provider.cpp` beside the other extracted
decisions, declared in `source/counters/detail/pmu.hpp`, and paired with
the off-Linux reduced-catalog stub in the same file's `#if !defined(__linux__)`
block (`provider.cpp:45-64`, the FR-042 rule).

Rationale: D-5 assigns synthesis to the provider ("the provider synthesizes
the entry for the one open and probe"), and the synthesis needs the
provider's own machinery: the probe, the errno table, the scope rule. The
free-function-with-side-table shape is the same one
`noteProbedKinds`/`probedKindsAt` already cross between
`provider.cpp:839` and `system.cpp:533-543`.

Reading recorded per X.1: an unpublished device part answers the standing
no-object wording, FR-002. On the CLI route that line already comes from
`System::object` (`source/counters/system.cpp:311`, printed by
`runner.cpp:997`). A device-shaped path whose object exists but whose
directory holds no PMU device (`machine/r4000` among them) reaches the
counters arm instead; there the synthesis error reuses the same wording,
`no object at path '<path>'`, so both routes print one standing line shape
and no new line joins the set. The candidate that invents a new refusal
line was rejected: FR-002 says "the standing no-object-at-path refusal".
A library-caller route keeps the runner's standing unavailable-column
behavior (`runner.cpp:1445-1455`), the rule `cli.cpp:963-964` names.

Alternatives considered: a new virtual on `ProviderIface`
(`include/speedgun-ng/counters_provider.hpp:447-461`), rejected because it
moves a public header (R-12); a down-cast from `ProviderIface` to
`PmuProvider`, rejected as a cast no contract makes sound (constitution
X.2); synthesis during `enumerate`, impossible because the addresses arrive
after the catalog freezes.

## R-07. The synthesized entry takes the device's settings from the seeded records

Decision: `hasRatioPair`, provider index, and read mode for a raw record
come from the records the provider already seeded under that device node;
the probe supplies type, config word, availability, and probed kinds.

Rationale: FR-003 requires "the same exclusion, group-read, and time-pair
settings a catalog row of that device receives". The exclusion and group
bits are not per-row at all: `fillAttr` (`group_io.cpp:204-224`) sets
`exclude_kernel`, `exclude_hv`, the group read format, and the enabled and
running time pair for every leaf of every device, so a raw entry with
`words = {{0, configWord}}` gets them by construction. The ratio-pair flag
and the provider index are seeded per record (`system.cpp:63-78`), the
read mode is one device-wide verdict (`provider.cpp:291` through
`entryReadSelectionFor`, declared `pmu.hpp:742`). Re-running the
page-bit verdict for a raw entry (`devicePageFastVerdict`,
`provider.cpp:218-243`) would add opens for a fact the seeded records
already state, so it stands rejected.

## R-08. Disclosure needs no writer change

Decision: the four FR-006 carriers print the address as typed with no code
change in the report or JSON layer.

Rationale, measured per carrier: the console column label loop prints
`result.metricLabels[column]` beside the value
(`source/harness/report.cpp:727-736`); `metricLabels` is filled from the
column label, and the column label is the address verbatim
(`runner.cpp:1437-1443`, `runner.cpp:1586-1597`); the JSON writes the same
label per row metric (`json_report.cpp:532-538`) and the label list
(`json_report.cpp:873-880`); the option-disclosure console line prints the
settled value as the source spelled it
(`source/harness/cli.cpp:106-128`), joined with `;` for the accumulating
`counter` row (`source/harness/options.cpp:2106-2119`, accumulate rule at
`options.cpp:1471-1482`), and the JSON carries the same uses at
`json_report.cpp:1118-1121`. The address text never moves: the gather
keeps items verbatim (`options.cpp:1441-1462`, `1631-1653`) and the
environment name rule is `SPEEDGUN_` plus the long name
(`options.cpp:1372-1377`). The schema document and the two readers stay:
version constant `json_report.cpp:58` at 1.16, x-orders at
`docs/schema/report-v1.json:7` (top level), `:516-541` (benchmark entry,
`metricLabels` at 531), `:620-642` (row), `:689-696` (`metrics[]`, `label`
first); the loader reads the label, the labels list, and the version back
at `source/harness/json_load.cpp:366`, `:2833-2841`, `:3422-3426` and
`:3507`. The schema checker unit runs the written document against the
file (`test/source/harness_json_schema_test.cpp`), so SC-003's schema
sentence is that gate read.

Alternatives considered: a raw marker key, rejected by D-4 (no new key, no
minor move).

## R-09. Limits and the pass plan need no change

Decision: the raw column counts against its device's programmable-counter
limit through the existing pass-plan read, and the limit line stays
byte-identical.

Rationale: the planner takes each column's leaf addresses from the spine,
splits each at the last slash (`runner.cpp:81-89` at the use inside
`runner.cpp:1610-1623`), and reads the limit of that object
(`object->programmableCounters()` at `runner.cpp:1622`, source word
through `objectLimitSource`, `counters_system.hpp:235-236`). A raw leaf's
address `cpu/r4000` splits to the object `cpu`, the device that publishes
the figure, so FR-009 is the existing rule with a raw leaf in the address.
The disclosure line print stands at `runner.cpp:1636-1639` and the scripted
fixture that pins its shape and the JSON `counterLimits` record stands at
`test/source/harness_limit_disclosure_test.cpp` (device tree at 206-219,
scripted run at 288-310, line reads at 562-583).

## R-10. The hunter keeps its contract, including its own address flow

Decision: no hunter file changes. Its refusal stays byte-identical and its
run feeds the same runner column path it feeds today.

Rationale: D-7. The measured flow: the option route refuses at
`hunt_cli.cpp:640-645` (helper `rawCodeOf` at `hunt_cli.cpp:160-172`), the
library route refuses at `hunt.cpp:729-733`, and a hunt run copies its
expansion into `runOptions.counterAddresses` at `hunt.cpp:421` and
`hunt.cpp:1049`, so any address that reaches the runner from the hunter is
catalog-shaped already. The pinned contract rows stand at
`specs/029-hunter-core/contracts/hunt-cli.md:59` and `:83`, the test pin at
`test/source/harness_hunt_cli_test.cpp:394-398`, and the guide text at
`docs/pages/harness.md:2577-2579` and the example line at
`docs/pages/harness.md:2621`.

## R-11. Host facts behind the acceptance pair

Decision: SC-001 runs `cpu/instructions` beside `cpu/rc0` on this host, and
the scripted units pin type `4` and word `0xc0`.

Rationale, measured on this host at lane start: `/proc/cpuinfo` reports
`vendor_id AuthenticAMD`, `cpu family 26`, `model 68`;
`/sys/bus/event_source/devices/cpu/type` reads `4`;
`/sys/bus/event_source/devices/cpu/events/instructions` reads
`event=0xc0`. The scripted fixture builder already stands a type-numbered
cpu device (`writeDevice(root, "cpu", "4", ...)` at
`test/source/counters_pmu_provider_test.cpp:309`, builder at 241-268 with
the `withCpumask` arm at 247, scripted open recorder at 184, scripted set
at 179-216, reset at 220-234), so the scripted pin reuses it unchanged.

## R-12. Numbers: version 0.35.0, three numbers stand, one conditional

Decision: D-6 executed as planned: `VERSION 0.35.0` at the project block
(`CMakeLists.txt:82`, the comment above it at 78-81 carries the
reservation reading), report schema 1.16 (`json_report.cpp:58`), harness
SOVERSION 22 (`CMakeLists.txt:456`), counters SOVERSION 8
(`CMakeLists.txt:172`) with its comment recording that this lane touches
no public header.

Rationale: the SOVERSION rule and its comment history stand in the counters
block (`CMakeLists.txt:133-138`, with the specs/050 header-move entry at
167-171 naming the condition); the specs/048 note at 453-455 is the
precedent for a comment that records a kept number. Every symbol this lane
needs is already declared where it is used: `resolveLeafCore` at
`counters_measurement.hpp:607-608`, `LeafSet` and `open` at
`counters_provider.hpp:147-155` and `:460-461`, the harness side behind
private headers (`source/harness/detail/internal.hpp:640`, `:1579-1590`).
If implementation later moves a public header, the same block's rule takes
the number to 9 with the comment, and FR-012's readings record that case;
the planned change does not. The version string reaches the binary through
the `SG_PROJECT_VERSION` compile definition (`CMakeLists.txt:361-365`,
written into the report at `json_report.cpp:1262`, printed by
`printVersion`, `source/harness/detail/options.hpp:462`).

## R-13. Docs carriers and the page cap

Decision: FR-010's one change updates three texts and touches no other
line: the option row description (`options.cpp:560-565`), the guide option
table row (`docs/pages/harness.md:675`, the same sentence today), and the
guide counter section (`docs/pages/harness.md:637-651`), plus the README
quick row (`README.md:970`) in place.

Rationale: the drift gate compares README option rows to the option table
by name (`tools/docs/example_drift.py:798-802`, table read at 567-581), so
the description text is prose the author writes well while the gate
matches the option name; the quick-reference row rule splits
`Owner::member` rows, and this feature adds none. The page cap is `PAGE_LINE_CAP = 1004`
(`tools/docs/example_drift.py:256`); the page measures 1003 lines, so the
row stays one line and the cap stands. The guide's hunt refusal prose
(:2577-2579) and the hunter's own section stay as they are; the counter
section gains the raw sentence and one pointer to the standing hunter
refusal (D-7, FR-010).

## R-14. Gate reads, measured at this head

Decision: the lane runs the full gate set of FR-013 with no CI edit.

Measurements at head: the marker read
`git grep -c LCOV_EXCL -- source include` sums 84; the coverage summary
gate stands at
`tools/dbc/coverage_gate.sh` (100 percent line arm at 51-53, the recorded
branch allowance `allowance=21` at 108); the trace command runs lcov with
`--ignore-errors mismatch,negative` and nothing else
(`cmake/coverage.cmake:68-81`, line 73), the `inconsistent` class comment
at 47-59 records why it stands gone, and the extract and summary arms
stand at `coverage.cmake:111-135`. The sanitizer and thread-sanitizer
presets exist (`CMakePresets.json` names `ci-sanitize` at 121 and `ci-tsan`
at 130), the release presets `ci-ubuntu` and `ci-rocky` at 157 and 168, and
the checked-in daily loop is the one `AGENTS.md` states. Test registration
patterns: counters unit `test/CMakeLists.txt:553-559`, harness unit with
the source-harness include `test/CMakeLists.txt:1837-1841`.

## R-15. Performance: measured at resolve time, exempt where the loop never moves

Decision: two measurements and one recorded exemption.

Measurement one, resolve path: the scripted fixture records every
`perf_event_open` call and its config word
(`test/source/counters_pmu_provider_test.cpp:184`), so the pin per raw
address is one probe pair and one table insert, and zero opens for a
second resolve of one address and for a wide leaf. The `type` read and the
`cpumask` presence stand at device load (`provider.cpp:325-342`, run once
by the constructor at `provider.cpp:767`), and the raw arm reads the
loaded device record, so a raw address joins no sysfs read.
Measurement two, report path: the diff read. `git diff --name-only` must
show no change to `report.cpp`, `json_report.cpp`, `catalog.cpp`, or any
file under `example/`, and the timed loop carries no new instruction
because the run-side code path is the existing fold of a spine over a
window. The exemption is recorded: the work is resolve-time, bounded, and
inside the untimed checkpoint (`cli.cpp:960-970`); no designated
critical-path code changes, and Principle VII's baseline infrastructure
stands under the constitution's open deferral, so no new baseline joins
this lane. A raw column's measured values print through the same column
machinery a catalog column uses, so SC-001's two-column run doubles as the
read that no report-path figure moved.

## R-16. Existing behaviors the lane must not disturb, with their pins

- Base suite unchanged: the pinned counter refusal lines at
  `test/source/harness_cli_test.cpp:729-755` and the listing-before-walk
  order at 769-775 stay byte-identical (SC-004).
- Hunter refusal text: byte-identical pin at
  `test/source/harness_hunt_cli_test.cpp:394-398`; the library-route line
  contract at `hunt.cpp:729-733` untouched (FR-007, US4-2).
- Counter-limit line: byte-shape pin pattern at
  `test/source/harness_limit_disclosure_test.cpp:562-583` (FR-009).
- Catalog lines: the print at `catalog.cpp:43-60` untouched; the
  `--counter=/no/such/leaf --list` order pin at
  `harness_cli_test.cpp:769-775` (FR-015).
- Schema document: no edit; the checker at
  `test/source/harness_json_schema_test.cpp` runs the written document
  against `docs/schema/report-v1.json` at 1.16 (SC-003, SC-006).
- No test sleeps, reads a clock, names a made-up cpu, or needs a made-up
  kernel figure (FR-011): every scripted scenario runs on the seam
  (`kernel_calls.hpp:59-109`, install at `:109`), and the forked
  provider-script pattern stands at `test/source/harness_gap_test.cpp:1-12`.
