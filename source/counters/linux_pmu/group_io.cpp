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

namespace sg::counters::detail
{
namespace
{

// Where one managed leaf's point comes from inside its device group.
enum class slot_source : std::uint8_t
{
  member,
  time_enabled,
  time_running
};

struct leaf_slot
{
  std::size_t group = 0;
  std::size_t index = 0;  // member position, unused for the time pair
  slot_source source = slot_source::member;
};

// One requested leaf resolved against the merged catalog.
struct resolved_leaf
{
  std::size_t device = 0;
  const pmu_entry* entry = nullptr;
  slot_source source = slot_source::member;
};

// Device to group index, assigned in first-appearance order so the
// group layout is deterministic (FR-024).
class group_layout
{
public:
  explicit group_layout(const std::size_t devices)
      : m_group_of_device(devices, static_cast<std::size_t>(-1))
  {
  }

  void assign(const std::vector<resolved_leaf>& leaves) noexcept
  {
    for (const auto& leaf : leaves) {
      if (leaf.source != slot_source::member) {
        continue;
      }
      if (m_group_of_device[leaf.device] != static_cast<std::size_t>(-1)) {
        continue;
      }
      m_group_of_device[leaf.device] = m_group_count;
      ++m_group_count;
    }
  }

  [[nodiscard]] auto group_of(const std::size_t device) const noexcept
      -> std::size_t
  {
    return m_group_of_device[device];
  }

  [[nodiscard]] auto count() const noexcept -> std::size_t
  {
    return m_group_count;
  }

private:
  std::vector<std::size_t> m_group_of_device;
  std::size_t m_group_count = 0;
};

// Resolves every requested address against the catalog in the order
// asked, so the sink order equals the column order (C-PRO-2). Null on
// any address this provider cannot serve.
auto resolve(const pmu_state& state,
             const leaf_set& leaves,
             std::vector<resolved_leaf>& out) -> bool
{
  if (leaves.addresses.empty()) {
    return false;
  }
  std::map<std::string, std::size_t> device_index;
  for (std::size_t index = 0; index < state.devices.size(); ++index) {
    device_index.emplace(state.devices[index].path, index);
  }
  out.clear();
  out.reserve(leaves.addresses.size());
  for (const auto& address : leaves.addresses) {
    const auto slash = address.rfind('/');
    if (slash == std::string::npos) {
      return false;
    }
    const std::string device_name = address.substr(0, slash);
    const std::string leaf_name = address.substr(slash + 1);
    const auto located = device_index.find(device_name);
    if (located == device_index.end()) {
      return false;
    }
    const auto& device = state.devices[located->second];
    const pmu_entry* found = nullptr;
    for (const auto& entry : device.entries) {
      if (entry.name == leaf_name) {
        found = &entry;
        break;
      }
    }
    if (found == nullptr || found->avail != availability::countable) {
      return false;
    }
    slot_source source = slot_source::member;
    if (leaf_name == "enabled") {
      source = slot_source::time_enabled;
    } else if (leaf_name == "running") {
      source = slot_source::time_running;
    }
    out.push_back(resolved_leaf {
        .device = located->second, .entry = found, .source = source});
  }
  return true;
}

auto fill_attr(perf_event_attr& attr,
               const pmu_device& device,
               const pmu_entry& entry) -> void
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

struct group_state
{
  int leader = -1;
  std::vector<int> members;
  std::vector<std::uint64_t> values;
  std::uint64_t enabled = 0;
  std::uint64_t running = 0;
};

}  // namespace

struct pmu_window final : window_reader
{
  std::vector<leaf_slot> slots;
  std::vector<group_state> groups;
  std::vector<std::uint64_t> scratch;

  // Every leaf opens into exactly one group, so the leaf count bounds the
  // member count of every group; the scratch covers the header and one word
  // per member of the widest group, and the sampling path never grows it.
  explicit pmu_window(const std::size_t leaf_count)
  {
    set_thunk(&read_direct);
    scratch.resize(kHeaderWords + leaf_count);
  }

  pmu_window(const pmu_window&) = delete;
  auto operator=(const pmu_window&) -> pmu_window& = delete;
  pmu_window(pmu_window&&) = delete;
  auto operator=(pmu_window&&) -> pmu_window& = delete;

  ~pmu_window() override
  {
    for (auto& group : groups) {
      for (const int fd : group.members) {
        ::close(fd);
      }
    }
  }

  void read_points(point_sink& sink) noexcept override
  {
    for (auto& group : groups) {
      const auto want = static_cast<std::size_t>(
          (kHeaderWords + group.members.size()) * sizeof(std::uint64_t));
      // LCOV_EXCL_START : coverage exclusion (T066): the arm needs a group
      // the kernel cannot serve in this shape, a leader answering with fewer
      // than the three header words. `open_group_window` enables every group
      // with `PERF_EVENT_IOC_ENABLE` before returning, so the kernel always
      // reports the header. The scratch is sized in the constructor, and
      // every leaf opens into exactly one group, so the buffer covers the
      // header and one word per member of any group: the read below never
      // grows it, and the sampling path allocates nothing (FR-026).
      const auto got = ::read(group.leader, scratch.data(), want);
      if (got < static_cast<long>(kHeaderWords * sizeof(std::uint64_t)))
      {  // A group the kernel could not read
         // this action reports no
        // point; the fold reads the gap as zero and the pair discloses
        // ratio 0. A count is never fabricated.
        group.enabled = 0;  // LCOV_EXCL_LINE
        group.running = 0;  // LCOV_EXCL_LINE
        std::ranges::fill(group.values, 0);  // LCOV_EXCL_LINE
        continue;  // LCOV_EXCL_LINE
      }  // LCOV_EXCL_BR_LINE
      // LCOV_EXCL_STOP
      const auto count = static_cast<std::size_t>(scratch[0]);
      group.enabled = scratch[1];
      group.running = scratch[2];
      for (std::size_t member = 0; member < group.members.size(); ++member) {
        group.values[member] = member < count  // LCOV_EXCL_BR_LINE
            ? scratch[kHeaderWords + member]  // LCOV_EXCL_LINE
            : 0;  // LCOV_EXCL_LINE
      }  // LCOV_EXCL_BR_LINE
    }  // LCOV_EXCL_BR_LINE
    for (const auto& slot : slots) {
      const auto& group = groups[slot.group];
      // LCOV_EXCL_BR_START : coverage exclusion (T066): the "fewer values
      // than members" arm needs the kernel to report an `nr` below the
      // member count, which a group read never does while every member is
      // enabled, and the switch has no fourth enumerator to fall through to.
      switch (slot.source) {  // LCOV_EXCL_BR_LINE
        case slot_source::member:
          sink.put(group.values[slot.index]);
          break;
        case slot_source::time_enabled:
          sink.put(group.enabled);
          break;
        case slot_source::time_running:
          sink.put(group.running);
          break;
      }  // LCOV_EXCL_BR_LINE
    }  // LCOV_EXCL_BR_LINE
  }  // LCOV_EXCL_BR_LINE

  // The compiled plan hands the window over as the base reference
  // `read_thunk` declares, and `pmu_open_window` constructs it as this
  // final type, so the reference names a group window on every call.
  // `final` fixes the target of the `read_points` call, so the sampling
  // path takes one indirect call and no vtable lookup (FR-022, T146).
  static auto read_direct(window_reader& base,
                          point_sink& sink) noexcept -> void
  {
    static_cast<pmu_window&>(base).read_points(sink);
  }
};  // LCOV_EXCL_BR_STOP

// The fast-mode window: one mapped page per member leaf, read inside one
// sampling action, and the enabled/running pair taken from the leader's
// page (FR-040, FR-041). The window runs for real on any host the kernel
// lets open a per-process user event, so the reads below carry no blanket
// coverage exclusion; the two arms no host reaches are marked at their own
// sites.
struct pmu_fast_window final : window_reader
{
  struct member
  {
    std::unique_ptr<fast_context> context;
    std::uint64_t value = 0;
  };

  std::vector<leaf_slot> slots;
  std::vector<member> members;
  // The member whose page carries the group's enabled/running pair, the
  // pair a syscall-mode window reads from the leader (FR-041).
  std::size_t leader = 0;
  std::uint64_t enabled = 0;
  std::uint64_t running = 0;

  pmu_fast_window() { set_thunk(&read_direct); }

  pmu_fast_window(const pmu_fast_window&) = delete;
  auto operator=(const pmu_fast_window&) -> pmu_fast_window& = delete;
  pmu_fast_window(pmu_fast_window&&) = delete;
  auto operator=(pmu_fast_window&&) -> pmu_fast_window& = delete;

  ~pmu_fast_window() override = default;

  void read_points(point_sink& sink) noexcept override
  {
    for (auto& one : members) {
      // LCOV_EXCL_BR_START : coverage exclusion (T140): the retry arm. The
      // sequence moves only when the kernel rewrites the page between the
      // two reads of it, which a test cannot force deterministically. The
      // same verdict is covered for both arms by `fast_pair_stable` inside
      // `fast_decode` in `test/source/counters_linux_pmu_seam_test.cpp`.
      if (fast_context_read(*one.context, one.value)  // LCOV_EXCL_BR_LINE
          == fast_read_verdict::unstable)  // LCOV_EXCL_BR_LINE
      {
        // The page sequence moved under the read; the protocol's
        // stated fallback is a second attempt (FR-040).
        // clang-format off: gcov records every line of a call, and lcov
        // excludes the line its marker sits on, so the marker and the call
        // share one line; the column limit would split them apart again.
        static_cast<void>(fast_context_read(*one.context, one.value));  // LCOV_EXCL_LINE
        // clang-format on
      }  // LCOV_EXCL_BR_LINE
      // LCOV_EXCL_BR_STOP
    }
    // The enabled/running pair rides the leader's user page, the same
    // pair the group read takes from the leader, so the multiplex
    // ratio is computed inside folds in this read mode too (FR-041).
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that reports
    // no pair. It needs the kernel to rewrite the leader's page between the
    // two reads of its sequence, which a test cannot force
    // deterministically; the comparison itself is covered for both arms by
    // `fast_pair_stable`.
    if (!fast_context_time_pair(*members[leader].context, enabled, running))
    {  // LCOV_EXCL_BR_LINE
      // A leader whose page disclosed no stable pair this action
      // reports none; the fold reads the gap as zero and the pair
      // discloses ratio 0. A time is never fabricated.
      enabled = 0;  // LCOV_EXCL_LINE
      running = 0;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    for (const auto& slot : slots) {
      // LCOV_EXCL_BR_LINE : coverage exclusion (T140): the switch's
      // implicit no-case arc. `slot_source` has three enumerators and all
      // three are taken above, so the arc is the block the compiler emits
      // for a value the enumeration cannot hold.
      switch (slot.source) {  // LCOV_EXCL_BR_LINE
        case slot_source::member:
          sink.put(members[slot.index].value);
          break;
        case slot_source::time_enabled:
          sink.put(enabled);
          break;
        case slot_source::time_running:
          sink.put(running);
          break;
      }  // LCOV_EXCL_BR_LINE
    }
  }

  // The compiled plan hands the window over as the base reference
  // `read_thunk` declares, and `pmu_open_fast_window` constructs it as
  // this final type, so the reference names a fast window on every
  // call. `final` fixes the target of the `read_points` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto read_direct(window_reader& base,
                          point_sink& sink) noexcept -> void
  {
    static_cast<pmu_fast_window&>(base).read_points(sink);
  }
};

namespace
{

// Every requested leaf must have the catalog's fast mode recorded, so
// the read the plan performs is the read the catalog disclosed
// (FR-023, C-PRO-4).
auto all_fast(const std::vector<resolved_leaf>& leaves) -> bool
{
  for (const auto& leaf : leaves) {
    if (leaf.source == slot_source::member
        && leaf.entry->mode != read_mode::fast_rdpmc)
    {
      return false;
    }
  }
  return true;
}

auto open_group_window(const pmu_state& state,
                       const std::vector<resolved_leaf>& leaves,
                       const group_layout& layout,
                       const target& where) -> std::unique_ptr<window_reader>
{
  if (layout.count() == 0) {
    return nullptr;
  }
  const auto [pid, cpu] = leader_pid(where);
  auto window = std::make_unique<pmu_window>(leaves.size());
  window->groups.resize(layout.count());
  window->slots.reserve(leaves.size());
  for (const auto& one : leaves) {
    const std::size_t group = layout.group_of(one.device);
    if (one.source != slot_source::member) {
      window->slots.push_back(
          leaf_slot {.group = group, .index = 0, .source = one.source});
      continue;
    }
    perf_event_attr attr {};
    fill_attr(attr, state.devices[one.device], *one.entry);
    const bool is_leader = window->groups[group].leader < 0;
    attr.disabled = is_leader ? 1U : 0U;
    const long fd = ::syscall(SYS_perf_event_open,
                              &attr,
                              pid,
                              cpu,
                              is_leader ? -1 : window->groups[group].leader,
                              PERF_FLAG_FD_CLOEXEC);
    if (fd < 0) {
      return nullptr;
    }
    const int handle = static_cast<int>(fd);
    if (is_leader) {
      window->groups[group].leader = handle;
    }
    window->groups[group].members.push_back(handle);
    window->slots.push_back(
        leaf_slot {.group = group,
                   .index = window->groups[group].members.size() - 1,
                   .source = slot_source::member});
  }
  // LCOV_EXCL_START : coverage exclusion (T066): both arms need a
  // `perf_event_open` that succeeds and then fails its reset or enable.
  // The open either refuses outright, at the `fd < 0` guard above, or
  // returns a leader the kernel accepts; there is no in-process path that
  // hands back a group whose ioctl the kernel then refuses.
  for (auto& group : window->groups) {  // LCOV_EXCL_BR_LINE
    if (group.leader < 0) {  // LCOV_EXCL_BR_LINE
      return nullptr;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    group.values.assign(group.members.size(), 0);  // LCOV_EXCL_LINE
    // Reset and enable the group once, so every member shares one
    // enable instant and the enabled/running pair describes the group.
    if (::ioctl(group.leader, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) != 0
        || ::ioctl(group.leader, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP)
            != 0)  // LCOV_EXCL_BR_LINE
    {
      return nullptr;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_LINE
  return window;  // LCOV_EXCL_LINE
  // LCOV_EXCL_STOP
}

// The body that opens a member context. The refusals are marked at their
// own sites: the multi-source refusal and the empty-member refusal carry
// fixtures in `test/source/counters_linux_pmu_seam_test.cpp`, and the
// context-open refusal is the same refusal that fixture drives directly.
auto open_fast_window(const pmu_state& state,
                      const std::vector<resolved_leaf>& leaves,
                      const group_layout& layout,
                      const target& where) -> std::unique_ptr<window_reader>
{
  if (layout.count() != 1) {
    // The mapped-page protocol reads the counters of one event source;
    // a plan spanning several sources takes the group path.
    return nullptr;
  }
  auto window = std::make_unique<pmu_fast_window>();
  window->slots.reserve(leaves.size());
  std::size_t leader = static_cast<std::size_t>(-1);
  for (const auto& one : leaves) {
    if (one.source != slot_source::member) {
      window->slots.push_back(
          leaf_slot {.group = 0, .index = 0, .source = one.source});
      continue;
    }
    // The mapped-page read addresses one config word, the `config` word a
    // core PMU event encodes into. An entry whose encoding also sets
    // `config1` or `config2` names a different event once those words are
    // dropped, so the window refuses it and the caller's group path
    // encodes the whole entry (FR-037, FR-040).
    std::uint64_t config = 0;
    bool single_word = true;
    for (const auto& [word, value] : one.entry->words) {
      if (word == 0) {
        config = value;
      } else {
        single_word = false;
      }
    }
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that refuses a
    // multi-word encoding. It needs a countable entry that sets a config
    // word above the first, which no core PMU event does; the refusal is
    // asserted for a synthetic such entry in
    // `test/source/counters_linux_pmu_seam_test.cpp`.
    if (!single_word) {  // LCOV_EXCL_BR_LINE
      return nullptr;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    auto context =
        fast_context_open(state.devices[one.device].type, config, where);
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that refuses
    // the whole window because one member's event was refused. It needs a
    // catalog entry the kernel counts, whose event `perf_event_open` then
    // refuses; the refusal itself is covered for both arms by
    // `context_open_refusal_scenario` in
    // `test/source/counters_linux_pmu_seam_test.cpp`.
    if (!context) {  // LCOV_EXCL_BR_LINE
      return nullptr;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    if (leader == static_cast<std::size_t>(-1)) {
      leader = window->members.size();
    }
    window->members.push_back(
        pmu_fast_window::member {.context = std::move(context), .value = 0});
    window->slots.push_back(leaf_slot {.group = 0,
                                       .index = window->members.size() - 1,
                                       .source = slot_source::member});
  }
  window->leader = leader;
  return window;
}

}  // namespace

auto pmu_open_fast_window(const pmu_state& state,
                          const leaf_set& leaves,
                          const target& where) -> std::unique_ptr<window_reader>
{
  std::vector<resolved_leaf> resolved;
  if (!resolve(state, leaves, resolved)) {
    return nullptr;
  }
  group_layout layout(state.devices.size());
  layout.assign(resolved);
  return open_fast_window(state, resolved, layout, where);
}

auto pmu_open_window(const pmu_state& state,
                     const leaf_set& leaves,
                     const target& where) -> std::unique_ptr<window_reader>
{
  std::vector<resolved_leaf> resolved;
  if (!resolve(state, leaves, resolved)) {
    return nullptr;
  }
  group_layout layout(state.devices.size());
  layout.assign(resolved);
  if (state.fast_available && all_fast(resolved)) {
    // The catalog discloses fast_rdpmc for the entries of a fast-capable
    // host, so the read must be the mapped-page read; a fast window the
    // kernel refuses is a recoverable open failure, never a silent
    // downgrade to a read the catalog does not describe (FR-023).
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the falling-through
    // arm. It needs a fast-capable catalog whose leaf set the fast window
    // then refuses, which `open_fast_window` answers only when a member
    // event is refused (marked at its own site above). The accepting arm
    // is reached on every host the probe passes.
    if (auto fast = open_fast_window(state, resolved, layout, where); fast)
    {  // LCOV_EXCL_BR_LINE
      return fast;
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
  }
  return open_group_window(state, resolved, layout, where);
}

}  // namespace sg::counters::detail

#endif  // __linux__
