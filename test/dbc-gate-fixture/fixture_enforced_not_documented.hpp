#ifndef SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_ENFORCED_NOT_DOCUMENTED_HPP
#define SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_ENFORCED_NOT_DOCUMENTED_HPP

#include <speedgun-ng/dbc.hpp>

namespace sg::test::dbc::gate_fixture
{

/**
 * @brief Function that enforces a precondition with no documentation of it.
 *
 * This is the enforced-not-documented drift case (FR-028). Pairing gate
 * must detect SG_REQUIRE without a matching \pre section.
 */
inline int enforced_not_documented(int x)
{
  SG_REQUIRE(x > 0, "x > 0");
  return x + 1;
}

}  // namespace sg::test::dbc::gate_fixture

#endif
