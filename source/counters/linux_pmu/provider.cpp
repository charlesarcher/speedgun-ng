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

#if defined(__linux__)
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
pmu_provider::pmu_provider()
    : m_state(std::make_unique<detail::pmu_state>())
{
}

pmu_provider::~pmu_provider() = default;

void pmu_provider::enumerate(object_sink& /*sink*/) const {}

std::unique_ptr<window_reader> pmu_provider::open(const leaf_set& /*leaves*/,
                                                  const target& /*where*/)
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
auto is_event_file(const std::string_view name) noexcept -> bool
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
auto read_paranoid() -> int
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

auto probe_device(detail::pmu_device& device, const bool fast_capable) -> void
{
  bool countable = false;
  for (auto& entry : device.entries) {
    if (entry.words.empty()) {
      entry.avail = availability::not_encodable;
      continue;
    }
    // The probe runs once per target kind the entry can be counted on. A
    // device-scoped entry binds one processor for every task, so its own
    // scope refuses the per-task kind. That refusal is `scope_refused`,
    // which a caller can tell from an encoding refusal, and no syscall runs
    // for the kind the scope already refuses (FR-021, FR-022).
    availability probed = availability::scope_refused;
    if (!device.device_scoped) {
      probed = detail::pmu_probe(device.type, entry.words, target {});
    }
    const availability on_cpu = detail::pmu_probe(
        device.type, entry.words, target {.kind = target_kind::cpu, .cpu = 0});
    // A kind the kernel counts settles the entry, and the entry's own scope
    // decides which kinds that is (FR-021).
    if (on_cpu == availability::countable) {
      probed = availability::countable;
    } else if (probed != availability::countable) {
      probed = on_cpu;
    }
    entry.avail = probed;
    if (entry.avail == availability::countable) {  // LCOV_EXCL_BR_LINE
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
      entry.mode =
          detail::entry_read_selection_for(entry.avail, fast_capable).mode;
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
    detail::pmu_entry enabled {
        .name = "enabled",
        .description =
            "wall-clock nanoseconds this event group has been enabled; "
            "the multiplex ratio denominator (FR-041)",
        .words = {},
        .is_time_pair = true,
        .avail = availability::countable,
        .mode = read_mode::syscall,
    };
    detail::pmu_entry running {
        .name = "running",
        .description =
            "wall-clock nanoseconds this event group has run; "
            "the ratio against 'enabled' discloses multiplexing (FR-041)",
        .words = {},
        .is_time_pair = true,
        .avail = availability::countable,
        .mode = read_mode::syscall,
    };
    device.entries.push_back(std::move(enabled));
    device.entries.push_back(std::move(running));
    device.has_time_pair = true;
  }
  // LCOV_EXCL_STOP
}

auto load_device(const std::filesystem::path& dir)
    -> std::optional<detail::pmu_device>
{
  std::error_code code;
  const std::string type_text = slurp(dir / "type");
  // LCOV_EXCL_START : coverage exclusion (T066): both device guards. Every
  // directory the kernel publishes under
  // `/sys/bus/event_source/devices/` carries a non-empty numeric `type`
  // file, and the directory is a read-only sysfs entry, so a test cannot
  // supply one that is empty or unparsable. The 24 devices of the
  // reference host all load.
  if (type_text.empty()) {  // LCOV_EXCL_BR_LINE
    return std::nullopt;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
  const int type = std::atoi(type_text.c_str());
  if (type < 0) {  // LCOV_EXCL_BR_LINE
    return std::nullopt;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
  // LCOV_EXCL_STOP
  detail::pmu_device device;
  device.path = dir.filename().string();
  device.type = type;
  // The core PMU and its hybrid per-core instances count a thread's own
  // events; every other published device binds one processor for every
  // task, so its entries refuse a per-task target by their own scope.
  device.device_scoped = device.path != "cpu" && device.path != "cpu_core"
      && device.path != "cpu_atom";
  device.description = "perf event source '" + device.path + "', PMU type "
      + std::to_string(type);

  const auto format_dir = dir / "format";
  for (auto it = std::filesystem::directory_iterator(format_dir, code);
       it != std::filesystem::directory_iterator();
       it.increment(code))
  {
    std::vector<detail::format_range> ranges;
    // LCOV_EXCL_BR_LINE : coverage exclusion (T066): the refusing side. Every
    // file the kernel publishes under a PMU `format/` directory describes a
    // config-bit field, and sysfs is read-only, so no fixture can place a
    // file there that the parser rejects.
    if (detail::parse_format_field(  // LCOV_EXCL_BR_LINE
            slurp(it->path()),
            ranges))
    {  // LCOV_EXCL_BR_LINE
      device.formats.emplace_back(it->path().filename().string(),
                                  std::move(ranges));
    }
  }

  const auto events_dir = dir / "events";
  for (auto it = std::filesystem::directory_iterator(events_dir, code);
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
    if (!is_event_file(name)) {  // LCOV_EXCL_BR_LINE
      continue;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    // LCOV_EXCL_BR_STOP
    const std::string text = slurp(it->path());
    detail::pmu_entry entry;
    entry.name = name;
    entry.description = detail::alias_description(name, text);
    // LCOV_EXCL_BR_START : coverage exclusion (T066): the refusing side. It
    // needs a kernel alias naming a field the device `format/` directory does
    // not publish, or a value wider than the published ranges. Both facts
    // live in read-only sysfs.
    if (!detail::pmu_compose_config(  // LCOV_EXCL_BR_LINE
            detail::parse_attr(text),
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

// The vendored table belongs to the core PMU: the mapfile selects one
// architecture directory for the running CPU, and those rows describe
// core events (FR-038). Kernel aliases of the same name win.
// LCOV_EXCL_START : coverage exclusion (T140): the whole merge. The rows it
// adds are the core-PMU events a runner whose `perf_event_open` is refused
// never enumerates, because the kernel grants it no core event source to
// probe, so the constructor below calls this only on a host that grants the
// syscall. Every device the refused runner loads is a non-core device, and
// every non-core device takes the kernel-wins arm above.
auto merge_vendored(detail::pmu_device& device) -> void
{
  const std::string directory =
      detail::pmu_select_directory(detail::pmu_ident_current());
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the empty-selection arm.
  // It needs a CPU no row of the pinned mapfile matches, and the CPU comes
  // from `CPUID` at run time.
  if (directory.empty()) {  // LCOV_EXCL_BR_LINE
    return;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
  // LCOV_EXCL_BR_STOP
  const auto& table = detail::pmu_load_table(directory);
  for (const auto& row : table) {
    if (!detail::scope_reaches(device.path, row.unit)) {
      continue;
    }
    const bool taken = std::ranges::any_of(device.entries,
                                           [&](const detail::pmu_entry& entry)
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
    detail::pmu_entry entry;
    entry.name = row.name;
    entry.description = detail::table_description(row);
    if (!detail::pmu_compose_config(row.fields, device.formats, entry.words)) {
      entry.words.clear();
    }
    device.entries.push_back(std::move(entry));
  }
  // LCOV_EXCL_STOP
  // LCOV_EXCL_LINE : coverage exclusion (T066): the closing block of
  // `merge_vendored`, the same unexecuted-block report the `load_device`
  // epilogue above carries.
}  // LCOV_EXCL_LINE
}  // namespace

namespace detail
{

// The two parameters are both strings and both matter: the first is a
// kernel device name and the second a table's scope label, and swapping
// them answers a different question. The fixture drives both orders, so
// a swap cannot pass unnoticed.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto scope_reaches(const std::string& device_path,
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
  if (folded.empty() || folded == "core") {
    return device_path == "cpu" || device_path == "cpu_core"
        || device_path == "cpu_atom";
  }
  return device_path == folded || device_path == "uncore_" + folded;
}

auto entry_read_selection_for(const availability probed,
                              const bool fast_capable) noexcept
    -> entry_read_selection
{
  // LCOV_EXCL_BR_START : coverage exclusion (T056): the switch's default
  // arm. The object's disassembly shows the dispatch as a compare and an
  // unsigned jump above the range, so this arm answers only a value past
  // the last enumerator, and the enumeration is closed. The fixture drives
  // each of the six states through this selector.
  switch (probed) {
    case availability::countable:
      return {.mode = fast_capable ? read_mode::fast_rdpmc : read_mode::syscall,
              .publish_pair = true,};
    case availability::permission_blocked:
    case availability::not_encodable:
    case availability::absent:
    case availability::scope_refused:
    case availability::gap:
      return {.mode = read_mode::syscall, .publish_pair = false};
  }
  // LCOV_EXCL_BR_STOP
  // Unreachable behind the closed enumeration; a build with contract
  // checking compiled out still needs a value.
  // LCOV_EXCL_START : coverage exclusion (T056): the fall-through past a
  // switch that covers every enumerator. The fixture drives each of the
  // six states through this selector, and no value outside the
  // enumeration exists to reach it.
  return {.mode = read_mode::syscall, .publish_pair = false};
  // LCOV_EXCL_STOP
}

// Splits a kernel event_attr file into its field/value pairs: the text
// is "field=value[,field=value...]", every value hexadecimal or
// decimal.
auto parse_attr(const std::string& text)
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

auto to_hex(const std::uint64_t value) -> std::string
{
  constexpr char digits[] = "0123456789abcdef";
  std::string out = "0x";
  bool leading = true;
  for (int shift = 60; shift >= 0; shift -= 4) {
    const auto nibble = static_cast<unsigned>((value >> shift) & 0xFU);
    if (nibble == 0 && leading && shift != 0) {
      continue;
    }
    leading = false;
    out.push_back(digits[nibble]);
  }
  return out;
  // LCOV_EXCL_LINE : the epilogue block of a by-value return; the `return`
  // above carries the call count.
}  // LCOV_EXCL_LINE

// Every catalog entry is described (FR-037). A kernel alias describes
// itself with the event_attr text the kernel publishes, verbatim, so a
// reader can reproduce the encoding; a vendored entry uses the table's
// own prose and names the event code when the table carries none.
auto alias_description(const std::string& name,
                       const std::string& text) -> std::string
{
  if (!text.empty()) {
    return "kernel event configuration: " + text;
  }
  return "kernel event '" + name + "'; the kernel publishes no "
         "configuration text for this alias";
}

auto table_description(const pmu_table_entry& entry) -> std::string
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
        out += " (event code " + to_hex(value) + ")";
      }
    }
  }
  // The scope-label guard. Both operand directions are measured by the seam
  // fixtures: a row carrying "DFPMC" takes the label, and a row carrying
  // "none" or no unit at all does not.
  // LCOV_EXCL_BR_LINE : coverage exclusion (T140): the arc from the empty-unit
  // operand straight to the false target. It needs a table row whose `Unit` is
  // the empty string, and every row the granting host parses and every seam
  // fixture builds carries `none` or a scope label, so no row on that host
  // takes it; the CI runner's own trace takes it once.
  if (!entry.unit.empty()  // LCOV_EXCL_BR_LINE
      && entry.unit != "none")  // LCOV_EXCL_BR_LINE
  {  // LCOV_EXCL_BR_LINE
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

auto pmu_probe(const int type,
               const std::vector<std::pair<int, std::uint64_t> >& words,
               const target& where) -> availability
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
  const auto [pid, cpu] = detail::leader_pid(where);
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
    return availability::countable;
    // LCOV_EXCL_STOP
  }
  switch (errno) {  // LCOV_EXCL_BR_LINE
    case EINVAL:
    case EOPNOTSUPP:
    case ENOENT:
      return availability::not_encodable;
    default:  // LCOV_EXCL_LINE
      // LCOV_EXCL_LINE : coverage exclusion (T066): the permission arm needs
      // a host whose kernel answers `perf_event_open` with `EACCES`. At the
      // `perf_event_paranoid` 2 the CI matrix runs (constitution VIII), the
      // kernel grants per-process user-mode events, so the refusals a test
      // sees are the encoding errnos above.
      return availability::permission_blocked;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
}

}  // namespace detail

pmu_provider::pmu_provider()
    : m_state(std::make_unique<detail::pmu_state>())
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

  detail::pmu_probe_fast(*m_state);
  const bool fast_capable = m_state->fast_available;
  // The permission level the availability probe ran at rides the device
  // description, so a reader of the catalog learns the level at which
  // the kernel answered each test-open, and which refusals are that
  // level (FR-039).
  const int paranoid = read_paranoid();
  const std::string level =
      paranoid < 0  // LCOV_EXCL_BR_LINE
                    // LCOV_EXCL_LINE : coverage exclusion (T066): the same
                    // unreadable sysctl the `read_paranoid` fallback above is
                    // excluded for.
      ? "an unreadable perf_event_paranoid"  // LCOV_EXCL_LINE
      : "perf_event_paranoid " + std::to_string(paranoid);  // LCOV_EXCL_BR_LINE
  // The probe verdict rides the device description, so a reader of the
  // catalog learns which mechanism the entries disclose and, when the
  // fast one is refused, the reason it was refused (FR-023).
  // LCOV_EXCL_BR_START : coverage exclusion (T140): the refusal wording, on
  // the same kernel-gate ground as the mode ternary in `probe_device` above.
  const std::string verdict = "; the availability probe ran at " + level
      + (fast_capable  // LCOV_EXCL_BR_LINE
             ? "; user counter reads are probe-available, entries "  // LCOV_EXCL_LINE
               "disclose fast_rdpmc"  // LCOV_EXCL_LINE
             : "; user counter reads stay in syscall mode: "  // LCOV_EXCL_LINE
                 + m_state->fast_refusal);  // LCOV_EXCL_LINE
  // LCOV_EXCL_BR_STOP

  for (const auto& dir : devices) {
    auto device = load_device(dir);
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
    merge_vendored(*device);
    probe_device(*device, fast_capable);
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

pmu_provider::~pmu_provider() = default;

void pmu_provider::enumerate(object_sink& sink) const
{
  for (const auto& device : m_state->devices) {
    std::vector<catalog_seed> entries;
    entries.reserve(device.entries.size());
    for (const auto& entry : device.entries) {
      entries.push_back(catalog_seed {
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
          .unit = entry.is_time_pair  // LCOV_EXCL_BR_LINE
              ? std::string_view("nanoseconds")
              : std::string_view("none"),  // LCOV_EXCL_BR_LINE
          // LCOV_EXCL_BR_STOP
          .avail = entry.avail,
          .mode = entry.mode,
          .frequency_hz = 0,
          .scaled = false,
          // LCOV_EXCL_BR_START : coverage exclusion (T140): both arcs of the
          // ratio-pair flag, on the countable-entry ground above. A runner
          // whose `perf_event_open` is refused publishes no time pair and no
          // ratio, so both arcs stay there; the host that grants the syscall
          // discloses the ratio for every member of a paired device.
          .has_ratio_pair = device.has_time_pair  // LCOV_EXCL_BR_LINE
              && !entry.is_time_pair,  // LCOV_EXCL_BR_LINE
          // LCOV_EXCL_BR_STOP
      });
    }
    sink.add_object(object_seed {
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
std::unique_ptr<window_reader> pmu_provider::open(const leaf_set& leaves,
                                                  const target& where)
{
  return detail::pmu_open_window(*m_state, leaves, where);
  // LCOV_EXCL_STOP
}

#endif  // !__linux__

}  // namespace sg::counters
