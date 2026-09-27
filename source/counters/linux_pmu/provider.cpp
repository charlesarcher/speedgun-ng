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
  if (file >> value) {
    return value;
  }
  return -1;
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
          const int nibble = digit >= '0' && digit <= '9'
              ? digit - '0'
              : (digit >= 'a' && digit <= 'f'
                     ? digit - 'a' + 10
                     : (digit >= 'A' && digit <= 'F' ? digit - 'A' + 10 : -1));
          if (nibble < 0) {
            number = 0;
            break;
          }
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
}

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
}

// Every catalog entry is described (FR-037). A kernel alias describes
// itself with the event_attr text the kernel publishes, verbatim, so a
// reader can reproduce the encoding; a vendored entry uses the table's
// own prose and names the event code when the table carries none.
auto alias_description(const std::string& name, const std::string& text)
    -> std::string
{
  if (!text.empty()) {
    return "kernel event configuration: " + text;
  }
  return "kernel event '" + name + "'; the kernel publishes no "
         "configuration text for this alias";
}

auto table_description(const detail::pmu_table_entry& entry) -> std::string
{
  std::string out = entry.description;
  if (out.empty()) {
    out = "hardware event '" + entry.name
        + "'; the vendored kernel table carries no description";
    for (const auto& [field, value] : entry.fields) {
      if (field == "event") {
        out += " (event code " + to_hex(value) + ")";
      }
    }
  }
  if (!entry.unit.empty() && entry.unit != "none") {
    // The table's Unit column is a scope label naming the shared unit
    // the event counts into (DFPMC, iMC, and the rest). It carries no
    // physical dimension: no vendored table in the pinned tree names a
    // physical unit. The label rides along as provenance; the catalog
    // unit stays the closed event count (FR-017).
    out += " [table scope: " + entry.unit + "]";
  }
  return out;
}

auto probe_device(detail::pmu_device& device, const bool fast_capable) -> void
{
  bool countable = false;
  for (auto& entry : device.entries) {
    if (entry.words.empty()) {
      entry.avail = availability::not_encodable;
      continue;
    }
    entry.avail = detail::pmu_probe(device.type, entry.words);
    if (entry.avail == availability::countable) {
      countable = true;
      entry.mode = fast_capable ? read_mode::fast_rdpmc : read_mode::syscall;
    }
  }
  if (countable) {
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
            "wall-clock nanoseconds this event group has actually run; "
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
}

auto load_device(const std::filesystem::path& dir)
    -> std::optional<detail::pmu_device>
{
  std::error_code code;
  const std::string type_text = slurp(dir / "type");
  if (type_text.empty()) {
    return std::nullopt;
  }
  const int type = std::atoi(type_text.c_str());
  if (type < 0) {
    return std::nullopt;
  }
  detail::pmu_device device;
  device.path = dir.filename().string();
  device.type = type;
  device.description = "perf event source '" + device.path + "', PMU type "
      + std::to_string(type);

  const auto format_dir = dir / "format";
  for (auto it = std::filesystem::directory_iterator(format_dir, code);
       it != std::filesystem::directory_iterator();
       it.increment(code))
  {
    std::vector<detail::format_range> ranges;
    if (detail::parse_format_field(slurp(it->path()), ranges)) {
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
    if (!is_event_file(name)) {
      continue;
    }
    const std::string text = slurp(it->path());
    detail::pmu_entry entry;
    entry.name = name;
    entry.description = alias_description(name, text);
    if (!detail::pmu_compose_config(
            parse_attr(text), device.formats, entry.words))
    {
      entry.words.clear();
    }
    device.entries.push_back(std::move(entry));
  }
  return device;
}

// The vendored table belongs to the core PMU: the mapfile selects one
// architecture directory for the running CPU, and those rows describe
// core events (FR-038). Kernel aliases of the same name win.
auto merge_vendored(detail::pmu_device& device) -> void
{
  const std::string directory =
      detail::pmu_select_directory(detail::pmu_ident_current());
  if (directory.empty()) {
    return;
  }
  const auto& table = detail::pmu_load_table(directory);
  for (const auto& row : table) {
    const bool taken = std::ranges::any_of(device.entries,
                                           [&](const detail::pmu_entry& entry)
                                           { return entry.name == row.name; });
    if (taken) {
      continue;
    }
    detail::pmu_entry entry;
    entry.name = row.name;
    entry.description = table_description(row);
    if (!detail::pmu_compose_config(row.fields, device.formats, entry.words)) {
      entry.words.clear();
    }
    device.entries.push_back(std::move(entry));
  }
}

}  // namespace

namespace detail
{

auto pmu_probe(const int type,
               const std::vector<std::pair<int, std::uint64_t> >& words)
    -> availability
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
  const long fd =
      ::syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd >= 0) {
    ::close(static_cast<int>(fd));
    return availability::countable;
  }
  switch (errno) {
    case EINVAL:
    case EOPNOTSUPP:
    case ENOENT:
      return availability::not_encodable;
    default:
      return availability::permission_blocked;
  }
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
  const std::string level = paranoid < 0
      ? "an unreadable perf_event_paranoid"
      : "perf_event_paranoid " + std::to_string(paranoid);
  // The probe verdict rides the device description, so a reader of the
  // catalog learns which mechanism the entries disclose and, when the
  // fast one is refused, the reason it was refused (FR-023).
  const std::string verdict = "; the availability probe ran at " + level
      + (fast_capable ? "; user counter reads are probe-available, entries "
                        "disclose fast_rdpmc"
                      : "; user counter reads stay in syscall mode: "
                 + m_state->fast_refusal);

  for (const auto& dir : devices) {
    auto device = load_device(dir);
    if (!device.has_value()) {
      continue;
    }
    if (device->path == "cpu") {
      merge_vendored(*device);
    }
    probe_device(*device, fast_capable);
    // A device with nothing countable and nothing described is absent
    // from the catalog (FR-039); the tree never seeds an empty object.
    if (!device->entries.empty()) {
      device->description += verdict;
      m_state->devices.push_back(std::move(*device));
    }
  }
}

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
          .unit = entry.is_time_pair ? std::string_view("nanoseconds")
                                     : std::string_view("none"),
          .avail = entry.avail,
          .mode = entry.mode,
          .frequency_hz = 0,
          .scaled = false,
          .has_ratio_pair = device.has_time_pair && !entry.is_time_pair,
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

std::unique_ptr<window_reader> pmu_provider::open(const leaf_set& leaves,
                                                  const target& where)
{
  return detail::pmu_open_window(*m_state, leaves, where);
}

#endif  // !__linux__

}  // namespace sg::counters
