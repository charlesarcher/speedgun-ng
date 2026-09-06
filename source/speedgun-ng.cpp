#include <string>

#include "speedgun-ng/speedgun-ng.hpp"

exported_class::exported_class()
    : m_name {"speedgun-ng"}
{
}

auto exported_class::name() const -> char const*
{
  return m_name.c_str();
}
