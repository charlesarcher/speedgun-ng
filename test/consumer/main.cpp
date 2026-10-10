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
// It runs one benchmark through the harness entry point of the installed
// package (SC-010, D-1), so the link of speedgun-ng::harness, the
// registration macro, and the report all resolve downstream. The entry
// point registers the clock and pmu providers (R-03), so the consumer
// names no provider of its own.
//
// It then prints the catalog the linked package resolves (FR-023,
// SC-008). The downstream job compares that count against the build
// tree's, so a package that lost its event tables on the way to the
// prefix cannot pass. The count comes from the catalog the library
// publishes, so the program stays a consumer: it names no path and no
// table.

#include <cstdio>

#include <speedgun-ng/benchmark.hpp>
#include <speedgun-ng/counters_system.hpp>
#include <speedgun-ng/simulation.hpp>
#include <speedgun-ng/speedgun-ng.hpp>

// One benchmark registered through the harness macro, so the entry point
// has something to run (SC-010, D-1).
auto bmConsumer(sg::State& state) -> void
{
  double accumulator = 1.0;
  for (auto _ : state) {
    accumulator += accumulator * 1.000000001;
    if (accumulator < 0.0) {
      std::puts("consumer: the measured work went negative");
    }
  }
}

SG_BENCHMARK(bmConsumer);

auto main(int argc, char** argv) -> int
{
  ExportedClass const library;
  sg::simulationStart();
  if (library.name() == nullptr) {
    return 1;
  }

  // The entry point of the installed package: the context lines, one
  // measured row, and the run's exit status (SC-010).
  const int status = sg::speedgunMain(argc, argv);

  // The event tables reach a catalog entry, so the row count the linked
  // package resolves is the count the pmu objects publish.
  const auto pmuObjects = sg::counters::System::local().objects("pmu");
  if (!pmuObjects.has_value()) {
    std::puts("consumer: no pmu objects");
    return status;
  }

  std::size_t entries = 0;
  for (const auto* object : *pmuObjects) {
    entries += object->counters().size();
  }
  std::printf("consumer: pmu catalog entries %zu\n", entries);
  return status;
}
