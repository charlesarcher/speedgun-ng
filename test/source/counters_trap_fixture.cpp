// ============================================================================
// Trap fixture for counters misuse (T023): proves contract aborts.
//
// NOT registered as a ctest (it aborts). It is driven by:
//   - counters_trap_checked_test (checked builds, default dev = enforce)
//   - the consumer-release CI job (ignore builds)
//
// Behavior per mode (argv[1]):
//   - metric-before-finish, fold-range, fold-out-of-extent,
//     push-cross-thread, push-decrement, push-foreign-sample,
//     push-mixed-owner, recorder-cross-thread, and scope-cross-thread
//     are semantic-gated sites: they abort in checked builds, so their
//     markers stay absent; under ignore they survive and print their markers.
//   - overrun is an SG_REQUIRE_ALWAYS site (FR-027 memory safety is
//     never semantic-gated): it aborts in EVERY configuration, marker
//     absent everywhere.
// A marker "counters-trap-survived-<mode>" printed after the violating
// call means the violation was NOT caught.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>

#include "speedgun-ng/counters.hpp"

namespace
{

using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::FakeProvider;
using sg::counters::PushCounter;
using sg::counters::PushProvider;
using sg::counters::System;

using Events = Dim<0, 1>;

auto setup() -> sg::counters::Plan
{
  auto provider = std::make_unique<FakeProvider>();
  provider->addObject("package-1/core-3", "cpu3", "core", "third core");
  provider->addCounter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->addCounter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  provider->setPoints("package-1/core-3", "cycles", {100, 300}, 200);
  provider->setPoints("package-1/core-3", "instructions", {1000, 3100}, 2100);
  const auto registered = System::local().registerProvider(std::move(provider));
  if (!registered.has_value()) {
    std::fprintf(stderr, "fixture: provider registration failed\n");
    std::exit(2);
  }
  const auto core = *System::local().object("package-1/core-3");
  const auto cycles = *core.counter<Events>("cycles");
  const auto instructions = *core.counter<Events>("instructions");
  auto compiled = compile(System::local(), instructions / cycles);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "fixture: plan compile failed\n");
    std::exit(2);
  }
  return std::move(*compiled);
}

auto survived(std::string_view mode) -> void
{
  std::printf("counters-trap-survived-%.*s\n",
              static_cast<int>(mode.size()),
              mode.data());
  std::fflush(stdout);
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::string_view mode =
      argc >= 2 ? std::string_view {argv[1]} : std::string_view {};

  if (mode == "push-cross-thread" || mode == "push-decrement") {
    auto push = std::make_unique<PushProvider>();
    auto handle = push->addCounter("bytes", "bytes", "hot-path bytes");
    const auto registered = System::local().registerProvider(std::move(push));
    if (!registered.has_value()) {
      std::fprintf(stderr, "fixture: push provider registration failed\n");
      std::exit(2);
    }
    if (mode == "push-cross-thread") {
      std::thread foreign {[&handle] { handle.add(1); }};
      foreign.join();
      survived(mode);
      return 0;
    }
    const auto machine = *System::local().object("machine");
    const auto bytes = *machine.counter<Events>("bytes");
    const Expression<Events> counted {bytes};
    const auto plan = compile(System::local(), counted);
    if (!plan.has_value()) {
      std::fprintf(stderr, "fixture: push plan compile failed\n");
      std::exit(2);
    }
    auto rec = plan->recorder(4);
    rec.sample();
    handle.add(1500);
    rec.sample();
    // Unsigned wrap back below the previous point: a fold-time
    // decrement, tier-3 misuse (FR-035).
    handle.add(0ULL - 1400ULL);
    rec.sample();
    static_cast<void>(counted.fold(rec.view(), 1, 2));
    survived(mode);
    return 0;
  }

  if (mode == "push-mixed-owner") {
    // Two push counters, one declared per thread, in one window. The
    // expression names the main thread's counter last, so the leaf set
    // ends on an owner the sampling thread holds, which is the order
    // under which a plan holding a foreign counter passed the window's
    // one-owner guard (FR-035).
    auto push = std::make_unique<PushProvider>();
    auto mainBytes = push->addCounter("main-bytes", "ops", "main bytes");
    auto workerBytes = mainBytes;
    std::thread worker {[&push, &workerBytes]
                        {
                          workerBytes = push->addCounter(
                              "worker-bytes", "ops", "worker bytes");
                          workerBytes.add(10);
                        }};
    worker.join();
    const auto registered = System::local().registerProvider(std::move(push));
    if (!registered.has_value()) {
      std::fprintf(stderr, "fixture: push provider registration failed\n");
      std::exit(2);
    }
    const auto machine = *System::local().object("machine");
    const auto onMain = *machine.counter<Events>("main-bytes");
    const auto onWorker = *machine.counter<Events>("worker-bytes");
    const Expression<Events> mixed {onWorker + onMain};
    const auto plan = compile(System::local(), mixed);
    if (!plan.has_value()) {
      std::fprintf(stderr, "fixture: push plan compile failed\n");
      std::exit(2);
    }
    auto rec = plan->recorder(4);
    rec.sample();
    rec.sample();
    survived(mode);
    return 0;
  }

  if (mode == "push-foreign-sample") {
    // One push counter, declared on a worker thread and sampled by the
    // thread that compiled the plan. The leaf set names a single owner,
    // so the one-owner window guard at `PushProvider.cpp:120` admits it
    // and the violation reaches the sampling-side guard at `:40`, which
    // is the only site that detects it (FR-035, FR-031).
    auto push = std::make_unique<PushProvider>();
    std::thread worker {[&push]
                        {
                          auto workerOps = push->addCounter(
                              "worker-ops", "ops", "worker ops");
                          workerOps.add(10);
                        }};
    worker.join();
    const auto registered = System::local().registerProvider(std::move(push));
    if (!registered.has_value()) {
      std::fprintf(stderr, "fixture: push provider registration failed\n");
      std::exit(2);
    }
    const auto machine = *System::local().object("machine");
    const auto onWorker = *machine.counter<Events>("worker-ops");
    const Expression<Events> counted {onWorker};
    const auto plan = compile(System::local(), counted);
    if (!plan.has_value()) {
      std::fprintf(stderr, "fixture: push plan compile failed\n");
      std::exit(2);
    }
    auto rec = plan->recorder(4);
    rec.sample();
    rec.sample();
    survived(mode);
    return 0;
  }

  auto compiled = setup();

  if (mode == "metric-before-finish") {
    const auto core = *System::local().object("package-1/core-3");
    const auto cycles = *core.counter<Events>("cycles");
    const auto instructions = *core.counter<Events>("instructions");
    const auto ipc = instructions / cycles;
    sg::counters::Scope window {compiled};
    const auto result = window.metric(ipc);
    static_cast<void>(result);
    survived(mode);
    return 0;
  }

  if (mode == "fold-range") {
    const auto core = *System::local().object("package-1/core-3");
    const auto cycles = *core.counter<Events>("cycles");
    const auto instructions = *core.counter<Events>("instructions");
    const auto ipc = instructions / cycles;
    sg::counters::Scope window {compiled};
    window.start();
    window.finish();
    const auto result = ipc.fold(window.view(), 1, 0);
    static_cast<void>(result);
    survived(mode);
    return 0;
  }

  if (mode == "fold-out-of-extent") {
    const auto core = *System::local().object("package-1/core-3");
    const auto cycles = *core.counter<Events>("cycles");
    const auto instructions = *core.counter<Events>("instructions");
    const auto ipc = instructions / cycles;
    sg::counters::Scope window {compiled};
    window.start();
    window.finish();
    // The second conjunct of the extent guard at
    // counters_measurement.hpp and source/counters/fold.cpp: `i < j`
    // holds here while `j` names a point past the two the scope
    // recorded, so a fold that survives read an uncommitted point.
    const auto result = ipc.fold(window.view(), 0, 2);
    static_cast<void>(result);
    survived(mode);
    return 0;
  }

  if (mode == "recorder-cross-thread") {
    auto rec = compiled.recorder(4);
    rec.sample();
    std::thread foreign {[&rec] { rec.sample(); }};
    foreign.join();
    survived(mode);
    return 0;
  }

  if (mode == "scope-cross-thread") {
    const auto core = *System::local().object("package-1/core-3");
    const auto cycles = *core.counter<Events>("cycles");
    const auto instructions = *core.counter<Events>("instructions");
    const auto ipc = instructions / cycles;
    sg::counters::Scope window {compiled};
    window.start();
    std::thread foreign {[&window] { window.finish(); }};
    foreign.join();
    static_cast<void>(window.metric(ipc));
    survived(mode);
    return 0;
  }

  if (mode == "overrun") {
    auto rec = compiled.recorder(2);
    rec.sample();
    rec.sample();
    std::fflush(stdout);
    rec.sample();
    survived(mode);
    return 0;
  }

  std::fprintf(stderr,
               "fixture: unknown mode '%.*s'\n",
               static_cast<int>(mode.size()),
               mode.data());
  return 2;
}
