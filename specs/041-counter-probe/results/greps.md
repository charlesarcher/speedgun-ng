# The two hard greps (T059)

**Branch**: `041-counter-probe` | **Head**: `a0421f50` (the first two readings below also name the
head they were taken at) | **Host**: `gary` | **Date**: 2026-11-05
**Duty**: FR-073 (no unit performs a real `perf_event_open`) and FR-021 (no probe path
names a timer or a clock). Every output below is the literal answer of the command above
it, on the head tree, with the exit code stated.

## Grep 1: no unit performs a real `perf_event_open` (FR-073)

```console
$ git grep -l perf_event_open test/source/
test/source/counters_group_probe_test.cpp
test/source/counters_kernel_calls_test.cpp
test/source/counters_linux_pmu_seam_test.cpp
test/source/counters_pmu_provider_test.cpp
test/source/counters_pmu_test.cpp
test/source/counters_pmu_window_test.cpp
```

Exit 0. Six files named. Four of them carry the name in a comment that states the unit
needs no grant, one carries it in a scenario that asserts the refusal text names the
syscall, and one carries the real call.

The lane added four units under `test/source/`: `counters_group_probe_test.cpp`,
`counters_limit_fill_test.cpp`, `counters_limit_source_test.cpp`, and
`harness_limit_disclosure_test.cpp`. Exactly one of those four is named here:
`test/source/counters_group_probe_test.cpp`. **So the literal gate of tasks.md, "names no
file this lane added", does not hold at the head.** The named lane file holds a
sentence in its header comment. It calls nothing:

```console
$ git grep -n 'perf_event_open' test/source/counters_group_probe_test.cpp
test/source/counters_group_probe_test.cpp:8:// carry their counts on a runner whose `perf_event_open` is refused
```

The four lane units carry no syscall and no `SYS_perf` reference at all:

```console
$ git grep -n 'syscall\|SYS_perf' test/source/counters_group_probe_test.cpp \
    test/source/counters_limit_fill_test.cpp test/source/counters_limit_source_test.cpp \
    test/source/harness_limit_disclosure_test.cpp
$ echo $?
1
```

Exit 1, no line. The one real call in `test/source/` stands on master and the lane did
not touch it:

```console
$ git grep -n 'perf_event_open' test/source/counters_pmu_test.cpp
test/source/counters_pmu_test.cpp:177:      ::syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
test/source/counters_pmu_test.cpp:1230:// where only a granted perf_event_open reaches the path. A host that
```

`counters_pmu_test.cpp` stands in the lane's diff at no line at all: `git diff
--name-status 858c55d9..HEAD` does not list it. Its `hardwareEventProbe()` capability
probe is master's own, and it classifies grant, encoding refusal and permission refusal
before any scenario runs.

The other names are master's files, unchanged by the lane, and each is a comment except
the seam unit's refusal-text assertion:

```console
$ git grep -n 'perf_event_open' test/source/counters_kernel_calls_test.cpp test/source/counters_linux_pmu_seam_test.cpp test/source/counters_pmu_provider_test.cpp test/source/counters_pmu_window_test.cpp
test/source/counters_kernel_calls_test.cpp:6:// No unit test here needs a granted `perf_event_open`: the real set's
test/source/counters_linux_pmu_seam_test.cpp:8:// (perf_event_open, the mapping, the instruction itself) runs for real
test/source/counters_linux_pmu_seam_test.cpp:2197:// the kernel refuses: `perf_event_open` rejects a PMU type no kernel
test/source/counters_linux_pmu_seam_test.cpp:2209:  check(refusal.find("perf_event_open") != std::string::npos,
test/source/counters_linux_pmu_seam_test.cpp:3151:  // A runner whose perf_event_open is refused publishes no window. That
test/source/counters_pmu_provider_test.cpp:6:// probes every entry with one `perf_event_open` per kind, so a runner
test/source/counters_pmu_window_test.cpp:8:// is reached on every host, a runner whose `perf_event_open` is
```

**Reading at the later head, after one comment was reworded.** The call is made: the
comment was reworded, in `test: Name no syscall in the probe unit's comment`. The gate is
a grep over a name, a reviewer reads its output, and a file that names the syscall in
prose forces that reader to open the file to learn it calls nothing. The sentence now
names what the runner refuses, and the grep answers the base list:

```console
$ git grep -l perf_event_open test/source/
test/source/counters_kernel_calls_test.cpp
test/source/counters_linux_pmu_seam_test.cpp
test/source/counters_pmu_provider_test.cpp
test/source/counters_pmu_test.cpp
test/source/counters_pmu_window_test.cpp
```

Five files, all of them master's, none of them in this lane's diff.

**Verdict on FR-073**: held. No unit the lane added calls the syscall, and the group
probe answers every scenario through the scripted `KernelCalls` seam. The gap the first
reading exposed is a gate-shape fact worth keeping: a `-l` over a name cannot tell a call
from a sentence about a call, so the row states both the list and what each named line
holds.

## Grep 2: no probe path names a timer or a clock (FR-021)

```console
$ git grep -n 'nanosleep\|clock_gettime\|std::this_thread\|sleep' source/counters/linux_pmu/group_probe.cpp source/counters/linux_pmu/limit_fill.cpp
$ echo $?
1
```

Exit 1, no line. Both files are lane additions (`A` in `git diff --name-status`) and the
pattern covers the wider `sleep` substring, not only the three names of tasks.md, so no
timer, no clock, and no sleeping helper of any spelling stands in the probe path.

**Verdict on FR-021**: held. Empty answer, as tasks.md promises.

## Grep 3: the limit-source chain

```console
$ git grep -n 'recordLimitSource\|noteCounterLimit' -- source | sort
source/counters/detail/limit_source_record.hpp:45:auto recordLimitSource(std::string_view objectPath, LimitSource source) -> void;
source/counters/limit_source.cpp:35:auto detail::recordLimitSource(std::string_view objectPath,
source/counters/linux_pmu/limit_fill.cpp:118:  recordLimitSource(device.path, fill.source);
source/harness/detail/json_report.hpp:333:auto noteCounterLimit(CounterLimitEntry entry) -> void;
source/harness/json_report.cpp:1144:auto noteCounterLimit(CounterLimitEntry entry) -> void
source/harness/runner.cpp:1609:        detail::noteCounterLimit(
```

Exit 0, six lines. The shape is one writer per side and one call site per side: the
detail header declares `recordLimitSource`, `source/counters/limit_source.cpp` defines
it, `limit_fill.cpp:118` is its only caller; the report header declares
`noteCounterLimit`, `json_report.cpp:1144` defines it, `runner.cpp:1609` is its only
caller. No third name reaches either symbol, so the disclosure chain has one door.

## Follow-up at `a0421f50` (same session, after this file was written)

The branch moved one commit past the head this file was written for: `a0421f50 test: Name no
syscall in the probe unit's comment`, one line, in the header comment of
`test/source/counters_group_probe_test.cpp`. That line is the only match grep 1 had in a file
this lane added. Re-run at that commit:

```console
$ git grep -l perf_event_open test/source/
test/source/counters_kernel_calls_test.cpp
test/source/counters_linux_pmu_seam_test.cpp
test/source/counters_pmu_provider_test.cpp
test/source/counters_pmu_test.cpp
test/source/counters_pmu_window_test.cpp
$ git grep -n 'perf_event_open' test/source/counters_group_probe_test.cpp
$ echo $?
1
```

Five files named, none of them added by this lane, so the literal gate holds at `a0421f50`.
Greps 2 and 3 stand as quoted above: the commit touches no probe path and neither symbol of
the disclosure chain. The sentence in the unit's header now reads "a runner that refuses
every counter open", and the scripted-seam reading it carries is unchanged.
