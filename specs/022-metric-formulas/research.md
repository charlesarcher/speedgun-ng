# Research: Metric Formulas

**Feature**: `022-metric-formulas` | **Spec**: [spec.md](./spec.md)

**Audit point**: `ab32400b86321f98f13b9c66d4488e15831b47f6`

Fourteen records follow. Each carries the decision, what drives it, the
alternatives and why they are rejected, and the consequences the plan and
`tasks.md` have to honor. Counts come from the inventory read in the spec,
measured over `external/pmu-events/arch/x86` at the audit point.

## D-01: A closed grammar over the constructs the pinned tree uses

**Decision**: The parser accepts one closed list: decimal, hexadecimal and
exponent literals, unary minus, `+`, `-`, `*`, `/`, parentheses, an event
reference with optional `@modifier@` and an optional privilege suffix, with
backslash escapes decoded at the scan including the escaped comma inside a
modifier, a reference to another metric by name, the conditional
`<value> if <test> else <value>` form with `<` and `>` comparisons, the
five functions FR-004 names (`min`, `max`, `d_ratio`, `has_event`,
`source_count`), and a `#`-prefixed literal constant. Anything else refuses
the row at load with a reason that names the construct it met first.

**Drivers**: 5958 rows carry an expression, 267 carry the conditional form,
275 carry a literal constant, 250 use `max`, 190 use `min`, 74 use `d_ratio`,
35 use `has_event` and 31 use `source_count`. Measured over the tree, every
call of the five functions takes exactly two arguments: `min` is 176
runtime-and-runtime plus 14 runtime-and-constant, `max` is 129
constant-and-runtime plus 96 runtime-and-constant plus 25
runtime-and-runtime, and `d_ratio` is 74 runtime-and-runtime. A modifier
with an escaped comma, as in `cpu_core@DTLB_LOAD_MISSES.STLB_HIT\,cmask\=1@`,
holds one argument, so the scan must treat an escaped comma as text. The tree
uses no other operator, and the comparisons are only `<` and `>`. A closed
list is what makes the coverage obligation of Principle VI satisfiable: an
open grammar has branches no fixture reaches.

**Alternatives considered**: a shim compatible with the upstream `jevents`
grammar, rejected because upstream accepts constructs the pinned tree does
not use, and every one of them would be an untested branch. A recursive
descent with an expression library, rejected because the tree needs nine
constructs and an external dependency would widen the purity scans for
nothing. An error-recovering parser that keeps the parseable part of a bad
row, rejected because a half-applied formula prints a plausible wrong
number, which is worse than a missing metric.

**Consequences**: every construct in the list needs a fixture and every
refusal needs one too. A re-pin that introduces a construct shows up as a
failing sweep test naming the row (FR-038), which is the designed
behaviour. The refusal reason is a one-line string built from the construct
name and the offset.

## D-02: Parsing runs once per directory at load, inside the loader

**Decision**: The metric rows of a directory are parsed when that
directory's table image is built, once. The parsed form and the refusal
record live in the image. A later request reuses the image and performs no
parsing.

**Drivers**: FR-019 forbids per-read parsing. The loader already caches the
per-directory image, so the parsed form has a home with the right lifetime.
`addEntry` is the single place a row becomes an entry, so the metric branch
belongs beside the counter branch it sits next to.

**Alternatives considered**: parse on first lookup, rejected because the
catalog needs the whole directory parsed anyway and a lookup order would
change which rows are parsed; parse at build time and embed a compiled
form, rejected because it widens the build inputs, makes the embedded blob
harder to review, and moves the refusal report away from where the reader
sees it.

**Consequences**: load does more work, bounded by D-14. The image grows by
the parsed spines and the refusal records, which is a one-time memory cost
per directory used. The refusal records must be reachable from the
catalog path, so the image keeps them beside the entries.

## D-03: Two passes with a colored depth-first resolution

**Decision**: Pass one parses every row into a spine whose metric
references are placeholder leaves holding a name. Pass two resolves each
placeholder against the directory's name index, splicing the referenced
metric's spine in place, and walks the reference graph with white, gray and
black marks. A back edge to a gray node is a cycle.

**Drivers**: 2973 rows reference another metric of their own directory, so
reference resolution is the common case and the expensive one. Splicing is
already the operation `splice` performs on `ExprCore`, so a resolved
reference costs the same as a hand-composed composition. The color walk is
the standard linear test and it names every member of the cycle rather than
one.

**Alternatives considered**: resolve during pass one with a lazy recursive
parse, rejected because a cycle would recurse without a bound and the
recursion depth would depend on table text; a topological sort with Kahn's
counting, rejected because it identifies the cyclic set as a residue and
loses the path that made the cycle, which the spec asks the message to
name; a limit on reference depth in place of cycle detection, rejected
because a deep acyclic chain would be refused while a cycle of the same
depth would pass.

**Consequences**: the name index must exist before resolution, so
resolution is a stage after every row is parsed. A metric that resolves
into a leaf-only spine of the supported node kinds needs no further work,
because the result is an ordinary `ExprCore`.

## D-04: A cycle refuses every member and names the cycle

**Decision**: When the walk finds a cycle, every metric on it is refused at
load with the kind `CYCLE` and a one-line reason that lists the names in
the order the walk met them. The rest of the directory loads.

**Drivers**: The spec requires a cycle to refuse every metric it contains
and to name the cycle. A refusal per member keeps the catalog account
exact: rows plus refusals equal rows.

**Alternatives considered**: refuse only the outermost metric, rejected
because the inner one stays listed as countable and a caller who asks for
it gets the same cycle by another door; resolve with a fixed-point
iteration, rejected because the vendored cycles are true cycles with no
base case, so a fixed point does not exist.

**Consequences**: the refusal record needs a reason string that can hold a
name list, and the catalog prints it in the same column a counter refusal
reason uses.

## D-05: One pinned `ScaleUnit` conversion table

**Decision**: One table maps a `ScaleUnit` spelling to a factor and the
unit text to print. It covers every distinct spelling of the selected
directory. An unlisted spelling refuses the metric at load and names the
text.

**Drivers**: 3222 rows carry the field and the frequent spellings are
`100%` (2579), `1per_instr` (174), `1MB/s` (163), `1e3per_1k_instr` (58) and
`64Bytes` (54). A prefix means the number: the same formula printed with
and without its factor differs by orders of magnitude, which is the kind of
error a reader cannot spot. `MetricResult` has no unit field and FR-018
keeps it that way, so the factor is applied before the value reaches the
result and the text travels beside it.

**Alternatives considered**: port the upstream `jevents` scaling decoder,
rejected because it interprets a wider grammar than the pinned tree uses and
its unreachable arms fail Principle VI; apply no factor and print the raw
ratio, rejected because a residency metric would print a fraction where the
table promises a percentage; guess from the text's shape, rejected because a
wrong guess is silent.

**Consequences**: the table is data, so the sweep test asserts every
spelling present in the tree resolves, and a re-pin that adds a spelling
fails the test before any wrong number prints. The unit text is stored
verbatim, so a reader sees the table's spelling.

## D-06: Literal constants read host facts the library already publishes

**Decision**: A `#`-prefixed constant resolves from a small fact table
whose readers are host facts the library already obtains: online processor
count, core count, package count, and the calibrated reference frequency.
A constant with no reader refuses the metric and names the constant.

**Drivers**: 275 rows use the mechanism, led by `#SMT_on` (148),
`#num_packages` (65), `#has_pmem` (33), `#num_dies` (23),
`#num_cpus_online` (23), `#core_wide` (12), `#num_cores` (12) and
`#SYSTEM_TSC_FREQ` (11). The clarification of 2026-10-19 forbids guessing.
Four of the eight already have a reader in the object tree or the
calibration path, so the mechanism is useful on this host without a new
source of truth.

**Alternatives considered**: read DMI or SMBIOS for `#has_pmem` and
`#num_dies`, rejected because it adds a privileged filesystem read to the
counters library for two constants; hard-code a per-directory constant
table, rejected because it reports another machine's topology as this
one's; treat an unknown constant as zero, rejected because a zero silently
changes a formula's value.

**Consequences**: `#has_pmem`, `#num_dies` and `#core_wide` refuse on this
host until a real reader exists, and the catalog says so with the
constant's name. `#SMT_on` compares the thread count against the core count,
which the fake provider can drive both ways, so both branches of every
conditional that reads it are covered.

## D-07: Functions and conditionals fold onto the existing kinds at load

**Decision**: `<value> if <test> else <value>` selects one arm once, at
load, from the host facts and the table image. `has_event(name)` answers
against the set of events the host can encode. `source_count(name)` counts
table sources that define the event name and answers zero for a name no
source defines. `min(a, b)` and `max(a, b)` become one node of the new kind
D-09 appends. `d_ratio(x, y)` lowers to `x / max(y, 1.0)`, so the
zero-denominator case yields the numerator's scale and no division by zero
reaches a fold.

**Drivers**: All three inputs are known before measurement: the host facts
come from the object tree, the encode set from selection, and the source
list from the image. Folding at load keeps the compiled spine free of
decision nodes, so `compileCore` sees the node kinds it already handles. It
also gives 267 conditional rows one branch each, which is what makes their
coverage reachable. `min` and `max` need a runtime comparison, because 176
and 25 of their calls carry two runtime arguments, so they cannot fold to a
constant. The 74 `d_ratio` calls always carry a runtime denominator, and
upstream defines the call as a quotient that survives a zero denominator.

**Alternatives considered**: evaluate the conditional per read, rejected
because the host facts do not change during a run and a decision node would
widen the compiled plan and the fold; treat a missing `else` as zero,
rejected for the same reason D-06 rejects a guessed constant; drop
`source_count` from the grammar, rejected because 31 rows use it and they
would then refuse with a reason no reader can act on.

**Consequences**: the fold happens after resolution, so a conditional whose
test names a refused metric refuses the outer row with the inner refusal
named. Each helper needs a fixture per outcome, and the two-branch
conditional is covered by a fake host on each side of its core count. The
`d_ratio` lowering reads the denominator as a count, where a value between
zero and one cannot occur, so the guard matches the upstream rule for every
row of the pinned tree; the limit is written beside the lowering.

## D-08: One token kind for an event reference, escape decoded at scan

**Decision**: An event reference is one token carrying the event name, the
optional `@modifier@` text and an optional privilege suffix, with backslash
escapes decoded during the scan. The token is produced as written in the
table, so `cstate_pkg@c6\-residency@` reaches the encoder as
`cstate_pkg@c6-residency@`.

**Drivers**: 707 rows carry a backslash escape inside the modifier text and
88 carry a privilege suffix. The escape exists because the upstream
converter reserves characters, so the decoded form is the one the encoder
already accepts. 19 of the 279 `skylakex` rows depend on it.

**Alternatives considered**: pass the raw text to the encoder, rejected
because the encoder receives a backslash it was never given a rule for;
strip the modifier and use the bare event name, rejected because the
modifier selects the sub-event and dropping it changes what is counted;
support the full upstream escape set, rejected because the tree uses hyphen
and a short list, and untested arms cost coverage.

**Consequences**: the escape table is a closed list with one fixture per
escaped character, and the privilege suffix is kept as text the encoder
already understands so no new availability kind is needed.

## D-09: The compiled metric reuses `ExprCore` leaves and the existing fold

**Decision**: A resolved metric is an `ExprCore` whose leaves are the same
counter leaves a hand-composed expression builds. It composes through
`scaleAll` for the `ScaleUnit` factor and `splice` for references, then
travels through `foldPairsCore`, `metricCore` and `compileCore`. The spine
gains two appended kind codes, 5 for the minimum node and 6 for the maximum
node, so `min` and `max` of two runtime values are representable.

**Drivers**: The machinery exists at the audit point with the node kinds a
formula needs, plus scale. Reusing it means a metric value and a hand-built
value over identical counts agree by construction, which is the check the
spec sets for the harness column. It also keeps the public surface small:
the metric feature adds a producer of spines, and no new consumer. Two
runtime arguments in 201 of the `min` and `max` calls is the one construct
the five existing kinds cannot carry, so the spine grows two kind codes.

**Alternatives considered**: a parallel metric evaluator with its own fold,
rejected as a second source of numeric truth and a second coverage surface;
a new node kind for a metric reference that survives to compile time,
rejected because `splice` already resolves references at composition time
and a surviving reference node would widen the plan format.

**Consequences**: no change to `MetricResult`, to `Availability` or to any
public signature, which is the whole basis for keeping both `SOVERSION`
values. The two appended codes are additive inside `detail::ExprCore`, in
the comment that lists the kind codes: an expression built by any existing
caller cannot carry them, so every existing fold and plan behaves exactly as
before. `min` and `max` reach the fold as one arm each, which is two new
branches the coverage plan must drive, and the dimension rules of
`Expression<D>` apply to metrics as they apply to composed counters, so a
ratio of like quantities stays the common shape.

## D-10: One public header, one lookup, refusal by value

**Decision**: `include/speedgun-ng/counters_metrics.hpp` declares a lookup
by name that returns either a metric view or a recoverable refusal, and the
view carries the expression, the group text and the unit text. A load-time
refusal carries a `MetricRefusal` kind from the closed set in
`data-model.md`. A metric that loaded and cannot be counted carries no
refusal kind: it carries one of the existing `Availability` values.

**Drivers**: FR-015 and FR-016 ask for a name in and a foldable expression
out, with a one-line refusal carrying near-miss names, on the pattern of the
object lookup that already answers that shape. A view that carries the
group and unit text keeps `MetricResult` untouched, which D-09 depends on.

**Alternatives considered**: an overload on the existing object lookup,
rejected because a metric is a different kind of thing and an overload would
hide which lookup failed in one shared error; a parsed-formula entry point,
rejected because the spec's out-of-scope list keeps a public formula string
out of this feature and a name is what a reader has; throwing on a miss,
rejected because the harness must print one line and continue for a metric
that exists but cannot be counted.

**Consequences**: one new exported symbol group in the counters archive,
which the header-purity and nm gates observe. The near-miss list reuses the
existing edit-distance-style helper the object lookup uses, so no second
suggester appears.

## D-11: The catalog prints one line per metric row, refused rows included

**Decision**: `printObject` gains metric lines after the counter lines of
the object path, each carrying the metric name, the group text, the unit
text and either an availability value or a refusal reason. Rows refused at
load are printed with their reason.

**Drivers**: The spec requires the listing to account for every metric row
of the selected directory, which makes the listing the audit trail for D-01
refusals, D-04 cycles, D-05 unknown scales and D-06 unavailable constants.
The counter line format already carries `description`, `unit`, `mode`,
`avail` and `refusal`, so a metric line needs no new field.

**Alternatives considered**: print only countable metrics, rejected because
the refusal report the spec asks for would have no surface; a separate
`--metrics` listing, rejected because a second option duplicates the
discovery path and the spec names `--catalog`; folding metrics into the
counter lines, rejected because a reader could not tell a metric from an
event.

**Consequences**: `harness_catalog_test.cpp` fixtures that count lines must
be updated with the new lines, and the metric lines appear under the same
object path the counters use, so the tree shape stays stable for readers of
the existing output.

## D-12: A repeatable metric option, ordered, deduplicated, fail-fast

**Decision**: The harness gains a metric option that may be repeated. Columns
appear in the order the names appear on the command line, a name given twice
yields one column, a name that matches no metric prints one line and exits
with status 2 before any measurement, and a metric that exists but cannot be
counted yields a refusal column while the run continues.

**Drivers**: The hard rule for command-line values is one line and status 2,
and the spec's clarification places the failure before measurement so a
typo costs nothing. `runner.cpp` already builds metric columns from attached
metrics and then from named addresses, and a missing leaf there prints a line
and continues, so the two kinds of failure already have two precedents in the
same function.

**Alternatives considered**: treat an unknown metric as a refusal column,
rejected because the spec distinguishes a name that does not exist from a
metric that cannot be counted, and the first is a user error; validate after
the first repetition, rejected because it wastes a measured run; allow a
metric file to be named, rejected as scope the spec does not ask for.

**Consequences**: `cli.cpp` gains the option row and the early validation,
`README.md` gains the matching row in the option table, `cli_shape.sh` checks
that parity, and the harness archive keeps `SOVERSION` 6 because no harness
signature changes shape.

## D-13: The stepping rides a private field and the key grows one field

**Decision**: `PmuIdent` in `source/counters/detail/pmu.hpp` gains a stepping
field, read where CPUID is already read. `mapfileKey` appends it, producing
`<vendor>-<family>-<model>-<stepping>`, and `pmuSelectDirectory` matches the
mapping file's first column against that key with the pattern form the file
uses.

**Drivers**: The header `PmuIdent` sits in is private, so the field reaches
no public surface. The pinned `mapfile.csv` splits model `0x55` by stepping
through `GenuineIntel-6-55-[01234],v1.37,skylakex,core` and
`GenuineIntel-6-55-[56789ABCDEF],v1.25,cascadelakex,core`, and replaying all
46 rows against the current key `GenuineIntel-6-55` matches nothing, so a
model-55 part selects no directory today. Upstream added the stepping to the
CPUID string in 2018 for this split.

**Alternatives considered**: keep a two-field key and match with a prefix
rule, rejected because a prefix match makes the first row of a model win
regardless of stepping, which is the bug in another position; special-case
model `0x55`, rejected because 45 other rows carry classes a fix must honor
anyway; select by microcode or family model lists, rejected because the
mapping file is the pinned source of truth.

**Consequences**: the pattern form is a character class over hex digits, so
matching needs one small matcher with fixtures on both sides of the boundary
(FR-035) and a sweep over every class row of the pinned file (FR-036). A
model-55 machine starts selecting a directory it never selected, which
changes the leaves and metrics it exposes: that is the fix working, and the
test record says so. Public headers do not change, so D-09's `SOVERSION`
answer holds.

## D-14: Load budget, docs budget, and the coverage plan

**Decision**: Load parses every metric row of the selected directory inside
a recorded budget, and the coverage plan pairs a corpus of malformed inputs
with a sweep over the pinned rows. The documentation move extends the
existing page section through a second region of
`example/docs/counter_metrics.cpp` and records the `PAGE_LINE_CAP` raise the
new lines need.

**Drivers**: The `skylakex` directory holds 4068 rows, 279 of them metric
rows: 156 of those carry a scale text, 19 carry a literal or a conditional and
38 carry an escape. The 279 are the size the load path must carry. Principle VI
needs every refusal branch driven, which a corpus
supplies, and every accepted construct driven, which the sweep supplies from
real table text. `README.md` stands at 638 lines against `PAGE_LINE_CAP` 638
and `SECTION_LINE_CAP` is 25, so a documentation change is a budget edit and
the raise is a recorded decision with its reason named.

**Alternatives considered**: parse lazily per row to shrink the load figure,
rejected by D-02; keep the coverage plan to hand-written cases, rejected
because the hand-written set cannot reach the arms of the scale table and the
escape list; put the new section in `docs/pages/harness.md`, rejected because
the page already owns the capability section the drift gate checks and a
second owner file would widen the shared list.

**Consequences**: the load figure is read once in a test that parses the
selected directory and asserts the row account plus a generous time bound
expressed in counted work, with no wall-clock figure asserted, so the fake
provider stays the only source of numbers. The documentation task carries
the `PAGE_LINE_CAP` change and the section count check in one commit, so the
drift gate is green at every commit of the feature.
