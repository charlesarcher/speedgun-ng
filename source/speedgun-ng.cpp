#include <string>

#include "speedgun-ng/speedgun-ng.hpp"

#include "speedgun-ng/dbc.hpp"

exported_class::exported_class()
    : m_name {"speedgun-ng"}
{
  SG_INVARIANT(!m_name.empty(), "stored name is non-empty");
  SG_ENSURE(m_name == "speedgun-ng", "name() returns the project name");
}

exported_class::~exported_class()
{
  SG_INVARIANT(!m_name.empty(), "stored name is non-empty");
}

auto exported_class::name() const -> char const*
{
  SG_REQUIRE(!m_name.empty(),
             "the object is in a valid state (class invariant)");
  SG_INVARIANT(!m_name.empty(), "stored name is non-empty");
  char const* const result = m_name.c_str();
  SG_ENSURE(result == m_name.c_str(),
            "returns a non-owning pointer to the stored string");
  return result;
}
