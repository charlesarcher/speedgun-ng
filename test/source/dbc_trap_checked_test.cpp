// ============================================================================
// Red checked-build trap verification (T005).
//
// Spawns the trap fixture as a subprocess and asserts the *checked-build*
// (default dev = enforce) behavior: the semantic-gated site must abort BEFORE
// printing the marker, so the marker is absent from the fixture's output
// (SC-002/FR-018 half (a)). The ignore-build contrast (marker present,
// always-on site still fires) is asserted by the consumer-release CI job
// (T016).
//
// Cross-platform: uses std::system with output redirection to a file, so it
// needs no fork/exec/pipe plumbing. The fixture's existence is checked first so
// a missing binary cannot false-pass as "no marker".
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

auto main(int argc, char** argv) -> int
{
  if (argc < 2) {
    std::fprintf(stderr, "usage: dbc_trap_checked_test <fixture-executable>\n");
    return 2;
  }
  std::string fixture = argv[1];

  {
    std::ifstream probe(fixture);
    if (!probe) {
      std::fprintf(stderr,
                   "DBC TRAP-CHECKED FAIL: fixture not found: %s\n",
                   fixture.c_str());
      return 1;
    }
  }

  std::string out = "/tmp/dbc_trap_checked_out.txt";
  std::remove(out.c_str());

  std::string cmd = "\"" + fixture + "\" > \"" + out + "\" 2>&1";
  // The fixture aborts; we inspect the redirected output, not the exit code.
  int const status = std::system(cmd.c_str());
  static_cast<void>(status);

  std::ifstream f(out);
  std::ostringstream body;
  if (f) {
    body << f.rdbuf();
  }
  std::string content = body.str();

  bool has_marker = content.find("gated-site-passed") != std::string::npos;
  if (has_marker) {
    std::fprintf(stderr,
                 "DBC TRAP-CHECKED FAIL: gated site did NOT abort in a checked "
                 "build " "(marker present):\n%s\n",
                 content.c_str());
    return 1;
  }

  std::printf(
      "dbc_trap_checked_test PASS: gated site aborted (no marker) in the "
      "checked build\n");
  return 0;
}
