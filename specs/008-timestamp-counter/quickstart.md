# Quickstart: Raw Time-Stamp Counter

**Feature**: `008-timestamp-counter` | **Date**: 2026-09-29 | **Plan**: [plan.md](plan.md)

A validation run guide: every section is a command plus an observable verdict
(constitution X.4). Contracts: [contracts/](contracts/). Requirements:
[spec.md](spec.md). Prerequisites: an x86-64 Linux development host, the
compiler the `dev` preset selects, `cmake >= 3.20`, `doxygen` on `PATH` for the
pairing gate, no privileges, and nothing written under `/sys`. 008 adds no
source file and no header, so the scratch programs below live outside the
repository tree under `/tmp/sg008/`.

## Host facts this guide rests on

```sh
uname -m
grep -m1 '^flags' /proc/cpuinfo | tr ' ' '\n' \
  | grep -Ex 'constant_tsc|nonstop_tsc|rdtscp|tsc' | sort
ls -l /sys/devices/system/cpu/tsc_khz
```

Expected on the development host: `x86_64`; the flag line reports
`constant_tsc`, `nonstop_tsc`, `rdtscp`, and `tsc`, so the build executes the
instruction; and `ls` reports `No such file or directory` for
`/sys/devices/system/cpu/tsc_khz`, so the host publishes no counter frequency.

That combination makes this host the proving ground for the feature. Under 007
the entry was withheld here, because the catalog gated it on that missing
file. Under 008 the entry publishes anyway, and the sections below observe it.
A host that publishes a frequency passes the same sections for the same
reasons, and reaches none of the calibration branches that 008 deleted.

## 1. Build the tree

```sh
cmake --preset=dev
cmake --build --preset=dev
```

Expected: both commands exit 0. The library builds with no new source file,
because `CMakeLists.txt:658` globs `source/counters/*.cpp` with `GLOB_RECURSE`
and `CONFIGURE_DEPENDS`.

## 2. The entry is listed at the fast read mode (FR-001, SC-001)

```sh
ctest --preset=dev -R counters_tsc_test --output-on-failure
./build/dev/test/counters_tsc_test
```

Expected: exit 0, and the console prints four lines:

```text
tsc entry: mode fast_tsc, unit none, frequency 0, description 'raw time-stamp counter ticks; a count asserting no rate'
accessor and lookup both name 'tsc'
instructions per tick: 0.000491, running ratio 1.000000, scaled no
counters_tsc_test PASS: raw entry published, accessor interchangeable
```

The enumeration behind the first line resolves the `machine` object and finds
the entry among its counters. The entry sits at `read_mode::fast_tsc` and
reports `availability::countable`. The test guards its presence assertions on
the same build condition the provider uses, so one binary asserts the entry on
a host that executes the instruction and asserts its absence on a build that
does not (FR-001, FR-008).

## 3. Field values: a count with no rate attached (FR-002)

Build the scratch program once and reuse it for sections 3 to 6. The second
include path carries the generated export header; without it the compile stops
on the missing include.

```sh
mkdir -p /tmp/sg008
sg_build() {
  c++ -std=c++23 -I include -I build/dev/export \
    -o "/tmp/sg008/$1" "/tmp/sg008/$1.cpp" \
    build/dev/libspeedgun-ng.a \
    build/dev/hwloc_vendor-prefix-*/stage/lib/libhwloc_embedded.a -ldl \
    build/dev/_simdjson/libsimdjson.a build/dev/_zlib/libz.a \
    build/dev/_hdrhistogram/src/libhdr_histogram_static.a -lm -lrt \
    build/dev/_yaml-cpp/libyaml-cppd.a
}
```

`/tmp/sg008/scenarios.cpp`:

```cpp
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>

#include "speedgun-ng/counters.hpp"

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::fake_provider;
using sg::counters::read_mode;
using sg::counters::scope;
using sg::counters::system;
using sg::counters::unit_name;

using events = dim<0, 1>;

namespace
{
void burn_cpu()
{
  volatile double work = 0.0;
  for (int i = 0; i < 200000; ++i) {
    work += static_cast<double>(i);
  }
  static_cast<void>(work);
}

std::size_t digest(const std::vector<catalog_entry>& entries)
{
  std::size_t total = entries.size();
  for (const auto& entry : entries) {
    total += entry.name.size() + entry.description.size();
  }
  return total;
}
}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "fields";

  if (mode == "register-after") {
    const auto early = system::local().tsc();
    std::printf("accessor before registration: %s\n",
                early.has_value() ? "counter" : early.error().message.c_str());
    auto late = std::make_unique<clock_provider>();
    const bool accepted =
        system::local().register_provider(std::move(late)).has_value();
    std::printf("registration afterwards: %s\n",
                accepted ? "accepted" : "refused");
    return accepted ? 0 : 1;
  }

  // Every provider registers before any object is resolved: resolving one
  // object opens the registration boundary.
  auto clock = std::make_unique<clock_provider>();
  if (!system::local().register_provider(std::move(clock)).has_value()) {
    return 1;
  }
  if (mode == "compose") {
    auto fake = std::make_unique<fake_provider>();
    fake->add_object("core-0", "core0", "core", "the core under test");
    fake->add_counter("core-0", "instructions", "ops", "instructions retired");
    fake->set_points("core-0", "instructions", {}, 1000);
    if (!system::local().register_provider(std::move(fake)).has_value()) {
      return 1;
    }
  }
  const auto machine = *system::local().object("machine");

  if (mode == "fields") {
    for (const auto& entry : machine.counters()) {
      if (entry.name != "tsc") {
        continue;
      }
      std::printf("name %s\ndescription %s\nunit %s\n"
                  "mode fast_tsc %d\navail countable %d\n"
                  "frequency_hz %llu\nscaled %d\n",
                  std::string(entry.name).c_str(),
                  std::string(entry.description).c_str(),
                  std::string(unit_name(entry.unit)).c_str(),
                  entry.mode == read_mode::fast_tsc ? 1 : 0,
                  entry.avail == availability::countable ? 1 : 0,
                  static_cast<unsigned long long>(entry.frequency_hz),
                  entry.scaled ? 1 : 0);
    }
    return 0;
  }

  if (mode == "readonly") {
    const std::size_t before = digest(machine.counters());
    for (int i = 0; i < 1000; ++i) {
      static_cast<void>(*system::local().tsc());
    }
    std::printf("catalog digest before %zu after %zu\n",
                before,
                digest(machine.counters()));
    return 0;
  }

  if (mode == "compose") {
    const auto core = *system::local().object("core-0");
    const auto instructions = *core.counter<events>("instructions");
    const auto raw = *system::local().tsc();
    const auto per_tick = instructions / raw;
    const auto compiled = compile(system::local(), per_tick);
    if (!compiled.has_value()) {
      std::printf("compile refused\n");
      return 1;
    }
    scope window {*compiled};
    window.start();
    burn_cpu();
    window.finish();
    const auto folded = window.metric(per_tick);
    std::printf("instructions per tick %.6f, running ratio %.6f, scaled %s\n",
                folded.value,
                folded.running_ratio,
                folded.scaled ? "yes" : "no");
  }
  return 0;
}
```

```sh
sg_build scenarios
/tmp/sg008/scenarios fields
```

Expected: exit 0 and seven lines.

```text
name tsc
description raw time-stamp counter ticks; a count asserting no rate
unit none
mode fast_tsc 1
avail countable 1
frequency_hz 0
scaled 0
```

The frequency reads 0 and the scaled flag reads 0, both the zero defaults the
library never writes for this entry. The description asserts a count and names
no rate. The unit is the closed token 007 assigned, and it maps to
`dim<0, 1>` at `include/speedgun-ng/counters_core.hpp:178`, so the entry is a
counted source.

## 4. Interchangeability: compose, compile, fold (FR-004, SC-002)

```sh
/tmp/sg008/scenarios compose
```

Expected: exit 0 and one line of the shape

```text
instructions per tick 0.000556, running ratio 1.000000, scaled no
```

The value varies with the host and the run. Three facts are fixed. The
accessor's result is divided by a counted source, so the quotient type-checks
through the existing algebra with no special case. `compile()` accepts it. The
fold carries full disclosure: a value, a running ratio, and a scaled flag
whose omission the `metric_result` shape makes unrepresentable.

This is the interchangeability proof. A bespoke counter type carrying its own
unit and calibration would have failed at the division, and no amount of
method matching would have repaired it.

## 5. No read: the catalog is unchanged after a thousand calls (FR-005)

```sh
/tmp/sg008/scenarios readonly
```

Expected: exit 0 and

```text
catalog digest before 209 after 209
```

The digest sums the entry count with every entry's name and description length.
Identical figures before and after a thousand accessor calls mean the lookup
mutated nothing: no hardware read, no plan, no recorder.

## 6. The registration boundary stays open (FR-006)

```sh
/tmp/sg008/scenarios register-after
```

Expected: exit 0 and two lines:

```text
accessor before registration: no clock provider is registered, so the tree holds no time-stamp entry to resolve (specs/008-timestamp-counter FR-007)
registration afterwards: accepted
```

The accessor runs first and leaves the boundary open, so the clock provider
registers in the same process afterwards. The catalog freezes at the open
transition (FR-009), and the accessor never reaches it.

## 7. The recoverable error in a fresh process (FR-007)

`system::local()` is a process singleton, so a program that registers a
provider first can never observe the unregistered branch. The accessor call
must be the first statement of the program.

`/tmp/sg008/unregistered.cpp`:

```cpp
#include <cstdio>

#include "speedgun-ng/counters.hpp"

// The first statement of the process touches the system (FR-007):
// system::local() has opened nothing and no provider is registered.
auto main() -> int
{
  const auto absent = sg::counters::system::local().tsc();
  if (absent.has_value()) {
    std::printf("a counter came back with no provider registered\n");
    return 1;
  }
  std::printf("no counter: %s\n", absent.error().message.c_str());
  std::printf("suggestions: %zu\n", absent.error().suggestions.size());
  return 0;
}
```

```sh
sg_build unregistered
/tmp/sg008/unregistered
```

Expected: exit 0 and

```text
no counter: no clock provider is registered, so the tree holds no time-stamp entry to resolve (specs/008-timestamp-counter FR-007)
suggestions: 0
```

The failure is a recoverable error in the house shape: a lowercase message, the
failing input named, and the requirement id in parentheses. No counter comes
back, the suggestion list is empty because no catalog exists to suggest from,
and the process can still register a provider afterwards (section 6). It is a
plain `if` returning `std::unexpected` and carries no contract macro, because a
caller may handle it.

The shipped `counters_tsc_test` reaches the same branch by ordering its first
scenario before any registration.

## 8. The repaired clock test, and the reader opening the entry (FR-003)

```sh
ctest --preset=dev -R counters_clock_push_test --output-on-failure
./build/dev/test/counters_clock_push_test
```

Expected: exit 0, and the console prints

```text
tsc: mode fast_tsc, unit none, frequency 0, description 'raw time-stamp counter ticks; a count asserting no rate'
clock: the catalog publishes the tsc entry: yes
counters_clock_push_test PASS: clock, push, and composites
```

The second line comes from the assertion that the reader opens the entry
exactly where the catalog publishes it. Under 007 that assertion opened a
window on some hosts and refused on others, following the sysfs file. Under
008 both sites key on the build guard, so the two agree on every host.

## 9. Header purity (FR-010)

```sh
bash test/counters_header_purity.sh .
```

Expected: exit 0 and `counters_header_purity: clean`.

The scan covers the substrings `perf_event`, `clock_gettime`, `rdpmc`, and
`rdtsc` across every `counters*.hpp`, plus the standalone uppercase acronym
`PMU` across the five core headers, `counters_system.hpp` among them. The token
`tsc` is a term the scan declines to check, recorded at
`test/counters_header_purity.sh:16` to `:18`, because FR-023 of 007 mandates
`fast_tsc` as a read-mode name and a substring scan would flag required
vocabulary. The accessor reuses that token, which is why it passes. Two rules
bind the new documentation: it says "time-stamp counter" and "instruction", and
it never writes the read instruction's spelling, in prose or in a comment.

## 10. Gates

```sh
cmake --preset=dev && cmake --build --preset=dev && ctest --preset=dev
cmake --build build/dev -t dbc-gate
python3 tools/prose/prose_gate.py --check all --mode tree
cmake --preset=ci-ubuntu && cmake --build build
cmake --preset=ci-sanitize && cmake --build build/sanitize
```

The pairing gate in the second line needs `doxygen` on `PATH`. Expected on this
branch:

```text
doc-gate: 136 interfaces, 0 gaps
pair-gate: 136 interfaces, 0 gaps
```

007 recorded 135 interfaces; 008 adds the one accessor. Zero gaps in both
directions is the check that matters for a new public function: a documented
`\pre` with no check and a check with no documented `\pre` each fail.

The `--mode tree` flag on the third line is mandatory. The default range mode
reads only committed content, so on an uncommitted branch it reports a green
verdict over an empty range.

The fourth line is the release-preset build constitution IX requires for a
public API change. Run it alone, and compare the log's `warning:` count against
the pre-change baseline, because clang-tidy and cppcheck report without failing
the build.

The fifth line builds the sanitizer tree. Run its suite with the CI option
block:

```sh
cd build/sanitize
ASAN_OPTIONS="strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1" \
UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
ctest --output-on-failure --no-tests=error
```

Expected: every test passes. `detect_leaks=1` and `halt_on_error=1` are the
two that carry the verdict; `print_stacktrace=1` makes a finding name its own
frames.

### Coverage, and the platforms this repository does not build

Local coverage is delegated to the CI coverage job. The `coverage` target's
last step is `genhtml`, which loads the perl `GD` module when it runs, and that
module is absent on the development host. The omission is a recorded decision
so it reads as one.

No macOS and no Windows job exists in the workflow. Constitution amendment
2.7.0 suspends the Windows MSVC preset-build gate for the vendored-autotools
lifetime, and 2.8.0 defers macOS to developer-local. SC-006 of this feature
names the Linux matrix alone, for that reason.

## Success-criteria index

The commands below produce each verdict. PASS means the command exited zero
with the outcome its section predicts. Raw output belongs under
`.omo/evidence/008-timestamp-counter/`, which is machine-local and
git-ignored.

| SC | Section | Command |
|---|---|---|
| SC-001 | 2 | `ctest --preset=dev -R counters_tsc_test --output-on-failure` |
| SC-002 | 4 | `/tmp/sg008/scenarios compose` |
| SC-003 | 3 | `/tmp/sg008/scenarios fields` |
| SC-004 | 10 | `ctest --preset=dev -R counters_overhead -V`, with the figure published in `docs/pages/counters-overhead.md` |
| SC-005 | 10 | the overhead benchmark brackets its sampling action with the published entry |
| SC-006 | 1, 8, 9, 10 | `ctest --preset=dev`, `bash test/counters_header_purity.sh .`, `cmake --build build/dev -t dbc-gate`, and the release-preset build |

Sections 5, 6, and 7 carry no success criterion of their own. They are the
evidence behind FR-005, FR-006, and FR-007, and a reviewer reads them next to
section 2.
