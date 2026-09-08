#ifndef SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_MISSING_DOCS_HPP
#define SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_MISSING_DOCS_HPP

#include <speedgun-ng/dbc.hpp>

namespace sg::test::dbc::gate_fixture
{

/**
 * @brief Public function deliberately lacking \pre and \post sections.
 *
 * This fixture is used to prove the documentation-presence gate (FR-027).
 * A public interface must document its contracts; absence of \pre/\post
 * must be reported.
 */
inline int missing_docs(int x)
{
  // Intentionally no SG_* enforcement and no contract documentation sections.
  // (If SG were present this would also be an enforced-not-documented case.)
  return x + 1;
}

}  // namespace sg::test::dbc::gate_fixture

#endif
