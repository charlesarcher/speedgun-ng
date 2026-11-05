// The record of which finder filled a published counter limit, and the one
// exported reader of that record (specs/041-counter-probe FR-040a, FR-043,
// FR-050, FR-053, D-3, D-3a).

#include <map>
#include <string>
#include <string_view>

#include "detail/limit_source_record.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{
namespace
{

// The finder that filled each published limit, keyed by the object's
// canonical path (FR-040a). A source that answers no limit records
// nothing, so a path this table misses is a path that publishes no
// source. The figure itself lives in the table `system.cpp` keeps beside
// the tree at `:187-191`, and this table carries the one word that stands
// beside it. The two tables sit apart and share no key, because
// `system.cpp` takes no edit here and the provider writes both from one
// seed (PC-5, FR-043). The table is filled while a provider registers and
// read only after the catalog opens, so no read races a write (FR-009).
auto limitSourceTable() -> std::map<std::string, LimitSource>&
{
  static std::map<std::string, LimitSource> table;
  return table;
}

}  // namespace

auto detail::recordLimitSource(std::string_view objectPath,
                               LimitSource source) -> void
{
  auto& table = limitSourceTable();
  const std::string path(objectPath);
  if (source == LimitSource::NONE) {
    // The writer states that the path publishes no limit, so the path
    // holds no source either: the reader answers `NONE` for it through
    // the miss (FR-040a).
    table.erase(path);
    return;
  }
  table.insert_or_assign(path, source);
}

auto detail::clearLimitSources() -> void
{
  limitSourceTable().clear();
}

auto objectLimitSource(std::string_view objectPath) noexcept -> LimitSource
{
  const auto& table = limitSourceTable();
  const auto recorded = table.find(std::string(objectPath));
  if (recorded == table.end()) {
    return LimitSource::NONE;
  }
  const auto answer = recorded->second;
  SG_ENSURE(answer != LimitSource::NONE,
            "a stored record names the finder that filled the limit (FR-043)");
  return answer;
}

}  // namespace sg::counters
