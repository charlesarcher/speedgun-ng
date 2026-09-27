// ============================================================================
// TDD test for the Linux PMU provider (T043; US6 scenarios 1..7).
//
// Public-surface testing only: the detail seam functions (ident,
// directory selection, encode) are not exported and no test in this
// repository includes source/ internals, so selection and encoding are
// asserted through catalog shape: the vendored table entered the cpu
// object only if CPUID selection matched a directory, and every
// catalog state comes from the probe/encode pipeline. Privileged
// evidence (multiplex ratio below 1, developer host) is recorded in
// the PR per tasks.md; this test never fails for missing privileges
// (SC-002). Registration precedes the open boundary; frameworkless
// check()/fail() convention.
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
using sg::counters::read_mode;
using sg::counters::scope;
using sg::counters::system;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

constexpr std::string_view kDevicesRoot = "/sys/bus/event_source/devices";

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
auto sysfs_alias_text(const std::string& device, const std::string& alias)
    -> std::string
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
  for (const object* obj : pmu_objects) {
    const auto entries = obj->counters();
    check(!entries.empty(), "every pmu object carries catalog entries");
    const auto aliases = sysfs_alias_names(std::string(obj->path()));
    for (const auto& entry : entries) {
      check(!entry.description.empty(),
            "every pmu catalog entry is described (FR-037)");
      // Scenario 3's closed-state assertion travels here: the probe
      // pipeline only ever reports one of these three states; absent
      // objects are never seeded (FR-039).
      check(entry.avail == availability::countable
                || entry.avail == availability::permission_blocked
                || entry.avail == availability::not_encodable,
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
}

// Scenario 4: the reported availability is consistent with the probe
// result and with the host's paranoia level. Clocks and push stay
// countable; hardware entries report whatever the kernel's own test-open
// says, at whatever level the host runs (never a hard-fail either way,
// SC-002).
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

  std::size_t countable = 0;
  std::size_t blocked = 0;
  std::size_t unencodable = 0;
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
        case availability::absent:
          fail("absent is never seeded by the provider");
      }
      if (entry.mode == read_mode::fast_rdpmc) {
        ++fast;
      }
    }
  }
  // The paranoid-2 hardware-entry state, recorded from the catalog this
  // host produced: the counts beside the probe that produced them.
  std::printf("pmu availability: %zu countable, %zu permission_blocked, "
              "%zu not_encodable, %zu fast_rdpmc\n",
              countable,
              blocked,
              unencodable,
              fast);

  // The fast (mapped-page rdpmc) read is the one mechanism the provider
  // gates on the paranoid level, at "the kernel grants user counter
  // reads at 1 or below". Above that level no entry may claim it, and
  // the claim is a catalog fact a reader can check (FR-023, FR-039).
  if (paranoid > 1) {
    check(fast == 0,
          "no PMU entry claims fast_rdpmc above perf_event_paranoid 1 "
          "(FR-023, FR-039)");
  }
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
            "a permission refusal surfaces as permission_blocked, not "
            "not_encodable (FR-039)");
    } else {
      check(unencodable > 0,
            "an encoding refusal surfaces as not_encodable, not "
            "permission_blocked (FR-039)");
    }
  }
}

// Scenario 5: a group of resolved countable events samples in
// syscall mode through a real scope; the folded quotient equals the
// raw-column quotient exactly (quickstart 10). Skips with the reason
// printed when the host refuses (SC-002).
auto group_read_scenario() -> void
{
  const auto cpu = *system::local().object("cpu");
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
  check(delta_enabled > 0, "time_enabled advances across the window");
  check(delta_running > 0 && delta_running <= delta_enabled,
        "time_running stays inside time_enabled (ratio pair sound)");
  std::printf(
      "group read delivered members and the enabled/running pair; " "ratio "
                                                                    "%f\n",
      static_cast<double>(delta_running) / static_cast<double>(delta_enabled));
  // Privileged multiplex evidence (ratio below 1 under contention)
  // is scenario 6: developer-host material for the PR per tasks.md.
  std::printf("fold disclosure: running_ratio %f, scaled %d\n",
              metric.running_ratio,
              metric.scaled ? 1 : 0);
}

// A per-cpu target reaches the kernel with a cpu bound and no pid, so
// the kernel grants it only at a paranoia level this host does not
// offer. The refusal surfaces as a recoverable construction error in the
// untimed region, never as a read-time surprise (FR-024, FR-031).
auto cpu_target_scenario() -> void
{
  const auto cpu = *system::local().object("cpu");
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

// Scenario 7: an expression over a leaf the catalog reports as not
// countable is a recoverable construction error naming the leaf and its
// catalog state, refused in the untimed region before any provider
// window opens (FR-024). The leaf itself still resolves, so user code
// can branch on the catalog state (FR-007).
auto unavailable_leaf_scenario() -> void
{
  const auto cpu = *system::local().object("cpu");
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
  if (!system::local().register_provider(std::move(clock)).has_value()) {
    fail("clock provider registers");
  }
  if (!system::local().register_provider(std::move(pmu)).has_value()) {
    fail("pmu provider registers");
  }
  const auto pmu_objects = system::local().objects("pmu");
  if (!pmu_objects.has_value()) {
    fail("pmu objects selectable by kind");
  }

  merge_and_catalog_scenario(*pmu_objects);
  availability_scenario(*pmu_objects);
  group_read_scenario();

  // Scenario 6 (multiplexed group, ratio below 1, kernel scaled
  // estimate) needs the privilege to oversubscribe counters: it is
  // developer-privileged evidence recorded in the PR per tasks.md.
  std::printf("SKIP scenario 6: multiplex ratio needs a privileged host; "
              "developer evidence recorded in the PR (tasks.md T043)\n");
  cpu_target_scenario();
  unavailable_leaf_scenario();

  std::printf("counters_pmu_test PASS: merge, availability, group read\n");
  return 0;
#endif
}
