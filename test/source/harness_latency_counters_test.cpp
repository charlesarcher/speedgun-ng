// ============================================================================
// T004, T005 (specs/048-hwlat-probe): the two counter sources the `hwlat`
// method reads. The model-specific system-management-interrupt counter
// stands behind `dev/cpu/<cpu>/msr` and answers only for a part that
// documents it (FR-007, FR-008, FR-009, FR-010), and the
// non-maskable-interrupt count stands in the `NMI` line of
// `proc/interrupts` (FR-009). The rows read fake trees the file-root seam
// installs, so no row needs root, a kernel module or a host counter
// (FR-001, FR-032). This unit stands on the frameworkless check()/fail()
// convention, the way harness_noise_test carries it.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

#include <unistd.h>

#include "../../source/detail/proc_root.hpp"
#include "detail/jitter_probe.hpp"
#include "detail/latency_counters.hpp"

namespace
{

int failures = 0;

auto fail(const char* what) -> void
{
  ++failures;
  std::printf("FAILED: %s\n", what);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The cpu indexes of the fake trees. The unit takes the index as its
// argument and reads only the tree the seam installs, so a row holds on a
// host that holds no such cpu (FR-001, FR-032).
constexpr std::uint32_t kFirstCpu = 0;
constexpr std::uint32_t kSecondCpu = 1;

// The rights of a node the process may read.
constexpr auto kNodeRights = std::filesystem::perms::owner_read
    | std::filesystem::perms::owner_write | std::filesystem::perms::group_read;

// The rights of a node the process may read and cannot write, so an open
// that asks for write rights refuses (FR-007, D-3).
constexpr auto kReadOnlyNodeRights =
    std::filesystem::perms::owner_read | std::filesystem::perms::group_read;

// The node path of one cpu, the path spec D-3 pins and the seam resolves
// under the file root (FR-007, FR-001).
auto nodePath(std::uint32_t cpu) -> std::string
{
  return "dev/cpu/" + std::to_string(cpu) + "/msr";
}

// The file-root seam of one scenario: a fresh tree the reads resolve
// under, installed for the scenario and cleared when it ends, the way the
// noise suite carries its tracing root (specs/036 FR-017, FR-019).
class CounterRootGuard
{
public:
  CounterRootGuard()
  {
    static int sequence = 0;
    m_root = std::filesystem::path("harness_latency_counters_root_")
        / ("pid" + std::to_string(::getpid()) + "-"
           + std::to_string(sequence++));
    std::filesystem::remove_all(m_root);
    std::filesystem::create_directories(m_root);
    sg::detail::installTestProcRoot(m_root);
  }

  ~CounterRootGuard()
  {
    sg::detail::installTestProcRoot(std::nullopt);
    std::error_code removeFailed;
    std::filesystem::remove_all(m_root, removeFailed);
  }

  CounterRootGuard(const CounterRootGuard&) = delete;
  auto operator=(const CounterRootGuard&) -> CounterRootGuard& = delete;

  auto root() const -> const std::filesystem::path& { return m_root; }

private:
  std::filesystem::path m_root;
};

// One file under the scenario root: the bytes written whole, then the
// rights set, so the node answers the read the row plans.
auto plant(const std::filesystem::path& root,
           const std::string& relative,
           const std::string& bytes,
           std::filesystem::perms rights) -> void
{
  const auto path = root / std::filesystem::path(relative);
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  check(static_cast<bool>(file), "the fake file opens for writing (T004)");
  file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  file.close();
  std::error_code rightsFailed;
  std::filesystem::permissions(
      path, rights, std::filesystem::perm_options::replace, rightsFailed);
  check(!rightsFailed, "the fake file takes the rights the row plants (T004)");
}

// The bytes of one planted file, read whole, or no bytes when the file stands
// unopenable.
auto plantedBytes(const std::filesystem::path& root,
                  const std::string& relative) -> std::string
{
  std::ifstream file(root / std::filesystem::path(relative), std::ios::binary);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// The node image of one counter figure: padding up to the counter
// address, then the figure as `kSmiCountBytes` bytes with the least
// significant byte first (FR-007).
auto nodeImage(std::uint64_t value) -> std::string
{
  std::string bytes(static_cast<std::size_t>(sg::detail::kSmiCountAddress),
                    '\0');
  for (std::size_t index = 0; index < sg::detail::kSmiCountBytes; ++index) {
    bytes.push_back(static_cast<char>((value >> (8U * index)) & 0xffULL));
  }
  return bytes;
}

// The count of one cpu through the descriptor pair, the shape the probe
// stands on: one open, one read, one close (P-2).
auto countThroughPair(std::uint32_t cpu) -> std::optional<std::uint64_t>
{
  const auto descriptor = sg::detail::openMsrNode(cpu);
  if (!descriptor.has_value()) {
    return std::nullopt;
  }
  const auto count = sg::detail::smiCountOf(*descriptor);
  sg::detail::closeMsrNode(*descriptor);
  return count;
}

// The count one positive control node answers, the value a fault group
// plants beside its fault shape (FR-032).
constexpr std::uint64_t kControlCount = 0x000000000000c001ULL;

// One positive control beside a fault shape: the node of the second cpu,
// planted in the same tree, so an absent answer stands for the fault and
// for nothing else.
auto plantControlNode(const std::filesystem::path& root) -> void
{
  plant(root, nodePath(kSecondCpu), nodeImage(kControlCount), kNodeRights);
}

// The control row of one fault group: the same tree answers the count of
// the node it holds (FR-007).
auto checkControlNode(const char* what) -> void
{
  const auto count = countThroughPair(kSecondCpu);
  check(count.has_value() && *count == kControlCount, what);
}

// One path of the tree replaced by a directory: the read fails for every
// user, the root of a CI container included (specs/036 T010).
auto denyPath(const std::filesystem::path& root,
              const std::string& relative) -> void
{
  const auto path = root / std::filesystem::path(relative);
  std::filesystem::remove(path);
  std::filesystem::create_directories(path);
}

// One denied path put back, so the control text can stand in its place.
auto clearPath(const std::filesystem::path& root,
               const std::string& relative) -> void
{
  std::error_code removeFailed;
  std::filesystem::remove_all(root / std::filesystem::path(relative),
                              removeFailed);
}

// Whether the kernel's rights check binds this process on a node that carries
// no rights bits. A process that holds the discretionary-access override reads
// such a node, and the continuous integration runner of specs/036 runs as one,
// so a row that names a refusal reads the kernel's answer first and states the
// figure that answer owes (specs/036 T010, FR-032).
auto theRightsBind() -> bool
{
  const auto probe = sg::detail::openMsrNode(kFirstCpu);
  const bool refused = !probe.has_value();
  if (probe.has_value()) {
    sg::detail::closeMsrNode(*probe);
  }
  return refused;
}

// The `proc/cpuinfo` text of one vendor, the standing line shape (FR-010).
auto cpuInfoOf(const std::string& vendor) -> std::string
{
  return "processor\t: 0\nvendor_id\t: " + vendor
      + "\nmodel name\t: fake cpu\n";
}

// The `proc/interrupts` text of two cpus and three labeled rows: the
// column of the first cpu holds 10, 3 and 5, the column of the second
// holds 20, 4 and 6 (FR-009).
auto interruptsText() -> std::string
{
  return "           CPU0       CPU1\n"
         "  8:         10         20   IO-APIC   8-edge      timer\n"
         "NMI:          3          4   NMI       performance-metrics\n"
         " LOC:          5          6   Local     timer interrupts\n";
}

// T004, FR-007, D-3: one cpu reads the counter of its own node at the
// counter address, as one little-endian unsigned value.
auto theRegisterRows() -> void
{
  const CounterRootGuard guard;
  const std::uint64_t first = 0x0123456789abcdefULL;
  const std::uint64_t second = 0x000000000000002aULL;
  plant(guard.root(), nodePath(kFirstCpu), nodeImage(first), kNodeRights);
  plant(guard.root(), nodePath(kSecondCpu), nodeImage(second), kNodeRights);
  const auto descriptor = sg::detail::openMsrNode(kFirstCpu);
  check(descriptor.has_value(), "the node of the first cpu opens (FR-007)");
  const auto count = descriptor.has_value()
      ? sg::detail::smiCountOf(*descriptor)
      : std::optional<std::uint64_t> {};
  check(count.has_value() && *count == first,
        "the counter reads as one 8-byte little-endian value at the counter "
        "address (FR-007, D-3)");
  if (descriptor.has_value()) {
    sg::detail::closeMsrNode(*descriptor);
  }
  const auto other = countThroughPair(kSecondCpu);
  check(other.has_value() && *other == second,
        "the second cpu reads the counter of its own node (FR-007)");
}

// T004, FR-007, FR-011: a node that stands absent answers no count.
auto theAbsentNodeRows() -> void
{
  const CounterRootGuard guard;
  check(!countThroughPair(kFirstCpu).has_value(),
        "a node that stands absent answers no count (FR-007, FR-011)");
  plantControlNode(guard.root());
  checkControlNode(
      "the same tree answers the count of the node it holds (FR-007)");
}

// T004, FR-007, FR-011: a directory in the place of the node answers no
// count, the shape that holds for every user (specs/036 T010).
auto theDirectoryNodeRows() -> void
{
  const CounterRootGuard guard;
  std::filesystem::create_directories(
      guard.root() / std::filesystem::path(nodePath(kFirstCpu)));
  check(!countThroughPair(kFirstCpu).has_value(),
        "a directory in the place of the node answers no count "
        "(FR-007, FR-011)");
  plantControlNode(guard.root());
  checkControlNode(
      "the same tree answers the count of the node it holds (FR-007)");
}

// T004, FR-007, FR-011: a node that answers fewer bytes than the counter
// width stands unreadable, and its count stands absent.
auto theShortNodeRows() -> void
{
  const CounterRootGuard guard;
  const auto shortWidth =
      static_cast<std::size_t>(sg::detail::kSmiCountAddress) + 3;
  plant(guard.root(),
        nodePath(kFirstCpu),
        nodeImage(0x1122334455667788ULL).substr(0, shortWidth),
        kNodeRights);
  const auto descriptor = sg::detail::openMsrNode(kFirstCpu);
  check(descriptor.has_value(),
        "a node that answers a short count still opens (FR-007)");
  const auto count = descriptor.has_value()
      ? sg::detail::smiCountOf(*descriptor)
      : std::optional<std::uint64_t> {};
  if (descriptor.has_value()) {
    sg::detail::closeMsrNode(*descriptor);
  }
  check(!count.has_value(),
        "a node that answers fewer bytes than the counter width answers no "
        "count (FR-007, FR-011)");
  plantControlNode(guard.root());
  checkControlNode(
      "the same tree answers the count of the node it holds (FR-007)");
}

// T004, FR-007, FR-011: a node the process holds no rights on answers no count
// to a process the kernel checks. A process that holds the discretionary-access
// override reads the same node, so the row reads the kernel's answer and states
// the figure that answer owes (FR-032, specs/036 T010).
auto theDeniedNodeRows() -> void
{
  const CounterRootGuard guard;
  plant(guard.root(),
        nodePath(kFirstCpu),
        nodeImage(7),
        std::filesystem::perms::none);
  const auto count = countThroughPair(kFirstCpu);
  if (theRightsBind()) {
    check(!count.has_value(),
          "a node without rights answers no count (FR-007, FR-011)");
  } else {
    check(count.has_value() && *count == 7,
          "a node that carries no rights bits answers the count it holds to a "
          "process the kernel lets read past them (FR-007, FR-011)");
  }
  plantControlNode(guard.root());
  checkControlNode(
      "the same tree answers the count of the node it holds (FR-007)");
}

// T004, FR-007, D-3: the unit reaches the node for read alone. The node
// stands readable and stands no write rights, so an open that asks for
// write rights refuses the first row, and the bytes behind the counter
// stand unchanged after the pair runs.
auto theReadOnlyRows() -> void
{
  const CounterRootGuard guard;
  const std::uint64_t value = 0x0000000012345678ULL;
  const std::string image = nodeImage(value);
  plant(guard.root(), nodePath(kFirstCpu), image, kReadOnlyNodeRights);
  const auto count = countThroughPair(kFirstCpu);
  check(count.has_value() && *count == value,
        "a node that carries read rights alone answers its count "
        "(FR-007, D-3)");
  check(plantedBytes(guard.root(), nodePath(kFirstCpu)) == image,
        "the pair writes nowhere on the node (FR-007, D-3)");
}

// T005, FR-010, D-4, P-4: the gate takes its answer from the `vendor_id`
// text under the file root, so a test plants the text it needs. Each
// unsupported text stands beside a control text in the same tree, so an
// unsupported answer stands for the text and for nothing else.
auto theVendorGateRows() -> void
{
  const std::string control = cpuInfoOf("GenuineIntel");
  {
    const CounterRootGuard guard;
    plant(guard.root(), "proc/cpuinfo", cpuInfoOf("AuthenticAMD"), kNodeRights);
    check(!sg::detail::smiSourceSupported(),
          "the gate answers unsupported for a part that states no counter "
          "(FR-010, D-4)");
    plant(guard.root(), "proc/cpuinfo", control, kNodeRights);
    check(sg::detail::smiSourceSupported(),
          "the same tree answers supported for the documented vendor "
          "(FR-010, D-4)");
  }
  {
    const CounterRootGuard guard;
    plant(guard.root(), "proc/cpuinfo", "processor\t: 0\n", kNodeRights);
    check(
        !sg::detail::smiSourceSupported(),
        "a cpuinfo that states no vendor answers unsupported " "(FR-010, D-4)");
    plant(guard.root(), "proc/cpuinfo", control, kNodeRights);
    check(
        sg::detail::smiSourceSupported(),
        "the same tree answers supported once the line stands " "(FR-010, "
                                                                "D-4)");
  }
  {
    const CounterRootGuard guard;
    plant(guard.root(), "proc/cpuinfo", "vendor_id\t:\n", kNodeRights);
    check(!sg::detail::smiSourceSupported(),
          "a vendor line that states no value answers unsupported "
          "(FR-010, FR-011)");
    plant(guard.root(), "proc/cpuinfo", control, kNodeRights);
    check(
        sg::detail::smiSourceSupported(),
        "the same tree answers supported for a stated vendor " "(FR-010, D-4)");
  }
  {
    const CounterRootGuard guard;
    denyPath(guard.root(), "proc/cpuinfo");
    check(!sg::detail::smiSourceSupported(),
          "a cpuinfo the read cannot carry answers unsupported "
          "(FR-010, FR-011)");
    clearPath(guard.root(), "proc/cpuinfo");
    plant(guard.root(), "proc/cpuinfo", control, kNodeRights);
    check(
        sg::detail::smiSourceSupported(),
        "the same tree answers supported once the file reads " "(FR-010, D-4)");
  }
  {
    const CounterRootGuard guard;
    check(
        !sg::detail::smiSourceSupported(),
        "a tree that holds no cpuinfo answers unsupported " "(FR-010, FR-011)");
    plant(guard.root(), "proc/cpuinfo", control, kNodeRights);
    check(
        sg::detail::smiSourceSupported(),
        "the same tree answers supported once the file stands " "(FR-010, "
                                                                "D-4)");
  }
}

// T005, FR-009, FR-011: the reader takes one cpu's column of the `NMI`
// row, and every shape the file answers badly leaves no count.
auto theNmiRows() -> void
{
  const std::string text = interruptsText();
  const auto first = sg::detail::nmiCountOfText(text, kFirstCpu);
  check(first.has_value() && *first == 3,
        "the first cpu reads the figure of its own column of the NMI row "
        "(FR-009)");
  const auto second = sg::detail::nmiCountOfText(text, kSecondCpu);
  check(second.has_value() && *second == 4,
        "the second cpu reads the figure of its own column of the NMI row "
        "(FR-009)");
  const std::string oneColumn = "NMI:  10  NMI  performance-metrics\n";
  check(!sg::detail::nmiCountOfText(oneColumn, kSecondCpu).has_value(),
        "an NMI row that names fewer columns than the cpu asks for answers "
        "no count (FR-011)");
  const auto ownColumn = sg::detail::nmiCountOfText(oneColumn, kFirstCpu);
  check(ownColumn.has_value() && *ownColumn == 10,
        "the same row answers the column it names (FR-009)");
  const std::string rough = "NMI:  not-a-number  4\n";
  check(!sg::detail::nmiCountOfText(rough, kFirstCpu).has_value(),
        "an NMI figure that parses as no number answers no count (FR-011)");
  const auto otherColumn = sg::detail::nmiCountOfText(rough, kSecondCpu);
  check(otherColumn.has_value() && *otherColumn == 4,
        "the same row answers the column that holds a number (FR-009)");
  check(
      !sg::detail::nmiCountOfText("NMI:\n", kFirstCpu).has_value(),
      "an NMI row whose figure stands truncated answers no count " "(FR-011)");
  const std::string noRow = "  8:  10  20   IO-APIC  8-edge  timer\n";
  check(
      !sg::detail::nmiCountOfText(noRow, kFirstCpu).has_value(),
      "a text whose NMI row stands absent answers no count " "(FR-009, "
                                                             "FR-011)");
}

// T090, FR-009, FR-011: one token that carries digits beside text stands no
// figure, the shape a kernel column answers when a count stands with a unit.
// The column of digits alone beside it stands the figure the reader takes.
auto theNmiFigureBesideItsDigits() -> void
{
  const std::string mixed = "NMI:  12x  4\n";
  check(!sg::detail::nmiCountOfText(mixed, kFirstCpu).has_value(),
        "an NMI column whose digits stand beside text answers no count "
        "(T090, FR-011)");
  const auto second = sg::detail::nmiCountOfText(mixed, kSecondCpu);
  check(second.has_value() && *second == 4,
        "the same row answers the column of digits alone (T090, FR-009)");
}

// T005, FR-009, D-4: the new figure stands named apart, and the standing
// `interrupts` figure keeps its sum over every labeled row of the column.
auto theStandingInterruptsRows() -> void
{
  const CounterRootGuard guard;
  const std::string text = interruptsText();
  plant(guard.root(), "proc/interrupts", text, kNodeRights);
  const auto snapshot = sg::detail::precheckReadSnapshot({kFirstCpu});
  const auto standing = snapshot.at(kFirstCpu).interrupts;
  const auto nmi = sg::detail::nmiCountOfText(text, kFirstCpu);
  check(standing.has_value() && *standing == 18,
        "the standing interrupts figure keeps its sum over every labeled row "
        "of the column (FR-009)");
  const bool namedApart =
      nmi.has_value() && standing.has_value() && *nmi < *standing;
  check(namedApart,
        "the NMI figure stands named apart from the standing interrupts "
        "figure (FR-009, D-4)");
}

}  // namespace

auto main() -> int
{
  theRegisterRows();
  theAbsentNodeRows();
  theDirectoryNodeRows();
  theShortNodeRows();
  theDeniedNodeRows();
  theReadOnlyRows();
  theVendorGateRows();
  theNmiRows();
  theNmiFigureBesideItsDigits();
  theStandingInterruptsRows();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
