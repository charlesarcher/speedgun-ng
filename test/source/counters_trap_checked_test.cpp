// ============================================================================
// Trap verification for counters misuse (T023, T090, T105).
//
// Spawns the counters trap fixture per mode and asserts the behavior the
// configured contract semantic demands:
//
//   - enforce and quick_enforce: every mode must abort BEFORE printing its
//     survival marker (SC: FR-046 misuse, FR-018 fold range and extent,
//     FR-027 capacity, FR-031 per-thread plans and recorders). The
//     aborting configuration must also report the violation, so an
//     abort the contract facility did not produce is read as a mode
//     the fixture has no body for and fails the check.
//   - ignore and observe: the semantic-gated modes must run through and
//     print their marker, which proves the gated check emitted no code,
//     while the always-on overrun (SG_REQUIRE_ALWAYS) must still abort
//     with its marker absent. That contrast is the FR-027 release proof
//     (SC-002 half (b)); it runs in a release-configured binary directory:
//     cmake -S . -B build/release-dev -D CMAKE_BUILD_TYPE=Release
//       -D speedgun-ng_DEVELOPER_MODE=ON -D speedgun-ng_CONTRACTS=ignore
//     ctest --test-dir build/release-dev -R counters_trap
//
// The semantic arrives as SG_CONTRACTS_SEMANTIC, the PUBLIC compile
// definition of the speedgun-ng target this test links, so the
// expectation and the fixture under test always come from one
// configuration. Cross-platform: std::system with output redirection, as
// in the dbc precedent; a missing fixture binary is reported, never
// passed.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "speedgun-ng/dbc.hpp"  // SG_CONTRACTS_SEMANTIC

namespace
{

// Semantic 0 (ignore) and 1 (observe) let a gated violation through, so
// the gated modes survive; 2 (enforce) and 3 (quick_enforce) abort on it.
#if SG_CONTRACTS_SEMANTIC == 0 || SG_CONTRACTS_SEMANTIC == 1
constexpr bool kGatedChecksFire = false;
#else
constexpr bool kGatedChecksFire = true;
#endif

// The contract facility prints one report per violation it terminates on
// and never returns, so a report in the captured output names a guard
// the mode reached. A mode the fixture has no body for prints its own
// refusal and returns, and a fault in a mode body prints neither, so the
// report separates both from the abort under test. The quick_enforce
// semantic terminates on a trap and reports nothing by design, so the
// report cannot carry the check there and run_mode's refusal test does.
constexpr bool kViolationIsReported = SG_CONTRACTS_SEMANTIC != 3;

struct mode_case
{
  const char* name;
  bool always_on;  // SG_REQUIRE_ALWAYS: fires in every semantic
};

constexpr mode_case kModes[] = {{"metric-before-finish", false},
                                {"fold-range", false},
                                {"fold-out-of-extent", false},
                                {"overrun", true},
                                {"push-cross-thread", false},
                                {"push-decrement", false},
                                {"push-foreign-sample", false},
                                {"push-mixed-owner", false},
                                {"recorder-cross-thread", false},
                                {"scope-cross-thread", false}};

auto run_mode(const std::string& fixture,
              const mode_case& mode,
              const bool expect_abort) -> int
{
  const std::string name {mode.name};
  std::string out = "/tmp/counters_trap_checked_out.txt";
  std::remove(out.c_str());
  const std::string cmd =
      "\"" + fixture + "\" " + name + " > \"" + out + "\" 2>&1";
  const int status = std::system(cmd.c_str());

  std::ifstream f(out);
  std::ostringstream body;
  if (f) {
    body << f.rdbuf();
  }
  const std::string content = body.str();
  const std::string marker = "counters-trap-survived-" + name;
  const bool printed_marker = content.find(marker) != std::string::npos;
  const bool reported_violation =
      content.find("(predicate: ") != std::string::npos;
  // A mode the fixture has no body for lands on the refusal path, which
  // exits non-zero with the marker absent and prints no report in any
  // semantic, and quick_enforce prints no report for a real abort either.
  // The refusal literal is then the one signal separating the two, and
  // both fixtures write it.
  const bool refused =
      content.find("fixture: unknown mode") != std::string::npos;

  if (expect_abort) {
    if (status == 0) {
      std::fprintf(stderr,
                   "COUNTERS TRAP-CHECKED FAIL: mode '%s' exited 0, the "
                   "violation was not caught:\n%s\n",
                   name.c_str(),
                   content.c_str());
      return 1;
    }
    if (kViolationIsReported && !reported_violation) {
      std::fprintf(stderr,
                   "COUNTERS TRAP-CHECKED FAIL: mode '%s' exited %d without "
                   "the contract facility's violation report, so no guard of "
                   "that mode was reached:\n%s\n",
                   name.c_str(),
                   status,
                   content.c_str());
      return 1;
    }
    if (printed_marker) {
      std::fprintf(stderr,
                   "COUNTERS TRAP-CHECKED FAIL: mode '%s' printed its "
                   "survival marker despite aborting:\n%s\n",
                   name.c_str(),
                   content.c_str());
      return 1;
    }
    if (refused) {
      std::fprintf(stderr,
                   "COUNTERS TRAP-CHECKED FAIL: mode '%s' exited %d on the "
                   "fixture's refusal, so no guard of that mode was "
                   "reached:\n%s\n",
                   name.c_str(),
                   status,
                   content.c_str());
      return 1;
    }
    std::printf("counters trap mode '%s' aborted before the marker\n",
                name.c_str());
    return 0;
  }

  if (status != 0) {
    std::fprintf(stderr,
                 "COUNTERS TRAP-CHECKED FAIL: mode '%s' exited %d where the "
                 "gated check must have emitted no code:\n%s\n",
                 name.c_str(),
                 status,
                 content.c_str());
    return 1;
  }
  if (!printed_marker) {
    std::fprintf(stderr,
                 "COUNTERS TRAP-CHECKED FAIL: mode '%s' exited 0 without its "
                 "survival marker:\n%s\n",
                 name.c_str(),
                 content.c_str());
    return 1;
  }
  std::printf("counters trap mode '%s' survived with its marker\n",
              name.c_str());
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc < 2) {
    std::fprintf(stderr,
                 "usage: counters_trap_checked_test <fixture-executable>\n");
    return 2;
  }
  const std::string fixture = argv[1];
  {
    std::ifstream probe(fixture);
    if (!probe) {
      std::fprintf(stderr,
                   "COUNTERS TRAP-CHECKED FAIL: fixture not found: %s\n",
                   fixture.c_str());
      return 1;
    }
  }
  for (const auto& mode : kModes) {
    if (run_mode(fixture, mode, mode.always_on || kGatedChecksFire) != 0) {
      return 1;
    }
  }
  std::printf(
      "counters_trap_checked_test PASS: every misuse mode behaved as the "
      "configured contract semantic demands\n");
  return 0;
}
