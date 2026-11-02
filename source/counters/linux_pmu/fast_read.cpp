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
#include "kernel_calls.hpp"
#include "speedgun-ng/counters_provider.hpp"
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

// The move operations serve both platform branches: the release arm is
// the platform-independent `fastContextClose`, so one definition pair
// stands outside the branch and the branch's own close serves each
// build (specs/012 FR-013).
FastContext::FastContext(FastContext&& other) noexcept
    : fd {other.fd}
    , map {other.map}
    , mapLength {other.mapLength}
    , owner {other.owner}
    , pinnedCpu {other.pinnedCpu}
{
#if SG_CONTRACTS_SEMANTIC != 0
  const auto takenFd = other.fd;
  const void* takenMap = other.map;
  const auto takenLength = other.mapLength;
#endif
  other.fd = -1;
  other.map = nullptr;
  other.mapLength = 0;
  SG_ENSURE(fd == takenFd && map == takenMap && mapLength == takenLength,
            "a move construction leaves the value with the descriptor, the "
            "mapping, and the length the source held");
  SG_ENSURE(other.fd == -1 && other.map == nullptr && other.mapLength == 0,
            "a move construction leaves the source owning nothing");
}

auto FastContext::operator=(FastContext&& other) noexcept -> FastContext&
{
  SG_REQUIRE(this != &other, "a move assignment names two different values");
#if SG_CONTRACTS_SEMANTIC != 0
  const auto takenFd = other.fd;
  const void* takenMap = other.map;
  const auto takenLength = other.mapLength;
#endif
  fastContextClose(*this);
  fd = other.fd;
  map = other.map;
  mapLength = other.mapLength;
  owner = other.owner;
  pinnedCpu = other.pinnedCpu;
  other.fd = -1;
  other.map = nullptr;
  other.mapLength = 0;
  SG_ENSURE(fd == takenFd && map == takenMap && mapLength == takenLength,
            "a move assignment leaves the target with the descriptor, the "
            "mapping, and the length the source held");
  SG_ENSURE(other.fd == -1 && other.map == nullptr && other.mapLength == 0,
            "a move assignment leaves the source owning nothing");
  return *this;
}

// One-based counter-index validity over the page `index` (FR-040, R-011,
// FR-053). Index 0 means the kernel published no usable counter, and a
// counter number past the operand bound names no counter any host has;
// both fall back to the group read. A fixed counter arrives with
// kFixedCounterIndexMark set above its number, so the bound reads the
// number beneath the mark and leaves the marked index for the instruction,
// which the mark selects. Pure over the published value, so a synthetic
// page covers it in CI (T066; plan.md Coverage strategy).
auto fastIndexValid(const std::uint32_t index) noexcept -> bool
{
  const auto counterNumber = index & ~kFixedCounterIndexMark;
  return counterNumber != 0 && counterNumber <= kRnpmcMaxIndex;
}

// The counter width a page publishes, or the width the protocol reads at
// when the page publishes none (FR-040, R-011). The published width is
// the mask on every host that publishes one, so a host whose counters
// are not 48 bits wide is measured correctly. Pure over the published
// width, so a synthetic page covers it in CI (T066).
auto fastCounterWidth(const std::uint16_t published) noexcept -> std::uint32_t
{
  return published == 0 ? kRnpmcCounterWidth
                        : static_cast<std::uint32_t>(published);
}

// Whether the page sequence held still across a payload read (FR-040,
// R-011). The kernel bumps `lock` around every user-page update and the
// payload rides that update, so a moved sequence means the payload was
// read mid-rewrite and the caller retries. Pure over the two published
// values, so a synthetic page covers it in CI (T066).
auto fastPairStable(const std::uint32_t sequenceBefore,
                    const std::uint32_t sequenceAfter) noexcept -> bool
{
  return sequenceBefore == sequenceAfter;
}

auto fastPinningOk(const int pinnedCpu, const int currentCpu) noexcept -> bool
{
  return pinnedCpu < 0 || pinnedCpu == currentCpu;
}

auto fastDecode(const std::uint32_t sequenceBefore,
                const std::uint32_t sequenceAfter,
                const std::uint32_t index,
                const std::uint64_t capability,
                const std::uint64_t raw,
                const std::int64_t offset,
                const std::uint32_t width,
                std::uint64_t& value) -> FastReadVerdict
{
  // Protocol order (FR-040, R-011), the order the header documents: the
  // capability gate, then the one-based index the instruction takes, then
  // the sequence comparison that closes the window, and the kernel offset
  // and counter width applied last. Every arm that publishes no count
  // writes a zero, so no value read before the sequence moved survives
  // into the point the caller folds (FR-002, FR-003).
  if ((capability & 1U) == 0) {
    value = 0;
    return FastReadVerdict::NOT_ALLOWED;
  }
  if (!fastIndexValid(index)) {
    value = 0;
    return FastReadVerdict::NOT_ALLOWED;
  }
  if (!fastPairStable(sequenceBefore, sequenceAfter)) {
    value = 0;
    return FastReadVerdict::UNSTABLE;
  }
  // The recipe the kernel's own interface header documents: sign extend
  // the instruction result from the published `pmc_width` bits, then add
  // the page's offset. No mask follows the addition, so a cumulative
  // count past the published counter width stays a cumulative count: the
  // kernel holds the bits above the width in the offset, and masking the
  // sum drops them (FR-004).
  // The right shift runs on a signed value on purpose: an arithmetic
  // shift is the sign extension the recipe calls for, and no unsigned
  // spelling of it extends anything (FR-004).
  // NOLINTNEXTLINE(bugprone-signed-bitwise)
  const auto signExtended =
      static_cast<std::int64_t>(raw << (64U - width)) >> (64U - width);
  // The offset is unsigned and the extension signed, and the sum is cast
  // to the unsigned type the counter holds before anything compares it.
  // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
  value = static_cast<std::uint64_t>(signExtended + offset);
  // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
  SG_ENSURE(value == static_cast<std::uint64_t>(signExtended + offset),
            "an ok verdict leaves the offset plus the sign-extended "
            "instruction value (FR-004)");
  return FastReadVerdict::OK;
}

auto fastProbeAllows(const bool capabilityGranted,
                     const std::uint32_t index,
                     std::string& refusal) -> bool
{
  if (!capabilityGranted) {
    refusal = "the event page the kernel mapped publishes no cap_user_rdpmc "
              "bit, so this caller may not read counters from user space";
    return false;
  }
  if (!fastIndexValid(index)) {
    refusal = "the event page the kernel mapped publishes counter index "
        + std::to_string(index)
        + ", which names no counter the rdpmc instruction can read";
    return false;
  }
  refusal.clear();
  return true;
}

#if !defined(SG_PMU_FAST_X86)

std::unique_ptr<FastContext> fastContextOpen(const int,
                                             const std::uint64_t,
                                             const Target&,
                                             std::string* refusal)
{
  if (refusal != nullptr) {
    *refusal = "the host is not x86, so the mapped-page read does not apply";
  }
  return nullptr;
}

auto fastContextRead(const FastContext&, std::uint64_t&) -> FastReadVerdict
{
  return FastReadVerdict::NOT_ALLOWED;
}

auto fastContextTimePair(const FastContext&,
                         std::uint64_t&,
                         std::uint64_t&) -> bool
{
  return false;
}

void fastContextClose(FastContext&) {}

#else

namespace
{

// The page a `perf_event_open` descriptor maps, as the kernel declares
// it (include/uapi/linux/perf_event.h). The type is the kernel's, so
// every field below is read at the offset the running kernel's UAPI
// header fixes and no layout is mirrored here (R-011, Constitution I).
using EventPage = perf_event_mmap_page;

}  // namespace

std::unique_ptr<FastContext> fastContextOpen(const int type,
                                             const std::uint64_t config,
                                             const Target& where,
                                             std::string* refusal)
{
  const auto refuse =
      [refusal](std::string sentence) -> std::unique_ptr<FastContext>
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
  const auto [pid, cpu] = leaderPid(where);
  const long fd =
      kernelCalls().openEvent(&attr, pid, cpu, -1, PERF_FLAG_FD_CLOEXEC);
  // The scripted kernel stands a descriptor and a page in wherever the
  // test points the seam, so both arms of the open and the mapping
  // refusal carry their own counts over the fixtures
  // `counters_linux_pmu_seam_test.cpp` installs.
  if (fd < 0) {
    return refuse("perf_event_open was refused: "
                  + std::string(std::strerror(errno)));
  }
  auto context = std::make_unique<FastContext>();
  context->owner = std::this_thread::get_id();
  context->pinnedCpu = where.kind == TargetKind::CPU ? where.cpu : -1;
  context->fd = static_cast<int>(fd);
  context->mapLength = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  // The mapping is the kernel's own page type read at the offsets that
  // type declares, so no cast of a mirrored layout is involved and no P2
  // exception is claimed (Constitution I; plan Complexity Tracking).
  void* mapping = kernelCalls().mapPage(
      nullptr, context->mapLength, PROT_READ, MAP_SHARED, context->fd, 0);
  if (mapping == MAP_FAILED) {
    fastContextClose(*context);
    return refuse(
        "the event descriptor the kernel returned could not be " "mapped: "
        + std::string(std::strerror(errno)));
  }
  context->map = mapping;
  return context;
}

// The mapped-page read runs wherever a context stands, and the
// scripted kernel stands one in over the fixture page, so the read
// carries its own counts. The gates the body applies are covered for
// both arms by `fastDecode` and the `fastIndexValid` and
// `fastPairStable` seams in
// `test/source/counters_linux_pmu_seam_test.cpp`.
auto fastContextRead(const FastContext& context,
                     std::uint64_t& value) -> FastReadVerdict
{
  SG_REQUIRE(std::this_thread::get_id() == context.owner,
             "a mapped-page read runs on the thread that opened its "
             "context (FR-031, FR-040)");
  // The cpu-target pinning precondition, semantic-gated: a release build
  // configured `ignore` emits no check, and the per-sample cost stays
  // where FR-008 holds it. A thread-bound plan pins nothing and the
  // second conjunct is a compile-time-known-true compare against -1.
  SG_REQUIRE(fastPinningOk(context.pinnedCpu, ::sched_getcpu()),
             "a cpu-target fast-mode plan requires its sampling thread to "
             "run on the processor this plan opened its contexts on "
             "(FR-045)");
  const auto* page = static_cast<const EventPage*>(context.map);
  // The protocol the header publishes, in its order: snapshot the
  // sequence, take the payload and the instruction, compare the sequence
  // again (FR-040, R-011). Every gate below is decided by the page, so
  // `fastDecode` applies them and a caller the kernel refuses pays for
  // page loads only.
  const auto sequence = page->lock;
  _mm_lfence();
  const auto index = page->index;
  const auto offset = page->offset;
  const auto width = fastCounterWidth(page->pmc_width);
  const auto capability = static_cast<std::uint64_t>(page->cap_user_rdpmc);
  // A page that indexes nothing answers zero without the instruction,
  // the arm the zero-index fixture drives; the instruction itself runs
  // only where the processor maps the counter into the sampling
  // context, which no fixture page can stand in for, so that arm
  // carries the one marker the seam cannot reach (T140). The gates
  // around it are covered for both arms by `fastIndexValid` inside
  // `fastDecode`.
  // LCOV_EXCL_BR_START : coverage exclusion (T140): the arm that
  // withholds the instruction runs on a page that indexes nothing, and
  // the instruction itself reads the counter the index names; the read
  // faults on a host that never mapped the counter into this context,
  // so no fixture page and no refused host takes the arm. The gate the
  // decode applies is covered for both arms by `fastIndexValid` inside
  // `fastDecode`.
  std::uint64_t raw = 0;
  if (index == 0) {
    raw = 0ULL;
  } else {
    // LCOV_EXCL_START : coverage exclusion (T140): the instruction reads
    // the counter the index names, and the read faults on a host that
    // never mapped the counter into this sampling context, so no fixture
    // page and no refused host takes the arm. The statement is one line
    // and the pair of markers names it: the line marker the statement
    // carried before sat on the second line of a statement gcov
    // attributes to its first, so the gate saw the first line dark.
    raw = static_cast<std::uint64_t>(_rdpmc(static_cast<int>(index) - 1));
    // LCOV_EXCL_STOP
  }
  // LCOV_EXCL_BR_STOP
  _mm_lfence();
  return fastDecode(
      sequence, page->lock, index, capability, raw, offset, width, value);
}

// The pair read runs wherever a context stands, over the same fixture
// page the counter read uses; the stability gate the body applies is
// covered for both arms by `fastPairStable`.
auto fastContextTimePair(const FastContext& context,
                         std::uint64_t& enabled,
                         std::uint64_t& running) -> bool
{
  SG_REQUIRE(std::this_thread::get_id() == context.owner,
             "the enabled/running pair is read on the thread that opened "
             "its context (FR-031, FR-040)");
  SG_REQUIRE(fastPinningOk(context.pinnedCpu, ::sched_getcpu()),
             "a cpu-target fast-mode plan requires its sampling thread to "
             "run on the processor this plan opened its contexts on "
             "(FR-045)");
  const auto* page = static_cast<const EventPage*>(context.map);
  // The pair is payload of the same user-page update the counter value
  // rides, so it is read under the same seqlock snapshot (FR-041, R-011).
  const auto sequence = page->lock;
  _mm_lfence();
  const auto pageEnabled = page->time_enabled;
  const auto pageRunning = page->time_running;
  // The header reads the cycle counter, the scale, offset, and shift, the
  // page index, and the short-counter fields inside the sequence snapshot,
  // before the comparison that closes it (FR-007, FR-008). A field read
  // after that comparison can come from a later update.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
  const bool capUserTime = page->cap_user_time != 0;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
  const bool capUserTimeShort = page->cap_user_time_short != 0;
  const auto pageIndex = page->index;
  const auto timeShift = page->time_shift;
  const auto timeMult = page->time_mult;
  const auto timeOffset = page->time_offset;
  const auto timeCycles = page->time_cycles;
  const auto timeMask = page->time_mask;
  // The header samples the cycle counter only where the capability bit is
  // set and the enabled count differs from the running count.
  const auto cyc = (capUserTime && pageEnabled != pageRunning) ? __rdtsc() : 0U;
  _mm_lfence();
  // The arm that reports no pair needs the kernel to rewrite the page
  // between the two reads of its sequence, which a test cannot force
  // deterministically; the same comparison is covered for both arms by
  // `fastPairStable` (T140).
  // The arm needs the kernel to rewrite the page between the two reads
  // of its sequence, which a test cannot force deterministically; the
  // same comparison is covered for both arms by `fastPairStable` (T140).
  if (!fastPairStable(sequence, page->lock)) {  // LCOV_EXCL_BR_LINE
    return false;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_LINE
  // The kernel's own time recipe, applied to the fields this snapshot
  // holds (FR-007, FR-008, FR-009). The recipe is gated on the capability
  // bit and on an enabled count that differs from the running count, so a
  // page that names no time capability, or one whose counts already agree,
  // keeps the raw pair and the arithmetic below never runs.
  const EventTimeFields fields {
      .cap_user_time = capUserTime,
      .cap_user_time_short = capUserTimeShort,
      .time_enabled = pageEnabled,
      .time_running = pageRunning,
      .index = pageIndex,
      .time_shift = timeShift,
      .time_mult = timeMult,
      .time_offset = timeOffset,
      .time_cycles = timeCycles,
      .time_mask = timeMask,
      .cyc = cyc,
  };
  const auto pair = fastTimePair(fields);
  enabled = pair.enabled;
  running = pair.running;
  return true;
}

void fastContextClose(FastContext& context)
{
  // Both arms run on any host. `fast_context_lifetime_scenario` in
  // `test/source/counters_linux_pmu_seam_test.cpp` reaches them through
  // `~FastContext` with a descriptor and an anonymous mapping the test
  // opened itself, so no marker over this release stands and the coverage
  // gate measures it (T074, FR-027, FR-046).
  if (context.map != nullptr) {
    kernelCalls().unmapPage(context.map, context.mapLength);
    context.map = nullptr;
    context.mapLength = 0;
  }
  if (context.fd >= 0) {
    kernelCalls().closeFd(context.fd);
    context.fd = -1;
  }
}

#endif  // SG_PMU_FAST_X86

}  // namespace sg::counters::detail
