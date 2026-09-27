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

// The page structure version this reader implements. The published ABI
// (include/uapi/linux/perf_event.h) gives both the perf user-access page
// and a per-event page a `version`, the version of the structure, and a
// `compat_version`, the lowest version the kernel still serves. A page
// declaring either above the version implemented here is refused, since
// its later fields may have moved. The running kernel leaves both fields
// zero, and a page declaring nothing is read at the offsets this file
// compiles against, which that header fixes (R-011, fail closed).
//
// Pure over the two declared fields, so a synthetic page covers it in CI
// (T066; plan.md Coverage strategy).
auto fast_page_version_readable(const std::uint32_t version,
                                const std::uint32_t compat_version) noexcept
    -> bool
{
  return version <= kRnpmcPageVersion && compat_version <= kRnpmcPageVersion;
}

// One-based counter-index validity over the page `index` and the id the
// user-access page publishes for its slot (FR-040, R-011). Index 0 means
// the kernel published no usable counter, and an id outside the slot
// array is not the counter the index names; both fall back to the group
// read. Pure over the two published values, so a synthetic page covers it
// in CI (T066; plan.md Coverage strategy).
auto fast_index_valid(const std::uint32_t index,
                      const std::uint64_t slot_id) noexcept -> bool
{
  return index != 0 && index <= kRnpmcSlots && slot_id != 0
      && slot_id <= kRnpmcSlots;
}

// The counter width a page publishes, or the width the protocol reads at
// when the page publishes none (FR-040, R-011). Pure over the published
// width, so a synthetic page covers it in CI (T066).
auto fast_counter_width(const std::uint16_t published) noexcept -> std::uint32_t
{
  return published == 0 ? kRnpmcCounterWidth
                        : static_cast<std::uint32_t>(published);
}

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

auto fast_context_time_pair(const fast_context&, std::uint64_t&, std::uint64_t&)
    -> bool
{
  return false;
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
  } data[kRnpmcSlots];
};

// The leading fields of the per-event page the mmap of an event file
// descriptor returns. `lock` is the kernel's seqlock for the page,
// `index` the one-based counter index it publishes, `offset` the
// kernel's signed per-counter adjustment, and `pmc_width` the width
// counters are read at.
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

// LCOV_EXCL_BR_START : the paranoid-sysctl read shares the reason of the
// region below: the switch is read-only kernel state in a container
// namespace, and no test can make the read fail.
auto read_paranoid() -> int  // LCOV_EXCL_BR_LINE
{  // LCOV_EXCL_LINE
  std::ifstream file("/proc/sys/kernel/perf_event_paranoid");
  int value = -1;
  if (file >> value) {  // LCOV_EXCL_BR_LINE
    return value;  // LCOV_EXCL_LINE
  }
  return -1;  // LCOV_EXCL_LINE
}  // LCOV_EXCL_BR_STOP

// LCOV_EXCL_START : coverage exclusion (T066, P2 recorded in
// specs/007-counters-and-timers/plan.md Complexity Tracking): the residual
// kernel glue of the fast-read protocol, from the global userspace counter
// gate to the end of the file.
//
// The gate is the kernel's own. On this host
// `/proc/sys/kernel/perf_event_paranoid` reads 2, `pmu_probe_fast` returns at
// the `paranoid > 1` guard above, and everything below runs only after a probe
// that clears that level. The CI matrix is unprivileged (constitution VIII,
// SC-002), so the level is 2 there too. The pure halves the glue feeds are now
// free functions over injected page and index values and carry synthetic
// fixtures in `test/source/counters_linux_pmu_seam_test.cpp`: the
// structure-version gate, the one-based index and slot-id gate, the counter
// width, the capability gate, the sequence comparison, the kernel offset, and
// the width mask. What remains here is `perf_event_open`, `mmap`, the
// `_rdpmc` instruction, and the sysctl and sysfs reads, which no fixture
// can supply: the mmap of the perf user-access page fails with EACCES on
// this host (`/sys/bus/event_source/devices/cpu/rdpmc` reads
// "Permission denied"), and the fast-capable developer host the plan names
// is where the evidence is recorded.
//
// LCOV_EXCL_BR_START
// The global userspace counter gate. The kernel requires
// /proc/sys/kernel/perf_user_access enabled before a caller reads
// hardware counters outside the kernel
// (Documentation/arch/arm64/perf.rst). A host publishing the switch as
// zero refuses the read; a host publishing no switch leaves the
// paranoid level and the page capability word as the gates this probe
// reads, which is the -1 outcome (R-011).
auto read_user_access_switch() -> int
{
  std::ifstream file("/proc/sys/kernel/perf_user_access");
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
  if (const int user_access = read_user_access_switch(); user_access == 0) {
    state.fast_refusal =
        "the kernel publishes /proc/sys/kernel/perf_user_access as 0 and "
        "refuses hardware counter reads outside the kernel";
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
  // on the probe above, which established the permission sysctls, the
  // page size from sysfs, the page's declared version, and the
  // capability word from the mapping, all of them before any counter
  // is read.
  void* mapping = ::mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
  ::close(fd);
  if (mapping == MAP_FAILED) {
    state.fast_refusal = "the user counter page could not be mapped";
    return;
  }
  const auto* user = static_cast<const user_access_page*>(mapping);
  const bool readable =
      fast_page_version_readable(user->version, user->compat_version);
  const bool granted = (user->cap_user_rdpmc & 1U) != 0;
  ::munmap(mapping, length);
  if (!readable) {
    state.fast_refusal =
        "the user counter page declares a structure version this reader "
        "does not implement";
    return;
  }
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
  const auto* page = static_cast<const event_page*>(mapping);
  if (!fast_page_version_readable(page->version, page->compat_version)) {
    // A page whose declared version this reader does not implement may
    // have moved the fields read below, so the context is refused (R-011).
    fast_context_close(*context);
    return nullptr;
  }

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
  // Capability gate ahead of the instruction (FR-040, R-011): the
  // published protocol tests the capability before it takes the read,
  // so a caller the kernel grants no read capability never pays for the
  // instruction.
  if ((user->cap_user_rdpmc & 1U) == 0) {
    return fast_read_verdict::not_allowed;
  }
  // Seqlock snapshot: the kernel increments the page `lock` around every
  // user-page update, and `index`, `offset` and the counter width are the
  // payload that update carries (kernel/events/core.c,
  // perf_event_update_userpage). jevents/rdpmc reads the sequence from
  // the same field, and the comparison below closes the window (R-011).
  const auto sequence = page->lock;
  _mm_lfence();
  // One-based index validity: index 0 means the kernel published no
  // usable counter, and the caller falls back to the group read. The
  // bound is tested ahead of the subscript because the slot array is
  // indexed by it.
  const auto index = page->index;
  if (index == 0 || index > kRnpmcSlots) {
    return fast_read_verdict::not_allowed;
  }
  const auto id = user->data[index - 1].id;
  if (!fast_index_valid(index, id)) {
    return fast_read_verdict::not_allowed;
  }
  const auto offset = page->offset;
  const auto width = fast_counter_width(page->pmc_width);
  // Read barriers around the instruction: the kernel writes the
  // counter while a sampling action may read it, and the sequence
  // comparison below closes the window (R-011).
  const auto raw = static_cast<std::uint64_t>(_rdpmc(static_cast<int>(id) - 1));
  _mm_lfence();
  return fast_decode(
      sequence, page->lock, user->cap_user_rdpmc, raw, offset, width, value);
}

auto fast_context_time_pair(const fast_context& context,
                            std::uint64_t& enabled,
                            std::uint64_t& running) -> bool
{
  const auto* page = static_cast<const event_page*>(context.map);
  if (page == nullptr) {
    return false;
  }
  // The pair is payload of the same user-page update the counter value
  // rides, so it is read under the same seqlock snapshot (FR-041, R-011).
  const auto sequence = page->lock;
  _mm_lfence();
  const auto page_enabled = page->time_enabled;
  const auto page_running = page->time_running;
  _mm_lfence();
  if (sequence != page->lock) {
    return false;
  }
  enabled = page_enabled;
  running = page_running;
  return true;
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
}  // LCOV_EXCL_BR_STOP

// LCOV_EXCL_STOP

#endif  // SG_PMU_FAST_X86

}  // namespace sg::counters::detail
