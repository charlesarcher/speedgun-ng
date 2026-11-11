// ============================================================================
// The source of one published counter limit, read beside the figure
// (specs/041-counter-probe T030; FR-040a, FR-043, FR-050, FR-053, D-3,
// D-3a; contracts/limit-source.md).
//
// `objectLimitSource` answers the word the provider recorded beside the
// limit it published. The recorder is the private seam of
// `source/counters/detail/limit_source_record.hpp`, reached through the
// relative include the way `counters_counter_limits_test.cpp` reaches
// `linux_pmu/counter_limits.hpp`. Every case scripts the record, so no
// case opens a device, runs a group probe, or reads a counter.
//
// The scripted world is one fake provider: two objects publish a limit
// and one publishes none. The scenarios that touch the record alone run
// before the first catalog read, which closes provider registration
// (FR-009), so they show the accessor answers with the catalog still
// shut. The last scenario reads the catalog and the record of the same
// paths and compares the two answers (FR-040a). The record is process
// state, so `resetFixture` clears it between scenarios. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <utility>

#include "speedgun-ng/counters.hpp"

#include "../source/counters/detail/limit_source_record.hpp"

namespace
{

using sg::counters::FakeProvider;
using sg::counters::LimitSource;
using sg::counters::objectLimitSource;
using sg::counters::System;
using sg::counters::detail::clearLimitSources;
using sg::counters::detail::recordLimitSource;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS LIMIT SOURCE TEST FAIL: %s\n", what);
  std::exit(EXIT_FAILURE);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The scripted world: two objects publish a scripted limit and one
// publishes none, the shape `counters_objects_limit_test.cpp` scripts
// (FR-005, FR-006). Registration happens once, ahead of every scenario.
auto registerEverything() -> void
{
  auto pmu = std::make_unique<FakeProvider>();
  pmu->addObject("cpu", "cpu", "performance core cluster");
  pmu->addCounter("cpu", "cycles", "ops", "core cycles elapsed");
  pmu->setProgrammableCounters("cpu", 4U);
  pmu->addObject("cpu_atom", "cpu", "efficient core cluster");
  pmu->addCounter("cpu_atom", "cycles", "ops", "core cycles elapsed");
  pmu->setProgrammableCounters("cpu_atom", 2U);
  pmu->addObject("uncore_imc_0", "imc", "memory controller");
  pmu->addCounter("uncore_imc_0", "bytes", "bytes", "DRAM bytes read");
  const auto registered = System::local().registerProvider(std::move(pmu));
  check(registered.has_value(), "the scripted provider registers (FR-009)");
}

// The record is process state beside the tree, so every scenario starts
// from the empty table.
auto resetFixture() -> void
{
  clearLimitSources();
}

// FR-043, D-3: the word of a decoded figure answers DECODE.
auto recordedDecodeScenario() -> void
{
  resetFixture();
  recordLimitSource("cpu", LimitSource::DECODE);
  check(objectLimitSource("cpu") == LimitSource::DECODE,
        "a path recorded DECODE answers DECODE (FR-043, D-3)");
}

// FR-043, D-3: the word of a probed figure answers PROBE.
auto recordedProbeScenario() -> void
{
  resetFixture();
  recordLimitSource("cpu", LimitSource::PROBE);
  check(objectLimitSource("cpu") == LimitSource::PROBE,
        "a path recorded PROBE answers PROBE (FR-043, D-3)");
}

// FR-040a: a path the writer never reached answers NONE.
auto unrecordedPathScenario() -> void
{
  resetFixture();
  check(objectLimitSource("cpu") == LimitSource::NONE,
        "a path no provider recorded answers NONE (FR-040a)");
  check(objectLimitSource("machine") == LimitSource::NONE,
        "the machine root answers NONE with no record of its own (FR-040a)");
  check(objectLimitSource("no_such_object") == LimitSource::NONE,
        "a path the catalog holds nothing for answers NONE (FR-040a)");
}

// FR-040a: the writer can be told a path publishes no limit, and the
// record of that path goes away with the figure.
auto recorderToldNoLimitScenario() -> void
{
  resetFixture();
  recordLimitSource("cpu", LimitSource::PROBE);
  check(objectLimitSource("cpu") == LimitSource::PROBE,
        "the scripted answer stands until the writer revises it (FR-043)");
  recordLimitSource("cpu", LimitSource::NONE);
  check(objectLimitSource("cpu") == LimitSource::NONE,
        "a path recorded NONE publishes no limit and answers NONE (FR-040a)");
}

// FR-040a, D-3a: the seam clears the table, and nothing of the old
// records survives it.
auto clearLeavesNoRecordScenario() -> void
{
  resetFixture();
  recordLimitSource("cpu", LimitSource::DECODE);
  recordLimitSource("cpu_atom", LimitSource::PROBE);
  clearLimitSources();
  check(objectLimitSource("cpu") == LimitSource::NONE,
        "a cleared path answers NONE (FR-040a)");
  check(objectLimitSource("cpu_atom") == LimitSource::NONE,
        "a second cleared path answers NONE (FR-040a)");
  recordLimitSource("cpu", LimitSource::PROBE);
  check(objectLimitSource("cpu") == LimitSource::PROBE,
        "a record written after the clear answers its own source (FR-043)");
  check(objectLimitSource("cpu_atom") == LimitSource::NONE,
        "the clear left no record behind for the second path (FR-040a)");
}

// The provider the registration refuses: it records a source for two
// paths while it enumerates, the way the Linux PMU provider records
// beside the limit it fills, then seeds a path the catalog already holds
// (FR-008). It is never opened.
class RefusedProvider final : public sg::counters::ProviderIface
{
public:
  void enumerate(sg::counters::ObjectSink& sink) const override
  {
    recordLimitSource("cpu", LimitSource::PROBE);
    recordLimitSource("uncore_imc_0", LimitSource::PROBE);
    sink.addObject(sg::counters::ObjectSeed {.kind = "imc",
                                             .path = "uncore_imc_0",
                                             .alias = "",
                                             .description = "a second imc",
                                             .entries = {},
                                             .programmableCounters = 3U});
  }

  auto open(const sg::counters::LeafSet& /*leaves*/,
            const sg::counters::Target& /*where*/)
      -> std::unique_ptr<sg::counters::WindowReader> override
  {
    fail("a refused provider is never opened (FR-008)");
  }
};

// T029, S-2: a refused provider leaves no source behind. The records it
// wrote while it enumerated go back with its batch, so a path whose limit
// the batch never staged answers NONE, and a record standing ahead of
// the refusal keeps its word (FR-008, FR-040a). The registration must
// run with the catalog still shut, so this scenario stands ahead of the
// first catalog read.
auto refusedProviderLeavesNoSourceScenario() -> void
{
  resetFixture();
  recordLimitSource("cpu", LimitSource::DECODE);
  const auto refused =
      System::local().registerProvider(std::make_unique<RefusedProvider>());
  check(!refused.has_value(),
        "a provider seeding a held path is refused (FR-008)");
  check(objectLimitSource("uncore_imc_0") == LimitSource::NONE,
        "a refused provider leaves no source on a path it never published "
        "(FR-040a)");
  check(objectLimitSource("cpu") == LimitSource::DECODE,
        "a record standing ahead of the refusal keeps its word (FR-043)");
}

// FR-040a: the accessor and the published limit agree on one path. The
// scripted writer follows the rule the provider follows: it records a
// source beside a figure it published, and records nothing where it
// published nothing (FR-043). This scenario reads the catalog, so it
// stands last.
auto publishedLimitAndSourceAgreeScenario() -> void
{
  resetFixture();
  const auto cpu = System::local().object("cpu");
  check(cpu.has_value(), "the catalog holds the scripted cpu object (FR-002)");
  const auto imc = System::local().object("uncore_imc_0");
  check(imc.has_value(), "the catalog holds the scripted imc object (FR-002)");

  recordLimitSource("cpu", LimitSource::PROBE);
  const auto cpuLimit = cpu->programmableCounters();
  check(cpuLimit.has_value(),
        "the scripted cpu object publishes a limit (FR-005)");
  check(objectLimitSource("cpu") == LimitSource::PROBE,
        "a path with a published limit answers its recorded source (FR-040a)");

  const auto imcLimit = imc->programmableCounters();
  check(imcLimit == std::nullopt,
        "the scripted imc object publishes no limit (FR-005)");
  check(objectLimitSource("uncore_imc_0") == LimitSource::NONE,
        "the accessor answers NONE where the limit answers no value (FR-040a)");
}

}  // namespace

auto main() -> int
{
  registerEverything();
  recordedDecodeScenario();
  recordedProbeScenario();
  unrecordedPathScenario();
  recorderToldNoLimitScenario();
  clearLeavesNoRecordScenario();
  refusedProviderLeavesNoSourceScenario();
  publishedLimitAndSourceAgreeScenario();
  std::printf("counters_limit_source_test: PASS\n");
  return 0;
}
