#ifndef SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_CLEAN_HPP
#define SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_CLEAN_HPP

#include <speedgun-ng/dbc.hpp>

namespace sg::test::dbc::gate_fixture
{

/**
 * @brief A public function with empty precondition.
 *
 * \pre none
 * \post result >= 0
 */
inline int cleanWithNonePre(int x)
{
  // explicit none for pre; post is enforced
  int result = (x < 0 ? 0 : x);
  SG_ENSURE(result >= 0, "result >= 0");
  return result;
}

/**
 * @brief Fully documented and enforced public function (both gates must pass).
 *
 * \pre x >= 0
 * \post result > x
 */
inline int cleanFull(int x)
{
  SG_REQUIRE(x >= 0, "x >= 0");
  int result = x + 1;
  SG_ENSURE(result > x, "result > x");
  return result;
}

/**
 * @brief Public function with both contracts explicitly none (empty
 * set).
 *
 * \pre none
 * \post none
 */
inline void cleanBothNone()
{
  // no SG_* required for explicit none
}

}  // namespace sg::test::dbc::gate_fixture

#endif
