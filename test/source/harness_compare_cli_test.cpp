// ============================================================================
// T019, T025, T028, T032, T042, T044, T061: the compare mode of the regression
// gate driven through the entry point over the committed fixture documents
// of test/data (specs/026; FR-006, FR-011, FR-021, FR-024, FR-031, FR-034,
// FR-039, FR-040, FR-044, FR-045, FR-048, FR-049, FR-051, FR-052, FR-053,
// FR-054, FR-056, FR-057, FR-058, FR-074, SC-001, SC-002, SC-003, SC-004,
// SC-005, SC-006, SC-009, SC-013, D-13, D-14, D-19, E-10, spec D-6,
// spec D-20).
//
// The mode reads two documents and answers a verdict, so every case calls
// `speedgunMain` with the option and reads back both streams it prints
// on: the table on standard output, the fault line on standard error. The
// fixture documents are committed, so every number the table prints is a
// constant of this file. Both pairs hold one benchmark, `bmGate`, whose
// primary time is the cpu column of 1000 ns in the baseline; the pass
// contender answers 1030 ns and the fail contender 1090 ns, so the
// default rule `time@median:lower:5%` of spec D-5 prints +3.000% and
// +30.000 ns on one pair and +9.000% and +90.000 ns on the other. The
// baseline report carries speedgun 0.14.0 and the contender 0.15.0, both
// under schema 1.0. Beside the pairs stand two documents of that same
// benchmark at the baseline's own numbers: `gate_schema_11.json` carries the
// pass keys of the 1.1 format under speedgun 0.16.0, and
// `gate_schema_20.json` states major 2, which the reader refuses (D-13,
// SC-013). The zero-baseline pair holds that same contender against
// a baseline whose primary time column reads 0, the pair that a relative
// threshold measures not at all (FR-031). The noisy pair is that same pass
// pair with one field added, the `noiseWarning` object the contender's
// benchmark carries, so its row answers `NOISY` with the reason beside it
// while every number stands where the pass pair leaves it (FR-044, SC-006).
//
// One benchmark stands registered, so a mode that fell through to the run
// path would print a run row: the checks that own the whole of the output
// then fail, which is what makes the reading "no benchmark ran" a check
// and not an assumption (SC-005).
//
// The document cases of T044 write their file into the working directory
// the registered command runs in, beside the two stream captures, and each
// case owns a path of its own so a file an earlier case wrote cannot answer
// for a write that stands absent. The schema reaches the case through the
// fixture directory argument the command already passes: two levels above
// test/data stands the repository root, and `docs/schema/compare-v1.json`
// under it (D-14).
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "detail/json_fixture.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"

using sg::test::check;
using sg::test::fail;

namespace
{

constexpr const char* kOutPath = "harness_compare_cli_stdout.txt";
constexpr const char* kErrPath = "harness_compare_cli_stderr.txt";

// One path per document case, so no case reads the bytes an earlier case
// left behind (T044, FR-057, FR-058).
constexpr const char* kSchemaDocument = "harness_compare_cli_schema.json";
constexpr const char* kFirstRunDocument = "harness_compare_cli_first.json";
constexpr const char* kSecondRunDocument = "harness_compare_cli_second.json";
constexpr const char* kFitDocument = "harness_compare_cli_fit.json";
constexpr const char* kPlainDocument = "harness_compare_cli_plain.json";

// The two streams and the status of one entry-point call.
struct ModeRun
{
  int status;
  std::string out;
  std::string err;
};

// One entry-point call with both streams captured, the redirect shape of
// `harness_cli_test.cpp`. The mode prints its table on standard output and
// its one-line faults on standard error, so both are read back.
auto runMode(const std::vector<std::string>& arguments) -> ModeRun
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_compare_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const int savedOut = dup(STDOUT_FILENO);
  const int savedErr = dup(STDERR_FILENO);
  if (savedOut < 0 || std::freopen(kOutPath, "w", stdout) == nullptr) {
    fail("the standard output of the mode redirects");
  }
  if (savedErr < 0 || std::freopen(kErrPath, "w", stderr) == nullptr) {
    fail("the standard error of the mode redirects");
  }
  ModeRun run {
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data()), {}, {}};
  std::fflush(stdout);
  std::fflush(stderr);
  if (dup2(savedOut, STDOUT_FILENO) < 0 || dup2(savedErr, STDERR_FILENO) < 0) {
    fail("the two streams restore");
  }
  close(savedOut);
  close(savedErr);
  run.out = sg::test::readText(kOutPath);
  run.err = sg::test::readText(kErrPath);
  return run;
}

// The non-empty lines of one captured stream, in order.
auto linesOf(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

auto containsAll(const std::string& text,
                 std::initializer_list<const char*> needles) -> bool
{
  for (const char* needle : needles) {
    if (text.find(needle) == std::string::npos) {
      return false;
    }
  }
  return true;
}

auto endsWith(const std::string& text, const std::string& tail) -> bool
{
  return text.size() >= tail.size()
      && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

// The observed output of a case, for the reader of a failure.
auto dump(const char* what, const ModeRun& run) -> void
{
  std::fprintf(stderr,
               "%s answered %d with:\n--- stdout ---\n%s--- stderr ---\n%s",
               what,
               run.status,
               run.out.c_str(),
               run.err.c_str());
}

// The pass pair: status 0, the header of FR-056, and one `OK` row whose
// change columns carry the percent and the absolute delta. The table is
// the whole of the output, so no benchmark ran (SC-001, SC-005).
auto thePassPairAnswersZeroWithOneOkRow(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender});

  check(run.status == 0, "the pass pair answers status 0 (SC-001, FR-052)");
  check(run.err.empty(),
        "the pass pair prints nothing on standard error (SC-005)");

  const std::vector<std::string> lines = linesOf(run.out);
  if (lines.size() != 4) {
    dump("the pass pair", run);
  }
  check(lines.size() == 4,
        "two header lines, one column line and one row are the whole of the "
        "output, so no benchmark ran (SC-005)");
  if (lines.size() != 4) {
    return;
  }

  // FR-056: the header names the two files and, for each report, the
  // speedgun version and the schema version the load call read.
  check(lines[0].rfind("compare: baseline ", 0) == 0
            && containsAll(lines[0],
                           {baseline.c_str(), "speedgun 0.14.0", "schema 1.0"}),
        "the baseline header line names the file, its speedgun version and "
        "its schema version (FR-056)");
  check(lines[1].rfind("compare: contender ", 0) == 0
            && containsAll(lines[1],
                           {contender.c_str(), "speedgun 0.15.0", "schema 1.0"}),
        "the contender header line names the file, its speedgun version and "
        "its schema version (FR-056)");
  check(containsAll(lines[2],
                    {"benchmark",
                     "rule",
                     "baseline",
                     "contender",
                     "change%",
                     "change",
                     "state"}),
        "the column line names the seven columns of FR-053");

  // SC-005: the mode runs no benchmark, so no run row stands anywhere.
  check(run.out.find("iterations=") == std::string::npos
            && run.out.find("time/iter") == std::string::npos,
        "the output carries no benchmark row (SC-005)");

  // SC-001: the one row of the default rule answers `OK` and carries both
  // values and both change columns.
  check(lines[3].rfind("bmGate", 0) == 0
            && containsAll(lines[3],
                           {"time@median:lower:5%",
                            "1000.000 ns",
                            "1030.000 ns",
                            "+3.000%",
                            "+30.000 ns"})
            && endsWith(lines[3], "OK"),
        "the pass row names the benchmark, the default rule, the two values, "
        "the percent and the absolute delta, and answers OK (SC-001, FR-053, "
        "FR-054)");
}

// T061, FR-049: `--compare-rule` repeats and the rules keep the order the
// flags give them. Two rules over the pass pair answer two rows of one
// benchmark in flag order, and the tighter bound of the first flag is the
// one that fails the run.
auto twoWrittenRulesAnswerTwoRowsInFlagOrder(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender,
                               "--compare-rule=time@median:lower:1%",
                               "--compare-rule=time@median:lower:50%"});

  check(run.err.empty(), "two written rules print no fault (FR-049)");
  check(run.status == 1,
        "the tighter bound of the first flag fails the run (SC-002, FR-052)");

  const std::vector<std::string> lines = linesOf(run.out);
  if (lines.size() != 5) {
    dump("two written rules", run);
  }
  check(lines.size() == 5,
        "two header lines, one column line and two rows are the whole of the "
        "output (SC-005)");
  if (lines.size() != 5) {
    return;
  }
  check(lines[3].rfind("bmGate", 0) == 0
            && lines[3].find("time@median:lower:1%") != std::string::npos
            && endsWith(lines[3], "REGRESSED"),
        "the first row answers the rule the first flag wrote (FR-049)");
  check(lines[4].rfind("bmGate", 0) == 0
            && lines[4].find("time@median:lower:50%") != std::string::npos
            && endsWith(lines[4], "OK"),
        "the second row answers the rule the second flag wrote, in the order "
        "the flags stand (FR-049)");
}

// The fail pair: the contender sits 9 percent behind the default rule, so
// the row prints `REGRESSED` and the status is 1 (SC-002).
auto theFailPairAnswersOneWithARegressedRow(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_fail_base.json";
  const std::string contender = data + "/gate_fail_contender.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender});

  check(run.status == 1,
        "the regression verdict answers status 1 (SC-002, FR-052)");
  const std::vector<std::string> lines = linesOf(run.out);
  if (lines.size() != 4) {
    dump("the fail pair", run);
  }
  check(lines.size() == 4,
        "the regression prints the table and nothing else (SC-005)");
  if (lines.size() != 4) {
    return;
  }
  check(endsWith(lines[3], "REGRESSED")
            && containsAll(lines[3],
                           {"1000.000 ns",
                            "1090.000 ns",
                            "+9.000%",
                            "+90.000 ns"}),
        "the fail row prints REGRESSED with the two values and both change "
        "columns (SC-002, FR-053, FR-054)");
}

// D-19: a batch filter that leaves the pair with no shared name answers
// one line saying the pair matched nothing, at status 0 and with no table.
auto aFilterMatchingNothingAnswersZeroWithOneLine(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run =
      runMode({"--filter=zzz", "--compare=" + baseline + "," + contender});

  check(run.status == 0,
        "a pair the filter empties answers status 0 (D-19, FR-052)");
  const std::vector<std::string> lines = linesOf(run.out + run.err);
  if (lines.size() != 1) {
    dump("the empty pair", run);
  }
  check(lines.size() == 1,
        "the empty pair answers one line and prints no table (D-19)");
  if (lines.size() != 1) {
    return;
  }
  check(lines[0].find("match") != std::string::npos
            && lines[0].find("nothing") != std::string::npos,
        "that line says the pair matched nothing (D-19)");
}

// T028, FR-049, spec D-6: a written rule carries its scope as the trailing
// filter field, and that field is a search over the whole instance name
// (FR-024), so `bmGate` reaches both names of the fit pair. The readable
// benchmark answers one entry carrying its values, the complexity fit entry
// answers `SKIPPED` with no values beside it, and the run answers 0 while
// strict mode stands off (FR-011, spec D-20). The written rule replaces the
// default rule, so each name answers exactly one row (FR-034).
auto aScopedRuleAnswersOneRowAndTheFitEntry(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_fit_base.json";
  const std::string contender = data + "/gate_fit_contender.json";
  const ModeRun run = runMode({"--compare-rule=time@median:lower:5%:bmGate",
                               "--compare=" + baseline + "," + contender});

  check(run.status == 0,
        "a scoped rule over the fit pair answers status 0 while strict mode "
        "stands off (SC-003, FR-011)");
  check(run.err.empty(),
        "the scoped rule prints nothing on standard error (SC-003)");

  const std::vector<std::string> lines = linesOf(run.out);
  if (lines.size() != 5) {
    dump("the scoped rule over the fit pair", run);
  }
  check(lines.size() == 5,
        "the two header lines, the column line and two rows are the whole of "
        "the output, so the default rule stands absent beside a written rule "
        "(FR-034, SC-005)");
  if (lines.size() != 5) {
    return;
  }

  check(lines[3].rfind("bmGate ", 0) == 0
            && containsAll(lines[3],
                           {"time@median:lower:5%:bmGate",
                            "1000.000 ns",
                            "1030.000 ns",
                            "+3.000%",
                            "+30.000 ns"})
            && endsWith(lines[3], "OK"),
        "the named benchmark answers one entry carrying the written rule, the "
        "two values and both change columns, and answers OK (SC-003, FR-049, "
        "FR-053, FR-054)");
  check(lines[4].rfind("bmGateFit", 0) == 0
            && lines[4].find("time@median:lower:5%:bmGate") != std::string::npos
            && lines[4].find("n/a") != std::string::npos
            && endsWith(lines[4], "SKIPPED"),
        "the fit entry answers SKIPPED with no values beside it (FR-011, "
        "spec D-20)");
}

// T028, FR-039: a written rule whose filter reaches no benchmark in either
// report stops the run before any value is read: one line naming the rule
// and that filter, status 2, and no table.
auto aScopedRuleMatchingNothingAnswersTheNoMatchFault(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare-rule=time@median:lower:5%:nosuch",
                               "--compare=" + baseline + "," + contender});

  check(run.status == 2,
        "a rule filter that matches neither report answers status 2 (FR-039, "
        "FR-052, SC-004)");
  const std::vector<std::string> lines = linesOf(run.out + run.err);
  if (lines.size() != 1) {
    dump("the no-match rule", run);
  }
  check(lines.size() == 1,
        "the no-match rule answers one line and prints no table (FR-039, "
        "SC-004)");
  if (lines.size() != 1) {
    return;
  }
  check(lines[0].find("time@median:lower:5%:nosuch") != std::string::npos
            && lines[0].find("nosuch") != std::string::npos
            && lines[0].find("match") != std::string::npos,
        "that line names the rule text and the filter that matched nothing "
        "(FR-039)");
  check(run.out.find("benchmark") == std::string::npos
            && run.out.find("iterations=") == std::string::npos,
        "the no-match rule prints no table and runs no benchmark (SC-004, "
        "SC-005)");
}

// FR-048, FR-051: `--compare` with no comma, with an empty baseline side
// or with an empty contender side is a usage fault of the shape of the
// existing ones: one line naming the option and the text read, status 2,
// and no table.
auto aCompareWithNoPairAnswersTheUsageFault(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  for (const std::string& text : {baseline, "," + contender, baseline + ","}) {
    const ModeRun run = runMode({"--compare=" + text});

    check(run.status == 2,
          "a --compare that names no pair answers status 2 (FR-048, FR-051)");
    const std::vector<std::string> lines = linesOf(run.out + run.err);
    if (lines.size() != 1) {
      dump("the usage fault", run);
    }
    check(lines.size() == 1,
          "the usage fault prints one line and no table (FR-051, SC-004)");
    if (lines.size() != 1) {
      continue;
    }
    check(lines[0].find("--compare") != std::string::npos
              && lines[0].find(text) != std::string::npos,
          "the usage line names the option and the text it read (FR-051)");
    check(run.out.find("iterations=") == std::string::npos,
          "the usage fault runs no benchmark (SC-005)");
  }
}

// T032, FR-051, SC-004: the answer shape every fault row of the quickstart
// command table carries: status 2, exactly one line naming the fault across
// the two streams, and no table anywhere. `what` labels the case for the
// dump; `needles` name what that one line must carry.
auto oneFaultLine(const ModeRun& run,
                  const char* what,
                  std::initializer_list<const char*> needles) -> void
{
  check(run.status == 2, "the fault answers status 2 (FR-051, SC-004)");
  const std::vector<std::string> lines = linesOf(run.out + run.err);
  if (lines.size() != 1) {
    dump(what, run);
  }
  check(lines.size() == 1, "the fault answers exactly one line (FR-051)");
  if (lines.size() != 1) {
    return;
  }
  check(containsAll(lines[0], needles),
        "that line names the fault the contract carries (FR-051)");
  check(run.out.find("benchmark") == std::string::npos
            && run.out.find("iterations=") == std::string::npos,
        "the fault prints no table and runs no benchmark (SC-004, SC-005)");
}

// T032, FR-021, FR-051: a rule text whose threshold field carries a bare
// number is rejected where the option is read, before either document is
// opened. The line names the rule text and the threshold field of the fault.
auto aRuleWithABareThresholdAnswersTheThresholdFault(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare-rule=time@median:lower:5",
                               "--compare=" + baseline + "," + contender});

  oneFaultLine(run,
               "the rule with a bare threshold",
               {"time@median:lower:5", "threshold"});
}

// T032, FR-039, FR-051: a rule whose filter reaches no benchmark in either
// report stops before any value is read. The line names the whole rule text,
// the filter and the fact that it selects nothing.
auto aRuleFilterReachingNothingAnswersTheNoMatchFault(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run =
      runMode({"--compare-rule=time@median:lower:5%:nothing-matches",
               "--compare=" + baseline + "," + contender});

  oneFaultLine(
      run,
      "the rule filter that reaches nothing",
      {"time@median:lower:5%:nothing-matches", "nothing-matches", "match"});
}

// T032, D-13, FR-051, SC-013: the pairing reads the major of the schema
// version, so the committed `1.0` baseline compares against the `1.1`
// contender with the pass keys: the pair answers the table with its `OK`
// row at status 0, and the two header lines name the two versions.
auto aMinorSchemaVersionPairAnswersAnOkRow(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_schema_11.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender});

  check(run.status == 0,
        "a pair that shares its schema major answers status 0 (SC-013, D-13)");
  check(run.err.empty(), "the pair across the minor prints no fault (D-13)");

  const std::vector<std::string> lines = linesOf(run.out);
  if (lines.size() != 4) {
    dump("the pair across the schema minor", run);
  }
  check(lines.size() == 4,
        "the pair across the minor answers the table and nothing else "
        "(SC-005)");
  if (lines.size() != 4) {
    return;
  }
  check(containsAll(lines[0], {"speedgun 0.14.0", "schema 1.0"})
            && containsAll(lines[1], {"speedgun 0.16.0", "schema 1.1"}),
        "the two header lines name the two schema versions the pair carries "
        "(FR-056)");
  check(containsAll(lines[3], {"bmGate", "time@median:lower:5%", "1000.000 ns"})
            && endsWith(lines[3], "OK"),
        "the row of the pair across the minor answers OK (SC-013, FR-054)");
  check(run.out.find("REGRESSED") == std::string::npos
            && run.out.find("NOISY") == std::string::npos,
        "no row of the pair across the minor carries another word (SC-013)");
}

// T032, FR-006, FR-051, SC-013: a document of a major the reader does not
// know is refused inside the load call, before the pair reaches the
// pairing. The one line names the path and the version it carries.
auto aForeignSchemaMajorAnswersTheLoadFault(const std::string& data) -> void
{
  const std::string contender = data + "/gate_schema_20.json";
  const ModeRun run =
      runMode({"--compare=" + data + "/gate_ok_base.json," + contender});

  oneFaultLine(run,
               "the document of another schema major",
               {"gate_schema_20.json", "schema version", "2.0"});
}

// T032, FR-006, FR-051: a path the reader cannot open answers one line
// naming that path and the reader's own reason, at status 2, with no table.
auto aMissingDocumentAnswersTheLoadFault(const std::string& data) -> void
{
  const std::string missing = data + "/gate_nowhere.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare=" + missing + "," + contender});

  oneFaultLine(run, "the missing document", {missing.c_str(), "opened"});
}

// T032, FR-031, FR-051: a percent of a baseline that stands at zero holds no
// value, so a rule carrying the relative test measures this pair not at all
// and the walk stops where the value is read: one line naming the rule, the
// benchmark and the value that stands at zero, at status 2, with no table.
auto aRelativeRuleOverAZeroBaselineAnswersTheZeroFault(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_zerobase_base.json";
  const std::string contender = data + "/gate_zerobase_contender.json";
  const std::string ruleText = "time@median:lower:5%";
  const ModeRun run = runMode({"--compare-rule=" + ruleText,
                               "--compare=" + baseline + "," + contender});

  oneFaultLine(run,
               "the zero baseline under a relative threshold",
               {ruleText.c_str(), "bmGate", "zero"});

  // The value the rule reads stands in that line beside the zero, past the
  // rule text that carries its name as well.
  const std::vector<std::string> lines = linesOf(run.out + run.err);
  if (lines.size() != 1) {
    return;
  }
  const std::size_t ruleAt = lines[0].find(ruleText);
  const std::size_t past =
      ruleAt == std::string::npos ? 0 : ruleAt + ruleText.size();
  check(lines[0].find("time", past) != std::string::npos,
        "the line names the value the rule reads and the zero it stands at "
        "(FR-031)");
}

// T032, FR-074, FR-051: `--json=` names the comparison document and the write
// runs before anything prints, so a path the file system refuses answers one
// line naming that path, at status 2, with no table beside it.
auto aRefusedComparisonDocumentAnswersTheWriteFault(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const std::string document = data + "/gate-no-such-directory/compare.json";
  const ModeRun run = runMode(
      {"--compare=" + baseline + "," + contender, "--json=" + document});

  oneFaultLine(
      run, "the refused comparison document", {document.c_str(), "opened"});
}

// The comparison schema file, reached through the fixture directory the
// registered command passes (D-14).
auto schemaOf(const std::string& data) -> std::string
{
  return data + "/../../docs/schema/compare-v1.json";
}

// T044, FR-057, FR-058, D-14: `--json=` in compare mode writes one document,
// and the harness checker of the report format answers no disagreement
// between those bytes and `docs/schema/compare-v1.json`. That checker
// compares the key set, the key order and the value kinds at every object,
// so this case also owns the E-10 key order of the written bytes. The table
// stands beside the document, so the write adds nothing to either stream.
auto theWrittenDocumentPassesTheComparisonSchema(const std::string& data)
    -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender,
                               std::string("--json=") + kSchemaDocument});

  check(run.status == 0,
        "the run that writes the document answers status 0 (FR-057, SC-001)");
  check(run.err.empty(),
        "the document write prints nothing on standard error (SC-005)");
  check(linesOf(run.out).size() == 4,
        "the table stands beside the written document, so the write adds no "
        "line and runs no benchmark (FR-053, SC-005)");

  const std::string document = sg::test::readText(kSchemaDocument);
  if (document.empty()) {
    std::fprintf(
        stderr, "the document at '%s' holds no bytes\n", kSchemaDocument);
  }
  check(!document.empty(),
        "the mode wrote a document at the --json= path (FR-057)");

  const auto answer = sg::detail::checkReport(schemaOf(data), document);
  if (!answer.has_value()) {
    std::fprintf(stderr,
                 "checkReport answered '%s' over:\n%s\n",
                 answer.error().c_str(),
                 document.c_str());
  }
  check(answer.has_value(),
        "the written bytes hold no disagreement with the key set, the key "
        "order and the value kinds of docs/schema/compare-v1.json (FR-057, "
        "FR-058, D-14, E-10)");
}

// T044, FR-058, SC-009: the document holds the comparison the call answered
// and nothing that moves between two runs, so two runs of one command write
// the same bytes.
auto twoRunsWriteByteIdenticalDocuments(const std::string& data) -> void
{
  const std::string pair = "--compare=" + data + "/gate_ok_base.json," + data
      + "/gate_ok_contender.json";
  const ModeRun first =
      runMode({pair, std::string("--json=") + kFirstRunDocument});
  const ModeRun second =
      runMode({pair, std::string("--json=") + kSecondRunDocument});

  check(first.status == 0 && second.status == 0,
        "both runs of the one command answer status 0 (FR-058)");

  const std::string firstBytes = sg::test::readText(kFirstRunDocument);
  const std::string secondBytes = sg::test::readText(kSecondRunDocument);
  if (firstBytes.empty() || firstBytes != secondBytes) {
    std::fprintf(stderr,
                 "first run wrote %zu bytes, second run wrote %zu\n",
                 firstBytes.size(),
                 secondBytes.size());
  }
  check(!firstBytes.empty() && firstBytes == secondBytes,
        "the two runs wrote the same bytes at two paths (FR-058, SC-009)");
}

// T044, FR-057, E-10, spec D-22: every entry of the document carries its
// state word, and a value the entry leaves unanswered stands `null`, the
// report format's unanswered form. The fit pair gives both shapes in one
// document: the readable benchmark answers `OK` with four values, the
// complexity fit entry answers `SKIPPED` with four nulls. The written rule
// travels the `rules` array in the text the option gave.
auto theDocumentCarriesStateWordsAndNulls(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_fit_base.json";
  const std::string contender = data + "/gate_fit_contender.json";
  const ModeRun run = runMode({"--compare-rule=time@median:lower:5%:bmGate",
                               "--compare=" + baseline + "," + contender,
                               std::string("--json=") + kFitDocument});

  check(run.status == 0,
        "the run that writes the fit document answers status 0 (FR-057)");

  const std::string document = sg::test::readText(kFitDocument);
  check(containsAll(document,
                    {"\"rules\":[\"time@median:lower:5%:bmGate\"]",
                     "\"state\":\"OK\"",
                     "\"state\":\"SKIPPED\"",
                     "\"baseline\":null,\"contender\":null,"
                     "\"changePercent\":null,\"changeAbsolute\":null"}),
        "the document carries the written rule, the two state words, and the "
        "four nulls of the entry that answers no value (FR-057, E-10, "
        "spec D-20)");

  const auto answer = sg::detail::checkReport(schemaOf(data), document);
  if (!answer.has_value()) {
    std::fprintf(stderr,
                 "checkReport answered '%s' over:\n%s\n",
                 answer.error().c_str(),
                 document.c_str());
  }
  check(answer.has_value(),
        "the nulls of the unanswered values stand inside the kinds the "
        "schema states for them (FR-057, D-14)");
}

// T044, FR-058, SC-009: the document holds the comparison and nothing that
// differs between two machines or two runs, so no date, host or pid stands
// in it. The schema leaves the top-level object open, so the checker lets
// such a key through and this case owns its absence.
auto theDocumentCarriesNoDateHostOrPid(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_ok_base.json";
  const std::string contender = data + "/gate_ok_contender.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender,
                               std::string("--json=") + kPlainDocument});

  check(run.status == 0, "the run answers status 0 (FR-058, SC-001)");
  const std::string document = sg::test::readText(kPlainDocument);
  check(containsAll(document,
                    {"\"schemaVersion\"",
                     "\"speedgun\"",
                     "\"inputs\"",
                     "\"rules\"",
                     "\"entries\"",
                     "\"verdict\""}),
        "the document the byte-rule reads holds the six keys of E-10, so the "
        "absence below is read over a whole document (FR-057, E-10)");
  for (const char* key :
       {"\"date\"", "\"host\"", "\"pid\"", "\"timestamp\"", "\"user\""})
  {
    if (document.find(key) != std::string::npos) {
      std::fprintf(
          stderr, "the document carries %s:\n%s\n", key, document.c_str());
    }
    check(document.find(key) == std::string::npos,
          "the document carries no date, host or pid key beside the six the "
          "schema lists (FR-058, SC-009)");
  }
}

// T042, SC-006, FR-044: the guard of D-11 reads the warning the contender
// carries and marks the row, and a marked row leaves the run passing. The
// table prints why beside the state word, naming which of the three
// conditions fired, and the two value columns and the two change columns
// stand where the pass pair leaves them.
auto theNoisyPairAnswersZeroWithItsReason(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_noisy_base.json";
  const std::string contender = data + "/gate_noisy_contender.json";
  const ModeRun run = runMode({"--compare=" + baseline + "," + contender});

  check(run.status == 0,
        "a NOISY row leaves the run passing while the strict setting stands "
        "off (SC-006, FR-044, FR-052)");
  check(run.err.empty(),
        "the noisy pair prints nothing on standard error (SC-005)");

  const std::vector<std::string> lines = linesOf(run.out);
  if (lines.size() != 4) {
    dump("the noisy pair", run);
  }
  check(lines.size() == 4, "the noisy pair answers the table and nothing else");
  if (lines.size() != 4) {
    return;
  }
  check(lines[3].rfind("bmGate", 0) == 0
            && containsAll(lines[3],
                           {"time@median:lower:5%",
                            "1000.000 ns",
                            "1030.000 ns",
                            "+3.000%",
                            "+30.000 ns"}),
        "the noisy row carries the default rule, the two values and both "
        "change columns of the pass row, so the guard changes no number "
        "(FR-044)");

  const std::size_t stateAt = lines[3].find("NOISY");
  const std::size_t reasonAt = lines[3].find("reason=");
  check(stateAt != std::string::npos,
        "the carried warning marks the row NOISY (SC-006, FR-041)");
  check(reasonAt != std::string::npos,
        "the table prints the reason beside the state word (FR-044)");
  if (stateAt == std::string::npos || reasonAt == std::string::npos) {
    return;
  }
  check(stateAt < reasonAt,
        "the reason stands after the state word of the row (FR-044)");
  check(
      lines[3].find("contender report carries a noise warning")
          != std::string::npos,
      "the reason names which of the three conditions fired, here the "
      "warning the contender report carries (SC-006, D-19)");
}

// T042, SC-006, FR-045: `--compare-strict` over that same pair moves the
// verdict alone. The status answers 1, and the table stands byte for byte
// where the run without the option left it, so the word stays `NOISY` and
// the reason stays printed.
auto strictTurnsTheNoisyRowIntoAFailure(const std::string& data) -> void
{
  const std::string baseline = data + "/gate_noisy_base.json";
  const std::string contender = data + "/gate_noisy_contender.json";
  const std::string pair = "--compare=" + baseline + "," + contender;
  const ModeRun plain = runMode({pair});
  const ModeRun strict = runMode({pair, "--compare-strict"});

  check(plain.status == 0,
        "the noisy pair answers status 0 with the option absent (SC-006)");
  check(strict.status == 1,
        "the same pair answers status 1 with --compare-strict (SC-006, "
        "FR-045, FR-052)");
  check(strict.err.empty(),
        "the strict run prints its verdict in the table, so standard error "
        "stays empty (SC-005)");
  check(strict.out == plain.out,
        "the table of the strict run stands byte for byte where the run "
        "without the option left it, so the state word and the reason are "
        "unchanged (FR-044, FR-045)");

  const std::vector<std::string> lines = linesOf(strict.out);
  if (lines.size() != 4) {
    dump("the strict noisy pair", strict);
  }
  check(lines.size() == 4,
        "the strict run answers the table and nothing else (SC-005)");
  if (lines.size() != 4) {
    return;
  }
  check(lines[3].find("NOISY") != std::string::npos
            && lines[3].find("reason=") != std::string::npos,
        "the failing row still prints NOISY with its reason (FR-045, SC-006)");
}

// T055: the contender side of the load fault: the baseline reads and the
// contender does not, and the one line names the contender path (FR-051,
// E-09).
auto aMissingContenderAnswersTheLoadFault(const std::string& data) -> void
{
  const ModeRun run = runMode({"--compare=" + data + "/gate_ok_base.json,"
                               + data + "/gate_absent_contender.json"});
  oneFaultLine(run,
               "the contender document the file system refuses",
               {"gate_absent_contender.json"});
}

// T055: a batch filter that is no regular expression faults before the
// pair is read, the same line the run path answers (D-12, FR-050,
// FR-051).
auto anInvalidBatchFilterAnswersTheFilterFault(const std::string& data) -> void
{
  const ModeRun run = runMode({"--compare=" + data + "/gate_ok_base.json,"
                                   + data + "/gate_ok_contender.json",
                               "--filter=["});
  oneFaultLine(run,
               "the batch filter that is no regular expression",
               {"--filter", "invalid regular expression"});
}

// T055: a comparison path the open accepts and the file system refuses
// on the write: /dev/full opens and refuses every byte, so the mode
// answers the write fault, one line naming the path, and no table
// (FR-074, E-10).
auto aFullDeviceRefusesTheWrittenDocument(const std::string& data) -> void
{
  const ModeRun run = runMode({"--compare=" + data + "/gate_ok_base.json,"
                                   + data + "/gate_ok_contender.json",
                               "--json=/dev/full"});
  check(run.status == 2, "the refused write answers status 2 (FR-074)");
  check(run.out.empty(), "the refused write prints no table (FR-074)");
  const std::vector<std::string> lines = linesOf(run.err);
  if (lines.size() != 1) {
    dump("the write the full device refuses", run);
  }
  check(lines.size() == 1,
        "the refused write answers exactly one line " "(FR-074)");
  if (lines.size() != 1) {
    return;
  }
  check(lines[0].find("/dev/full") != std::string::npos
            && lines[0].find("could not be written") != std::string::npos,
        "that line names the path and the write the file system refused "
        "(FR-074)");
}

auto bmNeverRuns(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  // The registered command passes the fixture directory of test/data, the
  // shape of `test/CMakeLists.txt:614`.
  const std::string data = argc > 1 ? argv[1] : std::string();
  check(!data.empty(), "the registered command passes the fixture directory");

  // SC-005: a registered benchmark makes a fall-through to the run path
  // print a row, which the output-shape checks above own.
  auto handle = sg::registerBenchmark(&bmNeverRuns, "bmNeverRuns");
  static_cast<void>(handle);

  thePassPairAnswersZeroWithOneOkRow(data);
  twoWrittenRulesAnswerTwoRowsInFlagOrder(data);
  theFailPairAnswersOneWithARegressedRow(data);
  aFilterMatchingNothingAnswersZeroWithOneLine(data);
  aScopedRuleAnswersOneRowAndTheFitEntry(data);
  aScopedRuleMatchingNothingAnswersTheNoMatchFault(data);
  aCompareWithNoPairAnswersTheUsageFault(data);
  aRuleWithABareThresholdAnswersTheThresholdFault(data);
  aRuleFilterReachingNothingAnswersTheNoMatchFault(data);
  aMinorSchemaVersionPairAnswersAnOkRow(data);
  aForeignSchemaMajorAnswersTheLoadFault(data);
  aMissingDocumentAnswersTheLoadFault(data);
  aRelativeRuleOverAZeroBaselineAnswersTheZeroFault(data);
  aRefusedComparisonDocumentAnswersTheWriteFault(data);
  theWrittenDocumentPassesTheComparisonSchema(data);
  twoRunsWriteByteIdenticalDocuments(data);
  theDocumentCarriesStateWordsAndNulls(data);
  theDocumentCarriesNoDateHostOrPid(data);
  theNoisyPairAnswersZeroWithItsReason(data);
  strictTurnsTheNoisyRowIntoAFailure(data);
  aMissingContenderAnswersTheLoadFault(data);
  anInvalidBatchFilterAnswersTheFilterFault(data);
  aFullDeviceRefusesTheWrittenDocument(data);

  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
