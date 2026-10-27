// ============================================================================
// Trap verification for the hunt ranker (specs/032 FR-048, review B 20):
// spawns `harness_hunt_rank_trap_fixture` and asserts the abort the
// invariant of `rankEvents` carries, the shape
// `counters_trap_checked_test.cpp` runs for the recorder. The
// expectations follow SG_CONTRACTS_SEMANTIC, the PUBLIC definition of
// the library the fixture links: in a checked build the invariant
// aborts before the marker; in an ignore or observe build the fixture
// survives with it.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <sys/wait.h>

#include "speedgun-ng/dbc.hpp"  // SG_CONTRACTS_SEMANTIC

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HUNT RANK TRAP TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc < 2) {
    std::fprintf(stderr,
                 "usage: harness_hunt_rank_trap_test <fixture-executable>\n");
    return 2;
  }
  const std::string fixture = argv[1];
  {
    std::ifstream probe(fixture);
    check(static_cast<bool>(probe), "the fixture executable stands readable");
  }

  const std::string out =
      "/tmp/harness_hunt_rank_trap_" + std::to_string(getpid()) + ".txt";
  std::remove(out.c_str());
  const std::string command =
      "\"" + fixture + "\" > \"" + out + "\" 2>&1";
  const int status = std::system(command.c_str());
  std::string content;
  {
    std::ifstream reader(out);
    std::ostringstream body;
    if (reader) {
      body << reader.rdbuf();
    }
    content = body.str();
  }
  std::remove(out.c_str());

  const bool printedMarker =
      content.find("hunt-rank-trap-survived") != std::string::npos;
  const bool reportedViolation = content.find("(predicate: ")
      != std::string::npos;
  const bool exitedClean = WIFEXITED(status) && WEXITSTATUS(status) == 0;

#if SG_CONTRACTS_SEMANTIC == 0 || SG_CONTRACTS_SEMANTIC == 1
  // The invariant stands gated: the fixture survives with its marker and
  // exits clean (the FR-027 release proof the counters checker states).
  constexpr bool kInvariantFires = false;
#else
  constexpr bool kInvariantFires = true;
#endif

  if constexpr (kInvariantFires) {
    check(!printedMarker,
          "the invariant aborted the fixture before the marker (FR-048)");
    check(!exitedClean,
          "the abort stands the termination the invariant carries (FR-048)");
#if SG_CONTRACTS_SEMANTIC != 3
    // The quick_enforce semantic terminates on a trap and reports
    // nothing by design, the shape the counters checker records.
    check(reportedViolation,
          "the contract facility names the guard it terminated on (FR-048)");
#endif
  } else {
    check(exitedClean && printedMarker,
          "the gated invariant lets the fixture survive with its marker "
          "(FR-048)");
  }
  std::printf("hunt-rank-trap: ok\n");
  return 0;
}
