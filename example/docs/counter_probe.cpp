// One line per core device carries three words: the counter limit the library
// publishes, the finder that seated it, and the figure this file's own group
// search reaches beside them (specs/041-counter-probe T050; FR-061, FR-062,
// FR-073, SC-003). The question is the one a user asks of a host: how many
// copies of one event does this device schedule at once, and who told the
// library that number. The catalog holds the first two answers, and the search
// below reaches the third in this file's own copy. A host that refuses every
// open prints `probe=no limit` and exits 0, and it prints no word about kernel
// settings.

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The device names this example prints, which is D-1's core-device rule.
// `isProbeCoreDevice` keeps that rule inside the counters archive and the
// published surface answers no device name with it, so this file states the
// rule once, beside the lines the rule picks (D-3, D-3a, FR-031).
constexpr std::array<std::string_view, 3> kCoreDevices {
    "cpu",
    "cpu_core",
    "cpu_atom",
};
constexpr std::string_view kCorePrefix {"armv8_"};

[[nodiscard]] auto isCoreDevice(const std::string_view device) noexcept -> bool
{
  return std::ranges::find(kCoreDevices, device) != kCoreDevices.end()
      || device.starts_with(kCorePrefix);
}

// The search below this block belongs to the example, and the reason it stands
// here is the reason this file exists. The library asks a device this question
// inside the counters archive, and that answer reaches no caller of the
// published surface: the archive hides its internals, and it searches only
// where its own decode left the device silent, so a device with a decoded limit
// pays no open there (FR-030, D-3a). No command-line word asks the library for
// one probe either (D-2). One real open belongs in this file under FR-073, so
// the example pays its own opens and a reader puts the two figures on one line.
//
// A program that needs the count asks the catalog. The published pair answers
// it: `programmableCounters()` gives the count and `objectLimitSource()` names
// the finder that seated it. That pair is the way to ask a device. Read this
// search as the cost of the question: one group per size, one enabled read per
// size, a stop at the first size that does not run, every descriptor closed on
// every path, no sleep, and no clock read. A host whose permission level
// refuses the opens answers `probe=no limit`, and that word is the whole of
// what this program says about a refusal (FR-062).

// The device's own directory in the tree the kernel publishes. A PMU object's
// catalog path is the device name, so the name of a line opens the files that
// describe that device and no other (FR-033).
constexpr std::string_view kDevicesRoot {"/sys/bus/event_source/devices/"};

// The event the search opens, spelled the way a core device lists it under
// `events/`. The kernel maps the name to its generic branch event, and the
// search sends the words this device publishes for that name (D-9, FR-033). A
// device that lists no file of this name answers no figure and pays no open
// (FR-034).
constexpr std::string_view kProbeEvent {"branch-instructions"};

// D-4's cap: sizes run 1 upward and stop at the first size that does not run.
// A device that runs every size to the cap gives the search no bound to state
// (FR-016).
constexpr std::size_t kProbeCap = 32;

// The binding F-1 to F-3 measured: the calling thread, on whatever cpu it runs
// on (FR-013).
constexpr int kProbePid = 0;
constexpr int kProbeCpu = -1;

// The width of the one config word a copy carries, which is where the placed
// bits of one field run out.
constexpr unsigned kConfigBits = 64;

// The words one read of a group leader answers: the leader's own count, the
// nanoseconds the group stood enabled, and the nanoseconds it ran (F-3).
constexpr std::size_t kReadWords = 3;

// The first line of one kernel-published file. An unreadable file answers the
// empty string, which is the answer of a device that publishes nothing there.
[[nodiscard]] auto firstLine(const std::string& path) -> std::string
{
  auto file = std::ifstream(path);
  auto text = std::string();
  std::getline(file, text);
  return text;
}

// The number one kernel word carries, hexadecimal behind `0x` and decimal
// everywhere else.
[[nodiscard]] auto parseNumber(const std::string_view text) noexcept
    -> std::optional<std::uint64_t>
{
  const bool hexadecimal = text.starts_with("0x") || text.starts_with("0X");
  const auto digits = hexadecimal ? text.substr(2) : text;
  const char* const end = digits.data() + digits.size();
  auto value = std::uint64_t {};
  const auto read =
      std::from_chars(digits.data(), end, value, hexadecimal ? 16 : 10);
  if (read.ec != std::errc {} || read.ptr != end) {
    return std::nullopt;
  }
  return value;
}

// One bit range of one format file: `config:0-7,32-35` states two of them.
struct BitRange
{
  unsigned low = 0;
  unsigned high = 0;
};

// The bit ranges one format file states, and no range at all when any piece of
// it stands unreadable: a file the search cannot read in full states no range
// it may place. The search sends one config word, so a head that names another
// word states no range either.
[[nodiscard]] auto parseRanges(const std::string_view spec)
    -> std::vector<BitRange>
{
  auto ranges = std::vector<BitRange> {};
  const std::size_t colon = spec.find(':');
  if (colon == std::string_view::npos || spec.substr(0, colon) != "config") {
    return ranges;
  }
  std::size_t cursor = colon + 1;
  for (;;) {
    const std::size_t comma = spec.find(',', cursor);
    const std::size_t stop =
        comma == std::string_view::npos ? spec.size() : comma;
    const auto piece = spec.substr(cursor, stop - cursor);
    const std::size_t dash = piece.find('-');
    const auto low = parseNumber(
        dash == std::string_view::npos ? piece : piece.substr(0, dash));
    // A piece that carries no dash states one bit, the way the library reads
    // its own format files, and the high end never stands below the low one.
    const auto high = dash == std::string_view::npos
        ? low
        : parseNumber(piece.substr(dash + 1));
    if (!low.has_value() || !high.has_value() || *high < *low) {
      return {};
    }
    ranges.push_back(BitRange {.low = static_cast<unsigned>(*low),
                               .high = static_cast<unsigned>(*high)});
    if (comma == std::string_view::npos) {
      break;
    }
    cursor = comma + 1;
  }
  return ranges;
}

// The two words one copy of the searched event needs on this device. The type
// is the device's own, which is what asks the named device and the device
// serving the calling thread: on a hybrid host those are two devices with two
// answers (FR-033).
struct ProbeTarget
{
  std::uint32_t type = 0;
  std::uint64_t config = 0;
};

// Reads the two words out of the three files the device publishes. Each field
// the device's own event file names shifts into the ranges of the format file
// of that name, the low bits of the field into the first range, and the placed
// parts join with an OR. A field this device publishes no range for, or a field
// wider than the ranges it holds, leaves no words behind and the device pays no
// open (FR-034).
[[nodiscard]] auto probeTarget(const std::string_view device)
    -> std::optional<ProbeTarget>
{
  const auto root = std::string(kDevicesRoot) + std::string(device);
  const auto type = parseNumber(firstLine(root + "/type"));
  if (!type.has_value() || *type > std::numeric_limits<std::uint32_t>::max()) {
    return std::nullopt;
  }

  const auto fields = firstLine(root + "/events/" + std::string(kProbeEvent));
  if (fields.empty()) {
    return std::nullopt;
  }
  auto config = std::uint64_t {};
  std::size_t cursor = 0;
  for (;;) {
    const std::size_t comma = fields.find(',', cursor);
    const std::size_t stop = comma == std::string::npos ? fields.size() : comma;
    const auto field = std::string_view(fields).substr(cursor, stop - cursor);
    const std::size_t equals = field.find('=');
    if (equals == std::string_view::npos || equals == 0) {
      return std::nullopt;
    }
    const auto value = parseNumber(field.substr(equals + 1));
    const auto ranges = parseRanges(
        firstLine(root + "/format/" + std::string(field.substr(0, equals))));
    if (!value.has_value() || ranges.empty()) {
      return std::nullopt;
    }
    unsigned placed = 0;
    for (const auto& range : ranges) {
      if (placed >= kConfigBits) {
        return std::nullopt;
      }
      const unsigned width = range.high - range.low + 1;
      const std::uint64_t room =
          width >= kConfigBits ? ~std::uint64_t {} : (1ULL << width) - 1ULL;
      config |= ((*value >> placed) & room) << range.low;
      placed += width;
    }
    // The field has to fit the ranges it named, or the device states a shape
    // this one-word search cannot carry.
    if (placed < kConfigBits && (*value >> placed) != 0) {
      return std::nullopt;
    }
    if (comma == std::string::npos) {
      break;
    }
    cursor = comma + 1;
  }
  return ProbeTarget {.type = static_cast<std::uint32_t>(*type),
                      .config = config};
}

// The attribute of one copy, in the shape F-1 to F-5 measured: kernel and
// hypervisor excluded, the group opened disabled, and nothing pinned, so a
// device the host will not serve answers no figure and the run multiplexes as
// it did before (FR-013).
[[nodiscard]] auto probeAttribute(const ProbeTarget& target) -> perf_event_attr
{
  auto attr = perf_event_attr {};
  attr.type = target.type;
  attr.size = sizeof(perf_event_attr);
  attr.config = target.config;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  attr.disabled = 1;
  attr.pinned = 0;
  attr.read_format =
      PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
  return attr;
}

// What one size answered. The pair the one read returns is the only part that
// decides a figure, because an open result alone answers nothing (FR-014).
struct SizeVerdict
{
  bool permission = false;
  bool fits = false;
};

// Searches one size and releases everything it opened, the refusals included
// (FR-019). The enable and the disable answer no verdict of their own: an
// enable the kernel refused leaves the enabled word at zero, and the pair below
// reads that size as one that did not run (FR-014).
[[nodiscard]] auto searchOneSize(const ProbeTarget& target,
                                 const std::size_t size) -> SizeVerdict
{
  auto verdict = SizeVerdict {};
  auto opened = std::vector<int> {};
  opened.reserve(size);
  for (std::size_t copy = 0; copy < size; ++copy) {
    auto attr = probeAttribute(target);
    // The first copy leads the group and each later copy names it (FR-012).
    const int leader = opened.empty() ? -1 : opened.front();
    const long one = ::syscall(SYS_perf_event_open,
                               &attr,
                               kProbePid,
                               kProbeCpu,
                               leader,
                               PERF_FLAG_FD_CLOEXEC);
    if (one < 0) {
      verdict.permission = errno == EACCES || errno == EPERM;
      break;
    }
    opened.push_back(static_cast<int>(one));
  }

  if (opened.size() == size) {
    auto words = std::array<std::uint64_t, kReadWords> {};
    ::ioctl(opened.front(), PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    const long returned = ::read(opened.front(), words.data(), sizeof(words));
    ::ioctl(opened.front(), PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    // Both sides of this compare are a byte count, and the cast names the one
    // signed type they settle in.
    // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
    const bool readAll = returned == static_cast<long>(sizeof(words));
    verdict.fits = readAll && words[1] > 0 && words[2] == words[1];
  }

  for (const int descriptor : opened) {
    ::close(descriptor);
  }
  return verdict;
}

// The largest group this device runs whole. The figure stands absent where the
// host refuses the opens, where the device publishes no words for the event,
// and where every size to the cap ran whole: each of those holds no bound a
// reader may state (FR-016, FR-017, FR-018).
[[nodiscard]] auto ownGroupLimit(const std::string_view device)
    -> std::optional<std::size_t>
{
  const auto target = probeTarget(device);
  if (!target.has_value()) {
    return std::nullopt;
  }
  for (std::size_t size = 1; size <= kProbeCap; ++size) {
    const auto verdict = searchOneSize(*target, size);
    if (verdict.permission) {
      return std::nullopt;
    }
    if (!verdict.fits) {
      return size > 1 ? std::optional<std::size_t> {size - 1}
                      : std::optional<std::size_t> {};
    }
  }
  return std::nullopt;
}

[[nodiscard]] auto countWord(const std::optional<std::size_t>& count,
                             const std::string_view absent) -> std::string
{
  if (!count.has_value()) {
    return std::string(absent);
  }
  return std::to_string(*count);
}

[[nodiscard]] auto sourceWord(const sg::counters::LimitSource source) noexcept
    -> std::string_view
{
  switch (source) {
    case sg::counters::LimitSource::DECODE:
      return "decode";
    case sg::counters::LimitSource::PROBE:
      return "probe";
    case sg::counters::LimitSource::NONE:
      break;
  }
  return "none";
}

// The device, its limit, and the source of that limit come out of the catalog,
// so these lines carry the library's own two answers beside the example's one.
// The catalog answers in the order it holds its objects, so one host prints one
// order and the docs gate can diff the bytes.
auto printDeviceLines() -> void
{
  const auto devices = sg::counters::System::local().objects("pmu");
  if (!devices.has_value()) {
    return;
  }
  for (const auto* device : *devices) {
    const auto path = device->path();
    if (!isCoreDevice(path)) {
      continue;
    }
    const auto limit = countWord(device->programmableCounters(), "none");
    const auto source =
        std::string(sourceWord(sg::counters::objectLimitSource(path)));
    const auto probe = countWord(ownGroupLimit(path), "no limit");
    std::printf("counter limit: %s limit=%s source=%s probe=%s\n",
                std::string(path).c_str(),
                limit.c_str(),
                source.c_str(),
                probe.c_str());
  }
}

}  // namespace

auto bmProbeTick(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmProbeTick);

auto holdsWord(int argc, char** argv, std::string_view prefix) -> bool
{
  for (int index = 0; index < argc; ++index) {
    if (std::string_view(argv[index]).starts_with(prefix)) {
      return true;
    }
  }
  return false;
}

// The first catalog query opens the system and closes provider registration
// (FR-009), so main stages the two providers before it reads a limit. The
// device lines follow the report rows, and a listing answers the registry
// alone, because the run gate reads that listing for the registered set.
auto main(int argc, char** argv) -> int
{
  auto& counters = sg::counters::System::local();
  static_cast<void>(counters.registerProvider(
      std::make_unique<sg::counters::ClockProvider>()));
  static_cast<void>(
      counters.registerProvider(std::make_unique<sg::counters::PmuProvider>()));
  const auto status = sg::speedgunMain(argc, argv);
  if (!holdsWord(argc, argv, "--list")) {
    printDeviceLines();
  }
  return status;
}
