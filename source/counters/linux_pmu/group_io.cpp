// Windows for the Linux PMU provider: the syscall-mode group read and
// the mapped-page fast read (specs/007-counters-and-timers, US6/US7,
// T048, T052; FR-026, FR-041, FR-040, R-010, R-011). Off Linux no
// window ever opens (FR-042).

#ifdef __linux__

#  include <algorithm>
#  include <cstddef>
#  include <cstdint>
#  include <map>
#  include <memory>
#  include <string>
#  include <string_view>
#  include <vector>

#  include <linux/perf_event.h>
#  include <sys/ioctl.h>
#  include <sys/syscall.h>
#  include <unistd.h>

#  include "../detail/pmu.hpp"
#  include "kernel_calls.hpp"
#  include "speedgun-ng/counters_provider.hpp"
#  include "speedgun-ng/dbc.hpp"

namespace sg::counters::detail
{

auto groupReadShort(const std::int64_t returned,
                    const std::size_t headerBytes) noexcept -> bool
{
  // Both operands are a read count and a header size, and the cast names
  // the one signed type they are compared in. The read count is not
  // negative on this path, and the check asks for a common type here,
  // which is what the cast is.
  // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
  const bool shortRead = returned < static_cast<std::int64_t>(headerBytes);
  // A count the syscall refused is negative, and a negative count falls
  // below any header size, so a read that produced no count reads short
  // as well (FR-006).
  SG_ENSURE(returned < 0 ? shortRead : true,
            "the verdict is true exactly when the count is below the "
            "header size, and a count the syscall refused reads short "
            "(FR-006)");
  return shortRead;
}

auto fastPairDisclosed(const bool stable,
                       const std::uint64_t enabled,
                       const std::uint64_t running,
                       std::uint64_t& outEnabled,
                       std::uint64_t& outRunning) noexcept -> bool
{
  if (!stable) {
    outEnabled = 0;
    outRunning = 0;
    return false;
  }
  outEnabled = enabled;
  outRunning = running;
  // The postcondition states two facts, and DeMorgan's form of them
  // states neither on its own. It is a contract, so it keeps its shape.
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  SG_ENSURE(outEnabled == enabled && outRunning == running,
            "a stable pair publishes the page's own two values (FR-005)");
  return true;
}

namespace
{

// Where one managed leaf's point comes from inside its device group.
enum class SlotSource : std::uint8_t
{
  MEMBER,
  TIME_ENABLED,
  TIME_RUNNING,
  DISCLOSURE,
};

struct LeafSlot
{
  std::size_t group = 0;
  std::size_t index = 0;  // member position, unused for the time pair
  SlotSource source = SlotSource::MEMBER;
  // The plan's disclosure column for the window that owns this slot, or
  // `LeafSet::kNoDisclosureColumn`. A disclosure is written into the
  // column the plan named. A plan with more than one read group gives
  // each group its own column, and those columns follow the leaves
  // (FR-002).
  std::size_t disclosure = LeafSet::kNoDisclosureColumn;
};

// One requested leaf resolved against the merged catalog.
struct ResolvedLeaf
{
  std::size_t device = 0;
  const PmuEntry* entry = nullptr;
  SlotSource source = SlotSource::MEMBER;
  // The plan's disclosure column, carried on the resolved list because the
  // window builders read the list and not the leaf set the list came from.
  // A disclosure is written into the column the plan named rather than
  // through the sink's sequential cursor, because a plan with more than one
  // read group gives each group its own column and those columns follow the
  // leaves (FR-002).
  std::size_t disclosure = LeafSet::kNoDisclosureColumn;
};

// Device to group index, assigned in first-appearance order so the
// group layout is deterministic (FR-024).
class GroupLayout
{
public:
  explicit GroupLayout(const std::size_t devices)
      : m_groupOfDevice(devices, static_cast<std::size_t>(-1))
  {
  }

  void assign(const std::vector<ResolvedLeaf>& leaves) noexcept
  {
    for (const auto& leaf : leaves) {
      if (leaf.source != SlotSource::MEMBER) {
        continue;
      }
      if (m_groupOfDevice[leaf.device] != static_cast<std::size_t>(-1)) {
        continue;
      }
      m_groupOfDevice[leaf.device] = m_groupCount;
      ++m_groupCount;
    }
  }

  [[nodiscard]] auto groupOf(const std::size_t device) const noexcept
      -> std::size_t
  {
    return m_groupOfDevice[device];
  }

  [[nodiscard]] auto count() const noexcept -> std::size_t
  {
    return m_groupCount;
  }

private:
  std::vector<std::size_t> m_groupOfDevice;
  std::size_t m_groupCount = 0;
};

// Resolves every requested address against the catalog in the order
// asked, so the sink order equals the column order (C-PRO-2). Null on
// any address this provider cannot serve.
auto resolve(const PmuState& state,
             const LeafSet& leaves,
             std::vector<ResolvedLeaf>& out) -> bool
{
  if (leaves.addresses.empty()) {
    return false;
  }
  std::map<std::string, std::size_t> deviceIndex;
  for (std::size_t index = 0; index < state.devices.size(); ++index) {
    deviceIndex.emplace(state.devices[index].path, index);
  }
  out.clear();
  out.reserve(leaves.addresses.size());
  for (const auto& address : leaves.addresses) {
    const auto slash = address.rfind('/');
    if (slash == std::string::npos) {
      return false;
    }
    const std::string deviceName = address.substr(0, slash);
    const std::string leafName = address.substr(slash + 1);
    const auto located = deviceIndex.find(deviceName);
    if (located == deviceIndex.end()) {
      return false;
    }
    const auto& device = state.devices[located->second];
    const PmuEntry* found = nullptr;
    for (const auto& entry : device.entries) {
      if (entry.name == leafName) {
        found = &entry;
        break;
      }
    }
    if (found == nullptr || found->avail != Availability::COUNTABLE) {
      return false;
    }
    SlotSource source = SlotSource::MEMBER;
    if (leafName == "enabled") {
      source = SlotSource::TIME_ENABLED;
    } else if (leafName == "running") {
      source = SlotSource::TIME_RUNNING;
    }
    out.push_back(ResolvedLeaf {
        .device = located->second, .entry = found, .source = source});
  }
  return true;
}

auto fillAttr(perf_event_attr& attr,
              const PmuDevice& device,
              const PmuEntry& entry) -> void
{
  attr = perf_event_attr {};
  attr.type = static_cast<std::uint32_t>(device.type);
  attr.size = sizeof(perf_event_attr);
  for (const auto& [word, value] : entry.words) {
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
  attr.inherit = 0;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED
      | PERF_FORMAT_TOTAL_TIME_RUNNING;
}

// The group-read payload header: member count, enabled nanoseconds,
// running nanoseconds, then one value per member.
constexpr std::size_t kHeaderWords = 3;

struct GroupState
{
  int leader = -1;
  std::vector<int> members;
  std::vector<std::uint64_t> values;
  std::uint64_t enabled = 0;
  std::uint64_t running = 0;
};

}  // namespace

struct PmuWindow final : WindowReader
{
  std::vector<LeafSlot> slots;
  std::vector<GroupState> groups;
  std::vector<std::uint64_t> scratch;
  // True when a group read this action produced no count, so the
  // disclosure column names a gap beside the zeros (FR-006).
  bool gapped = false;

  // Every leaf opens into exactly one group, so the leaf count bounds the
  // member count of every group; the scratch covers the header and one word
  // per member of the widest group, and the sampling path never grows it.
  explicit PmuWindow(const std::size_t leafCount)
  {
    setThunk(&readDirect);
    scratch.resize(kHeaderWords + leafCount);
  }

  PmuWindow(const PmuWindow&) = delete;
  auto operator=(const PmuWindow&) -> PmuWindow& = delete;
  PmuWindow(PmuWindow&&) = delete;
  auto operator=(PmuWindow&&) -> PmuWindow& = delete;

  ~PmuWindow() override
  {
    for (auto& group : groups) {
      for (const int fd : group.members) {
        kernelCalls().closeFd(fd);
      }
    }
  }

  void readPoints(PointSink& sink) noexcept override
  {
    gapped = false;
    for (auto& group : groups) {
      const auto want = static_cast<std::size_t>(
          (kHeaderWords + group.members.size()) * sizeof(std::uint64_t));
      const auto got =
          kernelCalls().readGroup(group.leader, scratch.data(), want);
      if (groupReadShort(got, kHeaderWords * sizeof(std::uint64_t)))
      {  // A group the kernel could not
         // read this action reports no
        // point; the action is marked in the disclosure column beside the
        // zero counts, and the pair discloses ratio 0. A count is never
        // fabricated.
        gapped = true;
        group.enabled = 0;
        group.running = 0;
        std::ranges::fill(group.values, 0);
        continue;
      }
      const auto count = static_cast<std::size_t>(scratch[0]);
      group.enabled = scratch[1];
      group.running = scratch[2];
      for (std::size_t member = 0; member < group.members.size(); ++member) {
        group.values[member] =
            member < count ? scratch[kHeaderWords + member] : 0;
      }
    }
    for (const auto& slot : slots) {
      const auto& group = groups[slot.group];
      // The switch's no-case arc is a block the compiler emits for a
      // value the enumeration cannot hold; no input reaches it.
      switch (slot.source) {  // LCOV_EXCL_BR_LINE
        case SlotSource::MEMBER:
          sink.put(group.values[slot.index]);
          break;
        case SlotSource::TIME_ENABLED:
          sink.put(group.enabled);
          break;
        case SlotSource::TIME_RUNNING:
          sink.put(group.running);
          break;
        case SlotSource::DISCLOSURE:
          sink.putDisclosure(
              slot.disclosure,
              static_cast<std::uint64_t>(gapped ? Availability::GAP
                                                : Availability::COUNTABLE));
          break;
      }
    }
  }

  // The compiled plan hands the window over as the base reference
  // `ReadThunk` declares, and `pmu_open_window` constructs it as this
  // final type, so the reference names a group window on every call.
  // `final` fixes the target of the `readPoints` call, so the sampling
  // path takes one indirect call and no vtable lookup (FR-022, T146).
  static auto readDirect(WindowReader& base, PointSink& sink) noexcept -> void
  {
    static_cast<PmuWindow&>(base).readPoints(sink);
  }
};

// The fast-mode window: one mapped page per member leaf, read inside one
// sampling action, and the enabled/running pair taken from the leader's
// page (FR-040, FR-041). The window runs for real on any host the kernel
// serves, and the scripted kernel stands the pages in for the tests, so
// the reads below carry no blanket exclusion; the arms no input reaches
// keep their markers at their own sites.
struct PmuFastWindow final : WindowReader
{
  struct Member
  {
    std::unique_ptr<FastContext> context;
    std::uint64_t value = 0;
  };

  std::vector<LeafSlot> slots;
  std::vector<Member> members;
  // The member whose page carries the group's enabled/running pair, the
  // pair a syscall-mode window reads from the leader (FR-041).
  std::size_t leader = 0;
  std::uint64_t enabled = 0;
  std::uint64_t running = 0;
  // True when a member read or the pair read this action produced no
  // value, so the disclosure column names a gap beside the zeros
  // (FR-002, FR-003, FR-005).
  bool gapped = false;

  PmuFastWindow() { setThunk(&readDirect); }

  PmuFastWindow(const PmuFastWindow&) = delete;
  auto operator=(const PmuFastWindow&) -> PmuFastWindow& = delete;
  PmuFastWindow(PmuFastWindow&&) = delete;
  auto operator=(PmuFastWindow&&) -> PmuFastWindow& = delete;

  ~PmuFastWindow() override;

  void readPoints(PointSink& sink) noexcept override
  {
    gapped = false;
    for (auto& one : members) {
      // LCOV_EXCL_BR_START : coverage exclusion (T140): the retry arm. The
      // sequence moves only when the kernel rewrites the page between the
      // two reads of it, which a test cannot force deterministically. The
      // same verdict is covered for both arms by `fastPairStable` inside
      // `fastDecode` in `test/source/counters_linux_pmu_seam_test.cpp`.
      auto verdict = fastContextRead(*one.context, one.value);
      if (verdict  // LCOV_EXCL_BR_LINE
          == FastReadVerdict::UNSTABLE)  // LCOV_EXCL_BR_LINE
      {
        // The page sequence moved under the read; the protocol's
        // stated fallback is one more attempt (FR-040). The retry's own
        // verdict is the one that decides the point.
        verdict = fastContextRead(*one.context, one.value);  // LCOV_EXCL_LINE
      }  // LCOV_EXCL_BR_LINE
      // LCOV_EXCL_BR_STOP
      // Every verdict is judged, the retry's trigger among them: a read the
      // page refuses, and a retry that also fails, publish no count, so
      // the point is zero and the action is marked beside it
      // (FR-002, FR-003).
      if (verdict != FastReadVerdict::OK) {
        one.value = 0;
        gapped = true;
      }
    }
    // The enabled/running pair rides the leader's user page, the same
    // pair the group read takes from the leader, so the multiplex
    // ratio is computed inside folds in this read mode too (FR-041).
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that reports
    // no pair. It needs the kernel to rewrite the leader's page between the
    // two reads of its sequence, which a test cannot force
    // deterministically; the comparison itself and the extracted
    // disclosure decision are covered for both arms by `fastPairStable`
    // and `fastPairDisclosed` over synthetic pages in
    // `test/source/counters_linux_pmu_seam_test.cpp` (FR-005, FR-046).
    std::uint64_t pageEnabled = 0;
    std::uint64_t pageRunning = 0;
    const bool pairStable = fastContextTimePair(
        *members.at(leader).context, pageEnabled, pageRunning);
    if (!fastPairDisclosed(
            pairStable, pageEnabled, pageRunning, enabled, running))
    {
      // A leader whose page disclosed no stable pair this action reports
      // none; the pair discloses ratio 0 and the action is marked beside
      // the zero. A time is never fabricated.
      gapped = true;  // LCOV_EXCL_LINE
    }
    // LCOV_EXCL_BR_STOP
    for (const auto& slot : slots) {
      // The switch's no-case arc is a block the compiler emits for a
      // value the enumeration cannot hold; no input reaches it.
      switch (slot.source) {  // LCOV_EXCL_BR_LINE
        case SlotSource::MEMBER:
          sink.put(members[slot.index].value);
          break;
        case SlotSource::TIME_ENABLED:
          sink.put(enabled);
          break;
        case SlotSource::TIME_RUNNING:
          sink.put(running);
          break;
        case SlotSource::DISCLOSURE:
          sink.putDisclosure(
              slot.disclosure,
              static_cast<std::uint64_t>(gapped ? Availability::GAP
                                                : Availability::COUNTABLE));
          break;
      }
    }
  }

  // The compiled plan hands the window over as the base reference
  // `ReadThunk` declares, and `pmu_open_fast_window` constructs it as
  // this final type, so the reference names a fast window on every
  // call. `final` fixes the target of the `readPoints` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto readDirect(WindowReader& base, PointSink& sink) noexcept -> void
  {
    static_cast<PmuFastWindow&>(base).readPoints(sink);
  }
};

namespace
{

// One release for every member a fast window acquired (FR-013, FR-014).
// A member that opened releases through its own context; the two
// partial-open arms call this for the members they hold before refusing,
// and the destructor calls it for the whole window. Closing a context
// that already released touches nothing, so the three call sites reach one
// release and none of them can release twice.
void releaseFastMembers(std::vector<PmuFastWindow::Member>& acquired) noexcept
{
  for (auto& one : acquired) {
    // Every push into this vector sits after the open's own null check,
    // and that check returns before the push, so a member reaches this
    // loop only with a context to release (FR-014).
    SG_ASSERT(
        one.context != nullptr,
        "every member a fast window releases holds its context " "(FR-014)");
    fastContextClose(*one.context);
  }
  acquired.clear();
}

// Every requested leaf must have the catalog's fast mode recorded, so
// the read the plan performs is the read the catalog disclosed
// (FR-023, C-PRO-4).
auto allFast(const std::vector<ResolvedLeaf>& leaves) -> bool
{
  for (const auto& leaf : leaves) {
    if (leaf.source == SlotSource::MEMBER
        && leaf.entry->mode != ReadMode::FAST_RDPMC)
    {
      return false;
    }
  }
  return true;
}

auto openGroupWindow(const PmuState& state,
                     const std::vector<ResolvedLeaf>& leaves,
                     const GroupLayout& layout,
                     const Target& where) -> std::unique_ptr<WindowReader>
{
  if (layout.count() == 0) {
    return nullptr;
  }
  const auto [pid, cpu] = leaderPid(where);
  auto window = std::make_unique<PmuWindow>(leaves.size());
  window->groups.resize(layout.count());
  window->slots.reserve(leaves.size());
  for (const auto& one : leaves) {
    if (one.source == SlotSource::DISCLOSURE) {
      window->slots.push_back(LeafSlot {
          .group = 0,
          .index = 0,
          .source = one.source,
          .disclosure = one.disclosure,
      });
      continue;
    }
    const std::size_t group = layout.groupOf(one.device);
    if (one.source != SlotSource::MEMBER) {
      window->slots.push_back(
          LeafSlot {.group = group, .index = 0, .source = one.source});
      continue;
    }
    perf_event_attr attr {};
    fillAttr(attr, state.devices[one.device], *one.entry);
    const bool isLeader = window->groups[group].leader < 0;
    attr.disabled = isLeader ? 1U : 0U;
    const long fd =
        kernelCalls().openEvent(&attr,
                                pid,
                                cpu,
                                isLeader ? -1 : window->groups[group].leader,
                                PERF_FLAG_FD_CLOEXEC);
    if (fd < 0) {
      return nullptr;
    }
    const int handle = static_cast<int>(fd);
    if (isLeader) {
      window->groups[group].leader = handle;
    }
    window->groups[group].members.push_back(handle);
    window->slots.push_back(
        LeafSlot {.group = group,
                  .index = window->groups[group].members.size() - 1,
                  .source = SlotSource::MEMBER});
  }
  for (auto& group : window->groups) {
    // The first leaf a group carries opened as its leader: a leader open
    // the kernel refused returned at the guard above, so a group reaching
    // this loop holds an open leader. The guard the construction rules
    // out stands as the contract here (FR-026).
    SG_ASSERT(group.leader >= 0,
              "every group the window enables opens its leader (FR-026)");
    group.values.assign(group.members.size(), 0);
    // Reset and enable the group once, so every member shares one
    // enable instant and the enabled/running pair describes the group.
    if (kernelCalls().ioctlEvent(
            group.leader, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP)
            != 0
        || kernelCalls().ioctlEvent(
               group.leader, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP)
            != 0)
    {
      return nullptr;
    }
  }
  return window;
}

// The body that opens a member context. The refusals are marked at their
// own sites: the multi-source refusal and the empty-member refusal carry
// fixtures in `test/source/counters_linux_pmu_seam_test.cpp`, and the
// context-open refusal is the same refusal that fixture drives directly.
auto openFastWindow(const PmuState& state,
                    const std::vector<ResolvedLeaf>& leaves,
                    const GroupLayout& layout,
                    const Target& where) -> std::unique_ptr<WindowReader>
{
  if (layout.count() != 1) {
    // The mapped-page protocol reads the counters of one event source;
    // a plan spanning several sources takes the group path.
    return nullptr;
  }
  auto window = std::make_unique<PmuFastWindow>();
  window->slots.reserve(leaves.size());
  std::size_t leader = static_cast<std::size_t>(-1);
  for (const auto& one : leaves) {
    if (one.source == SlotSource::DISCLOSURE) {
      window->slots.push_back(LeafSlot {.group = 0,
                                        .index = 0,
                                        .source = one.source,
                                        .disclosure = one.disclosure});
      continue;
    }
    if (one.source != SlotSource::MEMBER) {
      window->slots.push_back(LeafSlot {
          .group = 0,
          .index = 0,
          .source = one.source,
          .disclosure = one.disclosure,
      });
      continue;
    }
    // The mapped-page read addresses one config word, the `config` word a
    // core PMU event encodes into. An entry whose encoding also sets
    // `config1` or `config2` names a different event once those words are
    // dropped, so the window refuses it and the caller's group path
    // encodes the whole entry (FR-037, FR-040).
    std::uint64_t config = 0;
    bool singleWord = true;
    for (const auto& [word, value] : one.entry->words) {
      if (word == 0) {
        config = value;
      } else {
        singleWord = false;
      }
    }
    if (!singleWord) {
      releaseFastMembers(window->members);
      return nullptr;
    }
    auto context =
        fastContextOpen(state.devices[one.device].type, config, where);
    if (!context) {
      releaseFastMembers(window->members);
      return nullptr;
    }
    if (leader == static_cast<std::size_t>(-1)) {
      leader = window->members.size();
    }
    window->members.push_back(
        PmuFastWindow::Member {.context = std::move(context), .value = 0});
    window->slots.push_back(LeafSlot {.group = 0,
                                      .index = window->members.size() - 1,
                                      .source = SlotSource::MEMBER});
  }
  window->leader = leader;
  return window;
}

// Appends the plan's disclosure column to the resolved leaves, so the
// window that owns it writes it last, after the counts and the ratio
// pair's two columns. The column names no leaf address, so it resolves to
// no entry and to no device (FR-007).
void appendDisclosure(const LeafSet& leaves,
                      std::vector<ResolvedLeaf>& resolved)
{
  if (leaves.disclosureColumn == LeafSet::kNoDisclosureColumn) {
    return;
  }
  resolved.push_back(ResolvedLeaf {
      .device = 0,
      .entry = nullptr,
      .source = SlotSource::DISCLOSURE,
      .disclosure = leaves.disclosureColumn,
  });
}

}  // namespace

PmuFastWindow::~PmuFastWindow()
{
  releaseFastMembers(members);
}

auto pmuOpenFastWindow(const PmuState& state,
                       const LeafSet& leaves,
                       const Target& where) -> std::unique_ptr<WindowReader>
{
  std::vector<ResolvedLeaf> resolved;
  if (!resolve(state, leaves, resolved)) {
    return nullptr;
  }
  appendDisclosure(leaves, resolved);
  GroupLayout layout(state.devices.size());
  layout.assign(resolved);
  return openFastWindow(state, resolved, layout, where);
}

auto pmuOpenWindow(const PmuState& state,
                   const LeafSet& leaves,
                   const Target& where) -> std::unique_ptr<WindowReader>
{
  std::vector<ResolvedLeaf> resolved;
  if (!resolve(state, leaves, resolved)) {
    return nullptr;
  }
  appendDisclosure(leaves, resolved);
  GroupLayout layout(state.devices.size());
  layout.assign(resolved);
  if (allFast(resolved)) {
    // The catalog mode is that device's own page verdict. A host-wide
    // instructions probe does not choose the read (FR-017). A fast window
    // the kernel refuses is a recoverable open failure. The group path is
    // the read the catalog describes for a plan that is not all fast
    // (FR-023).
    if (auto fast = openFastWindow(state, resolved, layout, where); fast) {
      return fast;
    }
  }
  return openGroupWindow(state, resolved, layout, where);
}

}  // namespace sg::counters::detail

#endif  // __linux__
