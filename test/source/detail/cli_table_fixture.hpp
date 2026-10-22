#ifndef SG_TEST_DETAIL_CLI_TABLE_FIXTURE_HPP
#define SG_TEST_DETAIL_CLI_TABLE_FIXTURE_HPP

#include <array>
#include <string_view>

#include <getopt.h>

// ============================================================================
// The fixture twin of the command-line option table, with two defects planted
// in it on purpose (specs/029-hunter-core T020; FR-083; SC-05; research D-21,
// PD-18; contracts/hunt-cli.md).
//
// The uniqueness gate `harness_cli_table_test.cpp` states reads the one
// `option longOptions[]` table of `source/harness/cli.cpp` as text, because
// that table is a function-local `static` array of `speedgunMain`
// (`cli.cpp:346`) no linked test can reach. A gate that has only ever seen a
// correct table proves nothing: the negative control lives here, a twin of that
// table in the same spelling - the `option` row type of `<getopt.h>`, the
// `no_argument` / `required_argument` / `optional_argument` argument kinds, the
// `nullptr` flag field, the terminating `{ nullptr, 0, nullptr, 0 }` row, and
// the value of every hunt row taken from a named `constexpr int` constant the
// way `cli.cpp:66-71` names the comparison constants.
//
// Two defects are planted in it, one each, and each is planted so that only the
// rule it tests can see it:
//
//   * the name `hunt-rank` stands twice, on two rows whose values differ, so
//     the duplicate name is a name defect and nothing else;
//   * `kCliFixtureHuntSeedOption` carries the number the budget row already
//     claims, on rows whose names differ, so the clash is a value defect and
//     nothing else. `getopt_long` answers that mistake by routing `--hunt-seed`
//     into the budget switch arm, which is the bug the value rule exists to
//     catch.
//
// Every other row and value is distinct, so the same two rules report nothing
// else here. No scenario runs `getopt_long` over this table and no scenario
// registers a provider: the twin is data, and the gate reads it in one process
// with no child and no host (FR-003).
//
// The names this fixture states are listed at the bottom of the file.
// ============================================================================

namespace sg::test
{

// The named value constants, in the form `cli.cpp:66-71` states them: a long
// option with no letter of its own takes a number of 256 or more, so it cannot
// collide with a short option. The numbers 256 to 259 are the ones the tree
// already holds and 270 to 277 are the ones contracts/hunt-cli.md assigns the
// hunt family inside the 270 to 279 block the lane reserves (D-22).
inline constexpr int kCliFixtureMetricMethodOption = 256;
inline constexpr int kCliFixtureCompareOption = 257;
inline constexpr int kCliFixtureCompareRuleOption = 258;
inline constexpr int kCliFixtureHuntOption = 270;
inline constexpr int kCliFixtureHuntNormOption = 271;
inline constexpr int kCliFixtureHuntRankOption = 272;
inline constexpr int kCliFixtureHuntTopOption = 273;
inline constexpr int kCliFixtureHuntNoiseOption = 274;
inline constexpr int kCliFixtureHuntRepeatsOption = 275;
inline constexpr int kCliFixtureHuntBudgetOption = 276;

// The planted clash: the seed row reuses the number the budget row claims,
// inside the reserved block. A contributor who copies a hunt row and renumbers
// it makes exactly this mistake, and it is the only value defect here.
inline constexpr int kCliFixtureHuntSeedOption = 276;

// The number the second `hunt-rank` row carries. It stands clear of every
// other row, inside the reserved block, so the duplicate name is reported by
// the name rule alone.
inline constexpr int kCliFixtureDuplicateRowOption = 278;

// The twin of the option table, in the order the real table lists its rows:
// the letter rows first, then the rows whose value is a named constant, then
// the hunt family, then the terminator.
inline constexpr std::array<option, 17> kCliFixtureTable {
    {
        {
            .name = "filter",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = 'f',
        },
        {
            .name = "list",
            .has_arg = no_argument,
            .flag = nullptr,
            .val = 'l',
        },
        {
            .name = "counter",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = 'c',
        },
        {
            .name = "metric",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = 'M',
        },
        {
            .name = "compare",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureCompareOption,
        },
        {
            .name = "compare-rule",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureCompareRuleOption,
        },
        {
            .name = "metric-method",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureMetricMethodOption,
        },
        {
            .name = "hunt",
            .has_arg = optional_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntOption,
        },
        {
            .name = "hunt-norm",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntNormOption,
        },
        {
            .name = "hunt-rank",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntRankOption,
        },
        // The planted duplicate: a row copied to add an option, with its name
        // left as the copy's.
        {
            .name = "hunt-rank",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureDuplicateRowOption,
        },
        {
            .name = "hunt-top",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntTopOption,
        },
        {
            .name = "hunt-noise",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntNoiseOption,
        },
        {
            .name = "hunt-repeats",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntRepeatsOption,
        },
        {
            .name = "hunt-budget",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntBudgetOption,
        },
        {
            .name = "hunt-seed",
            .has_arg = required_argument,
            .flag = nullptr,
            .val = kCliFixtureHuntSeedOption,
        },
        {
            .name = nullptr,
            .has_arg = 0,
            .flag = nullptr,
            .val = 0,
        },
    },
};

// The two defects, stated beside the table so the gate compares what it found
// against what was planted, each spelling written once.
inline constexpr std::string_view kCliFixturePlantedName {"hunt-rank"};
inline constexpr int kCliFixturePlantedValue = kCliFixtureHuntBudgetOption;

// ---------------------------------------------------------------------------
// The names this fixture states, for the task that reads it.
//
// kCliFixtureTable                    the twin of `option longOptions[]`, with
//                                     the planted duplicate name and the
//                                     planted value clash, and the `nullptr`
//                                     terminator row of the real table
// kCliFixtureMetricMethodOption,
// kCliFixtureCompareOption,
// kCliFixtureCompareRuleOption        the numbers the tree already holds
// kCliFixtureHunt*Option              the hunt numbers 270 to 277, with the
//                                     clash planted on the seed row
// kCliFixtureDuplicateRowOption       the number of the duplicated-name row
// kCliFixturePlantedName              the name that stands twice
// kCliFixturePlantedValue             the number that stands twice
//
// A gate reads the table as data, skipping the terminator row whose `name` is
// null, the way the source parse skips it:
//
//   for (const option& row : sg::test::kCliFixtureTable) {
//     if (row.name == nullptr) {
//       continue;
//     }
//     ...
//   }
}  // namespace sg::test

#endif  // SG_TEST_DETAIL_CLI_TABLE_FIXTURE_HPP
