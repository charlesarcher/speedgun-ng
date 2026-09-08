#ifndef SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_DRIFT_HPP
#define SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_DRIFT_HPP

#include <speedgun-ng/dbc.hpp>

namespace sg::test::dbc::gate_fixture
{

/**
 * @brief Function with documented precondition but no matching enforcement.
 *
 * \pre x > 0
 *
 * This is the documented-not-enforced drift case (FR-028). The doc gate
 * will see the \pre; the pairing gate must report drift (no SG_REQUIRE).
 */
inline int drift(int x)
{
  // Documented \pre but deliberately no SG_REQUIRE here.
  // (Body is header-inline per task requirement.)
  return x + 1;
}

}  // namespace sg::test::dbc::gate_fixture

#endif
