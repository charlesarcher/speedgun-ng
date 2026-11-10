# Feature Specification: Raw Event-Code Support (G-31)

**Feature Branch**: `051-raw-events`

**Created**: 2026-11-10

**Status**: Draft

**Input**: User description: "Add raw event-code support: PMU selection,
validation, disclosure, and tests. Catalog-only behavior remains the
default until the user requests raw." Owner rulings G-31 and G-39;
sequenced by G-40 behind the specs/049 text lane.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A user counts one raw event code as a report column (Priority: P1)

A user who knows the config word of an event on a device this host
publishes writes `--counter=cpu/r<hex>` (the perf raw spelling: a
lower-case `r` then hex digits, the config word holding at most
sixteen). The run adds one report
column, labeled with the address exactly as typed, counting that config
word on the named device. The hex text becomes the perf config word
with no reinterpretation; the device's own PMU type number addresses
the device. On this AMD host (vendor `AuthenticAMD`, family 26, model
68) the raw spelling of one event that the host itself publishes under
the core device's event aliases counts the same config word as the
named alias column in the same run.

**Why this priority**: this is the ruling's product: G-31 says users can
pass raw codes through a clear, validated path. Everything else guards
that door.

**Independent Test**: one run with two columns, the host alias name and
its raw spelling, for example `cpu/instructions` beside
`cpu/r<alias word read from this host>`; both columns count positive
values of the same word and the record carries both figures. A
scripted-kernel unit proves the same route on any host: a scripted
device with a known type, one scripted open, and the recorded
attribute words.

**Acceptance Scenarios**:

1. **Given** a host that publishes device `cpu` with PMU type `T`,
   **When** a run carries `--counter=cpu/r4000`, **Then** one column
   labeled `cpu/r4000` counts the config word `0x4000` on type `T`,
   with the same exclusion and group settings a catalog row carries.
2. **Given** this host, **When** the raw spelling of one of the host's
   own core-device alias events runs beside the alias's name in one
   run, **Then** both columns print positive counts of the same
   config word, both figures land in the record, and the recorded
   acceptance names the host's family and model.
3. **Given** the same raw address written twice in one command line,
   **When** the run resolves the columns, **Then** two columns stand
   with equal labels over one shared measured leaf, the way a
   duplicated catalog address behaves.

---

### User Story 2 - A bad raw code is refused with one clear line (Priority: P1)

A malformed or unsafe raw code never reaches measurement. An unknown
device answers the standing no-object-at-path line. A raw-shaped address
whose hex part is wider than sixteen digits answers a new line of the
standing shape, naming the width, at the leaf-match stage. A code the
kernel refuses answers the standing listed-as line with the state the
standing probe gives: NOT_ENCODABLE for a
kernel refusal that the encoding itself caused and PERMISSION_BLOCKED
otherwise. A run carrying a refused address ends with the standing
status, exactly as a refused catalog address does today.

**Why this priority**: an unchecked config word turns a benchmark run
into a kernel guess. The refusal is the validation half of G-31, and
every refusal is a testable line.

**Independent Test**: scripted-kernel units drive each refusal: no
matching device directory, a seventeen-digit hex leaf, an `EINVAL` from
the scripted open, and a permission errno. Each answers its one named
line, and the measurement never starts.

**Acceptance Scenarios**:

1. **Given** a device the host does not publish, **When** a run names
   `nosuch/r4000`, **Then** the run answers the standing
   no-object-at-path line naming that address, and the exit status
   stands at 2.
2. **Given** a raw-shaped leaf with seventeen hex digits, **When** the
   run resolves addresses, **Then** one line names the raw code width,
   and no measurement window opens.
3. **Given** a code the kernel refuses while opening, **When** the
   probe verdict is read, **Then** the column state is the state the
   errno table gives, and the run answers the standing listed-as line
   naming that state.

---

### User Story 3 - Raw use is disclosed where counters are disclosed (Priority: P2)

A reader of the console or of the JSON report sees the raw address
wherever a counter column identity stands today: the option-disclosure
line for options taken from a file or the environment, the console
column label, the JSON `metricLabels` list, and each row's
`metrics[].label`. No new JSON key
stands, so the report schema keeps its 1.16 minor and its key order.

**Why this priority**: G-31 requires disclosure by the existing report
rules. The address as typed already names the raw spelling, so the
existing carriers do the work and no reader-visible layout moves.

**Independent Test**: a scripted-kernel harness run prints the console
line and the JSON document; both carry the raw address, and the schema
check against `docs/schema/report-v1.json` passes at 1.16.

**Acceptance Scenarios**:

1. **Given** a run with one raw column, **When** the console report
   prints, **Then** the row carries the raw column with its address as
   the label.
2. **Given** the same run with a JSON report, **When** the document is
   read back, **Then** `metricLabels` and each row's `metrics[].label`
   name the raw address, the schema version stands at `1.16`, and the
   document carries no key the schema does not list.

---

### User Story 4 - Catalog-only behavior stays the default (Priority: P2)

A leaf that is not a raw-shaped address resolves exactly as at base:
catalog tables, kernel aliases, vendored rows, near-miss suggestions,
and the standing refusals. `--hunt` keeps refusing raw codes with its
standing line, because the hunter selects over the catalog. The typed
library route keeps its catalog contract for names that are not
raw-shaped.

**Why this priority**: the ruling names catalog-only as the default
until the user requests raw, and the existing hunter rule (specs/029
FR-005) is not disturbed by this feature.

**Independent Test**: the base suite passes with no changed expectation
in any existing scenario; the hunter raw-code refusal text stays
byte-for-byte the pinned contract.

**Acceptance Scenarios**:

1. **Given** every catalog scenario at base, **When** the suite runs at
   the lane head, **Then** every existing counter scenario answers what
   it answered at base.
2. **Given** `--hunt=cpu/r4000`, **When** the hunter CLI reads the
   selection, **Then** the pinned raw-code refusal line prints, exactly
   as specs/029 contracts it.

---

### Edge Cases

- A leaf shaped like raw but spelled with an upper-case `R`, or with a
  non-hex character after the `r`, stays on the catalog-name route:
  the standing refusal lines, the same as at base.
- The same config word written under two spellings (`r0043` and `r43`)
  names two distinct addresses and two distinct columns: identity is
  the address string, the standing rule of the leaf model.
- A raw code naming a device the host scopes by cpumask: a permission
  refusal there relabels to the scope's own verdict and the run
  answers `listed as scope-refused (FR-023)` at the standing status.
  A scripted unit carries this arm; this host's cpumask devices
  publish not-encodable codes, so the unit supplies the permission
  refusal.
- A raw code on a host with no usable PMU answers the standing
  availability lines; the lane's scripted units carry the behavior
  where the host cannot.
- A run carrying one refused raw address and several good addresses:
  the refusal stands and the run ends before measurement, the standing
  rule for refused addresses.
- A seventeen-digit leaf is raw-shaped and answers the width line; it
  never reaches the catalog-name route or the no-such-leaf line.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: When a counter address's leaf is a lower-case `r`
  followed by one or more hex digits of either case, the exact shape
  the pinned hunter contract recognizes, the counters core lookup
  shall treat the leaf as a raw event code and take its hex text as a
  64-bit config word, read as hexadecimal.
- **FR-002**: The provider shall address a raw code through the device
  part of the address, using that device's own PMU type number read
  from the kernel device tree; a device the host does not publish shall
  answer the standing no-object-at-path refusal naming the device.
- **FR-003**: The provider shall build the event attribute for a raw
  code from the device type and the config word, with the same
  exclusion, group-read, and time-pair settings a catalog row of that
  device receives.
- **FR-004**: The availability of a raw code shall come from the state
  the standing probe publishes for that attribute on that device: an
  open that succeeds stands COUNTABLE; a kernel refusal maps through
  the one errno table to NOT_ENCODABLE or PERMISSION_BLOCKED; the
  standing scope rule relabels a PERMISSION_BLOCKED on a
  device-scoped device to SCOPE_REFUSED, an encoding refusal keeping
  its own state; a refused state answers the standing listed-as line
  naming the state.
- **FR-005**: A raw-shaped address whose hex part is wider than sixteen
  digits shall stay raw-shaped and be refused at the leaf-match stage,
  ahead of any probe for that leaf, with one line in the standing
  origin, address, and message shape whose message reads
  `raw event code exceeds 16 hex digits`; it shall not reach the
  catalog-name route, and the run shall end with the standing
  refused-address status. The shape and width check stands ahead of
  the device lookup: when a leaf is too wide and names an unpublished
  device, the width line answers.
- **FR-006**: The raw address as the user typed it shall appear in the
  option-disclosure line when the option came from a file or the
  environment, in the console column label, in the JSON `metricLabels`
  entry, and in every row's `metrics[].label`; the report schema shall
  gain no key and shall keep the 1.16 minor.
- **FR-007**: Addresses whose leaf is not raw-shaped shall resolve as
  at base through catalog tables, kernel aliases, and vendored rows;
  `--hunt` shall keep its pinned raw-code refusal; the typed library
  counter resolution and a metric formula's event names shall resolve
  raw-shaped leaves through the same core lookup, to the unit `none`
  and its events^1 dimension, and shall keep catalog behavior for
  names that are not raw-shaped.
- **FR-015**: The harness sites that name a counter leaf and build its
  column shall consult the raw-aware core lookup for a raw-shaped
  leaf: the identity echo answers the standing listed-as line for a
  refused raw state, the column build takes the unit `none` and its
  events^1 dimension,
  and the no-such-leaf line stays reserved for names that are not
  raw-shaped. Every standing line stays byte-for-byte. The counters
  layer shall keep the raw entries in one raw-entry table beside the
  frozen tree, read by the core lookup, by the plan's leaf-record read,
  and by the measurement-window resolve, and never by the published leaf
  listings: the tree itself gains no
  leaf, the compiled plan gains the raw leaf's record, and the
  near-miss and `--catalog` listings answer as at base.
- **FR-008**: A raw address written more than once in one run shall
  print one column per occurrence over one shared measured leaf, the
  standing duplicate behavior of catalog addresses.
- **FR-009**: A raw column shall count against its device's
  programmable-counter limits the way a catalog column counts, and the
  standing counter-limit disclosure line shall answer unchanged.
- **FR-010**: The counter option description, its row in the README
  quick table, and the guide's counter section shall state the new
  acceptance and the standing hunter refusal in the same change; the
  README edit fits in place under the drift tool's page-line cap, and
  the drift reads stay clean.
- **FR-011**: Tests shall prove: one accept through the kernel seam
  with the recorded attribute naming type and config word; one unknown
  device; the seventeen-digit width refusal at the leaf-match stage;
  one probe refusal per errno class; the duplicate address printing
  two columns over one shared leaf; the typed route resolving a raw
  leaf to the unit `none` and its events^1 dimension; a metric formula
  resolving a raw event
  name; all four disclosure carriers of a raw column; a scripted plan
  that compiles and measures a raw leaf through the raw-entry table;
  the `--catalog` and near-miss listings answering byte-identical;
  the scripted attribute carrying the same exclusion, group-read, and
  time-pair settings as a catalog row of that device; the raw column
  counted against its device's limit with the standing counter-limit
  line byte-identical; a scripted device-scoped device whose open
  answers a permission errno, printing
  `listed as scope-refused (FR-023)` at the standing status; the
  width line winning when a too-wide leaf also names an unpublished
  device; and the byte-identical hunter refusal.
  No test sleeps, reads a clock, names a made-up cpu, or needs a
  made-up kernel figure.
- **FR-012**: The version shall take the next free minor past 0.34.0
  under the reservation rule (specs/029 FR-090); the report schema and
  the harness SOVERSION shall keep their numbers; and the counters
  SOVERSION shall keep 8 or take 9 with the public-header move named as
  the reason; each check shall be recorded with its reading.
- **FR-013**: Local acceptance at the squashed head on this host shall
  carry: the full suite, the coverage gate (100 percent lines, branch
  shortfall at most the standing allowance of 21, at most 84 markers,
  no new `LCOV_EXCL`, and no suppression class beyond the tracked
  command's `mismatch,negative`, the `inconsistent` class staying
  forbidden), the sanitizer and thread-sanitizer presets, the
  clang-tidy and cppcheck release build, and the two-column raw run
  of User Story 1 recorded with this host's CPU family and model
  (G-20, G-25, G-39).
- **FR-014**: The lane shall keep one branch, one squashed commit, one
  pull request with the template and the OmO identity, the push rules
  of the shared binding, and shall never merge and never close an
  issue.

### Key Entities

- **Raw event code address**: a counter address whose leaf spells `r`
  then hex digits, the hunter contract's shape; the device part
  selects the PMU, and a hex part beyond sixteen digits stands
  refused under FR-005.
- **Synthetic raw entry**: the provider-side entry for one raw address:
  the name as typed, the config word, the probe-read availability, and
  the device's settings; it enters no catalog listing.
- **Probe verdict**: the standing outcome of opening one event
  attribute, the state the standing probe publishes from the one
  errno table and the scope rule.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On this host one run carries the raw spelling of a host
  alias event beside the alias name; both columns print positive
  counts, both figures land in the record, and the record names
  family 26 (0x1A), model 68, and the alias event with the words
  read (this host's core `instructions` alias answers event `0xc0`,
  so the standing pair is `cpu/instructions` beside `cpu/rc0`). The
  scripted unit pins the exact PMU type number and config word.
- **SC-002**: The four refusal inputs (unknown device, seventeen-digit
  hex, encoding errno, permission errno) each answer exactly one named
  line with no measurement window; scripted units pin all four.
- **SC-003**: The raw address appears in all four disclosure carriers
  (the option line of a run whose option came from a file or the
  environment, the console label, `metricLabels`, and row
  `metrics[].label`) in the scripted disclosure scenario, and the
  schema document is unchanged.
- **SC-004**: The base suite passes unchanged; the hunter raw-code
  refusal text is byte-identical to the specs/029 contract.
- **SC-005**: The coverage read answers 100.0 percent lines, a branch
  shortfall of at most 21 rows, and at most 84 markers with no new
  marker.
- **SC-006**: The head answers version 0.35.0 (the next free minor at
  lane start), schema `1.16`, harness SOVERSION 22, and counters
  SOVERSION 8 with the no-public-header condition named, or 9 with
  the header it moved; each figure sits in the results record.

## Assumptions

- The raw spelling is perf's own: lower-case `r` plus hex digits, the
  same shape the hunter route already tests and refuses (specs/029
  FR-005). Uppercase leading `R` stays a catalog name.
- One raw code carries one config word (the perf raw spelling's
  meaning). Fields that would need config1 or config2 stay expressible
  through catalog rows, which the table parser already composes.
- The host publishes the core PMU under `/sys/bus/event_source/` with a
  readable type number; hosts without it answer the standing
  availability lines.
- The reservation rule of specs/029 FR-090 (the next free minor at
  rebase, with a recorded reason) is live at lane start; 0.35.0 is free
  at the specs/049 tip.
- The lane starts from the specs/049 squashed head after that lane's
  single-commit squash, and this lane's pull request stacks on the
  open 049 pull request until the owner lands it: the owner's
  sequencing (G-40) keeps 051 based on the 049 commit, and the
  stacked view of both commits in the pull request is the directed
  arrangement.

## Decisions

The feature description recommends one answer per open question, and
this spec takes each one:

- **D-1 (route)**: raw codes enter through the counter-address route:
  `--counter`, its environment twin, and `@address-list` files, which
  all feed the same address list, and through the typed library
  resolution, which reaches the same counters core lookup and yields
  a counter of the unit `none`, hence the events^1 dimension
  (FR-007). The hunter selection keeps
  its catalog contract: it selects over catalog candidates for
  ranking, and specs/029 FR-005 stands.
- **D-2 (width)**: sixteen hex digits is the 64-bit config word. A
  wider leaf is refused as too wide by the FR-005 line; the hex text
  is never truncated.
- **D-3 (safety)**: the kernel is the arbiter of unsafe. The standing
  probe opens the attribute before any measurement, and the one errno
  table maps refusals. No invented host checks join the path.
- **D-4 (disclosure)**: the address-as-typed is the disclosure. No new
  JSON key stands, so the schema minor stays 1.16; the two report
  readers, the loader and the schema check, see no new shape.
- **D-5 (synthetic entry)**: the counters core lookup recognizes the
  raw shape in place, and the provider synthesizes the entry for the
  one open and probe. The raw entries sit in one raw-entry table
  beside the frozen tree, keyed by address the way the probed-kinds
  records already are, and that table never adds a leaf to the tree.
  The plan's leaf-record read and the measurement path read the
  raw-entry table, the published listings never do (FR-015), and the
  tree itself gains no leaf. `--catalog`, the near-miss lists, and the published
  leaf views answer as at base.
- **D-6 (numbers)**: version 0.35.0; schema 1.16; harness SOVERSION
  22. The counters SOVERSION stays 8 while the lane touches no public
  header and takes 9 when the lane moves one, the rule recorded in
  `CMakeLists.txt` since specs/050; the lane aims at the first case,
  and FR-012's readings record which one landed.
- **D-7 (hunt)**: the hunter raw-code refusal stays byte-identical;
  this feature owns no hunter surface change.
- **D-8 (main page)**: the change is a value-grammar change on an
  existing option row. The main-page treatment is the updated option
  row and prose in `README.md` and the guide, per IV.1, with no new
  C++ example region: the public API surface does not move, and an
  example built on host-specific config words could not run on the CI
  matrix.
- **D-9 (G-32)**: automatic rank selection stays future work and out of
  this lane.
- **D-10 (hybrid rows)**: this host is AMD Zen 5. No Intel hybrid
  acceptance is claimed (G-38), and no other machine is named (G-24).

## Out of Scope

- `--hunt` and `State::setCounter`: their contracts stand unchanged.
  Metric formulas and the typed library resolution join the one
  raw-aware lookup (FR-007).
- New report keys, schema moves, public header moves, CI workflow
  edits, and README rewrites beyond the option-row and counter-prose
  updates of FR-010.
- Automatic rank selection (G-32) and hybrid-host acceptance (G-38).

## Constitution Checklist

- Main-page impact check (IV.1): yes, user-visible behavior changes on
  an existing option. The treatment is the updated option row and
  counter prose in the main page and the guide, with no new example
  region, for the reason D-8 states.
