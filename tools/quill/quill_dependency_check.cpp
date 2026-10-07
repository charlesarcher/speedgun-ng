// The runnable dependency proof for the vendored quill import
// (specs/009 FR-008, FR-008a; privacy contract audit A6).
//
// The five compiled dependencies prove their link at object level: their
// gate units carry an undefined reference that a merged archive member
// satisfies. quill cannot be proven that way. It is header-only, so a
// unit that takes the address of one quill function emits that function
// and its entire inline call graph as weak definitions in the same unit,
// and emits zero undefined symbols, at both -O0 and -O2 (research R-008).
// So this check proves the dependency the only way available to a
// header-only library: it links, runs, and asserts on what came out.
//
// FR-008a requires the check to exercise both libraries together.
// Emitting a fixed string would prove quill links and would say nothing
// about whether the counter interface interoperates with it, which is
// the property a future logging facility depends on. So this drives a real
// counter to a real value, logs that value, and then reads the log back
// and asserts the number is in it.
//
// The counter setup mirrors example/counters_standalone_example.cpp,
// which already registers a fake provider, resolves an object and its
// counters, compiles a plan, samples a recorder, and folds intervals.
// The speedgun-ng headers are included one by one, each include naming a
// single header, because every symbol this file names comes from exactly one
// of them and the project's analyzer gate asks for the direct include. The
// counters.hpp umbrella is absent from this file's include list.
//
// This file is a test. It is not part of the shipped archive, the
// installed artifact, or the exported target set (FR-008), and it is the
// one place in this feature that starts quill's backend, which is why
// Fixed decision 8 exempts it.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <print>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/core/Common.h>
#include <quill/sinks/FileSink.h>
#include <quill/sinks/StreamSink.h>

#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"

// One alias covers every type, and the bare name `system`
// would otherwise resolve to the global ::system function, which the analyzer
// reports as a missing direct include.
namespace counters = sg::counters;

// 1000 instructions against 400 cycles per sampling action, so every interval
// folds to an IPC of exactly 2.5. That value is the assertion: it is computed
// from the counter interface and then looked for in the log.
constexpr std::uint64_t instructions_per_sample = 1000;
constexpr std::uint64_t cycles_per_sample = 400;
constexpr double expected_ipc = 2.5;
constexpr double ipc_tolerance = 1e-9;
constexpr std::size_t sample_count = 4;
constexpr int synthetic_iterations = 1000;

using events = counters::Dim<0, 1>;
using time_dim = counters::Dim<1, 0>;

namespace
{

// Writes one diagnostic line to stderr. A write failure cannot be reported: the
// stream that would carry the report is the stream that failed. The explicit
// discard says exactly that, which Constitution X.2 requires beside a discard.
auto note(char const* text) noexcept -> void
{
  static_cast<void>(std::fputs(text, stderr));
}

auto fail(char const* stage, std::string const& detail) -> int
{
  std::println(stderr, "quill_dependency_check: {}: {}", stage, detail);
  return 1;
}

auto read_file(std::filesystem::path const& path) -> std::string
{
  auto const input = std::ifstream(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

// Drives the counter interface to one folded interval and hands the value to
// quill. Returns the folded IPC, or the reason the drive failed.
auto measure(quill::Logger* logger) -> std::expected<double, std::string>
{
  // Registers before the fake source: it supplies the machine object, and
  // reversing the two silently breaks machine resolution below.
  auto clock = std::make_unique<counters::clock_provider>();
  if (!counters::system::local()
           .register_provider(std::move(clock))
           .has_value())
  {
    return std::unexpected(std::string {"the clock provider was refused"});
  }

  auto fake = std::make_unique<counters::fake_provider>();
  fake->add_object("package-0/core-0", "core-0", "core", "first core");
  fake->add_counter(
      "package-0/core-0", "instructions", "ops", "instructions retired");
  fake->add_counter("package-0/core-0", "cycles", "ops", "core cycles");
  fake->set_points(
      "package-0/core-0", "instructions", {}, instructions_per_sample);
  fake->set_points("package-0/core-0", "cycles", {}, cycles_per_sample);
  if (!counters::system::local().register_provider(std::move(fake)).has_value())
  {
    return std::unexpected(std::string {"the fake provider was refused"});
  }

  auto const core = counters::system::local().object("package-0/core-0");
  if (!core.has_value()) {
    return std::unexpected(std::string {"package-0/core-0 did not resolve"});
  }
  auto const instructions = core->counter<events>("instructions");
  auto const cycles = core->counter<events>("cycles");
  if (!instructions.has_value() || !cycles.has_value()) {
    return std::unexpected(
        std::string {"instructions or cycles did not resolve"});
  }

  auto const machine = counters::system::local().object("machine");
  if (!machine.has_value()) {
    return std::unexpected(std::string {"the machine object did not resolve"});
  }
  auto const monotonic = machine->counter<time_dim>("monotonic");
  if (!monotonic.has_value()) {
    return std::unexpected(std::string {"machine monotonic did not resolve"});
  }

  auto const ipc = (*instructions) / (*cycles);
  auto const rate = (*instructions) / (*monotonic);

  auto const compiled = counters::compile(counters::system::local(), ipc, rate);
  if (!compiled.has_value()) {
    return std::unexpected(std::string {"the plan did not compile"});
  }

  // Capacity is samples plus the initial point, per FR-050.
  auto recorder = compiled->recorder(sample_count + 1);
  recorder.sample();
  for (std::size_t sample = 0; sample < sample_count; ++sample) {
    volatile double work = 0.0;
    for (int iteration = 0; iteration < synthetic_iterations; ++iteration) {
      work += 1.0;
    }
    static_cast<void>(work);
    recorder.sample();
  }

  auto const intervals = ipc.foldPairs(recorder.view());
  if (intervals.empty()) {
    return std::unexpected(std::string {"the recorder folded no intervals"});
  }
  auto const folded = intervals.back();

  LOG_INFO(
      logger,
      "quill_dependency_check: ipc {:.6f} from {} instructions over {} cycles",
      folded.value,
      instructions_per_sample,
      cycles_per_sample);
  LOG_INFO(logger,
           "quill_dependency_check: scaled={} running_ratio {:.6f}",
           folded.scaled ? "yes" : "no",
           folded.runningRatio);

  return folded.value;
}

// Asserts that the value reached the log, and that the counter library
// produced it independently of the log.
auto assert_reached_log(std::filesystem::path const& log_path,
                        double value) -> int
{
  auto const contents = read_file(log_path);
  if (contents.empty()) {
    return fail("log read", "quill produced no log output at all");
  }

  // std::to_string renders a double with the same six fractional digits the
  // {:.6f} specifier in measure produced, so the two agree by construction.
  auto const expected = "ipc " + std::to_string(value);

  if (!contents.contains(expected)) {
    std::println(stderr, "quill_dependency_check: assertion failed");
    std::println(stderr, "  expected the log to contain: {}", expected);
    std::println(stderr, "  counter library produced ipc {:.6f}", value);
    std::println(stderr, "  log contents follow:\n{}", contents);
    return 1;
  }

  if (value < expected_ipc - ipc_tolerance
      || value > expected_ipc + ipc_tolerance)
  {
    std::println(stderr,
                 "quill_dependency_check: assertion failed: folded ipc {:.6f} "
                 "is outside " "the {:.6f} the fake source describes",
                 value,
                 expected_ipc);
    return 1;
  }

  std::println("quill_dependency_check: PASS ipc {:.6f} reached the log",
               value);
  return 0;
}

auto run() -> int
{
  auto const log_path = std::filesystem::temp_directory_path()
      / "speedgun-ng-quill-dependency-check.log";
  std::error_code ignored;
  std::filesystem::remove(log_path, ignored);

  // quill's backend must run before a record can be drained: without it the
  // frontend queues fill and nothing is written.
  quill::Backend::start();

  auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      log_path.string(),
      []() -> quill::FileSinkConfig
      {
        quill::FileSinkConfig config;
        config.set_open_mode('w');
        return config;
      }(),
      quill::FileEventNotifier {});

  auto* logger = quill::Frontend::create_or_get_logger(
      "root",
      std::move(sink),
      quill::PatternFormatterOptions {"LOG_%(log_level:<9) %(message)",
                                      "%H:%M:%S",
                                      quill::Timezone::GmtTime});

  auto const measured = measure(logger);
  if (!measured.has_value()) {
    return fail("counter", measured.error());
  }

  quill::Backend::stop();

  auto const verdict = assert_reached_log(log_path, *measured);
  if (verdict == 0) {
    std::filesystem::remove(log_path, ignored);
  }
  return verdict;
}

}  // namespace

// main reports an escaped exception through C stdio because the analyzer
// models std::println as throwing and a throwing call in a handler is exactly
// what bugprone-exception-escape reports. C stdio carries no such annotation,
// so the diagnostic survives and the check stays green. Every expected
// failure already reports through run(), so this path only runs on an
// unexpected throw.
auto main() noexcept -> int
{
  try {
    return run();

  } catch (std::exception const& error) {
    note("quill_dependency_check: uncaught exception: ");
    note(error.what());
    note("\n");
    return 2;

  } catch (...) {
    note("quill_dependency_check: uncaught non-standard exception\n");
    return 2;
  }
}
