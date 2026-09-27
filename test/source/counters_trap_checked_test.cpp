// ============================================================================
// Checked-build trap verification for counters misuse (T023).
//
// Spawns the counters trap fixture per mode and asserts the checked-build
// (default dev = enforce) behavior: every mode must abort BEFORE printing
// its survival marker (SC: FR-046 misuse, FR-018 fold range, FR-027
// capacity, FR-031 per-thread plans and recorders). The ignore-build contrast
// (gated modes survive, the always-on overrun still fires) is asserted by the
// consumer-release CI job. Cross-platform: std::system with output redirection,
// as in the dbc precedent; a missing fixture binary is reported, never passed.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

constexpr const char* kModes[] = {"metric-before-finish",
                                  "fold-range",
                                  "overrun",
                                  "push-cross-thread",
                                  "push-decrement",
                                  "recorder-cross-thread",
                                  "scope-cross-thread"};

auto run_mode(const std::string& fixture, const char* mode) -> int
{
  std::string out = "/tmp/counters_trap_checked_out.txt";
  std::remove(out.c_str());
  const std::string cmd =
      "\"" + fixture + "\" " + mode + " > \"" + out + "\" 2>&1";
  const int status = std::system(cmd.c_str());

  std::ifstream f(out);
  std::ostringstream body;
  if (f) {
    body << f.rdbuf();
  }
  const std::string content = body.str();
  const std::string marker = "counters-trap-survived-" + std::string(mode);

  if (status == 0) {
    std::fprintf(stderr,
                 "COUNTERS TRAP-CHECKED FAIL: mode '%s' exited 0, the "
                 "violation was not caught:\n%s\n",
                 mode,
                 content.c_str());
    return 1;
  }
  if (content.find(marker) != std::string::npos) {
    std::fprintf(stderr,
                 "COUNTERS TRAP-CHECKED FAIL: mode '%s' printed its "
                 "survival marker despite aborting:\n%s\n",
                 mode,
                 content.c_str());
    return 1;
  }
  std::printf("counters trap mode '%s' aborted before the marker\n", mode);
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
  for (const char* mode : kModes) {
    if (run_mode(fixture, mode) != 0) {
      return 1;
    }
  }
  std::printf(
      "counters_trap_checked_test PASS: all misuse modes aborted in the "
      "checked build\n");
  return 0;
}
