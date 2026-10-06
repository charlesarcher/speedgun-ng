// The mapped-page fast read for the Linux PMU provider: per-thread
// contexts, the seqlock rdpmc protocol, and the capability probe that
// decides whether the mechanism is used at all
// (specs/007-counters-and-timers, US7, T052, FR-023, FR-040, R-011).
//
// The protocol is the kernel's own and nothing else: the caller opens
// one `perf_event_attr`, maps one page of the returned descriptor, and
// reads `lock`, `index`, `offset`, the `cap_user_rdpmc` bit of
// `capabilities`, and `pmc_width` from that mapping, issuing
// `rdpmc(index - 1)`. No sysfs attribute takes part in the read
// (include/uapi/linux/perf_event.h). The protocol follows
// jevents/rdpmc from andikleen/pmu-tools; no file from that project is
// copied here. Off Linux the probe reports the refusal and the provider
// keeps the syscall catalog (FR-042).

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include "../detail/pmu.hpp"
#include "speedgun-ng/dbc.hpp"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#  define SG_PMU_FAST_X86 1
#  include <thread>

#  include <linux/perf_event.h>
#  include <sched.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <x86intrin.h>
#endif

namespace sg::counters::detail
{

// One-based counter-index validity over the page `index` (FR-040,
// R-011). Index 0 means the kernel published no usable counter, and an
// index past the operand bound names no counter any host has; both fall
// back to the group read. Pure over the published value, so a synthetic
// page covers it in CI (T066; plan.md Coverage strategy).
auto fast_index_valid(const std::uint32_t index) noexcept -> bool
{
  return index != 0 && index <= kRnpmcMaxIndex;
}

// The counter width a page publishes, or the width the protocol reads at
// when the page publishes none (FR-040, R-011). The published width is
// the mask on every host that publishes one, so a host whose counters
// are not 48 bits wide is measured correctly. Pure over the published
// width, so a synthetic page covers it in CI (T066).
auto fast_counter_width(const std::uint16_t published) noexcept -> std::uint32_t
{
  return published == 0 ? kRnpmcCounterWidth
                        : static_cast<std::uint32_t>(published);
}

// Whether the page sequence held still across a payload read (FR-040,
// R-011). The kernel bumps `lock` around every user-page update and the
// payload rides that update, so a moved sequence means the payload was
// read mid-rewrite and the caller retries. Pure over the two published
// values, so a synthetic page covers it in CI (T066).
auto fast_pair_stable(const std::uint32_t sequence_before,
                      const std::uint32_t sequence_after) noexcept -> bool
{
  return sequence_before == sequence_after;
}

auto fast_pinning_ok(const int pinned_cpu,
                     const int current_cpu) noexcept -> bool
{
  return pinned_cpu < 0 || pinned_cpu == current_cpu;
}

auto fast_decode(const std::uint32_t sequence_before,
                 const std::uint32_t sequence_after,
                 const std::uint32_t index,
                 const std::uint64_t capability,
                 const std::uint64_t raw,
                 const std::int64_t offset,
                 const std::uint32_t width,
                 std::uint64_t& value) -> fast_read_verdict
{
  // Protocol order (FR-040, R-011), the order the header documents: the
  // capability gate, then the one-based index the instruction takes, then
  // the sequence comparison that closes the window, and the kernel offset
  // and counter width applied last. Every arm that publishes no count
  // writes a zero, so no value read before the sequence moved survives
  // into the point the caller folds (FR-002, FR-003).
  if ((capability & 1U) == 0) {
    value = 0;
    return fast_read_verdict::not_allowed;
  }
  if (!fast_index_valid(index)) {
    value = 0;
    return fast_read_verdict::not_allowed;
  }
  if (!fast_pair_stable(sequence_before, sequence_after)) {
    value = 0;
    return fast_read_verdict::unstable;
  }
  // The recipe the kernel's own interface header documents: sign extend
  // the instruction result from the published `pmc_width` bits, then add
  // the page's offset. No mask follows the addition, so a cumulative
  // count past the published counter width stays a cumulative count: the
  // kernel holds the bits above the width in the offset, and masking the
  // sum drops them (FR-004).
  const auto sign_extended =
      static_cast<std::int64_t>(raw << (64U - width)) >> (64U - width);
  value = static_cast<std::uint64_t>(sign_extended + offset);
  SG_ENSURE(value == static_cast<std::uint64_t>(sign_extended + offset),
            "an ok verdict leaves the offset plus the sign-extended "
            "instruction value (FR-004)");
  return fast_read_verdict::ok;
}

auto fast_probe_allows(const bool capability_granted,
                       const std::uint32_t index,
                       std::string& refusal) -> bool
{
  if (!capability_granted) {
    refusal = "the event page the kernel mapped publishes no cap_user_rdpmc "
              "bit, so this caller may not read counters from user space";
    return false;
  }
  if (!fast_index_valid(index)) {
    refusal = "the event page the kernel mapped publishes counter index "
        + std::to_string(index)
        + ", which names no counter the rdpmc instruction can read";
    return false;
  }
  refusal.clear();
  return true;
}

#if !defined(SG_PMU_FAST_X86)

void pmu_probe_fast(pmu_state& state)
{
  state.fast_available = false;
  state.fast_refusal =
      "the host is not x86, so the time-stamp read path does not apply";
}

std::unique_ptr<fast_context> fast_context_open(const int,
                                                const std::uint64_t,
                                                const target&,
                                                std::string* refusal)
{
  if (refusal != nullptr) {
    *refusal = "the host is not x86, so the mapped-page read does not apply";
  }
  return nullptr;
}

auto fast_context_read(const fast_context&, std::uint64_t&) -> fast_read_verdict
{
  return fast_read_verdict::not_allowed;
}

auto fast_context_time_pair(const fast_context&,
                            std::uint64_t&,
                            std::uint64_t&) -> bool
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

// The page a `perf_event_open` descriptor maps, as the kernel declares
// it (include/uapi/linux/perf_event.h). The type is the kernel's, so
// every field below is read at the offset the running kernel's UAPI
// header fixes and no layout is mirrored here (R-011, Constitution I).
using event_page = perf_event_mmap_page;

// The event the capability probe opens: the core PMU's own instruction
// event, which every x86 Linux kernel publishes and every caller that
// may count its own instructions may open (FR-023).
constexpr int kProbeType = PERF_TYPE_HARDWARE;
constexpr std::uint64_t kProbeConfig = PERF_COUNT_HW_INSTRUCTIONS;

}  // namespace

void pmu_probe_fast(pmu_state& state)
{
  // The kernel's own page states this host's account, so the probe opens
  // one real event and reads the mapping the descriptor returns
  // (FR-023, R-011). No sysctl and no sysfs attribute takes part: the
  // read protocol the header publishes consults neither, and a probe
  // that guessed the host's policy from a sysctl was the defect this
  // replaced.
  state.fast_available = false;
  const target where {};
  auto context =
      fast_context_open(kProbeType, kProbeConfig, where, &state.fast_refusal);
  // LCOV_EXCL_START : coverage exclusion (T140): the arm that carries the
  // kernel's own refusal, which `fast_context_open` already wrote into
  // `state.fast_refusal`. It needs a host that refuses a per-process
  // user-mode hardware event outright, which no test can arrange; the
  // refusal text it publishes is covered for both arms by
  // `context_open_refusal_scenario` in
  // `test/source/counters_linux_pmu_seam_test.cpp`.
  if (context) {  // LCOV_EXCL_BR_LINE
    const auto* page = static_cast<const event_page*>(context->map);
    state.fast_available = fast_probe_allows(
        page->cap_user_rdpmc != 0, page->index, state.fast_refusal);
    fast_context_close(*context);
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_STOP
}

std::unique_ptr<fast_context> fast_context_open(const int type,
                                                const std::uint64_t config,
                                                const target& where,
                                                std::string* refusal)
{
  const auto refuse =
      [refusal](std::string sentence) -> std::unique_ptr<fast_context>
  {
    if (refusal != nullptr) {
      *refusal = std::move(sentence);
    }
    return nullptr;
  };

  perf_event_attr attr {};
  attr.type = static_cast<std::uint32_t>(type);
  attr.size = sizeof(perf_event_attr);
  attr.config = config;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  const auto [pid, cpu] = leader_pid(where);
  const long fd =
      ::syscall(SYS_perf_event_open, &attr, pid, cpu, -1, PERF_FLAG_FD_CLOEXEC);
  // LCOV_EXCL_BR_START : coverage exclusion (T140): the granted arm of the
  // open. It needs a `perf_event_open` the kernel answers with a descriptor.
  // A runner whose `perf_event_open` is refused takes the refusing arm below
  // on every call, and the host that grants the syscall takes this one; the
  // refusal itself is measured for both arms by
  // `context_open_refusal_scenario` in
  // `test/source/counters_linux_pmu_seam_test.cpp`.
  if (fd < 0) {  // LCOV_EXCL_BR_LINE
    return refuse("perf_event_open was refused: "
                  + std::string(std::strerror(errno)));
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
  // LCOV_EXCL_START : coverage exclusion (T140): the context the granted arm
  // fills. The descriptor above exists only where the kernel granted the
  // open, so a runner whose `perf_event_open` is refused fills no context
  // here, and the host that grants it fills one on every granted call.
  auto context = std::make_unique<fast_context>();
  context->owner = std::this_thread::get_id();
  context->pinned_cpu = where.kind == target_kind::cpu ? where.cpu : -1;
  context->fd = static_cast<int>(fd);
  context->map_length = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  // LCOV_EXCL_STOP
  // The mapping is the kernel's own page type read at the offsets that
  // type declares, so no cast of a mirrored layout is involved and no P2
  // exception is claimed (Constitution I; plan Complexity Tracking).
  // LCOV_EXCL_START : coverage exclusion (T140): the arm needs a
  // `perf_event_open` that returns a descriptor its own mapping then
  // refuses. Every descriptor the kernel grants maps, so no host and no
  // fixture reaches this arm; the open refusal above is the reachable
  // half and `test/source/counters_linux_pmu_seam_test.cpp` covers it.
  void* mapping = ::mmap(
      nullptr, context->map_length, PROT_READ, MAP_SHARED, context->fd, 0);
  if (mapping == MAP_FAILED) {  // LCOV_EXCL_BR_LINE
    fast_context_close(*context);
    return refuse(
        "the event descriptor the kernel returned could not be " "mapped: "
        + std::string(std::strerror(errno)));  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_STOP
  // LCOV_EXCL_START : coverage exclusion (T140): the mapping hand-back and
  // the epilogue behind it. The descriptor handed back is the granted one,
  // so a runner whose `perf_event_open` is refused returns above, and the
  // host that grants it returns a context on every granted call.
  context->map = mapping;
  return context;
}

// LCOV_EXCL_STOP

// LCOV_EXCL_START : coverage exclusion (T140): the whole mapped-page read.
// It needs a context over a mapping the kernel handed back, and the only
// source of one is a granted `perf_event_open`. A runner whose
// `perf_event_open` is refused opens no context, so this body never runs
// there; the host that grants it runs the body on every sample. The gates
// the body applies are covered for both arms by `fast_decode` and the
// `fast_index_valid` and `fast_pair_stable` seams in
// `test/source/counters_linux_pmu_seam_test.cpp`.
auto fast_context_read(const fast_context& context,
                       std::uint64_t& value) -> fast_read_verdict
{
  SG_REQUIRE(std::this_thread::get_id() == context.owner,
             "a mapped-page read runs on the thread that opened its "
             "context (FR-031, FR-040)");
  // The cpu-target pinning precondition, semantic-gated: a release build
  // configured `ignore` emits no check, and the per-sample cost stays
  // where FR-008 holds it. A thread-bound plan pins nothing and the
  // second conjunct is a compile-time-known-true compare against -1.
  SG_REQUIRE(fast_pinning_ok(context.pinned_cpu, ::sched_getcpu()),
             "a cpu-target fast-mode plan requires its sampling thread to "
             "run on the processor this plan opened its contexts on "
             "(FR-045)");
  const auto* page = static_cast<const event_page*>(context.map);
  // The protocol the header publishes, in its order: snapshot the
  // sequence, take the payload and the instruction, compare the sequence
  // again (FR-040, R-011). Every gate below is decided by the page, so
  // `fast_decode` applies them and a caller the kernel refuses pays for
  // page loads only.
  const auto sequence = page->lock;
  _mm_lfence();
  const auto index = page->index;
  const auto offset = page->offset;
  const auto width = fast_counter_width(page->pmc_width);
  const auto capability = static_cast<std::uint64_t>(page->cap_user_rdpmc);
  // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that withholds
  // the instruction. The kernel assigns a nonzero index to every event it
  // opens on a host that grants the capability, so the arm needs a host
  // that grants the capability and indexes nothing; the same gate is
  // covered for both arms by `fast_index_valid` inside `fast_decode`.
  const auto raw = index == 0  // LCOV_EXCL_BR_LINE
      ? 0ULL
      : static_cast<std::uint64_t>(  // LCOV_EXCL_BR_LINE
            _rdpmc(static_cast<int>(index) - 1));  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
  _mm_lfence();
  return fast_decode(
      sequence, page->lock, index, capability, raw, offset, width, value);
  // LCOV_EXCL_STOP
}

// LCOV_EXCL_START : coverage exclusion (T140): the whole enabled/running pair
// read. It reads the same granted mapping the counter read above needs, so a
// runner whose `perf_event_open` is refused never runs it, and the host that
// grants the syscall runs it on every sampling action. The stability gate
// the body applies is covered for both arms by `fast_pair_stable`.
auto fast_context_time_pair(const fast_context& context,
                            std::uint64_t& enabled,
                            std::uint64_t& running) -> bool
{
  SG_REQUIRE(std::this_thread::get_id() == context.owner,
             "the enabled/running pair is read on the thread that opened "
             "its context (FR-031, FR-040)");
  SG_REQUIRE(fast_pinning_ok(context.pinned_cpu, ::sched_getcpu()),
             "a cpu-target fast-mode plan requires its sampling thread to "
             "run on the processor this plan opened its contexts on "
             "(FR-045)");
  const auto* page = static_cast<const event_page*>(context.map);
  // The pair is payload of the same user-page update the counter value
  // rides, so it is read under the same seqlock snapshot (FR-041, R-011).
  const auto sequence = page->lock;
  _mm_lfence();
  const auto page_enabled = page->time_enabled;
  const auto page_running = page->time_running;
  _mm_lfence();
  // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that reports no
  // pair. It needs the kernel to rewrite the page between the two reads of
  // its sequence, which a test cannot force deterministically; the same
  // comparison is covered for both arms by `fast_pair_stable`.
  if (!fast_pair_stable(sequence, page->lock)) {  // LCOV_EXCL_BR_LINE
    return false;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
  enabled = page_enabled;
  running = page_running;
  return true;
  // LCOV_EXCL_STOP
}

void fast_context_close(fast_context& context)
{
  // Both arms run on any host. `fast_context_lifetime_scenario` in
  // `test/source/counters_linux_pmu_seam_test.cpp` reaches them through
  // `~fast_context` with a descriptor and an anonymous mapping the test
  // opened itself, so no marker over this release stands and the coverage
  // gate measures it (T074, FR-027, FR-046).
  if (context.map != nullptr) {
    ::munmap(context.map, context.map_length);
    context.map = nullptr;
    context.map_length = 0;
  }
  if (context.fd >= 0) {
    ::close(context.fd);
    context.fd = -1;
  }
}

#endif  // SG_PMU_FAST_X86

}  // namespace sg::counters::detail
