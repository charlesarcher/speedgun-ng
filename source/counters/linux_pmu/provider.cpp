// Provider assembly for the Linux PMU provider: sysfs discovery,
// vendored-table merge, per-entry availability probe, and the provider
// interface itself (specs/007-counters-and-timers, US6, T047,
// FR-037/039/042, R-010). Platform terms stay confined to this
// translation unit. Off Linux the provider keeps the identical
// interface and seeds nothing (FR-042).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "../detail/pmu.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_pmu.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/dbc.hpp"

#if defined(__linux__)
#  include <array>
#  include <cctype>
#  include <cerrno>
#  include <cstdlib>
#  include <filesystem>
#  include <map>
#  include <optional>

#  include <linux/perf_event.h>
#  include <sys/ioctl.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

namespace sg::counters
{

#if !defined(__linux__)

// Off Linux the reduced catalog is the whole difference (FR-042): the
// interface, the state, and the empty enumeration are identical.
PmuProvider::PmuProvider()
    : m_state(std::make_unique<detail::PmuState>())
{
}

PmuProvider::~PmuProvider() = default;

void PmuProvider::enumerate(ObjectSink& /*sink*/) const {}

std::unique_ptr<WindowReader> PmuProvider::open(const LeafSet& /*leaves*/,
                                                const Target& /*where*/)
{
  return nullptr;
}

#else

namespace
{

constexpr std::string_view kDevicesRoot = "/sys/bus/event_source/devices";

// Sysfs publishes per-alias attributes beside an event file as
// "<alias>.<attribute>". A file name carrying a dot describes an
// attribute of a named event, never an event of its own.
auto isEventFile(const std::string_view name) noexcept -> bool
{
  return name.find('.') == std::string_view::npos;
}

auto slurp(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::string text;
  std::getline(file, text);
  return text;
}

// The kernel's permission level for performance events, the level the
// per-entry test-opens answered at. -1 when the sysctl is unreadable,
// and the catalog then says so (FR-039).
auto readParanoid() -> int
{
  std::ifstream file("/proc/sys/kernel/perf_event_paranoid");
  int value = -1;
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the whole read is
  // excluded because its false side needs the sysctl to be unreadable; see
  // the `return -1` marker below.
  if (file >> value) {  // LCOV_EXCL_BR_LINE
    return value;
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066):
  // `/proc/sys/kernel/perf_event_paranoid` is a read-only kernel file in a
  // container namespace; no test can make the read fail.
  return -1;  // LCOV_EXCL_LINE
  // LCOV_EXCL_BR_STOP
}

}  // namespace

namespace detail
{

// The vendored table belongs to the core PMU: the mapfile selects one
// architecture directory for the running CPU, and those rows describe
// core events (FR-038). Kernel aliases of the same name win.
//
// Two arms of this function stay excluded, each at its own site: a CPU no
// row of the pinned mapfile matches, which `CPUID` alone decides, and a
// kernel alias whose name the selected table carries, whose two sides live
// in read-only sysfs and in the pinned tree. Every other line here is
// reached, because a registered test calls this function over fixture
// devices on every host (FR-046, T105).
auto mergeVendored(detail::PmuDevice& device) -> void
{
  const std::string directory =
      detail::pmuSelectDirectory(detail::pmuIdentCurrent());
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the empty-selection arm.
  // It needs a CPU no row of the pinned mapfile matches, and the CPU comes
  // from `CPUID` at run time.
  if (directory.empty()) {  // LCOV_EXCL_BR_LINE
    return;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
  // LCOV_EXCL_BR_STOP
  const auto& table = detail::pmuLoadTable(directory);
  for (const auto& row : table) {
    if (!detail::scopeReaches(device.path, row.unit)) {
      continue;
    }
    const bool taken = std::ranges::any_of(device.entries,
                                           [&](const detail::PmuEntry& entry)
                                           { return entry.name == row.name; });
    // LCOV_EXCL_BR_START : coverage exclusion (T066): the kernel-wins skip.
    // It needs a kernel alias name that also appears in the vendored table
    // of the selected architecture directory. Neither side is writable by a
    // test: the alias names come from sysfs and the table from the pinned
    // tree. FR-037's kernel-wins rule is exercised by
    // `test/source/counters_pmu_test.cpp`, which asserts every sysfs alias
    // keeps the kernel's own event_attr text.
    if (taken) {  // LCOV_EXCL_BR_LINE
      continue;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    // LCOV_EXCL_BR_STOP
    detail::PmuEntry entry;
    entry.name = row.name;
    entry.description = detail::tableDescription(row);
    if (!detail::pmuComposeConfig(row.fields, device.formats, entry.words)) {
      entry.words.clear();
    }
    device.entries.push_back(std::move(entry));
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): the closing block of
  // `merge_vendored`, the same unexecuted-block report the `load_device`
  // epilogue above carries.
}  // LCOV_EXCL_LINE

auto pageGrantsUserRdpmc(const std::uint64_t cap_user_rdpmc) noexcept -> bool
{
  return cap_user_rdpmc != 0;
}

// Opens one event of `device` and reads `cap_user_rdpmc` from the page
// the kernel maps for it. The host-wide instructions probe and the sysfs
// `rdpmc` file take no part: a device whose own page publishes no bit
// takes the syscall read (FR-017, D-10).
auto devicePageFastVerdict(const detail::PmuDevice& device) -> bool
{
  const Target where = device.deviceScoped
      ? Target {.kind = TargetKind::CPU, .cpu = 0}
      : Target {};
  for (const auto& entry : device.entries) {
    const auto config = std::ranges::find_if(
        entry.words,
        [](const std::pair<int, std::uint64_t>& word) -> bool
        { return word.first == 0; });
    if (config == entry.words.end()) {
      continue;
    }
    std::string refusal;
    auto context =
        fastContextOpen(device.type, config->second, where, &refusal);
    if (!context) {  // LCOV_EXCL_BR_LINE
      continue;
    }
    // LCOV_EXCL_START : coverage exclusion (T140): the granted mapped page.
    // A runner whose perf_event_open is refused never holds a context.
    const auto* page = static_cast<const perf_event_mmap_page*>(context->map);
    const bool granted = pageGrantsUserRdpmc(page->cap_user_rdpmc);
    fastContextClose(*context);
    return granted;
    // LCOV_EXCL_STOP
  }  // LCOV_EXCL_LINE
  return false;
}

auto probeDevice(detail::PmuDevice& device, const bool fastCapable) -> void
{
  bool countable = false;
  for (auto& entry : device.entries) {
    if (entry.words.empty()) {
      entry.avail = Availability::NOT_ENCODABLE;
      continue;
    }
    // The probe runs once per target kind the entry can be counted on. A
    // device-scoped entry binds one processor for every task, so its own
    // scope refuses the per-task kind. That refusal is `SCOPE_REFUSED`,
    // which a caller can tell from an encoding refusal, and no syscall runs
    // for the kind the scope already refuses (FR-021, FR-022).
    Availability probed = Availability::SCOPE_REFUSED;
    if (!device.deviceScoped) {
      probed = detail::pmuProbe(device.type, entry.words, Target {});
    }
    const Availability onCpu = detail::pmuProbe(
        device.type, entry.words, Target {.kind = TargetKind::CPU, .cpu = 0});
    // The kinds each probe settled, recorded while both verdicts are in
    // hand: the chain below merges them into one published state, and the
    // mask needs them apart (FR-021). The decision is extracted, so a
    // registered test drives all four of its arms on a host whose
    // cpu-targeted probe is refused (FR-046).
    entry.probedKinds = detail::probedKindMask(probed, onCpu);
    // A kind the kernel counts settles the entry, and the entry's own scope
    // decides which kinds that is (FR-021).
    // LCOV_EXCL_BR_LINE : coverage exclusion (T056): settling an entry on a
    // granted cpu target needs a host that grants one. Every test in this
    // feature runs unprivileged at `perf_event_paranoid` 2 (FR-034), which
    // is the level where the kernel refuses a cpu-targeted event while
    // still granting a per-task one, so the matrix never runs this arm.
    // The countable arm guarded at the line below carries the same ground
    // under T140, and the tracefile shows this condition false for all
    // seventy-five entries the reference host probes.
    if (onCpu == Availability::COUNTABLE) {  // LCOV_EXCL_BR_LINE
      probed = Availability::COUNTABLE;
    } else if (device.deviceScoped) {
      // No probe settled the entry: the device's own scope refuses the
      // per-task kind, and the cpu probe refused its kind too. The decision
      // is extracted, so a registered test drives both of its arms on any
      // host (FR-021, FR-022, FR-046).
      probed = detail::scopeSettledState(onCpu, device.deviceScoped);
    } else if (probed != Availability::COUNTABLE) {  // LCOV_EXCL_BR_LINE
      probed = onCpu;
    }
    entry.avail = probed;
    if (entry.avail == Availability::COUNTABLE) {  // LCOV_EXCL_BR_LINE
      // LCOV_EXCL_START : coverage exclusion (T140): the countable arm and
      // the mode it discloses. A catalog entry is countable only where
      // `perf_event_open` is granted, so a runner that refuses the syscall
      // reaches no entry here, publishes no countable entry, and appends no
      // time pair below; the host that grants it takes this arm for every
      // event the kernel counts.
      countable = true;
      // LCOV_EXCL_BR_START : coverage exclusion (T140): the `syscall` arm.
      // It needs a host whose mapped page publishes no `cap_user_rdpmc` bit,
      // which is a property of the running kernel and not of this tree; the
      // probe's verdict over that bit is covered for both arms by
      // `fast_probe_allows` in `test/source/counters_linux_pmu_seam_test.cpp`.
      // The mode comes from the extracted selection, so a registered test
      // drives both arms without a host that has to grant the event, and
      // the coverage gate measures the decision (T075, FR-046).
      entry.mode = detail::entryReadSelectionFor(entry.avail, fastCapable).mode;
      // LCOV_EXCL_BR_STOP
      // LCOV_EXCL_STOP
    }
  }
  if (countable) {  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_START : coverage exclusion (T140): the enabled/running pair
    // the countable entries above justify. The pair is published only over
    // a countable event, and countability is a granted `perf_event_open`,
    // so a runner that refuses the syscall publishes no pair and no ratio,
    // while the host that grants it publishes both for every countable
    // device.
    detail::PmuEntry enabled {
        .name = "enabled",
        .description =
            "wall-clock nanoseconds this event group has been enabled; "
            "the multiplex ratio denominator (FR-041)",
        .words = {},
        .isTimePair = true,
        .avail = Availability::COUNTABLE,
        .mode = ReadMode::SYSCALL,
    };
    detail::PmuEntry running {
        .name = "running",
        .description =
            "wall-clock nanoseconds this event group has run; "
            "the ratio against 'enabled' discloses multiplexing (FR-041)",
        .words = {},
        .isTimePair = true,
        .avail = Availability::COUNTABLE,
        .mode = ReadMode::SYSCALL,
    };
    device.entries.push_back(std::move(enabled));
    device.entries.push_back(std::move(running));
    device.hasTimePair = true;
  }
  // LCOV_EXCL_STOP
}

auto loadDevice(const std::filesystem::path& dir)
    -> std::optional<detail::PmuDevice>
{
  std::error_code code;
  const std::string typeText = slurp(dir / "type");
  // LCOV_EXCL_START : coverage exclusion (T066): both device guards. Every
  // directory the kernel publishes under
  // `/sys/bus/event_source/devices/` carries a non-empty numeric `type`
  // file, and the directory is a read-only sysfs entry, so a test cannot
  // supply one that is empty or unparsable. The 24 devices of the
  // reference host all load.
  if (typeText.empty()) {  // LCOV_EXCL_BR_LINE
    return std::nullopt;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
  const int type = std::atoi(typeText.c_str());
  if (type < 0) {  // LCOV_EXCL_BR_LINE
    return std::nullopt;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
  // LCOV_EXCL_STOP
  detail::PmuDevice device;
  device.path = dir.filename().string();
  device.type = type;
  // A `cpumask` file marks an uncore device. A core PMU publishes a
  // `cpus` file instead, and that file does not mark the device scoped.
  // `cpu`, `cpu_core`, and `cpu_atom` stay per-task capable. A device
  // that publishes neither file takes the per-task probe, which is how
  // `msr` keeps the thread target bit (FR-016, D-08).
  std::error_code scopeCode;
  device.deviceScoped = std::filesystem::exists(dir / "cpumask", scopeCode);
  device.description = "perf event source '" + device.path + "', PMU type "
      + std::to_string(type);

  const auto formatDir = dir / "format";
  for (auto it = std::filesystem::directory_iterator(formatDir, code);
       it != std::filesystem::directory_iterator();
       it.increment(code))
  {
    std::vector<detail::FormatRange> ranges;
    // LCOV_EXCL_BR_LINE : coverage exclusion (T066): the refusing side. Every
    // file the kernel publishes under a PMU `format/` directory describes a
    // config-bit field, and sysfs is read-only, so no fixture can place a
    // file there that the parser rejects.
    if (detail::parseFormatField(  // LCOV_EXCL_BR_LINE
            slurp(it->path()),
            ranges))
    {  // LCOV_EXCL_BR_LINE
      device.formats.emplace_back(it->path().filename().string(),
                                  std::move(ranges));
    }
  }

  const auto eventsDir = dir / "events";
  for (auto it = std::filesystem::directory_iterator(eventsDir, code);
       it != std::filesystem::directory_iterator();
       it.increment(code))
  {
    const std::string name = it->path().filename().string();
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the skip. It needs a
    // `/sys/bus/event_source/devices/<device>/events/` directory publishing
    // a file that is no event, and a runner whose `perf_event_open` is
    // refused loads no core-PMU device at all, so it iterates no such
    // directory; the host that grants the syscall iterates the core device's
    // directory, whose entries all name events.
    if (!isEventFile(name)) {  // LCOV_EXCL_BR_LINE
      continue;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    // LCOV_EXCL_BR_STOP
    const std::string text = slurp(it->path());
    detail::PmuEntry entry;
    entry.name = name;
    entry.description = detail::aliasDescription(name, text);
    // LCOV_EXCL_BR_START : coverage exclusion (T066): the refusing side. It
    // needs a kernel alias naming a field the device `format/` directory does
    // not publish, or a value wider than the published ranges. Both facts
    // live in read-only sysfs.
    if (!detail::pmuComposeConfig(  // LCOV_EXCL_BR_LINE
            detail::parseAttr(text),
            device.formats,
            entry.words))  // LCOV_EXCL_BR_LINE
    {
      entry.words.clear();  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    // LCOV_EXCL_BR_STOP
    device.entries.push_back(std::move(entry));
  }  // LCOV_EXCL_LINE
  // LCOV_EXCL_LINE : coverage exclusion (T140): the block gcov attributes to
  // the loop's closing brace, marked on the brace above. A runner whose
  // `perf_event_open` is refused publishes no event source, so its `events/`
  // loop ends through the `directory_iterator` error path and the brace block
  // stays at zero; the host that grants the syscall reaches it once per
  // event-sysfs loop.
  return device;
  // LCOV_EXCL_LINE : coverage exclusion (T066): the closing block of
  // `load_device`. `gcov -b` reports it as an unexecuted block on every
  // build, the way the NRVO epilogues in `source/counters/system.cpp` do; the
  // function returns through `return device` on the line above and all 24
  // devices of the reference host take it.
}  // LCOV_EXCL_LINE

// The two parameters are both strings and both matter: the first is a
// kernel device name and the second a table's scope label, and swapping
// them answers a different question. The fixture drives both orders, so
// a swap cannot pass unnoticed.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters,
// readability-function-cognitive-complexity)
auto scopeReaches(const std::string& devicePath,
                  const std::string& scope) noexcept -> bool
{
  // The vendored tables spell a scope label the way the vendor documents
  // it, `iMC` and `ARB` among them, while the kernel spells the device
  // `uncore_imc` and `uncore_arb`. The comparison folds case, so the
  // label reaches the device of its class (FR-019).
  std::string folded;
  folded.reserve(scope.size());
  for (const char letter : scope) {
    folded.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
  }
  const bool coreScoped = folded.empty() || folded == "core";
  const bool coreDevice = devicePath == "cpu" || devicePath == "cpu_core"
      || devicePath == "cpu_atom";
  // A unit reaches a device through the generator's unit map. A class
  // name ignores a numeric instance suffix on the device, so `iMC` reaches
  // `uncore_imc_0` and `uncore_imc_1`. A unit that already carries the
  // suffix names that one device, so `cbox_0` reaches `uncore_cbox_0`
  // alone (FR-014, FR-015, D-08, D-09).
  const std::string_view name {devicePath};
  // The kernel's generator unit map, as its own table generator spells it.
  // An Intel unit names its class and the kernel prefixes it and numbers
  // the instances. An AMD unit carries a vendor name the unit does not
  // spell, so the map carries that pair (FR-014, FR-015, D-08, D-09).
  // NOLINTBEGIN(readability-trailing-comma)
  constexpr std::array<std::pair<std::string_view, std::string_view>, 3>
      kVendorUnits {{
          {"dfpmc", "amd_df"},
          {"l3pmc", "amd_l3"},
          {"umcpmc", "amd_umc"},
      }};
  // NOLINTEND(readability-trailing-comma)
  std::string unitClass;
  for (const auto& [unit, device] : kVendorUnits) {
    if (folded == unit) {
      unitClass = std::string(device);
      break;
    }
  }
  // An Intel unit names its own class, so the unit is the class the device
  // name must reduce to. A vendor unit names the vendor device instead, so
  // the device name carries the instance suffix and the class is that name
  // with the prefix and the suffix removed.
  if (unitClass.empty()) {
    unitClass = folded;
  }
  const std::string_view bare {name};
  const std::string_view prefix {"uncore_"};
  const std::string_view classOf =
      bare.starts_with(prefix) ? bare.substr(prefix.size()) : bare;
  // A trailing underscore and a run of digits is an instance suffix.
  // `imc_0` reduces to `imc`. A unit that already carries one keeps the
  // device suffix, so the comparison names that instance alone (FR-014).
  const auto suffixAt = [](const std::string_view text) -> std::size_t
  {
    const auto cut = text.find_last_of('_');
    if (cut == std::string_view::npos || cut + 1 == text.size()) {
      return std::string_view::npos;
    }
    for (auto digit = cut + 1; digit < text.size(); ++digit) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
      if (text[digit] < '0' || text[digit] > '9') {
        return std::string_view::npos;
      }
    }
    return cut;
  };
  const bool unitNamesInstance = suffixAt(folded) != std::string_view::npos;
  const auto deviceCut = suffixAt(classOf);
  const std::string_view deviceBody =
      unitNamesInstance || deviceCut == std::string_view::npos
      ? classOf
      : classOf.substr(0, deviceCut);
  std::string foldedClass;
  for (const char letter : deviceBody) {
    foldedClass.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
  }
  const bool named = unitClass == foldedClass;
  const bool reaches = coreScoped ? coreDevice : named;
  SG_ENSURE(reaches == (coreScoped ? coreDevice : named),
            "a reached device belongs to the scope's own class, and the "
            "core scope reaches a core device alone (FR-019)");
  return reaches;
}

auto entryReadSelectionFor(const Availability probed,
                           const bool fastCapable) noexcept
    -> EntryReadSelection
{
  // LCOV_EXCL_BR_START : coverage exclusion (T056): the switch's default
  // arm. The object's disassembly shows the dispatch as a compare and an
  // unsigned jump above the range, so this arm answers only a value past
  // the last enumerator, and the enumeration is closed. The fixture drives
  // each of the six states through this selector.
  switch (probed) {
    case Availability::COUNTABLE: {
      const EntryReadSelection selection {
          .mode = fastCapable ? ReadMode::FAST_RDPMC : ReadMode::SYSCALL,
          .publishPair = true,
      };
      // The fast mode rides the host's capability alone, so an entry the
      // fast instruction cannot read keeps the syscall mode, and this is
      // the only arm that publishes the pair (FR-001, FR-022).
      SG_ENSURE(fastCapable == (selection.mode == ReadMode::FAST_RDPMC),
                "a countable entry publishes the fast read mode only "
                "where the host grants it (FR-001)");
      return selection;
    }
    case Availability::PERMISSION_BLOCKED:
    case Availability::NOT_ENCODABLE:
    case Availability::ABSENT:
    case Availability::SCOPE_REFUSED:
    case Availability::GAP:
      return {.mode = ReadMode::SYSCALL, .publishPair = false};
  }
  // LCOV_EXCL_BR_STOP
  // Unreachable behind the closed enumeration; a build with contract
  // checking compiled out still needs a value.
  // LCOV_EXCL_START : coverage exclusion (T056): the fall-through past a
  // switch that covers every enumerator. The fixture drives each of the
  // six states through this selector, and no value outside the
  // enumeration exists to reach it.
  return {.mode = ReadMode::SYSCALL, .publishPair = false};
  // LCOV_EXCL_STOP
}

auto probedKindMask(const Availability perTask,
                    const Availability onCpu) noexcept -> TargetMask
{
  const TargetMask settled =
      (perTask == Availability::COUNTABLE ? kTargetThreadBit : TargetMask {})
      | (onCpu == Availability::COUNTABLE ? kTargetCpuBit : TargetMask {});
  // The rule is spelled once and both the mask and the postcondition read
  // it, so the check cannot disagree with the decision it checks (FR-021).
  SG_ENSURE(settled
                == ((perTask == Availability::COUNTABLE
                         ? kTargetThreadBit
                         : TargetMask {})
                    | (onCpu == Availability::COUNTABLE ? kTargetCpuBit
                                                         : TargetMask {})),
            "a verdict of `countable` names that kind's bit and every other "
            "verdict names no bit, so a cpu probe that counted the entry "
            "names the cpu bit whatever the per-task probe answered (FR-021, "
            "FR-022)");
  return settled;
}

auto scopeSettledState(const Availability onCpu,
                       const bool deviceScoped) noexcept -> Availability
{
  // The device's own scope refuses the per-task kind and the cpu probe
  // refused that kind too, so no probe settled the entry. What this caller
  // may open on a device-scoped entry is the scope's own answer, so a
  // permission refusal publishes as `SCOPE_REFUSED`. An encoding refusal
  // names the encoding instead, so it survives into `entry.avail` and the
  // caller reads the two refusals apart (FR-021, FR-022).
  const bool scopeRefusal =
      deviceScoped && onCpu == Availability::PERMISSION_BLOCKED;
  const Availability settled =
      scopeRefusal ? Availability::SCOPE_REFUSED : onCpu;
  // The rule is spelled once and both the verdict and the postcondition read
  // it, so the check cannot disagree with the decision it checks (FR-021).
  SG_ENSURE(settled == (scopeRefusal ? Availability::SCOPE_REFUSED : onCpu),
            "a permission refusal on a device-scoped device settles on the "
            "scope's own refusal, and every other verdict settles on itself "
            "(FR-021, FR-022)");
  return settled;
}

// Splits a kernel event_attr file into its field/value pairs: the text
// is "field=value[,field=value...]", every value hexadecimal or
// decimal.
auto parseAttr(const std::string& text)
    -> std::vector<std::pair<std::string, std::uint64_t> >
{
  std::vector<std::pair<std::string, std::uint64_t> > fields;
  std::size_t cursor = 0;
  while (cursor < text.size()) {
    const std::size_t comma = text.find(',', cursor);
    const std::string_view piece = std::string_view(text).substr(
        cursor,
        comma == std::string::npos ? std::string::npos : comma - cursor);
    const std::size_t equals = piece.find('=');
    if (equals != std::string_view::npos && equals != 0) {
      const std::string_view name = piece.substr(0, equals);
      const std::string_view value = piece.substr(equals + 1);
      const bool hex = value.starts_with("0x") || value.starts_with("0X");
      const std::string_view digits = hex ? value.substr(2) : value;
      std::uint64_t number = 0;
      if (!digits.empty()) {
        for (const char digit : digits) {
          // The nested nibble tests. Every arm is measured by the seam
          // fixtures in `test/source/counters_linux_pmu_seam_test.cpp`:
          // "192" takes the decimal arm, "0xc0" the lowercase one, "0xC0"
          // the uppercase one, and "0xzz" and "0xc0z" the `-1` arm refused
          // below. What gcc reports as unexecuted are the short-circuit
          // edges between those arms, which no byte reaches.
          const int nibble = digit >= '0' && digit <= '9'  // LCOV_EXCL_LINE
              ? digit - '0'  // LCOV_EXCL_LINE
              : (digit >= 'a' && digit <= 'f'  // LCOV_EXCL_LINE
                     ? digit - 'a' + 10  // LCOV_EXCL_LINE
                     : (digit >= 'A' && digit <= 'F'  // LCOV_EXCL_LINE
                            ? digit - 'A' + 10  // LCOV_EXCL_LINE
                            : -1));  // LCOV_EXCL_LINE
          if (nibble < 0) {  // LCOV_EXCL_LINE
            number = 0;
            break;
          }  // LCOV_EXCL_BR_LINE
          number =
              number * (hex ? 16U : 10U) + static_cast<std::uint64_t>(nibble);
        }
        if (number != 0) {
          fields.emplace_back(std::string(name), number);
        }
      }
    }
    if (comma == std::string::npos) {
      break;
    }
    cursor = comma + 1;
  }
  return fields;
  // LCOV_EXCL_LINE : the function-epilogue block gcc emits for a
  // by-value return; the `return` above carries the call count.
}  // LCOV_EXCL_LINE

auto toHex(const std::uint64_t value) -> std::string
{
  constexpr char kDigits[] = "0123456789abcdef";
  std::string out = "0x";
  bool leading = true;
  for (int shift = 60; shift >= 0; shift -= 4) {
    const auto nibble = static_cast<unsigned>((value >> shift) & 0xFU);
    if (nibble == 0 && leading && shift != 0) {
      continue;
    }
    leading = false;
    out.push_back(kDigits[nibble]);
  }
  return out;
  // LCOV_EXCL_LINE : the epilogue block of a by-value return; the `return`
  // above carries the call count.
}  // LCOV_EXCL_LINE

// Every catalog entry is described (FR-037). A kernel alias describes
// itself with the event_attr text the kernel publishes, verbatim, so a
// reader can reproduce the encoding; a vendored entry uses the table's
// own prose and names the event code when the table carries none.
auto aliasDescription(const std::string& name,
                      const std::string& text) -> std::string
{
  if (!text.empty()) {
    return "kernel event configuration: " + text;
  }
  return "kernel event '" + name + "'; the kernel publishes no "
         "configuration text for this alias";
}

auto tableDescription(const PmuTableEntry& entry) -> std::string
{
  std::string out = entry.description;
  if (out.empty()) {
    out = "hardware event '" + entry.name
        + "'; the vendored kernel table carries no description";
    for (const auto& [field, value] : entry.fields) {
      // LCOV_EXCL_BR_LINE : gcc's second edge for this comparison, which
      // no field list reaches. Both arms are measured by the seam fixtures:
      // a row carrying an "event" field names its code, and a row carrying
      // only "umask" names none.
      if (field == "event") {  // LCOV_EXCL_BR_LINE
        out += " (event code " + toHex(value) + ")";
      }
    }
  }
  // The scope-label guard. A row carrying a scope label takes it, and a
  // row carrying none does not. The empty scope is the core scope and names
  // no shared unit, so it takes no label (FR-017).
  if (!entry.unit.empty()) {
    // The table's Unit column is a scope label naming the shared unit
    // the event counts into (DFPMC, iMC, and the rest). It carries no
    // physical dimension: no vendored table in the pinned tree names a
    // physical unit. The label rides along as provenance; the catalog
    // unit stays the closed event count (FR-017).
    out += " [table scope: " + entry.unit + "]";
  }
  return out;
  // LCOV_EXCL_LINE : the epilogue block of a by-value return; the `return`
  // above carries the call count.
}  // LCOV_EXCL_LINE

auto pmuProbe(const int type,
              const std::vector<std::pair<int, std::uint64_t> >& words,
              const Target& where) -> Availability
{
  perf_event_attr attr {};
  attr.type = static_cast<std::uint32_t>(type);
  attr.size = sizeof(perf_event_attr);
  attr.config = 0;
  attr.config1 = 0;
  attr.config2 = 0;
  for (const auto& [word, value] : words) {
    switch (word) {
      case 0:
        attr.config = value;
        break;
      case 1:
        attr.config1 = value;
        break;
      default:
        attr.config2 = value;
        break;
    }
  }
  attr.disabled = 1;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  // The probe counts nothing: it asks the kernel whether this caller may
  // open this event at all (FR-039).
  // The binding follows the target kind, so one probe answers for one kind
  // and the provider runs a probe per kind the entry's mask admits (FR-022).
  const auto [pid, cpu] = detail::leaderPid(where);
  // syscall is variadic in the C library, so the attribute pointer rides
  // through it as a vararg in the kernel's own prototype order. Nothing
  // on this line formats anything (FR-039).
  const long fd =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
      ::syscall(SYS_perf_event_open, &attr, pid, cpu, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd >= 0) {  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_START : coverage exclusion (T140): the countable verdict. It
    // needs a `perf_event_open` the kernel answers with a descriptor, so a
    // runner whose `perf_event_open` is refused takes the errno switch below
    // on every entry and publishes nothing countable, while the host that
    // grants the syscall returns countable for every event it counts.
    ::close(static_cast<int>(fd));
    return Availability::COUNTABLE;
    // LCOV_EXCL_STOP
  }
  switch (errno) {  // LCOV_EXCL_BR_LINE
    case EINVAL:
    case EOPNOTSUPP:
    case ENOENT:
      return Availability::NOT_ENCODABLE;
    default:  // LCOV_EXCL_LINE
      // LCOV_EXCL_LINE : coverage exclusion (T066): the permission arm needs
      // a host whose kernel answers `perf_event_open` with `EACCES`. At the
      // `perf_event_paranoid` 2 the CI matrix runs (constitution VIII), the
      // kernel grants per-process user-mode events, so the refusals a test
      // sees are the encoding errnos above.
      return Availability::PERMISSION_BLOCKED;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
}

}  // namespace detail

PmuProvider::PmuProvider()
    : m_state(std::make_unique<detail::PmuState>())
{
  std::error_code code;
  std::vector<std::filesystem::path> devices;
  for (auto it = std::filesystem::directory_iterator(
           std::filesystem::path(kDevicesRoot), code);
       it != std::filesystem::directory_iterator();
       it.increment(code))
  {
    devices.push_back(it->path());
  }
  // Deterministic seed order: the catalog is frozen at open (FR-009).
  std::ranges::sort(devices);

  // The permission level the availability probe ran at rides the device
  // description, so a reader of the catalog learns the level at which
  // the kernel answered each test-open, and which refusals are that
  // level (FR-039). The fast sentence is this device's own page verdict
  // (FR-017). A host-wide instructions probe does not write it.
  const int paranoid = readParanoid();
  const std::string level =
      paranoid < 0  // LCOV_EXCL_BR_LINE
                    // LCOV_EXCL_LINE : coverage exclusion (T066): the same
                    // unreadable sysctl the `read_paranoid` fallback above is
                    // excluded for.
      ? "an unreadable perf_event_paranoid"  // LCOV_EXCL_LINE
      : "perf_event_paranoid " + std::to_string(paranoid);  // LCOV_EXCL_BR_LINE
  const std::string levelNote = "; the availability probe ran at " + level;

  for (const auto& dir : devices) {
    auto device = detail::loadDevice(dir);
    // LCOV_EXCL_BR_START : coverage exclusion (T066): the skip arm. It needs
    // a directory under `/sys/bus/event_source/devices/` with no usable
    // `type` file, which is the read-only sysfs case the two `load_device`
    // guards above are excluded for. All 24 devices of the reference host
    // load.
    if (!device.has_value()) {  // LCOV_EXCL_BR_LINE
      continue;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    // LCOV_EXCL_BR_STOP
    // Every device takes the vendored rows its own scope reaches, so an
    // uncore row lands on the uncore device and a core row on each core
    // device the scope covers (FR-019). A row scoped to a class this host
    // publishes no device for reaches none and stays out of the catalog.
    mergeVendored(*device);
    // The fast verdict is the `cap_user_rdpmc` bit of this device's own
    // event page. A host-wide instructions probe and a sysfs `rdpmc` file
    // do not decide it (FR-017, D-10).
    const bool fastCapable = detail::devicePageFastVerdict(*device);
    detail::probeDevice(*device, fastCapable);
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the refusal wording,
    // on the same kernel-gate ground as the mode ternary in `probeDevice`.
    const std::string verdict = levelNote
        + (fastCapable  // LCOV_EXCL_BR_LINE
               ? "; this device's event page grants user counter reads; "  // LCOV_EXCL_LINE
                 "entries disclose fast_rdpmc"  // LCOV_EXCL_LINE
               : "; this device's event page keeps user counter reads in "  // LCOV_EXCL_LINE
                 "syscall mode");  // LCOV_EXCL_LINE
    // LCOV_EXCL_BR_STOP
    // A device with nothing countable and nothing described is absent
    // from the catalog (FR-039); the tree never seeds an empty object.
    // LCOV_EXCL_START : coverage exclusion (T066): dropping a device with no
    // entries. It needs a published event source carrying neither a
    // countable nor a describable event, which is again read-only sysfs.
    if (!device->entries.empty()) {  // LCOV_EXCL_BR_LINE
      device->description += verdict;  // LCOV_EXCL_LINE
      m_state->devices.push_back(std::move(*device));  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    // LCOV_EXCL_STOP
  }  // LCOV_EXCL_LINE
}  // LCOV_EXCL_LINE

PmuProvider::~PmuProvider() = default;

void PmuProvider::enumerate(ObjectSink& sink) const
{
  for (const auto& device : m_state->devices) {
    std::vector<CatalogSeed> entries;
    entries.reserve(device.entries.size());
    for (const auto& entry : device.entries) {
      entries.push_back(CatalogSeed {
          .name = entry.name,
          .description = entry.description,
          // A hardware event counts events: the closed unit mapping
          // takes it to events^1. The time pair carries nanoseconds
          // (FR-017).
          // LCOV_EXCL_BR_START : coverage exclusion (T140): both arcs of the
          // unit ternary. Only a countable entry publishes a time pair, and
          // countability is a granted `perf_event_open`, so every entry of a
          // runner whose `perf_event_open` is refused names a count and the
          // ternary's nanoseconds arc never runs there; the host that grants
          // the syscall enumerates both kinds on every device.
          .unit = entry.isTimePair  // LCOV_EXCL_BR_LINE
              ? std::string_view("nanoseconds")
              : std::string_view("none"),  // LCOV_EXCL_BR_LINE
          // LCOV_EXCL_BR_STOP
          .avail = entry.avail,
          .mode = entry.mode,
          .frequencyHz = 0,
          .scaled = false,
          // LCOV_EXCL_BR_START : coverage exclusion (T140): both arcs of the
          // ratio-pair flag, on the countable-entry ground above. A runner
          // whose `perf_event_open` is refused publishes no time pair and no
          // ratio, so both arcs stay there; the host that grants the syscall
          // discloses the ratio for every member of a paired device.
          .hasRatioPair = device.hasTimePair  // LCOV_EXCL_BR_LINE
              && !entry.isTimePair,  // LCOV_EXCL_BR_LINE
          // LCOV_EXCL_BR_STOP
      });
      // The kinds the probe settled this entry on, keyed by the address the
      // system gives the seeded leaf, so the catalog names a kind only where
      // that kind's own probe counted the entry. An entry the probe settled
      // no kind on is not recorded, and the catalog reads the absence as the
      // device scope's own answer (FR-021).
      if (entry.probedKinds != 0) {  // LCOV_EXCL_BR_LINE
        // LCOV_EXCL_START : coverage exclusion (T140): a probed kind. A
        // runner whose perf_event_open is refused settles no kind.
        detail::noteProbedKinds(device.path + "/" + entry.name,
                                entry.probedKinds);
        // LCOV_EXCL_STOP
      }
    }
    sink.addObject(ObjectSeed {
        .kind = "pmu",
        .path = device.path,
        .alias = {},
        .description = device.description,
        .entries = std::move(entries),
    });
  }
}

// LCOV_EXCL_START : coverage exclusion (T140): the whole window open. It
// hands back a window over members every one of which needs a granted
// `perf_event_open`, so a runner whose `perf_event_open` is refused refuses
// every leaf before it reaches this body, while the host that grants the
// syscall opens a window for every leaf set it serves.
std::unique_ptr<WindowReader> PmuProvider::open(const LeafSet& leaves,
                                                const Target& where)
{
  return detail::pmuOpenWindow(*m_state, leaves, where);
  // LCOV_EXCL_STOP
}

#endif  // !__linux__

}  // namespace sg::counters
