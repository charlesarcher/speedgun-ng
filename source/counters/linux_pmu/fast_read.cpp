// The mapped-page fast read for the Linux PMU provider: per-thread
// contexts, the sequence-checked rdpmc protocol, and the capability
// probe that decides whether the mechanism is used at all
// (specs/007-counters-and-timers, US7, T052, FR-023, FR-040, R-011).
// The protocol follows jevents/rdpmc from andikleen/pmu-tools; no file
// from that project is copied here. Off Linux the probe reports the
// refusal and the provider keeps the syscall catalog (FR-042).

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

#include "../detail/pmu.hpp"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#  define SG_PMU_FAST_X86 1
#  include <thread>

#  include <fcntl.h>
#  include <linux/perf_event.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <x86intrin.h>
#endif

namespace sg::counters::detail
{

auto fast_decode(const std::uint32_t sequence_before,
                 const std::uint32_t sequence_after,
                 const std::uint64_t capability,
                 const std::uint64_t raw,
                 const std::int64_t offset,
                 const std::uint32_t width,
                 std::uint64_t& value) -> fast_read_verdict
{
  // Protocol order (FR-040, R-011): the capability gate precedes the
  // read, the sequence comparison closes it, and the kernel offset and
  // counter width are applied last.
  if ((capability & 1U) == 0) {
    return fast_read_verdict::not_allowed;
  }
  if (sequence_before != sequence_after) {
    return fast_read_verdict::unstable;
  }
  const auto adjusted =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(raw) + offset);
  value = adjusted & ((1ULL << width) - 1ULL);
  return fast_read_verdict::ok;
}

#if !defined(SG_PMU_FAST_X86)

void pmu_probe_fast(pmu_state& state)
{
  state.fast_available = false;
  state.fast_refusal =
      "the host is not x86, so the time-stamp read path does not apply";
}

std::unique_ptr<fast_context> fast_context_open(const int type,
                                                const std::uint64_t config)
{
  return nullptr;
}

auto fast_context_read(const fast_context&, std::uint64_t&) -> fast_read_verdict
{
  return fast_read_verdict::not_allowed;
}

void fast_context_close(fast_context& context)
{
  static_cast<void>(context);
}

#else

namespace
{

// The published kernel ABI this protocol reads
// (include/uapi/linux/perf_event.h). Field order, sizes, and the
// padding between the header and the data array are fixed by that
// header; every field is read in place, so the layout is the ABI.
struct user_access_page
{
  std::uint32_t version;
  std::uint32_t compat_version;
  std::uint32_t offset;
  std::uint32_t flags;
  std::uint64_t cap_user_rdpmc;
  std::uint64_t cap_user_rdpmc_w;
  std::uint64_t cap_pmu_rdpmc_all;
  std::uint64_t cap_pmu_rdpmc_usr;
  std::uint64_t mm_page_size;
  std::uint64_t reserved_1;
  std::uint64_t reserved_2;
  std::uint64_t reserved_3;
  std::uint64_t time_enabled;
  std::uint64_t time_running;
  std::uint64_t reserved_4[32 * 1024];

  struct counter_slot
  {
    std::uint64_t value;
    std::uint64_t id;
  } data[1024];
};

// The leading fields of the per-event page the mmap of an event file
// descriptor returns. `index` is the kernel's sequence counter for the
// page, `offset` the kernel's signed per-counter adjustment, and
// `pmc_width` the width counters are read at.
struct event_page
{
  std::uint32_t version;
  std::uint32_t compat_version;
  std::uint32_t lock;
  std::uint32_t index;
  std::int64_t offset;
  std::uint64_t time_enabled;
  std::uint64_t time_running;
  std::uint64_t capabilities;
  std::uint16_t pmc_width;
  std::uint16_t time_shift;
  std::uint32_t time_mult;
  std::uint64_t time_offset;
  std::uint64_t time_zero;
  std::uint32_t size;
  std::uint32_t reserved_1;
  std::uint64_t time_cycles;
  std::uint64_t time_mask;
};

auto read_paranoid() -> int
{
  std::ifstream file("/proc/sys/kernel/perf_event_paranoid");
  int value = -1;
  if (file >> value) {
    return value;
  }
  return -1;
}

}  // namespace

void pmu_probe_fast(pmu_state& state)
{
  state.fast_available = false;
  const int paranoid = read_paranoid();
  if (paranoid > 1) {
    // The kernel gates user rdpmc on the same level that gates raw
    // per-thread events; above 1 the user page is not readable.
    state.fast_refusal = "perf_event_paranoid is " + std::to_string(paranoid)
                         + "; the kernel grants user counter reads at 1 or "
                           "below, so the mapped-page read stays unprobed";
    return;
  }
  std::ifstream shift_file("/sys/bus/event_source/devices/cpu/rdpmc");
  int shift = 0;
  if (!(shift_file >> shift) || shift < 12 || shift > 21) {
    state.fast_refusal =
        "the kernel publishes no usable user counter page "
        "('/sys/bus/event_source/devices/cpu/rdpmc' names no page size)";
    return;
  }
  const char* const path = "/sys/bus/event_source/devices/cpu/rdpmc";
  const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    state.fast_refusal =
        "the user counter page exists but this caller cannot open it; the "
        "kernel restricts it to an authorized reader";
    return;
  }
  const auto length = static_cast<std::size_t>(1) << shift;
  // P2 cast at the kernel ABI boundary (I, plan Complexity Tracking):
  // the mapping is the kernel's published perf_user_access page and
  // the fields are read in place at their ABI offsets. Soundness rests
  // on the probe above, which read the page size from sysfs and the
  // capability word from the mapping before any counter is read.
  void* mapping = ::mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
  ::close(fd);
  if (mapping == MAP_FAILED) {
    state.fast_refusal = "the user counter page could not be mapped";
    return;
  }
  const auto* user = static_cast<const user_access_page*>(mapping);
  const bool granted = (user->cap_user_rdpmc & 1U) != 0;
  ::munmap(mapping, length);
  if (!granted) {
    state.fast_refusal =
        "the user counter page publishes no read capability for this "
        "kernel configuration";
    return;
  }
  state.fast_available = true;
  state.fast_refusal.clear();
}

std::unique_ptr<fast_context> fast_context_open(const int type,
                                                const std::uint64_t config)
{
  auto context = std::make_unique<fast_context>();
  context->owner = std::this_thread::get_id();

  perf_event_attr attr {};
  attr.type = static_cast<std::uint32_t>(type);
  attr.size = sizeof(perf_event_attr);
  attr.config = config;
  attr.disabled = 0;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  const long fd =
      ::syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd < 0) {
    return nullptr;
  }
  context->fd = static_cast<int>(fd);
  void* mapping = ::mmap(
      nullptr, sizeof(event_page), PROT_READ, MAP_SHARED, context->fd, 0);
  if (mapping == MAP_FAILED) {
    ::close(context->fd);
    context->fd = -1;
    return nullptr;
  }
  context->map = mapping;

  std::ifstream shift_file("/sys/bus/event_source/devices/cpu/rdpmc");
  int shift = 0;
  if (!(shift_file >> shift) || shift < 12 || shift > 21) {
    fast_context_close(*context);
    return nullptr;
  }
  const int user_fd =
      ::open("/sys/bus/event_source/devices/cpu/rdpmc", O_RDONLY | O_CLOEXEC);
  if (user_fd < 0) {
    fast_context_close(*context);
    return nullptr;
  }
  const auto length = static_cast<std::size_t>(1) << shift;
  void* user_map = ::mmap(nullptr, length, PROT_READ, MAP_SHARED, user_fd, 0);
  ::close(user_fd);
  if (user_map == MAP_FAILED) {
    fast_context_close(*context);
    return nullptr;
  }
  context->user = user_map;
  context->user_length = length;
  return context;
}

auto fast_context_read(const fast_context& context, std::uint64_t& value)
    -> fast_read_verdict
{
  // Same-thread context binding (FR-031, FR-040): a context belongs to
  // the thread that opened it, so its pages are the right ones.
  if (std::this_thread::get_id() != context.owner) {
    return fast_read_verdict::not_allowed;
  }
  const auto* page = static_cast<const event_page*>(context.map);
  const auto* user = static_cast<const user_access_page*>(context.user);
  if (page == nullptr || user == nullptr) {
    return fast_read_verdict::not_allowed;
  }
  // One-based index validity: index 0 means the kernel published no
  // usable counter, and the caller falls back to the group read.
  const auto index = page->index;
  if (index == 0 || index > 1024) {
    return fast_read_verdict::not_allowed;
  }
  const auto id = user->data[index - 1].id;
  if (id == 0 || id > 1024) {
    return fast_read_verdict::not_allowed;
  }
  // Read barriers around the instruction: the kernel writes the
  // counter while a sampling action may read it, and the sequence
  // comparison below closes the window (R-011).
  _mm_lfence();
  const auto raw = static_cast<std::uint64_t>(_rdpmc(static_cast<int>(id) - 1));
  _mm_lfence();
  return fast_decode(
      index,
      page->index,
      user->cap_user_rdpmc,
      raw,
      page->offset,
      page->pmc_width == 0 ? kRnpmcCounterWidth : page->pmc_width,
      value);
}

void fast_context_close(fast_context& context)
{
  if (context.map != nullptr) {
    ::munmap(context.map, sizeof(event_page));
    context.map = nullptr;
  }
  if (context.user != nullptr) {
    ::munmap(context.user, context.user_length);
    context.user = nullptr;
    context.user_length = 0;
  }
  if (context.fd >= 0) {
    ::close(context.fd);
    context.fd = -1;
  }
}

#endif  // SG_PMU_FAST_X86

}  // namespace sg::counters::detail
