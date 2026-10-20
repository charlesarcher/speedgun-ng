#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{

auto registry() -> std::vector<std::unique_ptr<RegistryEntry>>&
{
  static std::vector<std::unique_ptr<RegistryEntry>> entries;
  return entries;
}

namespace
{

// The process default unit of FR-016. A function-local static carries
// it, because `SG_BENCHMARK` registers during static initialization and
// a namespace-scope object here would sit behind an initialization
// order that registration can reach first.
auto defaultTimeUnit() -> TimeUnit&
{
  static TimeUnit unit = TimeUnit::NANOSECONDS;
  return unit;
}

}  // namespace

auto addRegistryEntry(std::function<void(State&)> fn, std::string_view name)
    -> std::expected<RegistryEntry*, std::string>
{
  for (const auto& entry : registry()) {
    if (entry->name == name) {
      return std::unexpected("benchmark '" + std::string(name)
                             + "' is registered twice; the first registration "
                               "stays and the rest of the suite still runs "
                               "(FR-002)");
    }
  }

  auto entry = std::make_unique<RegistryEntry>();
  entry->name = name;
  entry->callable = std::move(fn);
  auto* stored = entry.get();
  registry().push_back(std::move(entry));
  return stored;
}

BenchmarkHandle::BenchmarkHandle(RegistryEntry& entry) noexcept
    : m_entry(&entry)
{
}

auto BenchmarkHandle::name() const noexcept -> std::string_view
{
  if (m_entry == nullptr) {
    return {};
  }
  return m_entry->name;
}

auto BenchmarkHandle::minTime(const std::int64_t nanoseconds)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->minTimeNs = nanoseconds;
  SG_ENSURE(m_entry->minTimeNs.value() == nanoseconds,
            "the entry carries the option (FR-015)");
  return *this;
}

auto BenchmarkHandle::warmupTime(const std::int64_t nanoseconds)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->warmupTimeNs = nanoseconds;
  SG_ENSURE(m_entry->warmupTimeNs.value() == nanoseconds,
            "the entry carries the option (FR-015)");
  return *this;
}

auto BenchmarkHandle::repetitions(const std::uint64_t count) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->repetitions = count;
  SG_ENSURE(m_entry->repetitions.value() == count,
            "the entry carries the option (FR-015)");
  return *this;
}

auto BenchmarkHandle::iterations(const std::uint64_t count) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->fixedIterations = count;
  SG_ENSURE(m_entry->fixedIterations.value() == count,
            "the entry carries the option (FR-013)");
  return *this;
}

auto BenchmarkHandle::measureProcessCpuTime() -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->processCpuTime = true;
  SG_ENSURE(m_entry->processCpuTime,
            "the entry selects the process CPU leaf (FR-004)");
  return *this;
}

auto BenchmarkHandle::useRealTime() -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  SG_REQUIRE(!m_entry->manualTime,
             "a real-time selection beside a manual-time selection "
             "violates the bound real time and manual time exclude each "
             "other (FR-008)");
  m_entry->realTime = true;
  SG_ENSURE(m_entry->realTime,
            "the entry makes the real column the primary time (FR-005)");
  return *this;
}

auto BenchmarkHandle::useManualTime() -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  SG_REQUIRE(!m_entry->realTime,
             "a manual-time selection beside a real-time selection "
             "violates the bound real time and manual time exclude each "
             "other (FR-008)");
  m_entry->manualTime = true;
  SG_ENSURE(m_entry->manualTime,
            "the entry makes the manual sum the primary time (FR-006)");
  return *this;
}

auto BenchmarkHandle::unit(const TimeUnit timeUnit) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->timeUnit = timeUnit;
  SG_ENSURE(m_entry->timeUnit == timeUnit,
            "the entry carries the unit of that family (FR-015)");
  return *this;
}

auto State::setIterationTime(const std::int64_t nanoseconds) -> void
{
  SG_REQUIRE(!m_callbackState,
             "setIterationTime in a callback state violates the callback-state "
             "rule that the callback state owns no run to accumulate for "
             "(R-05)");
  SG_REQUIRE(nanoseconds >= 0,
             "a negative iteration time violates the bound that an accepted "
             "duration carries at least zero nanoseconds (FR-007)");
  [[maybe_unused]] const std::int64_t before = m_manualNs;
  m_manualNs += nanoseconds;
  SG_ENSURE(m_manualNs == before + nanoseconds,
            "the manual accumulator carries the sum of every accepted value "
            "(FR-007)");
}

auto State::setComplexityN(const std::int64_t problemSize) -> void
{
  m_complexityN = problemSize;
  SG_ENSURE(m_complexityN.has_value() && *m_complexityN == problemSize,
            "the run carries the problem size the body stated (FR-019)");
}

auto BenchmarkHandle::addMetricCore(const detail::MetricSeed& seed)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a metric is attached before the run starts (E-02)");
  m_entry->metrics.push_back(seed);
  SG_ENSURE(m_entry->metrics.back().label == seed.label,
            "the entry carries the metric (FR-021)");
  return *this;
}

namespace
{

// The aggregate names every family prints (FR-013): a custom statistic may
// not take one of them, because the report prints the built-in fields and
// the attached ones in one row and a shared name would leave the reader
// without one value per name. The helper stands only in a contract, so a
// configuration with the runtime contracts compiled out holds no call.
[[maybe_unused]] auto isBuiltInStatisticName(const std::string_view name)
    -> bool
{
  for (const std::string_view builtIn : {std::string_view {"mean"},
                                         std::string_view {"median"},
                                         std::string_view {"sd"},
                                         std::string_view {"cv"},
                                         std::string_view {"min"},
                                         std::string_view {"max"},
                                         std::string_view {"n"}})
  {
    if (builtIn == name) {
      return true;
    }
  }
  return false;
}

}  // namespace

auto BenchmarkHandle::addStatistic(const std::string_view name,
                                   const StatUnit unit,
                                   const StatisticFunc& function)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a statistic is attached before the run starts (E-02)");
  SG_REQUIRE(!isBuiltInStatisticName(name),
             "a custom statistic named after a built-in aggregate name "
             "violates the rule that mean, median, sd, cv, min, max and n "
             "stay reserved for the built-in aggregates (FR-013)");
  m_entry->statistics.push_back(
      detail::StatisticSeed {std::string(name), unit, function});
  SG_ENSURE(m_entry->statistics.back().name == name,
            "the entry carries the statistic (FR-013)");
  return *this;
}

auto BenchmarkHandle::reportAggregatesOnly() -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->reportAggregatesOnly = true;
  SG_ENSURE(m_entry->reportAggregatesOnly,
            "the entry carries the report mode of that family (FR-016)");
  return *this;
}

auto BenchmarkHandle::displayAggregatesOnly() -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->displayAggregatesOnly = true;
  SG_ENSURE(m_entry->displayAggregatesOnly,
            "the entry carries the display mode of that family (FR-017)");
  return *this;
}

auto BenchmarkHandle::showPercentiles(const bool show) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->showPercentiles = show;
  SG_ENSURE(m_entry->showPercentiles == show,
            "the entry carries the percentile selection of that family "
            "(specs/021 FR-019)");
  return *this;
}

auto BenchmarkHandle::complexity(const BigO bigO) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->bigO = bigO;
  m_entry->bigOFun = {};
  SG_ENSURE(m_entry->bigO == bigO,
            "the entry carries the curve selection of that family (FR-019)");
  return *this;
}

auto BenchmarkHandle::complexity(BigOFun function) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "an option setter runs before the run starts (E-02)");
  m_entry->bigO = BigO::NONE;
  m_entry->bigOFun = std::move(function);
  SG_ENSURE(static_cast<bool>(m_entry->bigOFun),
            "the entry carries the curve function of that family (FR-020)");
  return *this;
}

namespace
{

// The one append path of the family calls. The first list a family
// states sets its arity, and every later list carries that count
// (FR-006).
auto appendArguments(RegistryEntry& entry,
                     std::vector<std::int64_t> values) -> void
{
  SG_REQUIRE(entry.args.empty() || entry.args.front().size() == values.size(),
             "an argument list unequal to the family arity violates the bound "
             "list size == family arity (FR-006)");
  [[maybe_unused]] const std::size_t arity = values.size();
  entry.args.push_back(std::move(values));
  SG_ENSURE(entry.args.back().size() == arity,
            "the entry carries the appended argument list (FR-001)");
}

}  // namespace

auto BenchmarkHandle::applyGuard() -> void
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
}

auto BenchmarkHandle::arg(const std::int64_t value) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  appendArguments(*m_entry, {value});
  SG_ENSURE(m_entry->args.back().front() == value,
            "the entry carries the appended argument (FR-001)");
  return *this;
}

auto BenchmarkHandle::args(std::initializer_list<std::int64_t> values)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  appendArguments(*m_entry, std::vector<std::int64_t>(values));
  SG_ENSURE(m_entry->args.back().size() == values.size(),
            "the entry carries the appended argument list (FR-001)");
  return *this;
}

auto BenchmarkHandle::args(std::vector<std::int64_t> values) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  [[maybe_unused]] const std::size_t arity = values.size();
  appendArguments(*m_entry, std::move(values));
  SG_ENSURE(m_entry->args.back().size() == arity,
            "the entry carries the appended argument list (FR-001)");
  return *this;
}

auto BenchmarkHandle::range(const std::int64_t low,
                            const std::int64_t high) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  SG_REQUIRE(low <= high,
             "a range low bound above its high bound violates the bound "
             "low <= high (FR-006)");
  for (const std::int64_t value :
       createRange(low, high, m_entry->rangeMultiplier))
  {
    appendArguments(*m_entry, {value});
  }
  SG_ENSURE(m_entry->args.back().front() == high,
            "the entry carries the grown range (FR-004)");
  return *this;
}

auto BenchmarkHandle::rangeMultiplier(const std::int64_t multiplier)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  SG_REQUIRE(multiplier >= 2,
             "a range multiplier below 2 violates the bound multiplier >= 2 "
             "(FR-006)");
  m_entry->rangeMultiplier = multiplier;
  SG_ENSURE(m_entry->rangeMultiplier == multiplier,
            "the entry carries the multiplier (FR-004)");
  return *this;
}

auto BenchmarkHandle::ranges(std::vector<std::pair<std::int64_t, std::int64_t>>
                                 bounds) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  std::vector<std::vector<std::int64_t>> grown;
  grown.reserve(bounds.size());
  for (const auto& bound : bounds) {
    SG_REQUIRE(bound.first <= bound.second,
               "a range low bound above its high bound violates the bound "
               "low <= high (FR-006)");
    grown.push_back(
        createRange(bound.first, bound.second, m_entry->rangeMultiplier));
  }
  [[maybe_unused]] const std::size_t before = m_entry->args.size();
  argsProduct(std::move(grown));
  SG_ENSURE(m_entry->args.size() > before,
            "the entry carries the product of the grown pairs (FR-004)");
  return *this;
}

auto BenchmarkHandle::denseRange(const std::int64_t low,
                                 const std::int64_t high,
                                 const std::int64_t step) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  SG_REQUIRE(low <= high,
             "a range low bound above its high bound violates the bound "
             "low <= high (FR-006)");
  SG_REQUIRE(
      step >= 1,
      "a denseRange step below 1 violates the bound step >= 1 " "(FR-006)");
  for (const std::int64_t value : createDenseRange(low, high, step)) {
    appendArguments(*m_entry, {value});
  }
  SG_ENSURE(m_entry->args.back().front() >= low
                && m_entry->args.back().front() <= high,
            "the entry carries the stepped range (FR-005)");
  return *this;
}

auto BenchmarkHandle::argsProduct(std::vector<std::vector<std::int64_t>> lists)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  for ([[maybe_unused]] const auto& list : lists) {
    SG_REQUIRE(!list.empty(),
               "an argsProduct list with no argument violates the bound "
               "list size >= 1 (FR-006)");
  }

  // The odometer of the cited revision: the first list advances on
  // every combination, and a list that wraps carries the next.
  std::vector<std::size_t> position(lists.size(), 0);
  std::size_t combinations = 1;
  [[maybe_unused]] const std::size_t before = m_entry->args.size();
  for (const auto& list : lists) {
    combinations *= list.size();
  }
  for (std::size_t combination = 0; combination < combinations; ++combination) {
    std::vector<std::int64_t> tuple;
    tuple.reserve(lists.size());
    for (std::size_t slot = 0; slot < lists.size(); ++slot) {
      tuple.push_back(lists[slot][position[slot]]);
    }
    appendArguments(*m_entry, std::move(tuple));

    for (std::size_t slot = 0; slot < lists.size(); ++slot) {
      position[slot] = (position[slot] + 1) % lists[slot].size();
      if (position[slot] != 0) {
        break;
      }
    }
  }
  SG_ENSURE(m_entry->args.size() == before + combinations,
            "the entry carries the product of the lists (FR-001)");
  return *this;
}

auto BenchmarkHandle::argName(const std::string_view label) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  SG_REQUIRE(m_entry->args.empty() || m_entry->args.front().size() == 1,
             "an argName label on a family of another arity violates the bound "
             "label count == family arity (FR-006)");
  m_entry->argNames = {std::string(label)};
  SG_ENSURE(m_entry->argNames.size() == 1 && m_entry->argNames.front() == label,
            "the entry carries the one label (FR-001)");
  SG_INVARIANT(
      m_entry->argNames.size() == 1 && m_entry->argNames.front() == label,
      "the label list is the one label this call set (FR-006)");
  return *this;
}

auto BenchmarkHandle::argNames(std::vector<std::string> labels)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  SG_REQUIRE(m_entry->args.empty() ||
                 m_entry->args.front().size() == labels.size(),
             "an argNames count unequal to the family arity violates the bound "
             "label count == family arity (FR-006)");
  [[maybe_unused]] const std::size_t labelCount = labels.size();
  m_entry->argNames = std::move(labels);
  SG_ENSURE(m_entry->argNames.size() == labelCount,
            "the entry carries the labels (FR-001)");
  SG_INVARIANT(m_entry->argNames.size() == labelCount,
               "the label list is the list this call set (FR-006)");
  return *this;
}

auto BenchmarkHandle::setup(std::function<void(State&)> callback)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a callback attaches before the run starts (E-02)");
  m_entry->setup = std::move(callback);
  SG_ENSURE(
      static_cast<bool>(m_entry->setup),
      "the entry's setup slot carries the attached callback (FR-018, " "R-10)");
  return *this;
}

auto BenchmarkHandle::teardown(std::function<void(State&)> callback)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a callback attaches before the run starts (E-02)");
  m_entry->teardown = std::move(callback);
  SG_ENSURE(static_cast<bool>(m_entry->teardown),
            "the entry's teardown slot carries the attached callback (FR-018, "
            "R-10)");
  return *this;
}

auto BenchmarkHandle::threads(const int count) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a thread call runs before the run starts (E-02)");
  SG_REQUIRE(count >= 1,
             "a thread count below one violates the bound that every thread "
             "count stands at 1 or above (FR-016, FR-024)");
  m_entry->threadCounts = {count};
  SG_ENSURE(m_entry->threadCounts.size() == 1 &&
                m_entry->threadCounts.front() == count,
            "the entry carries the single count in place of any set an "
            "earlier thread call stated (FR-016)");
  return *this;
}

// specs/022 FR-015: the family's preset name, stored as stated and read
// by the expansion, which parses it as it copies it (T013).
auto BenchmarkHandle::placement(const std::string_view presetName)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a placement call runs before the run starts (E-02)");
  m_entry->placement = std::string(presetName);
  SG_ENSURE(m_entry->placement.has_value()
                && *m_entry->placement == presetName,
            "the entry carries the stated name in place of any name an "
            "earlier placement call stated (FR-015)");
  return *this;
}

namespace detail
{

// FR-016, FR-024: the doubling of the cited ThreadRange. The bound test
// stands in the loop condition, so bounds that stand out of order, and a
// lower bound below one, describe the empty set here: where contracts stand
// the require of the call answers that mistake, and this is the account a
// build without contracts reaches. `std::int64_t` holds the doubled value past
// the bound, so no multiplication overflows.
auto buildThreadCounts(const int minCount,
                       const int maxCount) -> std::vector<int>
{
  std::vector<int> counts;
  for (std::int64_t count = minCount; count >= 1 && count <= maxCount;
       count *= 2)
  {
    counts.push_back(static_cast<int>(count));
  }
  // A `return counts;` would make `counts` the return object itself, and gcov
  // would then attach an unexecutable count-zero record to the closing brace.
  // Returning a separate object leaves the destructor of `counts` to run at the
  // brace, which every call executes (the pattern createRange states in
  // family.cpp).
  return std::vector<int>(counts);
}

// FR-016, FR-024: the stride of the cited DenseThreadRange, with the same
// bound account as buildThreadCounts. A step below one would repeat one
// count forever, so it stands in the condition and names the empty set
// (FR-024).
auto buildDenseThreadCounts(const int minCount,
                            const int maxCount,
                            const int step) -> std::vector<int>
{
  std::vector<int> counts;
  for (std::int64_t count = minCount;
       step >= 1 && count >= 1 && count <= maxCount;
       count += step)
  {
    counts.push_back(static_cast<int>(count));
  }
  // The separate return object keeps gcov's count-zero record off the brace
  // below, for the reason stated at buildThreadCounts.
  return std::vector<int>(counts);
}

}  // namespace detail

auto BenchmarkHandle::threadRange(const int minCount,
                                  const int maxCount) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a thread call runs before the run starts (E-02)");
  SG_REQUIRE(minCount >= 1 && minCount <= maxCount,
             "a thread range whose bounds stand below one or whose lower "
             "bound stands above the upper violates the bounds of FR-024 "
             "(FR-016, FR-024)");
  m_entry->threadCounts = detail::buildThreadCounts(minCount, maxCount);
  SG_ENSURE(!m_entry->threadCounts.empty() &&
                m_entry->threadCounts.front() == minCount,
            "the entry carries one thread count per doubling in the bounds, "
            "in place of any set an earlier thread call stated (FR-016)");
  return *this;
}

auto BenchmarkHandle::denseThreadRange(const int minCount,
                                       const int maxCount,
                                       const int step) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a thread call runs before the run starts (E-02)");
  SG_REQUIRE(minCount >= 1 && minCount <= maxCount && step >= 1,
             "a dense thread range whose bounds stand below one, whose "
             "lower bound stands above the upper, or whose step stands "
             "below one violates the bounds of FR-024 (FR-016, FR-024)");
  m_entry->threadCounts =
      detail::buildDenseThreadCounts(minCount, maxCount, step);
  SG_ENSURE(!m_entry->threadCounts.empty() &&
                m_entry->threadCounts.front() == minCount,
            "the entry carries one thread count per step in the bounds, in "
            "place of any set an earlier thread call stated (FR-016)");
  return *this;
}

auto BenchmarkHandle::threadRunner(ThreadRunnerFn runner) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a thread call runs before the run starts (E-02)");
  SG_REQUIRE(static_cast<bool>(runner),
             "an empty runner is not callable with a ThreadRun, violating "
             "the callable rule of the thread runner (FR-021)");
  m_entry->threadRunner = std::move(runner);
  SG_ENSURE(static_cast<bool>(m_entry->threadRunner),
            "the entry's runner slot carries the attached runner in place "
            "of any runner an earlier call attached (FR-021, R-10)");
  return *this;
}

auto registerBenchmark(std::function<void(State&)> fn,
                       std::string_view name) -> BenchmarkHandle
{
  SG_REQUIRE(static_cast<bool>(fn),
             "a registration carries a callable (FR-001)");

  auto added = addRegistryEntry(std::move(fn), name);
  if (!added.has_value()) {
    std::fprintf(stderr, "%s\n", added.error().c_str());
    return {};
  }

  SG_ENSURE(registry().back()->name == name,
            "the name is registered in registration order (FR-003)");
  return BenchmarkHandle(**added);
}

auto registerFixtureBenchmark(std::function<void(State&)> fn,
                              std::string_view name,
                              std::function<std::unique_ptr<Fixture>()> factory)
    -> BenchmarkHandle
{
  SG_REQUIRE(static_cast<bool>(fn),
             "a registration carries a callable (FR-001)");
  SG_REQUIRE(static_cast<bool>(factory),
             "a fixture registration carries a fixture factory (FR-017, R-09)");

  auto added = addRegistryEntry(std::move(fn), name);
  if (!added.has_value()) {
    std::fprintf(stderr, "%s\n", added.error().c_str());
    return {};
  }
  (*added)->fixtureFactory = std::move(factory);

  SG_ENSURE(registry().back()->fixtureFactory != nullptr,
            "the entry carries the fixture factory (FR-017)");
  return BenchmarkHandle(**added);
}

auto setDefaultTimeUnit(const TimeUnit timeUnit) -> void
{
  defaultTimeUnit() = timeUnit;
  SG_ENSURE(defaultTimeUnit() == timeUnit,
            "the library reads back the default it was given (FR-016)");
}

auto getDefaultTimeUnit() -> TimeUnit
{
  const TimeUnit current = defaultTimeUnit();
  SG_ENSURE(current == defaultTimeUnit(),
            "the read yields the process default of this moment (FR-016)");
  return current;
}

}  // namespace sg
