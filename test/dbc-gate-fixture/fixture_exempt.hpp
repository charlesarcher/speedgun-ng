#ifndef SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_EXEMPT_HPP
#define SPEEDGUN_NG_TEST_DBC_GATE_FIXTURE_EXEMPT_HPP

#include <speedgun-ng/dbc.hpp>

namespace sg::test::dbc::gate_fixture
{

class Exempt
{
public:
  // defaulted (exempt)
  Exempt() = default;
  Exempt(const Exempt&) = default;
  Exempt(Exempt&&) = default;
  auto operator=(const Exempt&) -> Exempt& = default;
  auto operator=(Exempt&&) -> Exempt& = default;

  // deleted (exempt)
  ~Exempt() = delete;

  // public with explicit none marker for genuinely empty contract set (FR-030)
  // (no enforcement required; pairing gate must accept explicit "none")
  /**
   * @brief Public entry with genuinely empty contracts.
   *
   * \pre none
   * \post none
   */
  void public_none() {}

  // friend declaration inside class (exempt per FR-029)
  friend void friend_target(int);

private:
  void private_member(int y) { SG_REQUIRE_ALWAYS(y >= 0, "y >= 0"); }

protected:
  void protected_member() {}
};

inline void friend_target(int x)
{
  static_cast<void>(x);
}

constexpr int constexpr_only(int v)
{
  return v * 2;
}

}  // namespace sg::test::dbc::gate_fixture

#endif
