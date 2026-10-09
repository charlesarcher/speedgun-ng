#include <cstdio>
#include <memory>
#include <string>
#include <utility>

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

}  // namespace sg
