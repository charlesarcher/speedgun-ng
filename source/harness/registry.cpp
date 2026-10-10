#include <cstddef>
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

// The one append path of the family calls. The first list a family
// states sets its arity, and every later list carries that count
// (FR-006).
auto appendArguments(RegistryEntry& entry,
                     std::vector<std::int64_t> values) -> void
{
  SG_REQUIRE(entry.args.empty() || entry.args.front().size() == values.size(),
             "an argument list unequal to the family arity violates the bound "
             "list size == family arity (FR-006)");
  const std::size_t arity = values.size();
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
  return *this;
}

auto BenchmarkHandle::args(std::initializer_list<std::int64_t> values)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  appendArguments(*m_entry, std::vector<std::int64_t>(values));
  return *this;
}

auto BenchmarkHandle::args(std::vector<std::int64_t> values) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  appendArguments(*m_entry, std::move(values));
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
  return argsProduct(std::move(grown));
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
  return *this;
}

auto BenchmarkHandle::argsProduct(std::vector<std::vector<std::int64_t>> lists)
    -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  for (const auto& list : lists) {
    SG_REQUIRE(!list.empty(),
               "an argsProduct list with no argument violates the bound "
               "list size >= 1 (FR-006)");
  }

  // The odometer of the cited revision: the first list advances on
  // every combination, and a list that wraps carries the next.
  std::vector<std::size_t> position(lists.size(), 0);
  std::size_t combinations = 1;
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
  return *this;
}

auto BenchmarkHandle::argName(const std::string_view label) -> BenchmarkHandle&
{
  SG_REQUIRE(m_entry != nullptr && !m_entry->runStarted,
             "a family call runs before the run starts (E-02)");
  SG_REQUIRE(m_entry->args.empty() || m_entry->args.front().size() == 1,
             "an argName label on a family of another arity violates the bound "
             "label count == family arity (FR-006)");
  m_entry->argNames.emplace_back(label);
  SG_ENSURE(m_entry->argNames.back() == label,
            "the entry carries the label (FR-001)");
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
  const std::size_t labelCount = labels.size();
  m_entry->argNames = std::move(labels);
  SG_ENSURE(m_entry->argNames.size() == labelCount,
            "the entry carries the labels (FR-001)");
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

}  // namespace sg
