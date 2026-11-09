# Feature Specification: Counter-count group probe

**Feature Branch**: `041-counter-probe`

**Created**: 2026-11-05

**Status**: Draft

**Input**: User description: roadmap item N6-P (`/workspace/speedgun-ng-harness-roadmap.md`
section 3 row N6-P, section 6 item 13), the counter-count group probe. Condensed
below; the request is the authority for scope, numbering and the accepted answers.

> Master knows the programmable-counter limit only from a CPUID decode: Intel and
> Zhaoxin from leaf 0xA, AMD and Hygon from leaves 0x80000001 and 0x80000022, on
> the core device only (`source/counters/linux_pmu/counter_limits.cpp`,
> `limitForDevice`). Arm (`armv8_*`) and Intel hybrid parts that answer leaf 0x23
> get no limit, so the N6 planner multiplexes for them. N6-P adds a
> vendor-neutral group-open probe: where the decode gives no value, the probe
> opens one group of k enabled copies of a general-purpose-only event and finds
> the largest k that fits. That k is the count, and it fills
> `Object::programmableCounters`, so the planner packs passes instead of
> multiplexing.

## Spec identity and run-time binding

- Base: `master` at `858c55d9d036e74284dc48cd6fb7e42988cc8cff` (`858c55d9`),
  dated 2026-11-05 at branch time. Counted at branch time on this branch:
  version `0.31.0` (`CMakeLists.txt:64`), report schema `1.14`
  (`kSchemaVersion`, `source/harness/json_report.cpp:49`), harness `SOVERSION`
  21 (`CMakeLists.txt:419`), counters archive `SOVERSION` 6
  (`CMakeLists.txt:143`), `LCOV_EXCL` markers 84
  (`git grep -c LCOV_EXCL -- source include`), `README.md` 1001 lines
  (`wc -l README.md`, owner ruling G-7 caps the page at 1004). Every symbol
  below is a name first and a line number second: a line number cites the audit
  point, and a search for the name is the way to re-find it when the lines move.
- Branch: `041-counter-probe`, cut from the base above after the pull request of
  `specs/046-hunter-topdown` AMD acceptance opened. Rebase only, no merge
  commit, and re-read both sides of every conflict: other lanes move `master`
  while this lane runs.
- Number: 041, reserved for N6-P. Counted at branch time: `specs/` holds 001 to
  040 and 042 to 047, with no `041-*` directory; `git ls-remote --heads origin`
  carries no `041-*` branch; `gh pr list --state all` answers 80 as the highest
  number and names no 041 branch. So 041 is free, and no renumbering is needed.
- Roadmap: the item lives in `/workspace/speedgun-ng-harness-roadmap.md`. That
  file is machine-local and is absent on this host, so the quoted request above
  and the binding list below are the spec's whole record of it. No statement
  here quotes a roadmap line it could not read.

## Measured facts (requirement 1)

The request states a conflict, and this lane settles it by measurement before it
states a requirement. specs/024 PC-3 and its research D-3 state that the kernel
grants an oversize group; the reading of the kernel source (`validate_group`,
`collect_events`) in the request states that an oversize group open fails with
`EINVAL`; the logs PC-3 cites are not in the repository. The measurement answers
the first reading, and the second one does not hold on the measured host.

`specs/041-counter-probe/tools/group_probe.c` holds the program. It opens one
group of k copies of `PERF_COUNT_HW_BRANCH_INSTRUCTIONS` with
`exclude_kernel=1`, `disabled=1` and `pinned=0` — the shape of the probe — then
enables the group, reads the group leader once, disables and closes. Run
`taskset -c 0,1 ./group_probe` to repeat it. No unit runs it, and it uses no
speedgun-ng code.

host Gary
kernel.ostype Linux
kernel.osrelease 7.3.1-1-cachyos
kernel.perf_event_paranoid 2
kernel.nmi_watchdog 0
event config 4, type PERF_TYPE_HARDWARE, exclude_kernel 1, disabled 1, pinned 0
cpuid leaf 0x80000022 eax 0x00000007 ebx 0x00084106 ecx 0x00000003 edx 0x00000000 (raw words; counter_limits.cpp decodes them)
cpuid leaf 0x0A eax 0x00000000 ebx 0x00000000 ecx 0x00000000 edx 0x00000000 (raw words)

## Table 1: one group of k copies, k = 1 to 32

| k | open | errno | time_enabled_ns | time_running_ns | runs fully |
|---|------|-------|-----------------|-----------------|------------|
| 1 | ok | - | 2460 | 2460 | yes |
| 2 | ok | - | 2150 | 2150 | yes |
| 3 | ok | - | 3110 | 3110 | yes |
| 4 | ok | - | 4450 | 4450 | yes |
| 5 | ok | - | 6340 | 6340 | yes |
| 6 | ok | - | 8520 | 8520 | yes |
| 7 | ok | - | 10450 | 8390 | no |
| 8 | ok | - | 10270 | 8520 | no |
| 9 | ok | - | 10240 | 8440 | no |
| 10 | ok | - | 10339 | 8429 | no |
| 11 | ok | - | 10450 | 8440 | no |
| 12 | ok | - | 10630 | 8510 | no |
| 13 | ok | - | 10720 | 8480 | no |
| 14 | ok | - | 10870 | 8500 | no |
| 15 | ok | - | 11120 | 8590 | no |
| 16 | ok | - | 11140 | 8540 | no |
| 17 | ok | - | 11290 | 8570 | no |
| 18 | ok | - | 11450 | 8600 | no |
| 19 | ok | - | 11670 | 8710 | no |
| 20 | ok | - | 11770 | 8680 | no |
| 21 | ok | - | 11810 | 8620 | no |
| 22 | ok | - | 12000 | 8670 | no |
| 23 | ok | - | 12150 | 8700 | no |
| 24 | ok | - | 12280 | 8710 | no |
| 25 | ok | - | 12480 | 8790 | no |
| 26 | ok | - | 12550 | 8740 | no |
| 27 | ok | - | 12730 | 8790 | no |
| 28 | ok | - | 12860 | 8790 | no |
| 29 | ok | - | 13030 | 8800 | no |
| 30 | ok | - | 13130 | 8820 | no |
| 31 | ok | - | 13289 | 8869 | no |
| 32 | ok | - | 13420 | 8880 | no |

Largest size that runs fully: 6

## Table 2: where does a group open refuse?

No refusal up to k = 96. Every group open answered 0.

## Table 3: the linear probe, and what it costs

Probe answer (largest size that runs fully): 6
Sizes opened: 7, opens paid: 28, enables 28, reads 7, closes 28
Probe wall time: 0.070 ms (CLOCK_MONOTONIC; a script reads the clock, no unit does)

Totals: opens ok 5212, opens refused 0, enables 556, disables 556, reads 39, closes 5212
Descriptors left open at exit: 0 (closes minus opens; must be 0)

The five facts this lane builds on:

- **F-1 An oversize group opens.** No size refused up to k = 96, so PC-3's
  reading stands and the `EINVAL` reading does not hold on this host and kernel.
  A probe that reads the open result alone answers 32 here, which is a wrong
  limit, so a wrong limit is the failure mode this measurement closes.
- **F-2 An oversize group never runs.** From k = 7 the leader reports a small
  `time_running` (about 8.4 microseconds, the residue of the enable and disable
  calls) against an `time_enabled` of the whole window. The kernel accepts the
  group and declines to schedule it.
- **F-3 One enabled read settles the question.** `time_running ==
  time_enabled` with `time_enabled` past zero holds in 50 trials of 50 for every
  size that fits and fails in 50 trials of 50 for every size that does not, at
  one read taken right after the enable call and again with a disable before the
  read (`tools/probe_window.c`, `results/probe-window-trials.txt`). No sleep, no
  spin and no clock read stands between the enable and that read, so the probe
  pays syscalls only.
- **F-4 The answer equals the decode on the measured host.** The probe answers 6
  and the decode of this part answers 6, with `kernel.nmi_watchdog` at 0, so the
  watchdog rule takes nothing. This is the check-mode equality of SC-003.
- **F-5 The probe is cheap.** The linear search to 6 opens 7 sizes, pays 28
  opens, 7 reads and 28 closes, and answers inside 0.1 ms on the measured host.
  The tool that measured it enables and disables each member, so it pays 28
  enables where the shipped probe pays 7 group ioctls on the leader. Closes
  equals opens, so the measured run leaves no descriptor behind.
  `results/requirement-1-shipped-shape.txt` prices both shapes on the
  acceptance host: 0.080 ms for the shipped pair, with a spread of 0.068 to
  0.095 ms across repeats. The wall time of one search is dominated by the
  scheduler, so the lane states it as a band and never as one number.

A second shape is a fact too, because it rules out a cheaper design: k
*independent* events (no group) all open and most report some running time,
because the kernel multiplexes them, so the count of events that run is not the
counter count. Only the one-group shape answers the question.

## Decisions

The request carries six open questions with recommended answers. Each one is
taken as the decision below and names the reading it settles. No implementer
reopens them.

- **D-1 Devices (Q-1).** Core devices only: `cpu`, `cpu_core`, `cpu_atom`, and a
  device part that starts with `armv8_`. Uncore devices (`amd_l3`, `amd_df` and
  their peers) stay out, and a later item of its own may take them. The probe
  runs only where `limitForDevice` answers no value for such a device, so a
  device the decode answers never pays for a probe.
- **D-2 Option (Q-2).** No new option, and no short option. The probe runs on
  its own where the decode gives no value. Check mode probes one device and
  prints the probe figure beside the decode figure. It runs through the example
  and a test seam. The command line holds no word for it. The request's rule holds: a value that a later
  change needs takes 360 or above, because 350 to 359 stand with top-down and
  300 to 319 stand free.
- **D-3 Source of the limit (Q-3), with the cost corrected at plan time.** The
  plan text names where a limit came from: `decode` or `probe`. The request
  recommended keeping the source in an internal record so the counters archive
  `SOVERSION` stays 6. That reading does not survive the audit of the tree:
  `source/harness/` includes no header under `source/counters/detail/` (zero
  matches over `source/harness/*.cpp` and `source/harness/detail/*.hpp`), and
  `SPEEDGUN_NG_EXPORT` stands only in headers under `include/speedgun-ng/`, so a
  counters-internal record is out of the harness's reach and FR-050 is
  unmeasurable. The lane therefore adds one exported free function over one
  closed enumeration, beside the object surface in
  `include/speedgun-ng/counters_system.hpp`:
  `objectLimitSource(std::string_view objectPath) -> LimitSource`, where
  `LimitSource` is `None`, `Decode` or `Probe`. A member of `Object` is the
  reading that does not work: every `Object` accessor reads the process table
  `programmableLimitTable()` through the private member `m_node`
  (`source/counters/system.cpp:446-455`, `include/speedgun-ng/counters_system.hpp:173-185`),
  and that file is out of scope (PC-5), so a member definition would force an
  edit there. The free function reads a table of the same shape that the provider
  fills at the same moment, and `Object::programmableCounters()` keeps its type,
  shape and rule. The exported counters surface gains one
  accessor, which is the same move specs/046 D-16 made when its accessor carried
  the archive `SOVERSION` from 5 to 6 (`specs/046-hunter-topdown/spec.md:89-90`),
  so the archive `SOVERSION` rises from 6 to 7 and the comment beside
  `CMakeLists.txt:143` names this record. The harness `SOVERSION` stays 21: the
  plan text and the report key are built from data the harness already holds, and
  no exported harness layout moves. The written report gains one key beside the
  limit, so `kSchemaVersion` moves from 1.14 to 1.15 and
  `docs/schema/report-v1.json` gains the key.
- **D-4 Group size (Q-4).** k runs from 1 upward and stops at the first size
  that does not fit; 32 is the cap. Fits means the reading of F-3, and an open
  result answers nothing on its own. A
  search that never finds a refusal reports no limit, because it has no answer
  inside the cap.
- **D-5 Arm (Q-5).** No Arm hardware run without a host the owner names. The Arm
  row stands pending with no hardware claim, and its acceptance is the seam
  tests only.
- **D-6 Cost (Q-6), with the moment fixed at plan time.** The probe runs once per
  core device per process, at the first catalog open of that process, and a run
  whose needed devices include a probed limit discloses it once (FR-050). F-5 states the
  cost: 28 opens and under 0.1 ms to answer 6. The request recommended running
  the
  probe lazily at the first plan that needs the limit. That moment is not
  reachable inside the published seeding model: the limit travels in
  `ObjectSeed`, the provider fills it once in `PmuProvider::enumerate`
  (`source/counters/linux_pmu/provider.cpp:842-853`), and `System` stages each
  published limit into its process table at registration
  (`source/counters/system.cpp:305`, a file out of scope, PC-5), so a plan can
  only ever read a limit the provider settled at registration. The lane therefore
  runs the probe inside that same `enumerate` pass, for the devices the decode
  leaves silent, one time per device, and keeps the probe behind that gate: a
  device the decode answers pays no open (FR-030). The first need for a limit
  arrives at registration, so no plan waits for a probe, and no device is probed
  twice.
- **D-7 One seam.** The probe opens through the kernel-call seam that PR #70
  landed (`source/counters/linux_pmu/kernel_calls.hpp`), beside the calls it
  already carries. No second seam joins the tree.
- **D-8 No second watchdog deduction.** The probe measures what the kernel
  grants now, so the watchdog rule that `usableCounters` applies to a decoded
  value does not apply again to a probed value. `kernel.nmi_watchdog` at 0 on
  the measured host makes F-4 the exact-equality case.
- **D-9 The event.** One general-purpose-only event per vendor family: on x86,
  `PERF_COUNT_HW_BRANCH_INSTRUCTIONS`; on Arm, one event the device lists that is
  neither cycles nor instructions. A device that lists no such event gets no
  limit and no probe.
- **D-10 Closure.** Every descriptor the probe opens is closed on every path,
  including a refused open, a short read and a permission refusal. The seam
  tests' descriptor count is the verifier.

## Preconditions

- **PC-1: the kernel-call seam.** PASS. PR #70 landed an internal seam for
  `perf_event_open`, `read`, `ioctl`, `mmap` and `close`
  (`source/counters/linux_pmu/kernel_calls.hpp`), and the counters tests drive
  it with an `errno` matrix. The probe rides this seam (D-7), so a unit scripts a
  group open that fails at size k without a real call.
- **PC-2: the limit reaches the planner.** PASS. A provider publishes
  `ObjectSeed::programmableCounters`
  (`include/speedgun-ng/counters_provider.hpp:78`), the linux_pmu provider fills
  it with `detail::usableCounters(...)`
  (`source/counters/linux_pmu/provider.cpp:848`), and
  `Object::programmableCounters()` feeds the pass planner at
  `source/counters/system.cpp:446` and the harness at
  `source/harness/runner.cpp:1593`. A filled limit therefore changes packing
  with no new channel.
- **PC-3: the host facts are read once.** PASS. The decode registers
  (`hostCounterLimitRegisters`) and the watchdog value (`hostWatchdogValue`) are
  read one time per process in `provider.cpp`, and the probe's result joins that
  shape (D-6).
- **PC-4: the scripted provider cannot script a refusal.** PASS as a gap this
  lane closes. `FakeProvider::setProgrammableCounters`
  (`include/speedgun-ng/counters_fake.hpp:169`) publishes a count; it cannot
  script a group open that fails at size k, so the probe's own units drive the
  seam. The fake provider stands outside that path.
- **PC-5: `system.cpp` stays out of this lane.** PASS. Another open change
  touches `source/counters/system.cpp`, so this lane edits nothing there. PC-2
  shows the limit reaches the planner without an edit there.

## Audit point

The state of `master` at `858c55d9`, read on 2026-11-05 on the host named in the
pull request body:

- `limitForDevice` answers no value for a device that is not `cpu`, `cpu_core`
  or `cpu_atom`, for a hybrid layout whose leaf 0x23 answers, and for vendor
  OTHER, which covers Arm and every other non-x86 host.
- `usableCounters` takes one counter off for the NMI watchdog when no fixed
  cycles counter is free.
- The registers and the watchdog value are read one time per process.
- The seam of PC-1 exists, and no group-open probe exists.
- The probe's own measurement runs on the host named in the pull request body:
  `kernel.perf_event_paranoid` at 2 answers every shape the probe uses, so the
  probe needs no wider gate (F-1 to F-5).

## Roadmap position

N6-P sits inside N6, the multi-pass planning lane of specs/024. N6 packs a plan
into passes when a device states a limit and multiplexes when it does not. This
lane gives the silent devices a limit and states where each limit came from. It
adds no counter path, no formula grammar, no new event and no new option.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Measure a full plan on a host whose decode is silent (Priority: P1)

A user runs a benchmark on an Arm board, or on an Intel hybrid part that answers
leaf 0x23. Today the tool cannot say how many counters those devices hold, so it
multiplexes every event, and each event runs for a slice of the window. With the
probe, the tool opens one group of copies of one event, finds the largest group
that runs at full rate, and plans packed passes instead. The user reads
complete-per-pass numbers where the same run used to report a multiplex ratio
below 1.

**Why this priority**: This is the whole value of the item. Without it, every
device the decode cannot answer keeps multiplexing, and no other story has
anything to report.

**Independent Test**: Script the seam so a group of 6 copies runs at full rate
and a group of 7 does not, ask for the limit of an `armv8_` device, and read 6
back from `Object::programmableCounters()`. The story is complete on its own:
the planner already packs passes for a device that states a limit (PC-2).

**Acceptance Scenarios**:

1. **Given** a core device whose decode answers no value and a seam that lets a
   group of 6 copies run at full rate, **When** the system needs that device's
   limit, **Then** `Object::programmableCounters()` answers 6.
2. **Given** the same scripted device, **When** the same process asks for the
   limit a second time, **Then** the seam's opener answers one further time, not
   once per ask.
3. **Given** a hybrid pair of scripted core devices with different limits,
   **When** the system plans one run over both, **Then** each device answers its
   own limit and each plan uses its own device's number.
4. **Given** a device the decode answers, **When** the system needs its limit,
   **Then** the probe opener answers no call at all and the decoded limit stands.

---

### User Story 2 - Check a probe answer against the decode (Priority: P2)

A maintainer on a host that both the decode and the probe can answer wants the
two figures side by side, on one command, with no code edit. The example prints
each core device, its limit, and where the limit came from, and it prints the
probe figure beside the decoded one for check mode. On a host with no
`perf_event_open` the same example prints `no limit` and exits 0.

**Why this priority**: It is the acceptance instrument for this lane and the
next host's first check, but a user who never checks still gets US1's value.

**Independent Test**: Build and run `example/docs/counter_probe.cpp` and compare
its output with `example/docs/counter_probe.cpp.expected`; then run it on a host
that refuses `perf_event_open` and read `no limit`.

**Acceptance Scenarios**:

1. **Given** a host whose decode answers a core device, **When** the example
   runs, **Then** the device's line names the limit, the source `decode`, and
   the probe figure beside it.
2. **Given** a host where the decode and the probe both answer a device and the
   watchdog rule takes nothing, **When** the example runs, **Then** the two
   figures are equal, and the record of the acceptance run states both.
3. **Given** a host that refuses every `perf_event_open`, **When** the example
   runs, **Then** it prints `no limit` for each core device, prints no error,
   and exits 0.

---

### User Story 3 - Read where a limit came from (Priority: P3)

A reader of a plan or of a written report sees a counter limit in the text. That
line now says whether the figure came from the decode or from the probe, so a
reader can weigh a probed figure and a decoded figure differently without
knowing this feature.

**Why this priority**: It changes no measurement. It makes each figure
attributable, which is what makes a probed number safe to publish.

**Independent Test**: Read the plan text of a scripted decode run and a scripted
probe run, then read the one new key of the written record of both runs back
through the report reader.

**Acceptance Scenarios**:

1. **Given** a device whose limit came from the decode, **When** the run's
   written record names the limit, **Then** that entry names `decode`, and the
   console prints no new line.
2. **Given** a device whose limit came from the probe, **When** the run's
   written record names the limit, **Then** that entry names `probe`, and the
   console line of FR-050 names the device, the figure and that word.
3. **Given** a document older than this key, **When** the report reader loads
   it, **Then** the run loads, and the reader answers no source. It guesses
   nothing.

---

### User Story 4 - Say nothing on a host that cannot answer (Priority: P3)

A user on a gated host, or inside a container that refuses `perf_event_open`,
gets the same run as today. The probe tries, learns nothing, and the planner
multiplexes. Nothing fails, nothing exits non-zero, and no line tells the user
to change a kernel setting.

**Why this priority**: The probe must not turn a working run into a broken one.
This story is the reason US1 is safe to ship.

**Independent Test**: Script the seam so every open answers `EACCES`, and script
it so the first open of size 1 answers a refusal; run the same plan in both
shapes and read no limit, multiplex, and exit 0.

**Acceptance Scenarios**:

1. **Given** a scripted host where every open answers `EACCES`, **When** the
   system needs a probed limit, **Then** the device answers no limit and the
   planner multiplexes.
2. **Given** a scripted host where a group of one copy does not run at full
   rate, **When** the system needs a probed limit, **Then** the device answers no
   limit.
3. **Given** any scripted refusal shape, **When** the run ends, **Then** every
   descriptor the probe opened stands closed.

### Edge Cases

- **A size fits and a later size refuses the open.** The search stops at the
  first size that does not fit, whether it refused the open or ran short, and the
  answer is the size before it.
- **Every size up to the cap fits.** The search reaches 32 (D-4), finds no
  refusal, and answers no limit; a cap-wide answer is not a limit of 32.
- **A group of one copy does not run at full rate.** The answer is no limit.
  There is no size below 1 to report.
- **A device lists no event the probe can use (D-9).** No probe runs, and the
  device answers no limit.
- **The read answers a short buffer.** The size counts as not fitting, and the
  answer is the size before it.
- **The kernel grants an oversize group.** F-1 and F-2 hold on the measured
  host: the open answers 0 and the group never runs, so `time_running` against
  `time_enabled` settles it. A host that answers `EINVAL` takes the same stop
  rule, so both readings of the kernel agree here.
- **The group runs at full rate at every size the kernel schedules.** The
  `time_enabled` of a group the kernel never schedules still grows, so a probe
  that compared `time_running` with zero would call an unscheduled group a fit;
  F-3's equality is what keeps that reading out.
- **The watchdog is on.** The decoded path keeps today's deduction (PC-2), and
  the probed value takes none (D-8), because the probe measured what the kernel
  grants with the watchdog already holding a counter.
- **A device is not a core device.** `amd_l3`, `amd_df` and every uncore device
  answer as they do today (D-1).
- **Vendor OTHER on a device named `cpu`.** The probe is about the device, not
  the vendor: a core device with no decoded value is probed, and the event rule
  D-9 decides whether it can be.
- **A second plan in the same process.** The first answer stands for the whole
  process, so a plan that asks again pays nothing (D-6).
- **A probe that answers 0.** No group of one copy runs at full rate, so 0 is
  not a limit a plan may pack to; the device answers no limit.

## Requirements *(mandatory)*

### The measurement (M)

- **FR-001**: The lane SHALL carry `specs/041-counter-probe/tools/group_probe.c`,
  a stand-alone program that opens one group of k copies of one
  general-purpose event and prints, for each k from 1 to its cap, the open
  result, `errno`, `time_enabled`, `time_running`, and whether the group ran
  fully. *Verifier: the program builds with a C compiler and no project header,
  and the table in this spec carries its output.*
- **FR-002**: The program SHALL report the size where a group open first
  refuses, and SHALL state that no size refused when none did. *Verifier: Table 2
  of this spec.*
- **FR-003**: The program SHALL report the cost of the linear search the probe
  performs: sizes opened, opens, enables, reads, closes, and wall time.
  *Verifier: Table 3 of this spec.*
- **FR-004**: The lane SHALL state the measured host, kernel,
  `kernel.perf_event_paranoid`, `kernel.nmi_watchdog`, the CPUID words the
  program reads, and the date, beside the table it quotes. *Verifier: the record
  at `specs/041-counter-probe/results/` names each.*
- **FR-005**: The requirements below SHALL hold for both kernel shapes: the one
  that grants an oversize group and never schedules it (F-1, F-2), and the one
  that answers `EINVAL` for an oversize open. *Verifier: two seam tests, one per
  shape.*

### The probe (P)

- **FR-010**: The counters archive SHALL provide one probe function that takes
  an options struct and the device, and answers the count of programmable
  counters the kernel grants that device, or no value. No parameter of it SHALL
  be a bare `bool`.
- **FR-011**: The probe SHALL open every event through the kernel-call seam of
  PC-1, and the lane SHALL add no second seam. *Verifier: the seam tests drive
  the probe with a scripted opener, and no probe path calls `syscall` or the
  C library's `perf_event_open` directly.*
- **FR-012**: The probe SHALL build one group per size: the first open leads the
  group, and every later copy names that leader as its `group_fd`.
  *Verifier: a scripted seam that records the `group_fd` of each open.*
- **FR-013**: The probe SHALL set `exclude_kernel`, SHALL start the group
  disabled, and SHALL NOT set `pinned`. The group SHALL bind to the calling
  thread (`pid` 0) on any cpu (`cpu` -1), and the probe SHALL NOT change the
  affinity of the calling thread. The attribute names the device: `type` is the
  number that device publishes at
  `/sys/bus/event_source/devices/<dev>/type`, and `config` is that device's own
  words for the event FR-033 names. F-1 to F-3 were read through the generic
  seat (`PERF_TYPE_HARDWARE`), and
  `results/requirement-1-shipped-shape.txt` reads the device pair on the
  acceptance host and answers the same 6. *Verifier: a scripted seam that
  records the attribute word and the `pid` and `cpu` of each open, beside the
  `counters_limit_fill_test` row that reads the device's type and words.*
- **FR-013a**: A device the calling thread never runs on SHALL answer no limit
  and SHALL NOT answer a figure, because its group is never scheduled and the fit
  test of F-3 fails. The lane SHALL state that limit in the docs page of FR-060, and the
  Intel hybrid row of SC-010 SHALL carry it as the reason it stands pending.
- **FR-014**: The probe SHALL enable the group, read the group leader once, and
  settle the size from that one read: a size fits when
  `time_running == time_enabled` and `time_enabled` is past zero. The probe SHALL
  NOT decide a size from the open result alone. *Verifier: a scripted open that
  answers 0 with a short run, which the probe SHALL call not fitting (F-1).*
- **FR-015**: The probe SHALL try sizes from 1 upward and SHALL stop at the first
  size that does not fit, whether that size refused its open, answered a short
  read, or ran short. The answer SHALL be the size before it.
- **FR-016**: The probe SHALL try no size above 32 (D-4). When every size to the
  cap fits, the probe SHALL answer no value, and no run SHALL report a limit of
  32 from that search.
- **FR-017**: When size 1 does not fit, the probe SHALL answer no value.
- **FR-018**: When an open answers `EACCES` or `EPERM`, the probe SHALL stop and
  answer no value, and the planner SHALL multiplex as it does today. *Verifier:
  the seam test of US4 scenario 1.*
- **FR-019**: The probe SHALL close every descriptor it opened on every path,
  including a refused open, a short read, a permission refusal, and the stop at
  the first size that does not fit. *Verifier: the descriptor-count check the
  seam tests already keep, applied to each probe case.*
- **FR-020**: The probe SHALL take no counter off for the NMI watchdog (D-8): its
  answer is what the kernel granted, with the watchdog already holding what it
  holds. *Verifier: `counters_limit_fill_test` stands the watchdog at 1 and
  reads a probed figure unchanged, beside the row that reads a decoded figure
  lose one counter. No seam hook stands between the two: `usableCounters` is a
  pure function of its arguments, and the probed arm never calls it.*
- **FR-021**: The probe SHALL sleep, spin and read no clock (F-3).
  *Verifier: the probe path calls no timer function, and the units hold no sleep.*
- **FR-022**: The probe SHALL answer one time per device per process, at the
  first catalog open of that process: the provider settles the answer inside the
  pass that publishes the limit, and every later read of that device's limit sees
  the settled answer with no further open (D-6). A process that opens the catalog
  and never plans a metric pays that probe, and the Budget note and FR-060 state
  its worst case. *Verifier: a scripted opener counts its calls across two
  enumerations and two plans of one process and answers one probe run per device.*

### The device and the event (E)

- **FR-030**: The probe SHALL run for a device only where the decode answers no
  value for it (D-1). A device the decode answers SHALL pay no open at all.
  *Verifier: the seam test of US1 scenario 4.*
- **FR-031**: The devices the probe may run for are `cpu`, `cpu_core`,
  `cpu_atom`, and a device part that starts with `armv8_` — the prefix rule
  `source/harness/hunt.cpp:44` already keeps. `limitForDevice` answers no value
  for all four today, so this rule adds the Arm prefix to the probe's own gate
  and moves no decode rule.
- **FR-032**: The probe SHALL NOT run for an uncore device such as `amd_l3` or
  `amd_df`, and such a device SHALL answer no value as it does today (D-1).
- **FR-033**: The event SHALL be one general-purpose event of the device, held as
  data the code reads. It SHALL NOT sit inside a formula. The x86 row names
  `branch-instructions`: the provider reads the words of that entry from the
  device's own `events/` listing and opens them on the device's own type number,
  so one rule serves every x86 core PMU and no encoding sits inside a formula
  (D-9, clarified at round 2). The generic UAPI constant is not the answer:
  `PERF_COUNT_HW_BRANCH_INSTRUCTIONS` is 4 in an id space the device's own
  encoding does not share (`/usr/include/linux/perf_event.h:71-72`), and a
  generic open asks the PMU serving the calling thread. It cannot ask the device
  it was asked about. The Arm row stands empty at this audit point: D-5 names no Arm host, the
  tree holds no Arm event table, and the acceptance host lists no `armv8_*` device.
  An `armv8_*` device therefore answers no event and no limit (FR-034), SC-010's Arm
  row stays pending, and naming that event needs the host that can read it.
- **FR-034**: When a device lists no event the probe may use, the probe SHALL NOT
  run, and the device SHALL answer no value.

### The published limit and the planner (R)

- **FR-040**: A probed count SHALL fill the same field the decode fills,
  `ObjectSeed::programmableCounters`
  (`include/speedgun-ng/counters_provider.hpp:78`), and
  `Object::programmableCounters()` (`include/speedgun-ng/counters_system.hpp:85`)
  SHALL answer it with no change to its type or shape.
- **FR-040a**: Each limit SHALL carry its source beside it from the fill site to
  the reader: the provider records the source with the limit it publishes, and
  `objectLimitSource(objectPath)` answers `LimitSource::DECODE` for a value the
  decode produced, `LimitSource::PROBE` for a value the probe produced, and
  `LimitSource::NONE` for a path that publishes no limit and for a path no
  provider recorded (D-3). The fill records on every arm, and a record of `NONE`
  erases, so a stale finder name never stands beside an absent figure.
  `objectLimitSource` SHALL answer `None` whenever
  `Object::programmableCounters()` answers no value for that path.
- **FR-041**: A plan for a device with a probed limit SHALL pack passes the way
  it packs them for a decoded limit, and a run SHALL report the method it used.
  *Verifier: the scripted pair of US1: the same leaf set plans one pass packed
  with a limit and multiplexes without one.*
- **FR-042**: A probed value SHALL NOT pass through the watchdog deduction that
  `usableCounters` applies to a decoded value (D-8), and a decoded value SHALL
  keep that deduction unchanged.
- **FR-043**: A probe that answers no value SHALL leave the run on today's path:
  no new refusal, no new exit status, no new console line beyond the one
  disclosure of FR-050, and multiplex where the run multiplexes today.
- **FR-044**: A device whose decoded limit is 1 SHALL keep today's answer of no
  value (`publishMinusOne`, `counter_limits.cpp:122-129`), and this rule SHALL
  not run on a probed value, which FR-017 already settles.
- **FR-045**: A probed count SHALL reach every reader of the limit as a decoded
  count of the same number reaches it, with no rule of its own.
  `settleMethod` (`source/harness/pass_plan.cpp:120-146`), `planPasses`
  (`source/harness/pass_plan.cpp:148-257`) and the hunt selection refusal
  (`source/harness/hunt_cli.cpp:712-738`) read the one figure and no source, so a
  small probed count can raise a hunt refusal the way a small decoded count
  already does. Where a probed limit raises a refusal the acceptance run records,
  the record SHALL name it. *Verifier: one test drives the same count through the
  decoded path and the probed path and reads one plan from both.*

### Disclosure (D)

- **FR-050**: A run whose needed devices include a limit that came from the probe
  SHALL print one console line naming that device, its count, and the word
  `probe`, once for each such device within one gathering of the limits. One
  `Runner::run` call gathers once, so one instance needing one probed device answers
  one line, the case the verifier scripts. A hunt gathers on each of its stages and
  passes, and a top-down companion gathers on a pass of its own, so a hunt SHALL
  print the line again in each gathering. No console line of the audit point SHALL
  change its bytes, the hunt refusal text of
  `source/harness/detail/hunt_selection.hpp:71-79` included, which
  `specs/029-hunter-core/contracts/hunt-cli.md` pins (D-3). A run whose
  needed devices all came from the decode prints nothing new, and the written
  record of FR-051 carries the source of every published limit, console line or
  no console line. *Verifier: a harness unit scripts a probed run through the
  kernel seam, a scripted device tree and the real provider
  (`installKernelCalls`, `devicesRoot`), so no fake answers a source and no
  exported layout moves (clarified at round 2).*
  *Verifier: a scripted run captures its console text and reads the new line and
  the unchanged pinned line.*
- **FR-051**: The written report SHALL carry one new run-level record,
  `counterLimits`, that lists one entry per needed device with its device name,
  its limit, and its source word `decode`, `probe` or `none`. The report of the
  audit point holds no counter limit anywhere, so the record opens that ground
  rather than joining a text that exists. `kSchemaVersion` SHALL move from 1.14 to
  1.15 (`source/harness/json_report.cpp:49`),
  `docs/schema/report-v1.json` SHALL carry the record in its `x-order` and
  `properties`, and no key of the record SHALL stand in `required`.
- **FR-052**: A document older than minor 1.15 SHALL load, and its reader SHALL
  answer no `counterLimits` entry rather than a guessed one. The read-back answer
  SHALL stay inside the harness and SHALL NOT join an exported layout of
  `include/speedgun-ng/benchmark.hpp`, so the harness `SOVERSION` stays 21
  (D-3). *Verifier: a writer and reader round-trip over both a 1.15 document and a
  1.14 document, and the `soVersionNumbers` test.*
- **FR-053**: The source SHALL travel over one exported free function
  (`objectLimitSource`), and no existing exported layout SHALL move, so the counters archive `SOVERSION` rises from 6 to 7
  for that one accessor and the harness `SOVERSION` stays 21 (D-3). The ledger
  comment at `CMakeLists.txt:131-133` states the rule that moves the hand-kept
  number with a public header change, and this change joins a symbol as well. The
  record of the branch SHALL state the check of both `SOVERSION` figures against
  the base values in `Spec identity and run-time binding`, and the commit body
  SHALL name the reason one accessor moved the archive number.

### Documentation, example and the main page (X)

- **FR-060**: The N6 section of `docs/pages/harness.md` (near line 1577) SHALL
  state how a limit is found: the decode first, the probe where the decode is
  silent, what a probe answer means, what it costs, and that the probe takes no
  watchdog counter off.
- **FR-061**: The lane SHALL add `example/docs/counter_probe.cpp` with its
  `.expected` file named `example/docs/counter_probe.expected`, registered in
  `example/CMakeLists.txt`, that prints each core device, its limit, and where
  the limit came from, or `no limit` where no `perf_event_open` answers.
  `tools/report_shape.py:313` builds the golden name from the target, so the
  reachable spelling carries no `.cpp`. The `.expected` file SHALL record the
  acceptance host's run, and the lane SHALL register no `report_shape_` fold test for it, so
  a runner that refuses `perf_event_open` cannot fail a byte compare it was never
  asked to win; three of the twelve goldens of `example/docs/` already stand
  unregistered.
- **FR-062**: The example SHALL exit 0 on a host that refuses every
  `perf_event_open`, and SHALL print no instruction about kernel settings.
- **FR-063**: `README.md` SHALL gain at most 3 lines (1001 at branch time, 1004
  is the cap of ruling G-7), or none. The record SHALL state the line count at
  both ends.
- **FR-064**: `tools/docs/example_drift.py` SHALL pass with the new example
  registered.

### Tests and discipline (T)

- **FR-070**: Each unit SHALL stand with the code it covers, and each SHALL read
  red at the base commit before its code lands (Principle X, specs/046 FR-029).
- **FR-071**: The seam tests SHALL cover: an `armv8_` device with a scripted
  limit of 6; a hybrid `cpu_core` and `cpu_atom` pair with different limits; an
  oversize open that refuses with `EINVAL`; an oversize open that answers 0 and
  never runs (F-2); a permission refusal; a size-1 refusal; the one-probe-per-
  device cache; and the rule that a device the decode answers pays no open.
- **FR-072**: No unit SHALL sleep, read a clock, or name a cpu that
  `sched_getaffinity` did not offer, and the suite SHALL pass under
  `taskset -c 0,1`.
- **FR-073**: Real opens SHALL stand only in `tools/group_probe.c` and
  `example/docs/counter_probe.cpp`, and in no unit.
- **FR-074**: Line coverage SHALL read 100 percent, the branch shortfall SHALL
  read 21 or less with the figure stated, and the `LCOV_EXCL` marker count SHALL
  stay at 84 or below with no new marker, no `--omit-lines`, no
  `--ignore-errors` and no `--remove`, and no code shaped only for coverage.
- **FR-075**: Every gate SHALL pass on the squashed head: `cmake -P
  cmake/lint.cmake`, `cmake -P cmake/spell.cmake`, `tools/docs/example_drift.py`,
  `tools/prose/prose_gate.py --base origin/master`, and
  `tools/dbc/dbc_pair_gate.py`.
- **FR-076**: The lane SHALL change no file under CI, no gate configuration, and
  no file that rules G-23 reserves for the squash step; it SHALL add no merge
  commit and rebase only.

## Clarifications

### Session 2026-11-05, round 1

A reviewer read the spec against the tree at `858c55d9` and named five openings with
no owner. D-1 to D-6 stayed out of scope, and each of the five has one ruling now.

- Q: Which channel carries a limit's source out of the counters archive, given the
  harness and the example link only the two public archives? → A: One exported free
  function over a closed enumeration, and the counters archive `SOVERSION` moves from
  6 to 7. The ledger comment at `CMakeLists.txt:131-133` states the rule that moves
  the hand-kept number with a public header change where no layout moved and no
  symbol joined; this change joins a symbol as well, so the rule binds twice. The
  reading that kept the number at 6, on the `counters::detail::compileCore` precedent
  of `include/speedgun-ng/counters_measurement.hpp:610`, stands rejected against that
  ledger sentence (D-3, FR-053).
- Q: Which text and which record carry the limit figure with its source, given the
  report of the audit point holds no counter limit anywhere? → A: One new run-level
  record `counterLimits`, one entry per needed device, and one new console line for a
  run whose needed devices include a probed limit. No existing console text moves its
  bytes, the hunt refusal line of
  `source/harness/detail/hunt_selection.hpp:71-79` included, and the read-back stays
  inside the harness, so the harness `SOVERSION` stays 21 (FR-050, FR-051, FR-052).
- Q: Which pid and cpu does the probe's group bind to? → A: `pid` 0 and `cpu` -1, the
  shape F-1 to F-3 measured at `perf_event_paranoid` 2. A system-wide open needs the
  value -1 for that control, so it answers nothing on the acceptance host, and pinning
  the calling thread would change the state of the measured process. FR-013a states
  the limit of the chosen shape: a device the calling thread never runs on answers no
  limit, and SC-010's hybrid row names it (FR-013, FR-013a, SC-010).
- Q: When does the probe pay, given D-6 names the first plan and the limit can only
  enter the published record at the provider's enumerate? → A: At the first catalog
  open of the process, once per device, inside that enumerate pass. No lazy channel
  exists without an edit to `source/counters/system.cpp`, which PC-5 freezes. The
  Budget note names the worst case, and the docs line of FR-060 states it (D-6,
  FR-022, SC-005).
- Q: Which host does the example's golden describe, given its output is
  host-dependent by FR-061 and FR-062? → A: The acceptance host, and the lane
  registers no `report_shape_` fold test for it, so a runner that refuses
  `perf_event_open` cannot fail a byte compare it never asked to win. Three of the
  twelve goldens of `example/docs/` already stand unregistered, and the compare rides
  the acceptance record under ruling G-20 (FR-061, SC-007).

### Session 2026-11-05, round 2

- Q: The attribute the probe composes names one device, and the attribute the lane measured names none. Which does the probe send? → A: The device's own. `type` is the number `/sys/bus/event_source/devices/<dev>/type` publishes, and `config` is the words of that device's own `branch-instructions` entry. `PERF_TYPE_HARDWARE` is 0 and `PERF_COUNT_HW_BRANCH_INSTRUCTIONS` is 4, and a generic open asks the PMU serving the calling thread: a search of `cpu_atom` from a thread on a core would answer the core's figure, and FR-045 seats passes on that figure. The event rule therefore answers a name, and the device's listing supplies the encoding. `results/requirement-1-shipped-shape.txt` reads the pair on the acceptance host and answers 6, the same bound the generic seat answered (FR-013, FR-033, D-9).
- Q: Who records a limit's source for a run whose limit no real provider produced, given the harness units build their runs with `FakeProvider`? → A: The harness unit that scripts a probed run registers a scripted device tree under the real provider, reads the figure that provider published beside the finder the catalog holds, and names the finder of its scenario through the internal accessor `recordLimitSource` where a figure stands. A scripted device publishes a `type` number and event words of the fixture's own, so a real `perf_event_open` against it lands on whatever PMU the host owns at that number, and no scripted device tree can make the search answer a chosen figure at the harness layer on an arbitrary host. The pair a scenario seats, a published figure beside a source word, is the pair the accessor's postcondition allows, and where the host publishes no figure the scenario seats nothing and asserts that pairing rule. The counters layer is where a finder is earned for real: `counters_limit_fill_test` and `counters_group_probe_test` drive the search through the kernel seam behind a scripted kernel, and `counters_limit_source_test` covers the seat the report reads. `FakeProvider` gains no member, so no exported layout moves and this lane's archive number rises for one accessor alone (FR-050, FR-053, D-3a).
- Q: Does a decoded limit's source reach any text a user reads, given SC-006 asks for the source beside every published limit and FR-050 prints only for a probed one? → A: The written record carries every published limit with its source, and the console line stays probe-only. A decoded run prints no new byte on any host of the CI matrix, so no golden, no pinned refusal text and no pass report moves (FR-050, FR-051, SC-006).
- Q: What does check mode measure, given FR-030 keeps the library from probing a device the decode answered and the example links the public headers alone? → A: The example owns a small search of its own, plainly marked as such, and SC-003 states what it compares. The acceptance record adds a reading taken through the attribute the shipped probe composes, so a host figure stands behind the composed attribute itself (FR-061, FR-073, SC-003).
- Q: Which cost figure does the lane publish, given five places read 0.078 ms and no artifact holds it? → A: The shipped attribute's reading, with its spread, from `results/requirement-1-shipped-shape.txt`. The 0.078 ms figure is dropped. The wall time of one search follows the scheduler, so the lane states a band and the cap of D-4 stays the number that bounds the worst case (F-5, SC-009, FR-060).
- Q: A probed value that reaches no figure leaves an earlier record standing beside it. What does the reader answer? → A: The fill records on every arm, and `NONE` erases. The reader's answer belongs to the state the catalog holds now (FR-040a, FR-043).

- Q: Which event does an `armv8_*` device carry, given the tree holds no Arm event
  table, D-5 names no Arm host, and the acceptance host lists no `armv8_*` device? → A:
  None at this audit point. `isProbeCoreDevice` answers true for the name, so the device
  rule is ready, and `probeEventForDevice` answers no event, so FR-034 keeps the device
  from paying an open. Naming the event needs a host that can list it, and SC-010's Arm
  row stays pending with that reason (FR-033, FR-034, SC-010, round 1 addendum).

The reviewer checked and left standing: a probed value of 1 against `publishMinusOne`
(FR-044), a cap-wide fit (FR-016), the Arm host rule (D-5), the option rule (D-2),
the watchdog rule (D-8), the tie inside the Arm event list (FR-033), the project
version, and the one-probe claim (SC-005, FR-022).

## Key Entities

- **Counter limit**: how many programmable counters one context may hold open at
  once on one device. It carries a count and a source, `decode` or `probe`.
- **Probe group**: one group of k copies of one general-purpose event, opened to
  ask the kernel one question. It exists for the length of one size of one probe.
- **Fit**: one size's answer. The group ran for all of the window it was enabled
  for. F-3 states the reading, and one enabled read settles it.
- **Device**: a countable object the provider enumerated: `cpu`, `cpu_core`,
  `cpu_atom`, an `armv8_` device, or an uncore object the probe leaves alone.
- **Watchdog holding**: the counter the NMI watchdog keeps, which the decoded path
  takes off and the probed path finds already taken.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With a scripted device that runs a group of 6 copies at full rate
  and not 7, the published limit reads 6, and the same leaf set plans packed
  passes where it multiplexes with no limit.
- **SC-002**: A device the decode answers pays 0 probe opens, counted at the
  seam.
- **SC-003**: On the host named in the pull request body, the check-mode run names
  the decoded figure and a searched figure for `cpu`, and the two are equal
  (F-4 on the measured part: 6 and 6). The example runs its own search, because
  FR-030 keeps the library from probing a device its decode answered, and
  `results/requirement-1-shipped-shape.txt` carries the same equality read
  through the attribute the shipped probe composes (clarified at round 2).
- **SC-004**: For every scripted refusal shape (`EACCES`, a size-1 short run, a
  short read, and a cap-wide fit) the device answers no limit, the run multiplexes
  as it does today, the exit status is unchanged, and 0 descriptors stay open.
- **SC-005**: Two plans of one process that both need one device's limit pay 1
  probe for that device.
- **SC-006**: 100 percent of published limits name their source in the written
  record, and a document older than minor 1.15 loads and answers no source. The
  console line of FR-050 prints for a probed limit only, so a decoded run reads
  its source from the record and prints no new byte (clarified at round 2).
- **SC-007**: The example builds, runs, and prints `no limit` and exit 0 where
  `perf_event_open` refuses. Its match against the `.expected` file of FR-061 is
  an acceptance reading on the host that recorded it (ruling G-20), and no
  registered fold test owns that compare.
- **SC-008**: Line coverage reads 100 percent, the branch shortfall reads 21 or
  less with its figure stated, the exclusion marker count reads 84 or less, and
  `README.md` reads 1004 lines or less.
- **SC-009**: On the measured host the linear search costs 28 opens and answers
  inside the band `results/requirement-1-shipped-shape.txt` states, and no shape
  inside the cap costs more than 528 opens (1 + 2 + … + 32).
- **SC-010**: Four rows have no host: Intel non-hybrid check-mode equality, Arm
  `armv8_*`, Intel hybrid with leaf 0x23 answered, and AMD Zen 4. They stand as
  pending rows with no hardware claim, and their acceptance is the seam tests of
  FR-071. The Intel hybrid row also carries the device-shape limit of FR-013a: a
  device the calling thread never runs on answers no limit, so a hybrid check
  needs a host whose threads reach the device the row asks about.

## Constraints

- Constitution I, II, VI, VII, VIII, X and XI bind in full: the standard-first
  P0 to P3 model, one contract per interface, 100 percent line coverage with the
  recorded branch shortfall, the hard CI gates, anti-slop code discipline, and
  ASD-STE100 Simplified Technical English in every generated line.
- Options structs, not `bool` parameters (the constitution's interface rule and
  the request's hard rule).
- JSON only through the JSON libraries the project already vendors.
- No new vendored data, no new runtime dependency, no path lookup at run time
  beyond what the provider already reads.
- No sleep, no clock read and no invented cpu in any unit; `sched_getaffinity`
  names the cpus, and the suite passes under `taskset -c 0,1`.
- A bad option value exits 2 with one quoted line, and only if a later change
  adds an option or a file input. D-2 declines that change.

## Scope

In scope:

- `source/counters/linux_pmu/counter_limits.{cpp,hpp}`,
  `source/counters/linux_pmu/provider.cpp`, the kernel-call seam
  (`source/counters/linux_pmu/kernel_calls.{hpp,cpp}`), and
  `source/counters/detail/`.
- `include/speedgun-ng/counters_provider.hpp` only if a public field proves
  necessary; D-3 settles that it is not, so the expectation is no edit there.
- `test/source/counters_counter_limits_test.cpp`,
  `test/source/counters_objects_limit_test.cpp`, new units beside the code they
  cover, and `test/CMakeLists.txt`.
- `docs/pages/harness.md`, `docs/schema/report-v1.json`, `example/docs/`,
  `example/CMakeLists.txt`, `README.md`.
- `specs/041-counter-probe/`, which holds this spec, the measurement program, and
  the acceptance record.

Out of scope:

- `source/counters/system.cpp`, which another open change edits (PC-5).
- Uncore devices (D-1), any new command-line option (D-2), any new event,
  formula, catalog row or vendored table.
- CI, the sanitizer and coverage job shapes, and every gate configuration
  (ruling G-23).
- Any fix to the hunt abort this lane's host recorded on 2026-11-05. specs/046
  carries that reading in its AMD acceptance record, on the branch of pull request
  81. That reading stands with its own lane.

## Assumptions

- The kernel of the measured host is the kernel the acceptance names; a kernel
  that answers `EINVAL` for an oversize group takes the same stop rule (FR-005),
  so no requirement waits on that difference.
- The probed figure describes the device as the kernel grants it now, so a
  `cgroup` limit or a second perf user can move it; the disclosure of FR-050 is
  what makes that readable, and no requirement promises a stable figure across a
  change to either.
- `perf_event_paranoid` at 2 answers every shape the probe opens, which the
  measurement shows; a stricter host takes FR-018's path.
- The `armv8_` prefix rule of `source/harness/hunt.cpp:44` names the same devices
  the counters lane will probe, so one spelling rule serves both.
- No Arm hardware is available without the owner naming a host (D-5).

## Budget note

The probe is one search of at most 32 sizes, once per core device per process,
paid at the first catalog open of that process (D-6, FR-022). On the measured
host it costs 28 opens, 7 group enables, 7 reads and 28 closes inside the band
F-5 states (F-5, `results/requirement-1-shipped-shape.txt`). A
device the decode answers pays nothing (FR-030), and every host of the CI matrix
answers the decode, so the suite pays no probe. The cap of D-4 bounds one search
at 528 opens, so a host with three silent core devices pays at most 1584 opens
and 0 ms of sleep at its first catalog open: an Intel hybrid part is the shape
that can reach it. No path inside the cap reads a clock.

## Dependencies

- PC-1's seam (PR #70) and PC-2's published field are both on master, so this
  lane needs no other lane's work to start.
- The schema move of FR-051 touches `docs/schema/report-v1.json` and
  `kSchemaVersion`, both of which other open lanes may also move; the rebase rule
  of `Spec identity and run-time binding` is how a collision settles, and the
  next free minor at rebase time wins.
- The acceptance needs the same host class the measurement ran on, plus the
  pending hosts of SC-010 when the owner names them.

## Citations

- `specs/024-multipass-collection/spec.md` PC-3 and its research D-3: the
  oversize-group reading this lane measures (F-1 settles it).
- `specs/042-topdown` and `specs/046-hunter-topdown`: the vendor and device
  readings the probe's event rule relies on.
- `include/speedgun-ng/counters_provider.hpp:78`,
  `include/speedgun-ng/counters_system.hpp:85`,
  `source/counters/linux_pmu/provider.cpp:842-853`,
  `source/counters/linux_pmu/counter_limits.cpp:288-317`,
  `source/counters/linux_pmu/counter_limits.cpp:122-129`,
  `source/counters/linux_pmu/kernel_calls.hpp:59-109`,
  `include/speedgun-ng/counters_fake.hpp:169`,
  `source/harness/hunt.cpp:36-44`, `source/harness/json_report.cpp:49`.
- Owner rulings in force: G-7 (README cap 1004), G-8 (branch allowance 21),
  G-17 (no person name in the repository except `LICENSE`), G-20 (acceptance is
  local, and the pull request body names the host, the kernel, the counts and the
  head sha), G-23 (squash each lane into one commit before the merge and measure
  acceptance on that head; no CI change without the owner's approval).

## Open questions

The six open questions of the request are answered in `Decisions` (D-1 to D-6)
and closed. Two readings this lane does not settle, and states:

- Whether a `cgroup` limit should reach the disclosure of FR-050 as a third
  source. It stays out here: the probe reports what the kernel grants, and the
  plan text names how the figure was found.
- Whether an uncore device should carry a probed limit at all (D-1 defers it to
  its own item). No requirement here predicts what that item will answer.

## Constitution Checklist

- Main-page impact check (IV.1): yes, in a narrow form. The N6 section of
  `docs/pages/harness.md` states how a limit is found, and that statement changes
  (FR-060). `README.md` carries at most 3 new lines and may carry none
  (FR-063); the record states the count at both ends.
- Principle II: the probe function, its options struct and the settled-limit
  record each carry one contract, and `tools/dbc/dbc_pair_gate.py` answers at the
  head with no gap (FR-075).
- Principle VI: 100 percent line coverage, the branch shortfall figure stated, and
  no new exclusion marker (FR-074).
- Principle VII: the cost is measured and disclosed before the design settles
  (F-5, D-6, the budget note).
- Principle VIII: every hard gate clears on the squashed head (FR-075, G-23).
- Principle X: no code shaped for coverage, no speculative generalization, and
  each unit rides the code it covers (FR-070, FR-074).
- Principle XI: ASD-STE100 Simplified Technical English throughout, and the
  commit template of Pull Request Quality with `Refs: specs/041-counter-probe
  N6-P` and `Approved-by:` as the last two lines.


---

## Acceptance extension: Intel final acceptance (2026-11-07)

`specs/048-hwlat-probe` carries the identity, the changed-file list, the shared
constraints and the clarification rounds for this phase. This section adds the
counter-count part and nothing else.

### Requirements - the counter-count part (IA41-001 to IA41-002)

- **IA41-001**: The lane SHALL follow the acceptance commands, the tools and the
  `counter_probe` example of this spec. The lane SHALL measure the Intel
  non-hybrid core-device check on the Intel host: the probe limit beside the
  decoded usable limit, with the watchdog rule applied exactly once. The record
  SHALL give the pmu and event selection, `kernel.nmi_watchdog`, the permissions,
  the open and read results, and the source of the published count.
- **IA41-002**: A permission refusal or an unavailable pmu SHALL stand as blocked
  hardware acceptance and never as success. The lane SHALL add no command-line
  option and SHALL expose no raw event code. The Arm, Intel hybrid and Zen 4 rows
  SHALL stay pending. A completed row SHALL NOT reopen unless new evidence
  contradicts it.


## Acceptance closeout: counter-count evidence (2026-11-08)

`specs/048-hwlat-probe` carries the identity, the changed-file list, the shared
constraints and the clarification rounds for this phase. This section adds the
counter-count part and nothing else. It keeps IA41-001 and IA41-002 as they
stands.

### Requirements - the counter-count closeout (IP-001 to IP-002)

- **IP-001**: The lane SHALL repeat the counter example run and the decode
  against probe parity measurement at the measured head, with the tooling and the
  event selection the spec already approves. The record SHALL give the pmu name
  and type, `kernel.nmi_watchdog`, the published limit with the source the code
  reads, the number of opens and reads, and the comparison of the two limits at
  the permissions this host answers now. The lane SHALL apply the watchdog rule
  once, and a second application SHALL NOT stand as a correction.
- **IP-002**: The lane SHALL keep two statements apart: the two limits agree at
  the `perf_event_paranoid` value this host carries, and any parity a historical
  environment requires stands a separate claim. Where a parity row needs a kernel
  control changed, the row SHALL read BLOCKED with the control and the value an
  operator would set, and the lane SHALL weaken no control. This section reads
  G-31 as a boundary on new application support, so it bars no existing hardware
  test tool. The lane SHALL add no raw-code path to the application and SHALL
  publish no raw configuration word that a row does not need.

---

## Counter work in the merged-head closeout (2026-11-08)

`specs/048-hwlat-probe` carries the identity, the changed-file list, the shared
constraints and the clarification rounds for this phase. This section adds the
counter-count work at the merged base and nothing else. `IP-001` and `IP-002` keep
the text they carry above, and this section states what each one asks of a run on
the new base.

`specs/048-hwlat-probe/spec.md` holds three numbered ruling lists: the
2026-11-07 list, numbers 1 to 43, the 2026-11-08 closeout list, numbers 1 to 28,
and this phase's list, numbers 29 to 42, whose heading reads `Rulings - this
phase (29 to 42)`. One bare ruling number lands in more than one of them, so
every ruling cite names that file, its number, and the list that holds it. Every
citation in this section names its file, then the thing it points at: a
requirement id, a criterion id, a ruling number with its list, a heading, or the
first cell of a lane-table row of `specs/048-hwlat-probe/plan.md`. A numeric line
range is the last choice, and it stands only for source code and for a row of a
record, which carries no id. A record row range is re-read where that record now
stands before a row writes it.

One cite in the base text of this file needs a handle, and this section states it.
`D-3 Source of the limit (Q-3)` under `Decisions` cites
`specs/046-hunter-topdown/spec.md` with a numeric line range for the move that
raised the counters archive `SOVERSION` from 5 to 6. That range sits in text the
base head carries, so it keeps its text, and the handle that governs it reads
`D-8 Public surface` under `Decisions` of `specs/046-hunter-topdown/spec.md`, the
decision that names the accessor `D-16` and that `SOVERSION` rise.

### The base this work reads

| Line | Value at the check on 2026-11-08 |
|---|---|
| Base head | `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the master head that carries pull request #100 and pull request #99 |
| Earlier closeout base | `2d4d8c57975a86db6c77536f7090aba2bdaf6417`. The `Identity and binding` table of `Acceptance closeout: measured Intel evidence (2026-11-08)` of `specs/048-hwlat-probe/spec.md` carries it as that table's `Base head` row, and `Intel acceptance closeout at the merged head (2026-11-08)` of that file calls it the measured head of the earlier pass. `specs/041-counter-probe/results/acceptance-final-intel.md` names no such base anywhere: the only shas in that record are `615c0964547e4e9768b8e2f2d79991033a0eb5bd` at `:16` and `:17`, and `87d745d6c7e62d4da80198c689ddc7078f08ff3d` at `:77` |
| Branch | `fix/intel-acceptance-closeout`, rebased onto the base head |
| Host | corsairone, Intel Core i9-9920X (Skylake-X), non-hybrid, one socket |
| Tool surface | `counter_probe`, the example of this spec, built as `docs_counter_probe` (`example/CMakeLists.txt:93`) |
| Final head | two heads, and ruling 29 of this phase's list names each. The squash after the Pass 1 writes produces the head Pass 2 runs at and every figure names. The squash after the Pass 2 writes produces the head the pull request carries. No sha stands for either at this writing, because neither squash has run. `git rev-parse HEAD` in a tree built from the head Pass 2 runs at prints it, the record prints that short sha beside every console it ran, and each record names the head the pull request carries beside it. The tip of `fix/intel-acceptance-closeout` reads as the head that branch holds at run time, and a tip an earlier squash replaced answers no row here |

The text above this section asks for the run at "the measured head" in `IP-001`,
and it prints no sha beside that phrase. Three shas stand beside that label in the
tracked files:
`615c0964547e4e9768b8e2f2d79991033a0eb5bd` as the `Measured head` row of
`specs/041-counter-probe/results/acceptance-final-intel.md:17`,
`87d745d6c7e62d4da80198c689ddc7078f08ff3d` as the rerun heading of
`specs/041-counter-probe/results/acceptance-final-intel.md:77`, and
`2d4d8c57975a86db6c77536f7090aba2bdaf6417` as the `Base head` row of the
`Identity and binding` table of `Acceptance closeout: measured Intel evidence
(2026-11-08)` of `specs/048-hwlat-probe/spec.md`. This section asks for no run at
those three heads. Every figure it prints names the head whose run printed it, and
every
figure no run printed reads `no figure`.

### What repeats at the final head

- The example run repeats. Three consoles of `./build/example/docs_counter_probe`
  run at the final head, in a tree built from that head, each alone on an idle host
  with its load witness beside it, each printing its own limit line (IP-001;
  rulings 12, 13 and 17 of the 2026-11-08 closeout list of
  `specs/048-hwlat-probe/spec.md`). A figure from those consoles stands only where
  its witness line reports no other compiler, test, capture or trace process and a
  one-minute load average below 2.0, the test ruling 30 of this phase's ruling
  list of that file sets. Otherwise that figure is void, and the lane re-takes it
  alone.
- The decode against probe parity reading repeats from those same consoles, with
  the tooling and the event selection this spec already approves. The surface stays
  `counter_probe`. `probe_window` takes no raw event word and answers no row here
  (ruling 14 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md`).
- The watchdog rule applies once. `source/counters/linux_pmu/limit_fill.cpp:93`
  holds that one application, on the decoded figure. The search arm at
  `source/counters/linux_pmu/limit_fill.cpp:96-110` holds two calls,
  `probeEventNameForDevice` at `:103` and `searchedLimit` at `:104`, and it holds
  no watchdog deduction, because the figure a whole group answers already carries
  the counter the watchdog holds. A second application SHALL NOT stand written as a
  correction (IP-001).

### Requirements restated for the merged base (IP-001 to IP-002)

- **IP-001**: The lane SHALL repeat the counter example run and the decode against
  probe parity reading at the final head, with the tooling and the event selection
  this spec already approves. The record SHALL give the pmu name and the device
  `type` that head reads, `kernel.nmi_watchdog`, `kernel.perf_event_paranoid`, the
  published limit with the source word the code writes, the count of opens and the
  count of reads each labeled measured or derived, and the comparison of the two
  limits at the permissions that host answers at that head. The lane SHALL apply the
  watchdog rule once.
- **IP-002**: The lane SHALL keep two claims apart, and each claim SHALL name the
  id that owns it. The first claim reads: the decoded limit and the probed limit
  agree at the `perf_event_paranoid` value this host answers at the final head.
  SC-003 owns that claim, and the measured parity row answers it. The second
  claim reads: any parity a restricted historical environment asks for stands on
  its own row. `SC-010` owns that claim, as ruling 11 of the 2026-11-08 closeout
  list of `specs/048-hwlat-probe/spec.md` states, and the row names the two lines
  that ask for it: the precondition in the `Audit point` list, which reads
  `kernel.perf_event_paranoid` at 2, and the `Session 2026-11-05, round 1` answer
  of this spec on the `pid` and `cpu` the group binds to, as ruling 16 of that
  list asks. Neither row borrows the other figure. Where a row needs an operator
  to change a kernel control, that row SHALL read BLOCKED, SHALL carry no figure,
  and SHALL print the exact operator line and the exact restore line. That is the
  one blocked-row shape ruling 42 of this phase's ruling list of
  `specs/048-hwlat-probe/spec.md` sets, and it governs the `IP-002` sentence under
  `Requirements - the counter-count closeout (IP-001 to IP-002)` that asks only for
  the control and the value an operator would set. The before-value stands read at
  the identity step of the same run, the step the `Identity` row of the `Lane
  Table` of `specs/048-hwlat-probe/plan.md` names, and the blocked row cites that
  step for the value the host answered before the change, which the operator reads
  before the change. Those two lines name values, and they figure no measurement of
  the blocked row. Two lines stand in it:
  `sysctl -w kernel.perf_event_paranoid=2` for the restricted reading, and
  `sysctl -w kernel.perf_event_paranoid=-1` to restore this host's value.
  The control stands at `/proc/sys/kernel/perf_event_paranoid`. The standing
  blocked row of the record,
  `specs/041-counter-probe/results/acceptance-final-intel.md:146`, gives the
  control, the action and the missing right, and it prints no command line, so a
  dated row must add the two lines above. No elevation route stands in a lane run
  here: `sudo -n -l` answers `sudo: a password is required` at exit 1
  (`specs/041-counter-probe/results/acceptance-final-intel.md:18` and `:147`), and
  `pkexec` needs an interactive admin authorization, because
  `org.freedesktop.policykit.exec` defaults to `auth_admin` at
  `/usr/share/polkit-1/actions/org.freedesktop.policykit.policy:66-70`. The lane
  SHALL weaken no control. Raw event code support stays future work: the lane
  SHALL add no raw-code path to the application and SHALL publish no raw
  configuration word, and it reads G-31 as the boundary on new application support
  that leaves existing hardware test tools standing.

### Rows the final head must answer

No console has run at the final head yet, so every figure below reads no figure.
Each row names its command and the run that must answer it.

| Row | Command | Figure | Run that must answer it |
|---|---|---|---|
| Limit line, one per console | `./build/example/docs_counter_probe` | no figure | the three consoles of IP-001 at the final head |
| Parity of the two limits at the permissions of that head | the same command | no figure | the same three consoles |
| PMU name and the device `type` line (IP-001) | `cat /sys/devices/cpu/caps/pmu_name`, `cat /sys/bus/event_source/devices/cpu/type` | no figure | the reading line of the same pass |
| Opens and reads as two numbers, each labeled measured or derived (IP-001, ruling 13 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md`) | the same command | no figure | the same three consoles, beside the code lines that ruling names |
| `perf_event_paranoid` at that head | `cat /proc/sys/kernel/perf_event_paranoid` | no figure | the reading line of the same pass |
| `nmi_watchdog` at that head | `cat /proc/sys/kernel/nmi_watchdog` | no figure | the reading line of the same pass |
| Parity with the control at the historical value `2` (IP-002; `SC-010`) | BLOCKED row | no figure | no run in scope answers it; the row prints `sysctl -w kernel.perf_event_paranoid=2` and the restore line `sysctl -w kernel.perf_event_paranoid=-1`, and cites the identity reading of the same run for the value this host answered before the change, the shape ruling 42 of this phase's ruling list of `specs/048-hwlat-probe/spec.md` sets |

The standing record answers `-1` for `perf_event_paranoid` and `0` for
`nmi_watchdog` at
`specs/041-counter-probe/results/acceptance-final-intel.md:124` and `:125`. Those
two lines stand inside `Acceptance closeout corrections (2026-11-08)`, which opens
at `:100` of that record and names no head. Those two readings therefore carry no
head label, and they answer no row above. Each row above reads `no figure` until
the run named beside it answers it. The shas that record does print read
`615c0964` at `:16` and `:17`, and `87d745d6` at `:77`, and a reading taken at
either keeps that sha as its label.

### The interpreter the contract target runs

No manifest declares an interpreter for this host. Two declarations stand in the
tree, and both serve the hosted lane. `.github/workflows/ci.yml:1265-1267`
installs `python3-yaml` for the `dbc-gate` job, and
`tools/dbc/dbc_gate_common.py:63-70` answers exit 2 with
`dbc gates require PyYAML (install python3-yaml)` where an interpreter carries no
yaml module. The `dbc-gate` target runs its two gate scripts with the interpreter
the tree configured, and `cmake/dbc-gate.cmake:44` and `:49` name that
interpreter. The record SHALL print the configured interpreter beside the yaml
version that interpreter answers, and SHALL give the `dbc-gate` target its exit.
Where the configured interpreter answers no yaml version, the row SHALL read
BLOCKED with that interpreter named.

### Boundaries this base keeps

The Arm row, the Intel hybrid row and the Zen 4 row stay pending, and this base
moves none of them. This host is a non-hybrid Intel part, and no Arm host and no
Zen 4 host stand in reach (IA41-002, IP-002). No completed row this host measured
contradicts them. The example of this spec answers its own limit line, and the
application gains no raw event code in this phase. SC-010 owns the pending class,
and it names a fourth row: the Intel non-hybrid check-mode equality row. The three consoles and
the parity reading IP-001 asks for at the final head answer that row, and no
reading stands closed here today.