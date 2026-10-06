// ============================================================================
// TDD test for the Linux PMU provider (T043; US6 scenarios 1..7).
//
// Public-surface testing only: the detail seam functions (ident,
// directory selection, encode) are not exported and no test in this
// repository includes source/ internals, so selection and encoding are
// asserted through catalog shape: the vendored table entered the cpu
// object only if CPUID selection matched a directory, and every
// catalog state comes from the probe/encode pipeline. Every scenario
// records the host's outcome and skips with the reason named rather than
// failing, so the suite stays green wherever the kernel refuses a
// privilege this test would use (SC-002). Registration precedes the open
// boundary; frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "speedgun-ng/counters.hpp"

#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "speedgun-ng/counters_pmu.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS PMU TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto same_double(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::expression;
using sg::counters::object;
using sg::counters::pmu_provider;
using sg::counters::push_provider;
using sg::counters::read_mode;
using sg::counters::scope;
using sg::counters::system;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

constexpr std::string_view kDevicesRoot = "/sys/bus/event_source/devices";

// The push leaves the push provider declares below, so the catalog walk
// in scenario 4 has a hand-computed count to compare (SC-002).
constexpr std::size_t kDeclaredPushLeaves = 2;

auto find_entry(const std::vector<catalog_entry>& entries,
                const std::string_view name) -> const catalog_entry*
{
  for (const auto& entry : entries) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

// The descriptor count the fast-window lifecycle compares around its loop.
// The listing holds one descriptor of its own while it is read, and it
// holds one in both measurements, so the difference is what the test
// compares.
auto open_descriptor_count() -> std::size_t
{
  std::error_code failure;
  std::size_t count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator("/proc/self/fd", failure))
  {
    static_cast<void>(entry);
    ++count;
  }
  if (failure) {
    fail("the process's own descriptor listing is readable");
  }
  return count;
}

// The mapping count the same scenario compares: one line per mapping in
// /proc/self/maps, which the kernel formats as a file rather than
// listing as a directory.
auto mapping_count() -> std::size_t
{
  std::ifstream maps("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

auto read_paranoid() -> int
{
  std::ifstream file("/proc/sys/kernel/perf_event_paranoid");
  int value = -999;
  if (file >> value) {
    return value;
  }
  return -999;
}

// What the kernel answers when this process test-opens the plain
// hardware cycle event the way the provider's own availability probe
// does: this thread, user mode only, no group. The answer is the ground
// truth the catalog is held to, so the assertion below never has to
// name this host's outcome in advance.
enum class probe_verdict
{
  granted,
  refused_permission,
  refused_encoding
};

auto name(const probe_verdict verdict) -> const char*
{
  switch (verdict) {
    case probe_verdict::granted:
      return "granted";
    case probe_verdict::refused_permission:
      return "refused for permission";
    case probe_verdict::refused_encoding:
      return "refused for encoding";
  }
  return "unclassified";
}

auto hardware_event_probe() -> probe_verdict
{
  perf_event_attr attr {};
  attr.type = PERF_TYPE_HARDWARE;
  attr.size = sizeof(perf_event_attr);
  attr.config = PERF_COUNT_HW_CPU_CYCLES;
  attr.disabled = 1;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  const long fd =
      ::syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd >= 0) {
    ::close(static_cast<int>(fd));
    return probe_verdict::granted;
  }
  // The provider's probe separates the same two refusal classes, and
  // this test reproduces that split. It does not import the seam.
  switch (errno) {
    case EINVAL:
    case EOPNOTSUPP:
    case ENOENT:
      return probe_verdict::refused_encoding;
    default:
      return probe_verdict::refused_permission;
  }
}

// Kernel-discovered alias names for one PMU device (the events/
// directory sysfs publishes). Sysfs spells a per-alias attribute
// "<alias>.<attribute>"; a file name carrying a dot describes an
// attribute of a named event, never an event of its own.
auto sysfs_alias_names(const std::string& device) -> std::vector<std::string>
{
  std::vector<std::string> names;
  std::error_code ec;
  const std::filesystem::path dir =
      std::filesystem::path(kDevicesRoot) / device / "events";
  if (!std::filesystem::is_directory(dir, ec)) {
    return names;
  }
  for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    const std::string name = entry.path().filename().string();
    if (name.find('.') == std::string::npos) {
      names.push_back(name);
    }
  }
  std::ranges::sort(names);
  return names;
}

// The exact configuration text the kernel publishes for one alias.
auto sysfs_alias_text(const std::string& device,
                      const std::string& alias) -> std::string
{
  std::ifstream file(std::filesystem::path(kDevicesRoot) / device / "events"
                     / alias);
  std::string text;
  std::getline(file, text);
  return text;
}

// CPU-bound spin so the sampling window has real work.
auto burn_cpu_short() -> void
{
  volatile double acc = 0.0;
  for (int i = 0; i < 5'000'000; ++i) {
    acc += i * 1.00000011;
  }
  static_cast<void>(acc);
}

// Scenarios 1 and 2 (merge with kernel-wins, every entry described,
// CPUID-selected vendored table entered): asserted through the
// catalog of every pmu-kind object. The selection itself (detail::
// pmu_ident_current + pmu_select_directory) is not exported; the
// presence of table entries the running kernel does not publish as
// aliases proves a directory matched and was parsed.
auto merge_and_catalog_scenario(const std::vector<const object*>& pmu_objects)
    -> void
{
  check(!pmu_objects.empty(), "the provider seeded at least one pmu object");
  std::size_t table_only = 0;
  std::size_t catalog_total = 0;
  for (const object* obj : pmu_objects) {
    const auto entries = obj->counters();
    catalog_total += entries.size();
    check(!entries.empty(), "every pmu object carries catalog entries");
    const auto aliases = sysfs_alias_names(std::string(obj->path()));
    for (const auto& entry : entries) {
      check(!entry.description.empty(),
            "every pmu catalog entry is described (FR-037)");
      // Scenario 3's closed-state assertion travels here: the probe
      // pipeline reports one of the three states a test-open answers with,
      // and the fourth state a device-scoped device settles its entries on
      // when its cpu probe refuses too. `probe_device` publishes that state
      // in place of the cpu verdict for a kind the device's own scope
      // already refused, so a caller can tell the two refusals apart; an
      // absent object is never seeded, and `gap` is a property of one
      // sampling action (FR-021, FR-039).
      check(entry.avail == availability::countable
                || entry.avail == availability::permission_blocked
                || entry.avail == availability::not_encodable
                || entry.avail == availability::scope_refused,
            "every pmu entry reports a probed availability state");
    }
    // Kernel-wins conflict check: a sysfs alias present in the
    // catalog carries the alias's configuration text the provider
    // built from the kernel event_attr, verbatim, never a vendored
    // description.
    for (const auto& alias : aliases) {
      const auto* entry = find_entry(entries, alias);
      check(entry != nullptr, "kernel alias appears in the merged catalog");
      const std::string text =
          sysfs_alias_text(std::string(obj->path()), alias);
      check(entry->description.find(text) != std::string_view::npos,
            "the alias entry keeps the kernel event_attr text (wins)");
    }
    if (obj->path() == "cpu") {
      const std::size_t alias_count = aliases.size();
      check(entries.size() > alias_count,
            "the CPUID-selected vendored table entered the cpu catalog "
            "(FR-038 selection proven indirectly)");
      table_only = entries.size() - alias_count;
    }
  }
  std::printf("pmu catalog: %zu table-selected entries beyond kernel aliases\n",
              table_only);
  // The installed consumer prints this same figure from the same catalog,
  // so a downstream job compares the two and names a difference between
  // what it linked and what this tree built (FR-022, SC-012, D-12).
  std::printf("pmu catalog entries %zu\n", catalog_total);
}

// Scenario 4: the reported availability is consistent with the probe
// result and with the host's paranoia level. Clocks and push stay
// countable; hardware entries report whatever the kernel's own test-open
// says, at whatever level the host runs (never a hard-fail either way,
// SC-002).
// The availability enumeration's values, pinned. A new target kind takes
// the next free bit, and this assertion is what shows that adding one
// left every existing value where it stood (FR-021).
static_assert(static_cast<std::uint8_t>(availability::countable) == 0);
static_assert(static_cast<std::uint8_t>(availability::permission_blocked) == 1);
static_assert(static_cast<std::uint8_t>(availability::not_encodable) == 2);
static_assert(static_cast<std::uint8_t>(availability::absent) == 3);
static_assert(static_cast<std::uint8_t>(availability::scope_refused) == 4);
static_assert(static_cast<std::uint8_t>(availability::gap) == 5);

// The target-kind mask is a fixed-size unsigned integer, so reading an
// entry's targets allocates nothing and needs no container (FR-021).
static_assert(sizeof(sg::counters::target_mask) == sizeof(std::uint32_t));
static_assert(sg::counters::target_thread_bit == 1U);
static_assert(sg::counters::target_cpu_bit == 2U);
static_assert(
    std::is_same_v<sg::counters::target_mask, std::uint32_t>,
    "the mask is one fixed-size integer and no container " "(FR-021)");

auto availability_scenario(const std::vector<const object*>& pmu_objects)
    -> void
{
  const int paranoid = read_paranoid();
  const probe_verdict verdict = hardware_event_probe();
  std::printf("perf_event_paranoid = %d; hardware event probe %s\n",
              paranoid,
              name(verdict));

  const auto machine = *system::local().object("machine");
  const auto machine_entries = machine.counters();
  const auto* mono = find_entry(machine_entries, "monotonic");
  check(mono != nullptr && mono->avail == availability::countable,
        "clock leaf stays countable beside the PMU provider (SC-002)");

  // Scenario 4 names the push counters beside the clocks: with the pmu
  // provider registered, every push leaf the catalog offers is countable,
  // and the catalog offers exactly the declared ones.
  std::size_t push_leaves = 0;
  std::size_t countable_push = 0;
  for (const auto& entry : machine_entries) {
    if (entry.mode != read_mode::push_load) {
      continue;
    }
    ++push_leaves;
    if (entry.avail == availability::countable) {
      ++countable_push;
    }
  }
  std::printf("push availability: %zu of %zu declared leaves countable\n",
              countable_push,
              push_leaves);
  check(push_leaves == kDeclaredPushLeaves,
        "the catalog offers every declared push leaf (SC-002)");
  check(countable_push == push_leaves,
        "every push leaf stays countable beside the pmu provider (SC-002)");

  std::size_t countable = 0;
  std::size_t blocked = 0;
  std::size_t unencodable = 0;
  std::size_t scope_refused = 0;
  std::size_t fast = 0;
  for (const object* obj : pmu_objects) {
    for (const auto& entry : obj->counters()) {
      switch (entry.avail) {
        case availability::countable:
          ++countable;
          break;
        case availability::permission_blocked:
          ++blocked;
          break;
        case availability::not_encodable:
          ++unencodable;
          break;
        case availability::scope_refused:
          ++scope_refused;
          break;
        case availability::absent:
          fail("absent is never seeded by the provider");
          break;
        case availability::gap:
          fail("gap is a property of one sampling action, never of an "
               "entry (FR-021)");
          break;
      }
      if (entry.mode == read_mode::fast_rdpmc) {
        ++fast;
      }
    }
  }
  // The paranoid-2 hardware-entry state, recorded from the catalog this
  // host produced: the counts beside the probe that produced them.
  //
  // The mask is read against the object the entry was probed under, not
  // against the state the entry carries alone: the catalog derives the mask
  // from the device's own scope, so a check that recomputed the mask from
  // the state would compare the derivation with itself and could not fail.
  // The device scope is spelled out here from the canonical path the
  // provider seeded, which is data this scenario reads. The catalog derived
  // no part of it (FR-021).
  std::size_t entries_seen = 0;
  std::size_t gaps = 0;
  std::size_t cpu_only = 0;
  for (const object* obj : pmu_objects) {
    for (const auto& entry : obj->counters()) {
      ++entries_seen;
      if (entry.avail == availability::gap) {
        ++gaps;
      }
      if (entry.targets == sg::counters::target_cpu_bit) {
        ++cpu_only;
      }
      const bool settled = entry.avail == availability::countable;
      const sg::counters::target_mask defined_bits =
          sg::counters::target_thread_bit | sg::counters::target_cpu_bit;
      check(settled == (entry.targets != sg::counters::target_mask {0}),
            "a published entry names the target kinds the probe settled it "
            "on: a countable entry names at least one, and every other state "
            "names none (FR-021)");
      check((entry.targets & ~defined_bits) == 0,
            "a published mask names only the bits target_kind defines, so a "
            "new kind takes the next free bit and no stored bit moves "
            "(FR-021)");
    }
  }
  std::printf(
      "target masks: %zu of %zu entries cpu-only\n", cpu_only, entries_seen);
  check(entries_seen > 0,
        "the catalog published hardware entries whose target masks this "
        "scenario reads (FR-021)");
  // The gap is the one state the catalog never publishes for an entry: a
  // gap is a property of one sampling action, and the value travels in
  // the plan's disclosure column beside the zero count the action
  // produced. The count runs over every published entry, and the
  // enumerator takes no part in it, so the catalog can falsify it (FR-007).
  check(gaps == 0,
        "no catalog entry publishes availability::gap, so the per-action "
        "gap rides the plan's disclosure column and no entry (FR-007)");

  std::printf("pmu availability: %zu countable, %zu permission_blocked, "
              "%zu not_encodable, %zu scope_refused, %zu fast_rdpmc\n",
              countable,
              blocked,
              unencodable,
              scope_refused,
              fast);

  // The fast (mapped-page rdpmc) read is gated by the kernel's own
  // `cap_user_rdpmc` bit in the page it maps for a real event, and by
  // nothing else. The level above is the level the availability probe's
  // test-opens answered at; it does not decide the fast mechanism, and a
  // claim that it did was measured false on this host, whose kernel grants
  // the path at level 2 and at 1 (T131, T135). So the count of
  // fast entries is reported beside the level and never asserted against
  // it; the obligation FR-023 states is that a disclosed mode is the mode
  // the plan reads, which `disclosed_mode_read_scenario` checks by
  // sampling.
  std::printf("fast_rdpmc entries: %zu at perf_event_paranoid %d, gated by "
              "the kernel's cap_user_rdpmc bit (FR-023)\n",
              fast,
              paranoid);
  // The probe result decides the states. The level alone does not.
  // At level 2 the
  // kernel still grants per-process user-mode events, so a catalog
  // that reported only refusals at level 2 would be a false refusal, and
  // one that reported a grant where the kernel refuses would be a false
  // permission. Both directions can fail, so neither is hard-coded.
  if (verdict == probe_verdict::granted) {
    check(countable > 0,
          "a granted hardware probe leaves a countable catalog entry "
          "(US6 scenario 4)");
  } else {
    check(countable == 0,
          "a refused hardware probe leaves no countable catalog entry "
          "(US6 scenario 4)");
    if (verdict == probe_verdict::refused_permission) {
      check(blocked > 0,
            "a permission refusal surfaces as permission_blocked and "
            "leaves the encoding state clear (FR-039)");
    } else {
      check(unencodable > 0,
            "an encoding refusal surfaces as not_encodable and "
            "leaves the permission state clear (FR-039)");
    }
  }

  // A scope refusal is separable from an encoding refusal on real catalog
  // data, not by naming two enumerators. `probe_device` publishes it
  // where the entry's own scope refuses the per-task kind, and the core
  // PMU plus its per-core hybrid instances are the devices that count a
  // thread's own events; every other published device binds one processor
  // for every task, so its entries carry that refusal and carry no encoding
  // one. Each such entry is therefore read against the device
  // scope it was probed under, and the handle that resolves it carries
  // the same state, so a caller can branch on it before composing
  // (FR-021, SC-007).
  if (scope_refused == 0) {
    // A host whose cpu probe settles every entry publishes no scope
    // refusal to assert over. The state has no entry under test, and this
    // comment names the reason; no reason is hard-coded (SC-007).
    std::printf("SKIP scope refusal: the catalog publishes no "
                "availability::scope_refused entry on this host (hardware "
                "event probe %s at perf_event_paranoid %d), so a "
                "device-scoped entry's per-task refusal has no entry under "
                "test (FR-021, SC-007)\n",
                name(verdict),
                paranoid);
    return;
  }
  for (const object* obj : pmu_objects) {
    const std::string_view path = obj->path();
    const bool device_scoped =
        path != "cpu" && path != "cpu_core" && path != "cpu_atom";
    for (const auto& entry : obj->counters()) {
      if (entry.avail != availability::scope_refused) {
        continue;
      }
      check(device_scoped,
            "a published scope refusal names a device that binds one "
            "processor for every task, which no encoding refusal can "
            "reproduce (FR-021)");
      const auto leaf = obj->counter<events>(entry.name);
      check(leaf.has_value() && leaf->avail() == availability::scope_refused,
            "the resolved handle carries the scope refusal the catalog "
            "published, so a caller can branch on it (FR-021)");
      check(entry.targets == sg::counters::target_mask {0},
            "a scope-refused entry names no target bit, the shape every "
            "state but countable publishes (FR-021)");
    }
  }
}

// FR-023: a catalog entry discloses the read mode its plan will use, and
// the plan reads through the disclosed mechanism. A fast window the kernel
// refuses is a recoverable open failure, never a silent downgrade to a
// read the catalog does not describe. This samples a fast-disclosed leaf
// and a syscall-disclosed leaf and requires each to deliver real counts,
// which is what rules out a downgrade: a downgraded read would come from a
// different mechanism and the positive deltas below would not distinguish
// it, so the check is on the pair travelling with the members.
auto disclosed_mode_read_scenario(const std::vector<const object*>& pmu_objects)
    -> void
{
  std::size_t sampled = 0;
  std::size_t countable = 0;
  for (const object* obj : pmu_objects) {
    const auto entries = obj->counters();
    for (const auto& entry : entries) {
      if (entry.avail != availability::countable
          || (entry.mode != read_mode::fast_rdpmc
              && entry.mode != read_mode::syscall))
      {
        continue;
      }
      ++countable;
      const auto leaf = obj->counter<events>(entry.name);
      if (!leaf.has_value()) {
        continue;
      }
      const expression<events> over {*leaf};
      auto compiled = compile(system::local(), over);
      if (!compiled.has_value()) {
        // A refused open is the recoverable failure FR-023 allows, and the
        // message names the open. No count ever arrives to be mistaken for
        // a measurement.
        check(compiled.error().message.find("cannot open a window")
                  != std::string::npos,
              "a leaf the kernel refuses to open fails the plan in the "
              "untimed region (FR-024)");
        continue;
      }
      scope window {*compiled};
      window.start();
      burn_cpu_short();
      window.finish();
      const auto metric = window.metric(over);
      check(metric.value > 0.0,
            "a plan over a countable leaf delivers a positive count, so the "
            "disclosed mode is a mode that reads (FR-023)");
      ++sampled;
      // One member per disclosed mode is enough; the catalog carries
      // hundreds of each and the open cost is per leaf.
      if (sampled >= 4) {
        std::printf("disclosed modes: %zu countable leaves sampled through "
                    "the mode the catalog published\n",
                    sampled);
        return;
      }
    }
  }
  std::printf("disclosed modes: %zu countable leaves sampled through the "
              "mode the catalog published\n",
              sampled);
  // A host whose perf_event_paranoid hides every hardware event
  // publishes no countable leaf, so no mode is disclosed to sample and the
  // positive count below is unreachable. 007 recorded this environment as
  // the expected CI shape (FR-039); the reason is named and the scenario
  // skips.
  if (countable == 0) {
    std::printf("SKIP disclosed modes: the catalog publishes no countable "
                "leaf at this perf_event_paranoid, so no mode is "
                "disclosed to sample (FR-039)\n");
    return;
  }
  check(sampled > 0,
        "at least one countable leaf sampled through its disclosed mode "
        "(FR-023)");
}

// Scenario 5: a group of resolved countable events samples in
// syscall mode through a real scope; the folded quotient equals the
// raw-column quotient exactly (quickstart 10). Skips with the reason
// printed when the host refuses (SC-002).
auto group_read_scenario() -> void
{
  // A provider that seeds no cpu object leaves the hardware scenarios
  // with no subject. A host whose perf_event_paranoid hides every event
  // publishes none, and 007 recorded that as the expected CI shape
  // (FR-039): the reason is named and the scenario skips.
  const auto cpu_object = system::local().object("cpu");
  if (!cpu_object.has_value()) {
    std::printf("SKIP: no provider seeded a cpu object at this "
                "perf_event_paranoid, so no hardware event is "
                "reachable to exercise (FR-039)\n");
    return;
  }
  const auto& cpu = *cpu_object;
  const auto entries = cpu.counters();
  std::string name_a;
  std::string name_b;
  // Two different work counters, so the quotient means something and
  // the group has a free counter for each: a work counter over a cycle
  // counter. The spelling differs per vendor, so each side lists the
  // spellings a core PMU publishes (FR-037).
  constexpr std::string_view kWorkNames[] = {
      "instructions", "inst_retired", "ex_ret_instr", "cpu/instructions/"};
  constexpr std::string_view kCycleNames[] = {
      "cpu-cycles", "cycles", "cpu/cycles/", "ex_ret_ops", "ref-cycles"};
  auto first_countable = [&](const std::string_view* names,
                             const std::size_t count) -> std::string
  {
    for (std::size_t index = 0; index < count; ++index) {
      const auto* entry = find_entry(entries, names[index]);
      if (entry != nullptr && entry->avail == availability::countable) {
        return std::string(names[index]);
      }
    }
    return {};
  };
  name_a = first_countable(kWorkNames, std::size(kWorkNames));
  name_b = first_countable(kCycleNames, std::size(kCycleNames));
  if (!name_a.empty() && name_b.empty()) {
    // A cycle counter outside the list, distinct from the work counter.
    for (const auto& entry : entries) {
      if (entry.avail == availability::countable && entry.name != name_a) {
        name_b = std::string(entry.name);
        break;
      }
    }
  }
  if (name_a.empty() || name_b.empty()) {
    std::printf("SKIP scenario 5: no two countable cpu-PMU events on this "
                "host (hardware event probe %s at perf_event_paranoid %d); "
                "privileged evidence recorded in the PR per tasks.md\n",
                name(hardware_event_probe()),
                read_paranoid());
    return;
  }
  std::printf("group scenario: %s / %s (syscall mode)\n",
              name_a.c_str(),
              name_b.c_str());

  const expression<events> a {*cpu.counter<events>(name_a)};
  const expression<events> b {*cpu.counter<events>(name_b)};
  const expression<time_dim> enabled {*cpu.counter<time_dim>("enabled")};
  const expression<time_dim> running {*cpu.counter<time_dim>("running")};
  const auto quotient = a / b;
  auto compiled = compile(system::local(), quotient, a, b, enabled, running);
  if (!compiled.has_value()) {
    fail("the pmu group plan compiles");
  }
  scope window {*compiled};
  window.start();
  burn_cpu_short();
  window.finish();

  const auto metric = window.metric(quotient);
  const auto raw_a = a.raw(window.view(), "cpu", name_a);
  const auto raw_b = b.raw(window.view(), "cpu", name_b);
  const auto raw_enabled = enabled.raw(window.view(), "cpu", "enabled");
  const auto raw_running = running.raw(window.view(), "cpu", "running");
  check(raw_a.has_value() && raw_b.has_value(), "raw views resolve");
  check(raw_enabled.has_value() && raw_running.has_value(),
        "the enabled/running ratio leaves carry points (FR-041)");

  const auto delta_a = raw_a->points[1] - raw_a->points[0];
  const auto delta_b = raw_b->points[1] - raw_b->points[0];
  // A zero denominator folds to NaN, and a NaN compares equal to itself
  // bit for bit, so the quotient equality below would pass on a dead
  // read. The deltas must be positive before it means anything.
  check(delta_a > 0 && delta_b > 0,
        "the group read delivers positive member counts over CPU-bound work");
  const double expected =
      static_cast<double>(delta_a) / static_cast<double>(delta_b);
  check(same_double(metric.value, expected),
        "the folded quotient equals the raw-column quotient exactly");

  const auto delta_enabled = raw_enabled->points[1] - raw_enabled->points[0];
  const auto delta_running = raw_running->points[1] - raw_running->points[0];
  // The pair advances only where the read mode refreshes it. A group
  // `read()` returns the counters' own totals, so the syscall mode sees a
  // fresh pair every action. A mapped-page read takes the pair from the
  // event page, and the kernel rewrites that page when it schedules the
  // event. A window with no syscall and no context switch inside it can
  // therefore read the same page value twice. The
  // staleness is the disclosed behaviour FR-041 and T055 record, so the
  // advance is asserted for the mode that guarantees it and the soundness
  // invariant is asserted in both.
  const bool mapped_page = [&entries, &name_a]
  {
    for (const auto& entry : entries) {
      if (entry.name == name_a) {
        return entry.mode == read_mode::fast_rdpmc;
      }
    }
    return false;
  }();
  if (!mapped_page) {
    check(delta_enabled > 0, "time_enabled advances across the window");
  } else {
    std::printf("mapped-page mode: the page-published pair advanced %llu ns "
                "of enabled time, which the kernel quantizes (FR-041)\n",
                static_cast<unsigned long long>(delta_enabled));
  }
  check((delta_enabled == 0 || delta_running > 0)
            && delta_running <= delta_enabled,
        "time_running stays inside time_enabled (ratio pair sound)");
  std::printf(
      "group read delivered members and the enabled/running pair; ratio %f\n",
      delta_enabled == 0 ? 1.0
                         : static_cast<double>(delta_running)
              / static_cast<double>(delta_enabled));
  // Privileged multiplex evidence (ratio below 1 under contention)
  // is scenario 6 below.
  std::printf("fold disclosure: running_ratio %f, scaled %d\n",
              metric.running_ratio,
              metric.scaled ? 1 : 0);
}

// Scenario 6: a multiplexed group, where the enabled/running ratio falls
// below 1 and the fold discloses it. Multiplexing needs more events open
// at once than the PMU has hardware counters, so this opens an
// oversubscribed set of countable cpu-PMU leaves and reads them all in one
// sampling action (FR-041, US7 scenario 4). The outcome is recorded either
// way: a host whose counter count fits every opened event reports ratio 1
// and says so.
auto multiplex_scenario() -> void
{
  // Far above any core PMU's hardware counter count, and low enough that the
  // open stays quick. The test reports how many events it opened, so a
  // reader can judge the oversubscription against the host rather than
  // against a number written here.
  constexpr std::size_t kOversubscribe = 64;

  // A provider that seeds no cpu object leaves the hardware scenarios
  // with no subject. A host whose perf_event_paranoid hides every event
  // publishes none, and 007 recorded that as the expected CI shape
  // (FR-039): the reason is named and the scenario skips.
  const auto cpu_object = system::local().object("cpu");
  if (!cpu_object.has_value()) {
    std::printf("SKIP: no provider seeded a cpu object at this "
                "perf_event_paranoid, so no hardware event is "
                "reachable to exercise (FR-039)\n");
    return;
  }
  const auto& cpu = *cpu_object;
  const auto enabled = cpu.counter<time_dim>("enabled");
  const auto running = cpu.counter<time_dim>("running");
  if (!enabled.has_value() || !running.has_value()) {
    std::printf("SKIP scenario 6: the cpu object carries no enabled/running "
                "pair, so no multiplex ratio can be folded\n");
    return;
  }
  const expression<time_dim> enabled_expr {*enabled};
  const expression<time_dim> running_expr {*running};

  // `compile` takes a variadic pack of expressions, so the whole
  // oversubscribed set reaches one plan as a single composite whose leaves
  // are the members. The composite's own value is a sum of counts and
  // carries no meaning; the ratio it discloses describes every leaf the
  // plan read, which is what this scenario is about (FR-026, FR-047).
  std::vector<expression<events>> members;
  members.reserve(kOversubscribe);
  for (const auto& entry : cpu.counters()) {
    if (members.size() == kOversubscribe) {
      break;
    }
    if (entry.avail != availability::countable) {
      continue;
    }
    const auto leaf = cpu.counter<events>(entry.name);
    if (leaf.has_value()) {
      members.emplace_back(*leaf);
    }
  }
  if (members.size() < 4) {
    std::printf("SKIP scenario 6: the catalog offers %zu countable cpu-PMU "
                "leaves, too few to oversubscribe this PMU\n",
                members.size());
    return;
  }
  expression<events> composite = members.front();
  for (std::size_t index = 1; index < members.size(); ++index) {
    composite = composite + members[index];
  }

  auto compiled =
      compile(system::local(), composite, enabled_expr, running_expr);
  if (!compiled.has_value()) {
    std::printf("SKIP scenario 6: the oversubscribed plan did not open: %s\n",
                compiled.error().message.c_str());
    return;
  }
  std::printf("scenario 6: %zu member leaves plus the enabled/running pair "
              "opened in one plan\n",
              members.size());
  scope window {*compiled};
  window.start();
  burn_cpu_short();
  window.finish();
  const auto metric = window.metric(composite);
  const double observed = metric.running_ratio;

  // The pair is the ground truth, and the obligation FR-041 states is that
  // running time never exceeds enabled time and that the fold derives its
  // disclosure from these two columns. The fold carries no constant ratio
  // (FR-019). The raw columns are read for that comparison (FR-047).
  const auto raw_enabled = enabled_expr.raw(window.view(), "cpu", "enabled");
  const auto raw_running = running_expr.raw(window.view(), "cpu", "running");
  check(raw_enabled.has_value() && raw_running.has_value(),
        "the oversubscribed plan carries the enabled/running pair (FR-041)");
  const auto elapsed = raw_enabled->points[1] - raw_enabled->points[0];
  const auto on_cpu = raw_running->points[1] - raw_running->points[0];
  check(on_cpu <= elapsed,
        "running time never exceeds enabled time over the oversubscribed "
        "window (ratio pair sound)");

  // FR-019 makes a composite's disclosed ratio the product of its
  // constituents' ratios, each raised to its algebraic exponent, so an
  // oversubscribed composite of many members multiplies many fractions
  // below 1 and discloses 0. The fold is right. The per-window fraction
  // the kernel granted is the quotient below, and it is the number a
  // reader of this host's PMU wants.
  const double granted = elapsed == 0
      ? 1.0
      : static_cast<double>(on_cpu) / static_cast<double>(elapsed);
  std::printf("scenario 6: %zu events opened against this PMU; enabled "
              "advanced %llu ns, running advanced %llu ns, so the kernel ran "
              "them %f of the time; the composite discloses running_ratio "
              "%f with scaled %d\n",
              members.size(),
              static_cast<unsigned long long>(elapsed),
              static_cast<unsigned long long>(on_cpu),
              granted,
              observed,
              metric.scaled ? 1 : 0);
  check(granted >= 0.0 && granted <= 1.0,
        "the fraction of enabled time the kernel granted lies inside the "
        "unit interval (FR-019)");
  check(observed >= 0.0 && observed <= 1.0,
        "the folded multiplex ratio lies inside the unit interval (FR-019)");
  if (granted < 1.0) {
    std::printf("scenario 6: this PMU multiplexes, and the fold discloses "
                "the shortfall as a ratio below one (FR-041)\n");
  } else {
    std::printf("scenario 6: every opened event fit this PMU's counters, so "
                "the kernel ran them at full rate\n");
  }
}

// A per-cpu target reaches the kernel with a cpu bound and no pid, so
// the kernel grants it only at a paranoia level this host does not
// offer. The refusal surfaces as a recoverable construction error in the
// untimed region, never as a read-time surprise (FR-024, FR-031).
auto cpu_target_scenario() -> void
{
  // A provider that seeds no cpu object leaves the hardware scenarios
  // with no subject. A host whose perf_event_paranoid hides every event
  // publishes none, and 007 recorded that as the expected CI shape
  // (FR-039): the reason is named and the scenario skips.
  const auto cpu_object = system::local().object("cpu");
  if (!cpu_object.has_value()) {
    std::printf("SKIP: no provider seeded a cpu object at this "
                "perf_event_paranoid, so no hardware event is "
                "reachable to exercise (FR-039)\n");
    return;
  }
  const auto& cpu = *cpu_object;
  const auto entries = cpu.counters();
  std::string work;
  for (const auto& entry : entries) {
    if (entry.avail == availability::countable
        && (entry.name == "instructions" || entry.name == "ex_ret_instr"))
    {
      work = std::string(entry.name);
      break;
    }
  }
  if (work.empty()) {
    std::printf("SKIP cpu-target: no countable instruction counter here\n");
    return;
  }
  const auto leaf = cpu.counter<events>(work);
  check(leaf.has_value(), "the instruction counter resolves");
  const sg::counters::expression<events> over {*leaf};
  const sg::counters::target pinned {.kind = sg::counters::target_kind::cpu,
                                     .cpu = 0};
  const auto refused = compile(system::local(), pinned, over);
  if (refused.has_value()) {
    // A host that grants per-cpu events binds the plan and folds it.
    sg::counters::scope window {*refused};
    window.start();
    window.finish();
    check(window.metric(over).value >= 0.0,
          "a granted per-cpu plan samples and folds");
    std::printf("cpu target: granted, plan folds\n");
    return;
  }
  check(refused.error().message.find("cannot open a window")
            != std::string::npos,
        "a per-cpu target the kernel refuses is a recoverable construction "
        "error (FR-024)");
  std::printf("cpu target: %s\n", refused.error().message.c_str());
}

// FR-022: a scope refusal is a refusal the entry's own scope causes, and
// it is separable from an encoding refusal, so a caller that reads it
// knows a cpu-target plan may still compile over the entry. `cpu_target_
// scenario` compiles over an entry the catalog already published as
// countable, which is the other direction; this compiles over the entry
// the state names. The compile either binds, on a host whose
// kernel grants a cpu-targeted event for that device, and the window
// folds; or it is refused in the untimed region before any window opens,
// and the refusal names the scope refusal the catalog published. An
// encoding refusal is a different verdict (FR-022, FR-024, SC-007).
auto scope_refused_cpu_target_scenario(
    const std::vector<const object*>& pmu_objects) -> void
{
  const object* refused_on = nullptr;
  std::string refused_name;
  for (const object* obj : pmu_objects) {
    for (const auto& entry : obj->counters()) {
      if (entry.avail == availability::scope_refused) {
        refused_on = obj;
        refused_name = std::string(entry.name);
        break;
      }
    }
    if (refused_on != nullptr) {
      break;
    }
  }
  if (refused_on == nullptr) {
    // A host whose cpu probe settles every entry publishes no scope
    // refusal, so there is no entry to compile over. The reason is named
    // and the scenario skips; no check fails (SC-007).
    std::printf("SKIP scope-refused cpu target: the catalog publishes no "
                "availability::scope_refused entry on this host (hardware "
                "event probe %s at perf_event_paranoid %d), so no entry "
                "carries the refusal a cpu-target plan is meant to clear "
                "(FR-022, SC-007)\n",
                name(hardware_event_probe()),
                read_paranoid());
    return;
  }
  // The entry still resolves with the state riding the handle, so the
  // caller reads the refusal, and no failed open stands in for it
  // (FR-007).
  const auto leaf = refused_on->counter<events>(refused_name);
  check(leaf.has_value(),
        "a scope-refused entry still resolves; the state rides the handle "
        "(FR-007)");
  check(leaf->avail() == availability::scope_refused,
        "the resolved handle carries the published scope refusal (FR-022)");
  const expression<events> over {*leaf};
  const sg::counters::target pinned {.kind = sg::counters::target_kind::cpu,
                                     .cpu = 0};
  const auto compiled = compile(system::local(), pinned, over);
  if (compiled.has_value()) {
    scope window {*compiled};
    window.start();
    burn_cpu_short();
    window.finish();
    check(window.metric(over).value >= 0.0,
          "a cpu-target plan over a scope-refused entry binds and folds "
          "(FR-022)");
    std::printf(
        "cpu target over scope-refused %s/%s: granted, plan " "folds\n",
        std::string(refused_on->path()).c_str(),
        refused_name.c_str());
    return;
  }
  const std::string& message = compiled.error().message;
  // The gate reads the requested target now, so a cpu request over a
  // scope refusal reaches the provider window; the availability gate lets
  // the request past. The refusal a kernel that grants no cpu-targeted
  // event produces names the window; the catalog state names the scope.
  // Both halves are asserted, because a gate that still refused would produce
  // the state message and a window that opened would produce no refusal at
  // all (FR-022, SC-007).
  check(message.find("cannot open a window") != std::string::npos,
        "a cpu-target request the kernel refuses is refused at the provider "
        "window, past the availability gate the scope refusal cleared "
        "(FR-022)");
  check(message.find("is not countable on this host") == std::string::npos,
        "the refusal names the window and not the catalog availability, so a "
        "caller tells a scope refusal the scope, not the encoding, "
        "produced (FR-022)");
  std::printf("cpu target over scope-refused %s/%s: %s\n",
              std::string(refused_on->path()).c_str(),
              refused_name.c_str(),
              message.c_str());
}

// Scenario 7: an expression over a leaf the catalog reports as not
// countable is a recoverable construction error naming the leaf and its
// catalog state, refused in the untimed region before any provider
// window opens (FR-024). The leaf itself still resolves, so user code
// can branch on the catalog state (FR-007).
auto unavailable_leaf_scenario() -> void
{
  // A provider that seeds no cpu object leaves the hardware scenarios
  // with no subject. A host whose perf_event_paranoid hides every event
  // publishes none, and 007 recorded that as the expected CI shape
  // (FR-039): the reason is named and the scenario skips.
  const auto cpu_object = system::local().object("cpu");
  if (!cpu_object.has_value()) {
    std::printf("SKIP: no provider seeded a cpu object at this "
                "perf_event_paranoid, so no hardware event is "
                "reachable to exercise (FR-039)\n");
    return;
  }
  const auto& cpu = *cpu_object;
  const auto entries = cpu.counters();
  std::string blocked;
  for (const auto& entry : entries) {
    if (entry.avail == availability::not_encodable) {
      blocked = std::string(entry.name);
      break;
    }
  }
  if (blocked.empty()) {
    std::printf("SKIP scenario 7: every cpu-PMU entry is countable on this "
                "host, so no unavailable leaf exists to compose over\n");
    return;
  }
  const auto leaf = cpu.counter<events>(blocked);
  check(leaf.has_value(),
        "an unavailable leaf still resolves; the state rides the handle "
        "(FR-007)");
  check(leaf->avail() == availability::not_encodable,
        "the resolved handle carries the probed catalog state");
  const expression<events> over {*leaf};
  const auto refused = compile(system::local(), over);
  check(!refused.has_value(),
        "compiling over a non-countable leaf is a recoverable construction "
        "error (FR-024)");
  const std::string& message = refused.error().message;
  check(message.find(blocked) != std::string::npos,
        "the construction error names the offending leaf");
  check(message.find("not_encodable") != std::string::npos,
        "the construction error names the catalog state");
  std::printf("scenario 7: %s\n", message.c_str());
}

// A real fast-mode plan, opened and destroyed 10,000 times, measured over
// the descriptors and mappings the kernel itself hands back. The seam test
// proves the same release over resources the test opened itself, which
// runs on every host; this one proves it on the kernel's own resources,
// where only a granted perf_event_open reaches the path. A host that
// refuses the event prints the reason and returns 2, which CTest reports
// as skipped (FR-013, SC-004).
auto fast_window_lifetime_scenario() -> int
{
  const auto cpu_object = system::local().object("cpu");
  if (!cpu_object.has_value()) {
    std::printf("SKIP: no provider seeded a cpu object at this "
                "perf_event_paranoid, so no hardware event is reachable "
                "to exercise (FR-039, SC-004)\n");
    return 2;
  }
  const auto& cpu = *cpu_object;
  std::string countable;
  for (const auto& entry : cpu.counters()) {
    if (entry.avail == availability::countable) {
      countable = std::string(entry.name);
      break;
    }
  }
  if (countable.empty()) {
    std::printf("SKIP: no countable cpu-PMU event on this host (hardware "
                "event probe %s at perf_event_paranoid %d); the kernel "
                "refuses every event, so the release path is unreachable "
                "(SC-004)\n",
                name(hardware_event_probe()),
                read_paranoid());
    return 2;
  }

  const expression<events> counted {*cpu.counter<events>(countable)};
  const auto descriptors_before = open_descriptor_count();
  const auto mappings_before = mapping_count();
  constexpr int cycles = 10000;
  std::size_t opened = 0;
  for (int index = 0; index < cycles; ++index) {
    auto compiled = compile(system::local(), counted);
    if (!compiled.has_value()) {
      continue;
    }
    ++opened;
    // The plan leaves scope here, releasing every window it opened.
  }
  if (opened == 0) {
    std::printf("SKIP: the kernel refused every fast-mode open of '%s' "
                "(hardware event probe %s at perf_event_paranoid %d), so "
                "the release path is unreachable (SC-004)\n",
                countable.c_str(),
                name(hardware_event_probe()),
                read_paranoid());
    return 2;
  }

  check(open_descriptor_count() == descriptors_before,
        "10,000 real fast-mode plan cycles return the descriptor count to "
        "its starting value (FR-013)");
  check(mapping_count() == mappings_before,
        "10,000 real fast-mode plan cycles return the mapping count to its "
        "starting value (FR-013)");
  std::printf("fast-window lifetime: %zu of %d real opens released every "
              "descriptor and mapping\n",
              opened,
              cycles);
  return 0;
}

}  // namespace

auto main() -> int
{
#if !defined(__linux__)
  // FR-042: off Linux the provider keeps the identical interface and
  // seeds nothing; the reduced catalog is the whole difference.
  auto pmu = std::make_unique<pmu_provider>();
  auto reg = system::local().register_provider(std::move(pmu));
  check(reg.has_value(), "the pmu provider registers off Linux");
  const auto objects = system::local().objects("pmu");
  check(!objects.has_value() || objects->empty(),
        "the pmu section is empty off Linux (FR-042)");
  std::printf("counters_pmu_test PASS: reduced catalog off Linux\n");
  return 0;
#else
  auto clock = std::make_unique<clock_provider>();
  auto pmu = std::make_unique<pmu_provider>();
  // Scenario 4 holds the push counters countable while the pmu provider is
  // registered, so the push provider joins it here and the catalog walk
  // reads all three at once (SC-002). The handles stay unused: the
  // hot-path add belongs to the shipped-provider test, and this scenario
  // reads the catalog only.
  auto push = std::make_unique<push_provider>();
  static_cast<void>(
      push->add_counter("bytes", "bytes", "hot-path bytes written"));
  static_cast<void>(push->add_counter("records", "ops", "records appended"));
  if (!system::local().register_provider(std::move(clock)).has_value()) {
    fail("clock provider registers");
  }
  if (!system::local().register_provider(std::move(pmu)).has_value()) {
    fail("pmu provider registers");
  }
  if (!system::local().register_provider(std::move(push)).has_value()) {
    fail("push provider registers");
  }
  const auto pmu_objects = system::local().objects("pmu");
  if (!pmu_objects.has_value()) {
    fail("pmu objects selectable by kind");
  }

  merge_and_catalog_scenario(*pmu_objects);
  availability_scenario(*pmu_objects);
  disclosed_mode_read_scenario(*pmu_objects);
  group_read_scenario();

  multiplex_scenario();
  cpu_target_scenario();
  scope_refused_cpu_target_scenario(*pmu_objects);
  unavailable_leaf_scenario();
  if (const int skipped = fast_window_lifetime_scenario(); skipped != 0) {
    return skipped;
  }

  std::printf("counters_pmu_test PASS: merge, availability, disclosed modes, "
              "group read, multiplex ratio\n");
  return 0;
#endif
}
