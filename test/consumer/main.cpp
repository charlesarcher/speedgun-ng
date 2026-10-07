// Downstream consumer test project (FR-026, SC-005, SC-009).
// Driven by the downstream-consumer CI job: install speedgun-ng to a
// prefix, configure, build, and run this program against it. This
// file must never name the vendored topology library; that absence
// is the point.
//
// It uses the public API exactly as the README instructs: the
// exported class from <speedgun-ng/speedgun-ng.hpp> and the
// simulationStart marker from the trace-marker header. Constructing
// the class and calling name() pulls a real symbol out of the
// library, and the marker call resolves the trace-marker header from
// the install tree, so the link and that header's reachability are
// exercised on every machine.
//
// It then registers the PMU provider through the installed headers and
// prints the catalog the linked package resolves (FR-023, SC-008). The
// downstream job compares that count against the build tree's, so a
// package that lost its event tables on the way to the prefix cannot
// pass. The count comes from the catalog the library publishes, so the
// program stays a consumer: it names no path and no table.

#include <cstdio>
#include <memory>

#include <speedgun-ng/counters_pmu.hpp>
#include <speedgun-ng/counters_system.hpp>
#include <speedgun-ng/simulation.hpp>
#include <speedgun-ng/speedgun-ng.hpp>

auto main() -> int
{
  ExportedClass const library;
  sg::simulationStart();
  if (library.name() == nullptr) {
    return 1;
  }

  auto provider = std::make_unique<sg::counters::PmuProvider>();
  if (!sg::counters::System::local()
           .registerProvider(std::move(provider))
           .has_value())
  {
    std::puts("consumer: the pmu provider does not register");
    return 1;
  }

  // The event tables reach a catalog entry, so the row count the linked
  // package resolves is the count the pmu objects publish.
  const auto pmu_objects = sg::counters::System::local().objects("pmu");
  if (!pmu_objects.has_value()) {
    std::puts("consumer: no pmu objects");
    return 1;
  }

  std::size_t entries = 0;
  for (const auto* object : *pmu_objects) {
    entries += object->counters().size();
  }
  std::printf("consumer: pmu catalog entries %zu\n", entries);
  return 0;
}
