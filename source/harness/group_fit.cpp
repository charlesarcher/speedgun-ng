// ============================================================================
// specs/039-context-groups T013, T014: the layout arm of the fit walk
// (FR-019 to FR-021, D-2, D-8, B-3 of contracts/group-partition.md).
//
// The arm runs beside the placement walk at the standing fit checkpoint, after
// the selection and before the first instance. It opens the one file the row
// settled, once, and hands back the single line the entry point prints, so a
// refusal leaves no measured work behind (FR-020). A run no source named a
// layout for reaches no file at all.
// ============================================================================

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{
namespace detail
{

// B-3: the file reads once, at the context count of the first selected
// instance. The reader states its postcondition: an accepted file covers `0`
// to that count exactly once (B-1), so the count doubles as the file's slot
// count, and every other instance of the batch fails against it (FR-020, D-8).
auto groupLayoutFitStatus(const std::string& path,
                          const std::vector<Instance*>& selected)
    -> std::pair<int, std::string>
{
  SG_REQUIRE(!selected.empty(),
             "the entry point reaches the fit walk with a selection (B-3)");
  if (path.empty()) {
    return {};
  }

  const auto contexts = static_cast<std::size_t>(selected.front()->threadCount);
  const auto partition = parseGroupLayout(path, contexts);
  if (!partition.has_value()) {
    // FR-018, FR-021: the reader already spelled the line, and it names the
    // path and the entry the fault stands in.
    return {2, partition.error()};
  }

  for (const auto* instance : selected) {
    if (static_cast<std::size_t>(instance->threadCount) != contexts) {
      // FR-020, FR-021: the batch stops on the first count the file cannot
      // carry, and the line names both counts.
      return {2,
              "--group-layout '" + path + "' names " + std::to_string(contexts)
                  + " slots, '" + instance->name + "' runs "
                  + std::to_string(instance->threadCount) + " contexts"};
    }
    // FR-019, D-2: the file names the run's one partition, and a preset that
    // names a partition for the read in hand names a second. The instance
    // carries the resolved preset, so a name a handle stated answers here as
    // the name the option settled. One context names no partition under any
    // preset (FR-010), so the pairing read stands above one context, where the
    // resolver's plan answers the unit of every position.
    if (instance->threadCount > 1 && instance->placement != Preset::NONE) {
      const auto plan =
          resolvePreset(instance->placement, layout(), instance->threadCount);
      const auto presetPartition =
          groupPartition(instance->placement,
                         plan,
                         static_cast<std::size_t>(instance->threadCount));
      if (!presetPartition.empty()) {
        return {2,
                "--group-layout '" + path
                    + "' cannot combine with --placement='"
                    + std::string(presetText(instance->placement)) + "'"};
      }
    }
  }
  return {};
}

}  // namespace detail
}  // namespace sg
