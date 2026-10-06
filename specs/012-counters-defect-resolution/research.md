# Phase 0 research: counters defect resolution

Every NEEDS CLARIFICATION marker in the technical context of
`plan.md` is resolved below. Each defect the specification names was
re-read at its cited site before this file was written, and the verdict
records what the lines show.

## Method

A defect is **confirmed** where the cited lines show the defect. A
defect is **suspected** where the code supports the reading and no run
on the affected hardware has shown it (specification, Purpose). Seven
defects are confirmed by reading alone. Two, I-01 and I-04, hold a
confirmed part and a suspected part; the Intel confirmation is deferred
(FR-039).

The kernel's own interface header, `include/uapi/linux/perf_event.h`
as installed at `/usr/include/linux/perf_event.h`, is the authority for
the fast-read decode recipe. Where the header and the tree disagree, the
header governs.

## Defect confirmation

| Defect | Priority | Verdict | Site that shows it |
| --- | --- | --- | --- |
| I-01 | P2 | confirmed, Intel part suspected | `source/counters/linux_pmu/table_parse.cpp:176-180` |
| I-02 | P2 | confirmed | `source/counters/linux_pmu/provider.cpp:266`, `:296` |
| I-03 | P2 | confirmed | `source/counters/linux_pmu/provider.cpp:102-111` |
| I-04 | P1 | confirmed, Intel part suspected | `source/counters/linux_pmu/fast_read.cpp:95-97`, `group_io.cpp:342-352`, `:361-367` |
| I-05 | P1 | confirmed | `source/counters/linux_pmu/group_io.cpp:238-248`, `fold.cpp:126-130` |
| I-06 | P1 | confirmed | `source/counters/system.cpp:418-426` |
| I-07 | P2 | confirmed | `source/counters/linux_pmu/table_parse.cpp:391`, `:403`, `:430` |
| I-08 | P1 | confirmed | `source/counters/detail/pmu.hpp:279-285`, `group_io.cpp:327`, `:552`, `:564` |
| I-09 | P2 | confirmed | `source/counters/plan.cpp:256-296` |
| I-10 | P2 | confirmed | `include/speedgun-ng/counters_clock.hpp` |

### I-01, Intel rows carry an encoding obligation no kernel format names

`add_entry` records a numeric key as an encoding field through the
`else if (parse_scalar(value, number))` arm at
`table_parse.cpp:176-180`. The arm treats every integer-valued key as a
field the kernel must publish. Measured over the pinned tree, an Intel
directory carries sampling keys and metadata keys at that scale:

| Directory | Rows | Rows carrying `SampleAfterValue` | Rows carrying `MSRIndex` | Rows carrying `CounterMask` |
| --- | --- | --- | --- | --- |
| `skylake` | 813 | 564 | 287 | 61 |
| `alderlake` | 895 | 526 | 86 | 63 |
| `amdzen4` | 577 | 0 | 0 | 0 |

`pmu_compose_config` at `provider.cpp:296` fails for any field name the
device `format/` directory does not publish, and the caller clears
`entry.words` on failure. An Intel row therefore carries at least one
field no `format/` entry names, so the row encodes no config word and
publishes as `not_encodable`. The same rule covers `amdzen4`, whose
sampling-key columns are empty, which is why the AMD counts stand
today. The defect is the parse rule, and it is confirmed. The count of
rows an Intel host encodes is the suspected part; the
reference host is AMD and no run on Intel hardware has measured it.

### I-02, vendored rows reach one device

`merge_vendored` at `provider.cpp:266` takes one `pmu_device&` and
appends every vendored row to it. Its own comment at `:258-260` records
the scope it assumes: the rows describe core events. A host that
publishes `cpu_core` and `cpu_atom` devices, or an `uncore_*` device,
receives no vendored row of its own scope. Confirmed.

### I-03, every availability probe asks as one target

`probe_device` at `provider.cpp:102-111` calls
`detail::pmu_probe(device.type, entry.words)` per entry. The probe
takes no target, so it opens a per-task event and reads the verdict for
a thread-bound caller. A device-scoped entry, an uncore or a power
entry, refuses the per-task event and publishes `permission_blocked`,
while a cpu-target plan over the same entry compiles on a host that
grants the cpu-targeted event. Confirmed.

### I-04, the fast read publishes a masked value and a stale retry

Three separate faults sit on the path.

`fast_decode` at `fast_read.cpp:95-97` computes
`adjusted = raw + offset` and then masks the sum to `pmc_width` bits.
The header names a different recipe at `perf_event.h:654-663`: sign
extend the value the instruction read from `pmc_width` bits, then
accumulate. A mask truncates a cumulative count at the published
counter width. At the reference host's rate a 48-bit counter crosses
that boundary inside a multi-hour run, and the 64-bit modular delta
across the crossing reports a value no counter held.

`pmu_fast_window::read_points` at `group_io.cpp:342-352` inspects the
verdict for `unstable` alone. A read the page refuses with
`fast_read_verdict::not_allowed` leaves `member::value` at the value an
earlier sampling action wrote, and the loop puts that value into the
managed column. The retry arm calls `fast_context_read` a second time
into the same storage; when the retry also fails, the value the read
before the sequence moved survives into the point.

`pmu_fast_window::read_points` at `group_io.cpp:361-367` writes a zero
enabled and running pair when the leader's page discloses no stable
pair. `leaf_ratio` at `fold.cpp:126-130` treats a pair with no elapsed
enabled time as a ratio of 1.0, so a window the kernel refused to time
reports the ratio of a window that ran for its whole length. The caller
holds no way to tell the two apart.

### I-05, a short group read is indistinguishable from a zero count

`pmu_window::read_points` at `group_io.cpp:238-248` fills zeros and
continues when the leader answers with fewer bytes than the group
header. The slots then put those zeros into the managed columns, so a
fold across the action reads a delta of zero, which is a count the read
never produced. A refused group read takes the same arm. No column
carries the failure. Confirmed.

### I-06, catalog resolution writes shared state

`system::handle_for` at `system.cpp:418-426` reads `m_impl->handles`,
and on a miss constructs the handle and writes it back with `emplace`.
Two threads resolving one canonical address both reach the `emplace`.
The parent and children walks at `:471`, `:511`, and `:554` call the
same function. Plan compilation writes the open flag on every call, so
the flag has no home that a compile does not write. Confirmed.

### I-07, the tables resolve through a build-time path

`SG_PMU_EVENTS_DIR` names a directory inside the source tree at
`table_parse.cpp:391`, `:403`, and `:430`. An installed archive carries
no such directory, so an installed package publishes no vendored row.
Confirmed.

### I-08, a fast window releases nothing

`fast_context` at `detail/pmu.hpp:279-285` holds `fd`, `map`, and
`map_length`, and declares no destructor. `fast_context_close` is a free
function declared at `pmu.hpp:339`. `pmu_fast_window` at
`group_io.cpp:327` declares `~pmu_fast_window() = default`, and its
`member` holds `std::unique_ptr<fast_context>` under the default
deleter, which runs no close. Every destroyed fast-mode plan therefore
leaks one descriptor and one mapping per member leaf. The partial-open
arms at `group_io.cpp:552` and `:564` return null after earlier members
were opened, and the same default deleter releases nothing. Confirmed.

Note the contrast with the syscall window at `group_io.cpp:204-217`,
whose destructor closes every member descriptor. The fast window is
the side that leaks.

### I-09, the calibration sits inside a coverage exclusion

`calibrate` at `plan.cpp:267-296` runs the bracketing measurement, and
the whole region from `:256` is wrapped in `LCOV_EXCL_START` and
`LCOV_EXCL_STOP`. The published floor therefore carries the two
bracketing clock reads, and no registered test can reach the code that
subtracts them. Confirmed. The marker count in `source/counters/`
falls by removing this one region, which satisfies FR-027.

### I-10, two leaves document an order their clock does not provide

`counters_clock.hpp` states one order guarantee for the clock class.
The per-thread CPU clock `machine/thread_cpu` runs per thread, so a new
thread's first sample can fall below a sample taken earlier on another
thread. The timestamp-counter leaf reads the processor's cycle counter
with no ordering fence, so two reads from different processors carry no
order. The class-level guarantee covers both leaves. Confirmed.

## Decisions

### D-01 Fast-read decode follows the header recipe

**Decision**: `fast_decode` sign extends the value the instruction read
from the published `pmc_width`, then adds the page's `offset`, and
publishes the full 64-bit sum. The point carries no mask.

**Rationale**: `perf_event.h:654-663` states the recipe and `:621-630`
states the read order. `offset` is documented at `:637` as the value
added to a hardware event value. The header's `count += pmc`
accumulates without a mask, so a masked point contradicts the authority
FR-004 names.

**Alternatives considered**: A 128-bit point keeps the mask and widens
the fold. Rejected: no caller reads a 128-bit point, the fold is
modular over 64 bits, and the column is fixed-width. A per-window
accumulating base keeps the mask and makes the point monotonic.
Rejected: it adds state to the hot path for a wrap that the header's
recipe does not produce (Principle VII).

### D-02 The pair is raw, the extrapolation stays in the fold

**Decision**: The window publishes `time_enabled` and `time_running`
as the page holds them. The extrapolation stays in `leaf_ratio`, which
computes the running delta over the enabled delta. A failed pair read
discloses the failure through the disclosure column, and the fold then
reports the ratio it can measure.

**Rationale**: FR-005 cites the header for the time computation. The
header documents the decode recipe, the read order, and the two pair
fields at `perf_event.h:638-639`. It states no scaling rule. The
scaling rule the library implements is the reciprocal of the scale
`perf_event_open(2)` documents for a multiplexed event, and
`fold.cpp:126-130` already computes that reciprocal. The arithmetic is
therefore already the documented one; the defect is that a failed read
is indistinguishable from a measured full-length window.

**Alternatives considered**: Move the extrapolation into the window and
publish a scaled count. Rejected: the fold needs the pair to compute a
composite ratio over several windows (007 FR-019), so publishing a
scaled count removes information the fold uses. Cite the extrapolation
to the header. Rejected: the header names no such rule, and a citation
that points at nothing is drift (Principle X.3).

### D-03 A refused read discloses a gap and keeps no earlier value

**Decision**: `read_points` inspects all three verdicts. A read that
does not return `fast_read_verdict::ok` writes a zero count into the
managed column and marks the action in the disclosure column. The retry
arm runs once, as the protocol states, and its failure takes the same
arm. No earlier value survives into the point.

**Rationale**: FR-002 and FR-003 require the gap and forbid the
earlier value. A syscall fallback was rejected in clarification, because
it makes the per-action cost unpredictable and Principle VII tracks that
figure.

**Alternatives considered**: Retry until the sequence holds. Rejected:
an unbounded loop on the hot path has no bounded cost (Principle VII).
Publish the last good value with the gap marked. Rejected: FR-003
forbids it, and a stale count is a wrong benchmark (Principle VI).

### D-04 The disclosure is one managed column written through `put`

**Decision**: A compiled plan reserves one additional managed column
per sampling action. The sampling action writes the entry's countability
value into it through the existing `point_sink::put`, beside the ratio
pair's two columns. `put` keeps its one-integer signature.

**Rationale**: FR-007 and the clarification of 2026-10-04 settled this
after reversing the earlier answer. `point_sink::put` takes one
`std::uint64_t` at `counters_provider.hpp:201`, the ratio rides two
ordinary columns, and availability is a field fixed at registration.
There is nothing to widen. The change adds no per-sample metadata.

**Alternatives considered**: A sentinel count such as `UINT64_MAX`.
Rejected: it publishes a wrong value with no disclosure (Principle VI).
A second channel beside the sink. Rejected: the caller then holds two
sources in step. Widen `put` to take the disclosure. Rejected: the
point carries no ratio field and no availability field to widen it
with, and the reversal is recorded in the specification.

### D-05 The thread sanitizer gets its own preset and its own job

**Decision**: A `ci-tsan` preset configures the thread sanitizer, and a
`tsan` job in `.github/workflows/ci.yml` builds the suite under it and
runs it. The preset carries the thread-sanitizer flags on the vendored
trees. No suppression file is needed.

**Rationale**: FR-012 requires the gate to run in CI so it cannot pass
on a run that never happened. The compilers reject a build carrying both
the thread sanitizer and the address sanitizer, so the existing
`ci-sanitize` preset keeps address and undefined behavior alone.

**Alternatives considered**: A suppressions file under version control.
Rejected for now: the flags cost nothing and a suppression list cannot
prove it stayed current. A suppressions file remains the fallback if
the flags prove unable to cover the vendored trees, and FR-012 accepts
either. A developer-local run only. Rejected: FR-012 requires a CI job.

### D-06 Catalog state is guarded where it is shared

**Decision**: `handle_for` and the plan-compile path take a lock
around the shared maps. The open flag moves out of the per-call compile
path and is set once, at the open boundary.

**Rationale**: FR-010 and FR-011 require safety from any number of
threads after open. The writes are two maps and one flag. A lock is the
shortest correct change, and the compile path is setup work, where a
lock costs nothing (Principle VII scopes P0 techniques to the sampling
path).

**Alternatives considered**: Concurrent containers with a sharded lock.
Rejected: no caller needs the throughput, and X.2 bars speculative
generality. A reader-writer lock. Rejected: the reads dominate and the
maps are small, so a plain lock is the shorter code.

### D-07 A fast window releases through `fast_context_close`

**Decision**: `fast_context` gains a destructor that calls
`fast_context_close`. The partial-open arms release the members they
acquired before returning null. The explicit close stays where the probe
uses it.

**Rationale**: FR-013 and FR-014 require the release. A destructor on
the owning type is the shortest path that covers every exit,
including the partial arms. It mirrors the syscall window's destructor
at `group_io.cpp:204-217`.

**Alternatives considered**: A custom deleter on the `unique_ptr`.
Rejected: the destructor is the ownership statement, and a custom
delimiter hides it. Release at plan destruction only. Rejected: a plan
destroyed after a failed open still holds what the failed open acquired.

### D-08 A numeric key is an encoding field only where the format exists

**Decision**: The parser records a numeric key as an encoding field
only where the key names a format the running device publishes. The
kernel's own spelling aliases `cmask`, `inv`, `edge`, and `offcore_rsp`
map onto `CounterMask`, `Invert`, `EdgeDetect`, and `OffcoreRsp`.

**Rationale**: FR-016 and FR-017 state both halves. The device `format/`
directory is the publication, and `provider.cpp:296` already consults
it. A sampling key or a metadata key carries no obligation, so the row
encodes.

**Alternatives considered**: A closed list of encoding key names in the
parser. Rejected: the kernel publishes new formats, and a closed list
makes a new kernel field invisible. Filter at composition time only.
Rejected: the row then carries a field the encoder must reject per row,
which puts the same test on the hot compile path.

### D-09 Vendored rows route by their table scope

**Decision**: The scope the table records for a row decides which
device receives it. A core-scoped row reaches each core device the
scope applies to, an uncore-scoped row stays on the uncore device, and
a row whose device the host does not publish stays out of the catalog.

**Rationale**: FR-019 states all three cases. The scope is data the
table already carries, which `provider.cpp:427-432` already reads for
the description.

**Alternatives considered**: Publish every vendored row on every
device and let the probe filter. Rejected: the probe cannot tell a
scope refusal from an encoding refusal, which is the defect I-03
records. Keep the single-device merge and duplicate it per device.
Rejected: it puts a core row on an uncore device, which FR-019 forbids.

### D-10 Availability separates a scope refusal from an encoding refusal

**Decision**: One enumeration carries the countability state, with a
value for a scope refusal and a value for an encoding refusal beside
`not_encodable`. A fixed-size bitmask beside the state names the target
kinds the entry can be counted on. The probe runs per target kind the
bitmask admits, so a cpu-target plan compiles over an entry whose scope
refuses a per-task event.

**Rationale**: FR-021 and FR-022 state the shape and the probe. A
bitmask allocates no memory, so reading the targets costs no allocation
on the harness path. The enumeration gains values and no stored value
moves.

**Alternatives considered**: One enumeration per concern. Rejected:
every caller holds two fields in step. A `std::set` or a vector of
target kinds. Rejected: it allocates on the read path. A separate
probe entry point per target. Rejected: the bitmask already says which
targets to probe.

### D-11 The tables travel inside the archive as static bytes

**Decision**: A build step compiles the vendored JSON into static
data. The existing simdjson parse path decodes those bytes at run
time. The embedding is unconditional. No configured data path is added.

**Rationale**: FR-023 and the clarification of 2026-10-03 settled the
form, the codec, and the absence of a knob. An installed archive has no
run-time location, so embedding removes the path lookup. The parse code
at `table_parse.cpp:208-251` already reads these files, so the form
adds no codec and no dependency.

**Alternatives considered**: A generated C++ row array. Rejected: a
larger archive and a slower compile, and it drops the rows SC-008
needs. Install the tables beside the library and resolve at run time.
Rejected: the archive links into the consumer's executable, so no
reliable path exists. A build option to turn the embedding off.
Rejected: it makes FR-023 and SC-008 hold on the default build only,
and X.2 bars configurability no caller needs.

### D-12 The calibration subtracts its bracket, no exclusion hides it

**Decision**: `calibrate` measures the bracketing pair under the
identical bracketing and subtracts it from the sampling-action cost. The
`LCOV_EXCL_START` region at `plan.cpp:256` is removed. A registered test
covers the calibration and fails when the subtraction is absent.

**Rationale**: FR-025 and FR-026 state both halves. Spec 011 measured
the bracket at 7.36 ns on the reference host, and the published floor
has to isolate the sampling action from it. Removing the region lowers
the exclusion marker count, which FR-027 requires.

**Alternatives considered**: Measure the bracket once per process and
cache it. Rejected: the floor is per plan, and the cache would make the
figure depend on which plan asked first. Keep the exclusion and cover
the code through a private entry point. Rejected: FR-026 forbids a
coverage exclusion hiding the calibration from the test.

### D-13 Each clock leaf states its own order guarantee

**Decision**: Each leaf documents its own guarantee, and the class
contract states no order on a leaf's behalf. The per-thread CPU clock's
guarantee permits a new thread's sample to fall below an earlier sample
from another thread. The timestamp-counter leaf states the precondition
that one thread takes both window endpoints, and gains no fence.

**Rationale**: FR-028 through FR-031 state each half. A contract the
code does not keep is a defect under Principle II. A fence helps only a
caller that reads across threads, and the harness takes both endpoints
on one thread, so a fence would cost a read per timestamp for nothing
(Principle VII).

**Alternatives considered**: Add a fence to the timestamp-counter leaf
so the class guarantee holds. Rejected by the clarification of
2026-10-04 and by FR-031. Drop the guarantee from the two leaves.
Rejected: FR-028 requires every leaf to state its own.

### D-14 The release ships as 0.3.0

**Decision**: `CMakeLists.txt:7` moves to `VERSION 0.3.0`.
`SOVERSION` stays at `PROJECT_VERSION_MAJOR`, which stays 0.

**Rationale**: FR-024 states the version and the precedent. Spec 008
took the minor for a change that added a member to `system` and changed
no signature. FR-021 has the same shape: it adds a countability value
and a bitmask beside the availability and changes no existing
signature. FR-007 widens no signature.

**Alternatives considered**: A patch bump, following spec 011's wording
for its purely additive change. Rejected: spec 011 added no field to a
public record. FR-021 adds a field beside the availability on a public
record, and the specification records why 011's wording does not
extend to that shape. A major bump. Rejected: no signature changes and
no member is removed, so no caller breaks.

### D-15 One release function serves the destructor and the arms

**Decision**: The member release becomes one internal function that
both `pmu_fast_window`'s destructor and the two partial-open arms call.
The everywhere-runnable half of SC-004 drives that same function over
descriptors and mappings the test opened itself, then compares
`/proc/self/fd` and `/proc/self/maps` with their values before the loop.
The host-dependent half opens a real plan and skips with
`SKIP_RETURN_CODE 2` where the kernel refuses the event.

**Rationale**: FR-014 already requires the partial arms to release what
the earlier members acquired, so the release is a named function in the
corrected shape and the test calls the production path. `FR-034`
requires every test to run unprivileged at `perf_event_paranoid` 2,
and SC-004 requires the everywhere-runnable test to measure real
release on a host whose kernel refuses `perf_event_open`. The seam test
at `test/source/counters_linux_pmu_seam_test.cpp` already calls
internal functions declared in `source/counters/detail/pmu.hpp`, so the
established pattern covers it.

**Alternatives considered**: An injection seam that hands the open path
a substituted resource source. Rejected: X.2 bars an extension hook for
a use case that exists only in a test, and the release function is
already a named function the correction needs. A test that counts a
counter the library increments. Rejected: SC-004 requires real
descriptors and real mappings. One test that skips where the kernel
refuses. Rejected: it skips on every CI runner and the release path
carries no branch coverage.

## The Intel confirmation is deferred

The reference host is an AMD Ryzen 9 9950X3D running Linux
7.2.4-1-cachyos with glibc 2.44, and it serves every figure this
feature publishes. I-01 and I-04 each hold a suspected part that an
Intel host would settle: the encodable-row count an Intel host reaches
over the pinned tree, and the fast-read decode on Intel hardware.

The deferral is recorded here and in `plan.md`, on the terms of FR-039.
The host class that settles both is an Intel host whose kernel grants
`cap_user_rdpmc` and whose PMU publishes the core event formats the
vendored Intel tables name. The repository owner expects to add one
later. No requirement, test, or gate in this feature depends on the
confirmation: SC-005 measures over the pinned tree against a synthetic
format list, so one number gates every host on the matrix. The
suspected parts of both defects stay in scope until that host answers.

## Figures this plan schedules

Four figures come from measurement on the reference host. Each is
recorded beside the requirement that names it, in the artifact named
there.

| Figure | Requirement | Where it is recorded | Run method |
| --- | --- | --- | --- |
| Core PMU group median, at `6aafd2d` and after | FR-008 | `quickstart.md` step 8, beside the recorded figure | `counters_overhead`, release preset, pinned processor, median over 64 repeats of 1000 actions |
| Clock-leaf median, at `6aafd2d` and after | FR-008 | `quickstart.md` step 8 | same run, same protocol |
| Intel encodable-row counts over the named synthetic format list | FR-020, SC-005 | `data-model.md`, event table row entity | the fixture that pins `skylake`, `icelake`, `alderlake`, `sapphirerapids` |
| Reference host's own-format counts beside them | FR-020, SC-005 | `data-model.md`, same table | the same fixture run with no synthetic list supplied |
| Archive size and linked executable size with the tables embedded | FR-023, SC-012 | `quickstart.md` step 10 | `size` on `libspeedgun-ng.a` and on the linked consumer, before and after |

The `amdzen4` and `amdzen5` counts are recorded beside the Intel counts
and must not fall (FR-020).

## TDD mode

**TDD mode is in force for this feature** (FR-033, Principle III). The
covering test for every requirement is written and observed failing at
`6aafd2d` before the correction that turns it green (FR-032). The task
artifact orders each test task before the code task it gates.

## Repository constraints this plan records

- No new event source, object kind, provider, or public header beyond
  the ones a requirement names (FR-043).
- No public header gains a platform term
  (`test/counters_header_purity.sh`, FR-036).
- The counters classes stay embeddable in fixed-iteration, per-thread
  benchmark loops, and no benchmarking-framework code enters the library
  (FR-035).
- Every correction lands in
  `specs/007-counters-and-timers/citations-log.md` in that file's entry
  format (FR-038). The format is a YAML block with the fields `date`,
  `task`, `section`, `figure_as_written`, `figure_measured`, `command`,
  `head`, and `must_not_move`. The frozen record at
  `specs/007-counters-and-timers/citations.md` takes no edit.
- Every changed interface keeps its doxygen contract paired with a
  registered enforcement counterpart, and the `dbc-gate` target proves
  the pairing (FR-037, Principle II).
- Every changed line reaches 100 percent line, branch, and contract
  coverage (FR-040), and the clang-tidy warning count of each touched
  translation unit does not rise (FR-041).
