# Contract: the group probe, the one question it asks, and its closed answer

**Feature**: specs/041-counter-probe | **Files**: `source/counters/detail/group_probe.hpp`
with `source/counters/linux_pmu/group_probe.cpp` | **Audit point**: `858c55d9`
**Spec**: FR-010 to FR-021, FR-033, F-1 to F-5, D-4, D-7, D-9, PC-1, PC-3

## What this contract covers

One function asks one question: how many copies of one event does one device schedule at
one time. This contract fixes the types, the one seam it calls, the fit test, the stop,
the cap, the closure rule, and the unit that proves each arm. The device and event rule
stands in `device-rule.md`, where the value is published stands in `provider-fill.md`,
and how a reader learns the source stands in `limit-source.md`.

## The types, as the archive declares them

```cpp
/// The largest size the search tries, which D-4 states and FR-016 caps.
inline constexpr std::size_t kGroupProbeMaxSize = 32;

/// Why one search stopped.
enum class GroupProbeStatus {
  OPEN_REFUSED,       ///< an open refused at this size
  RUNS_SHORT,         ///< this size ran below its enabled window (F-1)
  READ_SHORT,         ///< the read answered no full pair
  PERMISSION_REFUSED, ///< `EACCES` or `EPERM` refused an open (FR-018)
  CAP_REACHED,        ///< every size to 32 fit, so no bound is known (FR-016)
};

/// One search's input (FR-010).
struct GroupProbeOptions
{
  std::string deviceName;       ///< the device the answer is about
  std::uint32_t pmuType {};     ///< the device's own type number
  std::uint64_t eventConfig {};  ///< the words of the device's own entry the rule named
};

/// One search's answer.
struct GroupProbeAnswer
{
  std::optional<std::size_t> count {}; ///< the largest size that fit
  GroupProbeStatus status {GroupProbeStatus::OPEN_REFUSED};
};

[[nodiscard]] auto probeGroupLimit(const GroupProbeOptions& options) noexcept
    -> GroupProbeAnswer;
```

Five values, and no sixth. The search never sees an absent event: FR-034 keeps a device
with no event from reaching this function at all, so `NO_EVENT` would be a value no
caller can produce, and a value no call produces is a dark branch the coverage gate
counts. `startSize` and `maxSize` are absent for the same reason: D-4 fixes one cap of
32, and one search starts at 1, so a knob would state a freedom the spec withholds
(FR-016).

The status is disclosure, and the count is the decision. Every stop past size 1 answers
the same count whatever the reason was, and the reason is what the disclosure line of
FR-050 and the acceptance record state.

## The seam it calls, and the calls it never makes

Three fields of `KernelCalls`, each named once on one line
(`source/counters/linux_pmu/kernel_calls.hpp:61-77`):

| Call | Use |
|---|---|
| `openEvent(attr, 0, -1, groupFd, PERF_FLAG_FD_CLOEXEC)` | one copy of the event |
| `ioctlEvent(leader, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP)` | start the size's window |
| `readGroup(leader, words, 3 * sizeof(std::uint64_t))` | the one read that settles the size |
| `ioctlEvent(leader, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP)` | end the size's window |
| `closeFd(fd)` | every descriptor, on every path |

No `syscall`, no C-library `perf_event_open`, and no second seam (FR-011, D-7, FR-073).
The enable and the disable settle nothing on their own: an enable the kernel refused
leaves the enabled word at zero, and the fit test below reads that size as one that did
not run (FR-014). That is why neither result carries a branch of its own.

## The attribute word of one copy

`type` and `config` come from the options, `size` is `sizeof(perf_event_attr)`, and the
words carry `exclude_kernel`, `exclude_hv` and `disabled` and clear `pinned` (FR-013).
`read_format` asks for `TOTAL_TIME_ENABLED` and `TOTAL_TIME_RUNNING` alone, so the read
answers the leader's count, its enabled window, and its running window: three words, and
the pair F-3 settles a size with. The open binds `pid` 0 and `cpu` -1, the shape F-1 to
F-3 measured. FR-013a states what that binding cannot see, and the probe changes no
affinity of the calling thread (FR-013).

## The fit test, stated once

```text
fits(size)  :=  the one read of that size's leader answered all three words,
                time_enabled is past zero, and time_running == time_enabled
```

Three parts, and each carries a fact. The read stands after the enable, and once: F-5
counted 7 reads against 28 opens on the measured host. `time_enabled` past zero separates
a group the kernel never started from a pair of zeroes. The equality is the whole claim:
a group that shared the cpu with another answered `running` below `enabled` at every size
from 7 up in F-1, which is the reading that makes an open result alone worthless
(FR-014).

## The search, the stop, and the cap

Sizes run 1, 2, and up to `kGroupProbeMaxSize`. One size opens `size` copies of the same
type and config: the first as group leader with `group_fd` -1, and each later copy naming
the leader's descriptor (FR-012). The search stops at the first size that does not fit,
whatever the reason, and answers the size before it (FR-015).

| Stop | Answer | Status |
|---|---|---|
| an open refused with `EACCES` or `EPERM` | no count | `PERMISSION_REFUSED` (FR-018) |
| an open refused otherwise | the size before it, or none at size 1 | `OPEN_REFUSED` (FR-015, FR-017) |
| the read answered below three words | the size before it, or none at size 1 | `READ_SHORT` (FR-014, FR-015) |
| the pair says the size ran short | the size before it, or none at size 1 | `RUNS_SHORT` (FR-015, FR-017) |
| size 32 fitted | no count | `CAP_REACHED` (FR-016) |

The last row is the reason a search that found no refusal answers nothing: it holds no
bound, and a run that reported 32 would report the cap as hardware (FR-016). PC-3 and F-1
are the measured ground for that rule: on the Zen 5 host an open of 96 copies answered a
descriptor, so a search that trusted open results would have answered 96.

## The closure rule

Each size closes every descriptor it opened before the next size opens anything, the
refusals included (FR-019). One vector of descriptors stands inside the size, and one
release loop runs after the read and after every refusal, so no `return` sits between an
open and its release. F-5's reading `left open at exit: 0` holds inside the process as
well: the measured tool opened 28 and closed 28 for the answer 6.

## No sleep, no spin, no clock

The path holds no timer call, no clock read, no retry and no sleep (FR-021). The window
is the kernel's own, so the search costs opens, ioctls, one read and closes, and nothing
else. The tier gate's forbidden constructs (`test/tier_labels.sh`) hold here as they hold
in the units.

## The cost, and what it buys

F-5 measured 28 opens, 7 reads and 28 closes for the answer 6 on the Zen 5 host. The
measuring tool enables and disables each member, so it paid 28 enables where the
shipped search pays 7 group ioctls on the leader.
`results/requirement-1-shipped-shape.txt` prices both shapes and gives the shipped
pair 0.080 ms with a spread of 0.068 to 0.095 ms: the wall time of one search follows
the scheduler, so the lane publishes a band. D-4's cap bounds the worst case at 528
opens, and the FR-060 docs line and the Budget note state that bound.

## The unit and the scenarios

`test/source/counters_group_probe_test.cpp` scripts the seam and holds no real
`perf_event_open` (FR-073). The scripted kernel follows F-1: every open answers a
descriptor, and a group larger than the scripted bound answers a running word below its
enabled word.

| Scenario | Arm |
|---|---|
| `runsShortAtSevenAnswersSix` | the F-1 shape: 6 with 28 opens and 7 reads (FR-014, FR-015, F-5) |
| `refusedOpenAnswersTheSizeBefore` | `EINVAL` at size 7 answers 6 (FR-012, FR-015) |
| `permissionRefusalAnswersNothing` | `EACCES`, `EPERM` and `EINVAL` rows at size 1 (FR-018) |
| `shortReadAnswersNothing` | a read of one word (FR-014) |
| `sizeOneRunsShortAnswersNothing` | no bound below size 1 (FR-017) |
| `everySizeFittingAnswersNothing` | 528 opens and no count (FR-016) |
| `groupFdNamesTheLeader` | the leader and its members (FR-012) |
| `attributeCarriesTheMeasuredShape` | the words, `pid` 0 and `cpu` -1 (FR-013) |

Each scenario ends with the release check, which compares the opened and closed counts of
that scenario (FR-019).

## Verification

| Claim | Machine |
|---|---|
| every arm above reaches a case with no host and no kernel interface | `ctest --test-dir build -R counters_group_probe_test --output-on-failure` |
| the unit reads red at the base and green with the code | `results/red-readings.md` |
| no probe path names a timer, a clock or a `syscall` | `git grep -n 'nanosleep\|clock_gettime\|syscall' source/counters/linux_pmu/group_probe.cpp` |
| the search costs no second seam | `git grep -n 'syscall\|perf_event_open' source/counters/linux_pmu/group_probe.cpp` answers nothing |
| the dark-branch rows of the new file hold at zero | `tools/dbc/coverage_gate.sh` beside `results/coverage-base.txt` |
