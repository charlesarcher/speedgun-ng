// ============================================================================
// specs/039-context-groups T008: the group layout reader (FR-012 to FR-017,
// FR-021, R-3, contracts/group-layout.md B-1, A-1 to A-6).
//
// The grammar is the whole vocabulary: one document, one root mapping, one
// `groups` sequence, one entry per group holding a `name` and a `slots` list
// (FR-012, FR-017). yaml-cpp stands in this file beside the gate unit, the
// way topology.cpp carries hwloc alone: no header under `include/` names it,
// and the specs/006 purity scan and nm proof keep their readings (FR-016).
//
// Every fault answers one line, the error of the answer: the path as the
// caller spelled it, the entry the fault stands in by its name where it has
// one, and the reason (FR-018, FR-021, A-3). The partition travels as the
// value, so a fault leaves none behind (A-1) and nothing throws past this
// file. The document's bytes arrive through the harness's own file reader, so
// a path that opens and then fails to read answers that line with the reason
// that read answered, the shape `--options` keeps for the same path. The
// reader trims no text, folds no case, expands no path and reads no
// environment variable (A-4).
// ============================================================================

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <expected>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{
namespace detail
{
namespace
{

// The keys the grammar knows: the root's one key, an entry's two keys, and
// the key this release refuses by name (FR-012, FR-014, FR-017).
constexpr std::string_view kGroupsKey = "groups";
constexpr std::string_view kNameKey = "name";
constexpr std::string_view kSlotsKey = "slots";
constexpr std::string_view kCpusKey = "cpus";

// FR-015, D-4: the prefix of a name an entry left unstated, `g` plus the
// zero-based document index.
constexpr std::string_view kDefaultNamePrefix = "g";

// FR-015: the characters a name may carry, and the length it stands within.
constexpr std::string_view kNameCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
constexpr std::size_t kNameMax = 32;

// The one line a fault answers: the layout path, the entry the fault stands
// in where one stands, and the reason (FR-021, A-3).
auto refusal(const std::string& path,
             const std::string& speaker,
             const std::string& reason) -> std::string
{
  if (speaker.empty()) {
    return "group layout " + path + ": " + reason;
  }
  return "group layout " + path + ": " + speaker + " " + reason;
}

// FR-015: the characters and the length of one group name, read off the text
// the reader produced for the node. A nested value carries no text and so
// stands outside the rule (FR-017).
[[nodiscard]] auto nameHolds(const std::string& text) -> bool
{
  return !text.empty() && text.size() <= kNameMax
      && text.find_first_not_of(kNameCharacters) == std::string::npos;
}

// FR-013: the context position one `slots` member names. A member that is no
// text carries no text at all, so a nested member, a text that is no number
// and a text a number only partly fills all answer no position (FR-018,
// table C row 9). A number no `size_t` can hold answers the same line: no
// context of any run carries such a position.
[[nodiscard]] auto slotOf(const YAML::Node& member)
    -> std::optional<std::size_t>
{
  const auto& text = member.Scalar();
  std::size_t position = 0;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), position);
  if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  return position;
}

}  // namespace

// FR-012 to FR-017, B-1: the read. The document walks from the root down, so
// the first fault the grammar names answers the line, and the partition only
// travels on the accepted path (A-1, A-2).
auto parseGroupLayout(const std::string& path, const std::size_t contexts)
    -> std::expected<std::vector<ContextGroup>, std::string>
{
  SG_REQUIRE(contexts >= 1, "the reader names the slots of one instance (B-1)");

  // FR-018, table C row 1: the path stands unreadable as it stands, and the
  // line carries the reason the open or the read answered. The bytes come
  // through the harness's own file reader, the one reader that answers a read
  // that fails after a successful open, so the loader's stream reaches no
  // caller and the reader still opens the layout path alone (A-5 of the
  // partition contract, the unreadable-path edge case).
  const auto fileText = readTextFile(path);
  if (!fileText.has_value()) {
    return std::unexpected(refusal(path, "", fileText.error()));
  }

  std::vector<YAML::Node> documents;
  try {
    documents = YAML::LoadAll(*fileText);
  } catch (const YAML::Exception& reader) {
    // FR-018, table C row 2: the reader's own reason travels beside it.
    return std::unexpected(
        refusal(path, "", "the reader refuses the document: " + reader.msg));
  }

  // FR-017: one document, no second one. A file that carries no document
  // names no `groups` key at all.
  if (documents.empty()) {
    return std::unexpected(
        refusal(path, "", "the root holds no 'groups' sequence"));
  }
  if (documents.size() > 1) {
    return std::unexpected(
        refusal(path, "", "the file holds a second document"));
  }

  const YAML::Node root = documents.front();
  if (!root.IsMap()) {
    // FR-012, FR-018, table C row 4: a root that is no mapping names no
    // groups.
    return std::unexpected(
        refusal(path, "", "the root holds no 'groups' sequence"));
  }

  // A key the document left out answers no node at all, and an absent node
  // reads no kind: `groups` stands defined before its kind reads (B-1).
  const YAML::Node groups = root[std::string(kGroupsKey)];
  if (!groups || !groups.IsSequence()) {
    // FR-012, FR-018, table C row 3: the root names no groups, by its key or
    // by its kind.
    return std::unexpected(
        refusal(path, "", "the root holds no 'groups' sequence"));
  }
  for (const auto& pair : root) {
    const std::string key = pair.first.Scalar();
    if (key != kGroupsKey) {
      return std::unexpected(refusal(
          path, "", "the key '" + key + "' stands outside the grammar"));
    }
  }

  if (groups.size() == 0) {
    // FR-012, FR-018, table C row 5: one entry per group, at least one.
    return std::unexpected(
        refusal(path, "", "the 'groups' sequence stands empty"));
  }

  std::vector<ContextGroup> partition;
  std::set<std::string> names;
  std::vector<bool> claimed(contexts, false);

  for (std::size_t index = 0; index < groups.size(); ++index) {
    const YAML::Node entry = groups[index];
    const std::string byIndex = "entry " + std::to_string(index);
    if (!entry.IsMap()) {
      // FR-012, FR-018, table C row 6: an entry with no name stands named by
      // its index (A-3).
      return std::unexpected(refusal(path, byIndex, "is no mapping"));
    }

    // FR-015, D-4: the name the entry spells, or the default of its index.
    std::string name = std::string(kDefaultNamePrefix) + std::to_string(index);
    std::string speaker = byIndex;
    const YAML::Node named = entry[std::string(kNameKey)];
    if (named) {
      const std::string text = named.Scalar();
      if (!nameHolds(text)) {
        // FR-018, table C row 16: the line names the text, and the index
        // names the entry the text cannot (A-3).
        return std::unexpected(
            refusal(path,
                    byIndex,
                    "carries the name '" + text
                        + "', outside the allowed characters and length"));
      }
      name = text;
      speaker = "entry '" + text + "'";
    }
    if (!names.insert(name).second) {
      // FR-015, FR-018, table C row 15: one name stands unique.
      return std::unexpected(
          refusal(path, "", "the name '" + name + "' stands repeated"));
    }

    // FR-014, FR-017, FR-018, table C rows 13 and 14: the keys of one entry.
    for (const auto& pair : entry) {
      const std::string key = pair.first.Scalar();
      if (key == kCpusKey) {
        return std::unexpected(
            refusal(path,
                    speaker,
                    "carries the key 'cpus'; this release " "groups by slots"));
      }
      if (key != kNameKey && key != kSlotsKey) {
        return std::unexpected(
            refusal(path,
                    speaker,
                    "carries the key '" + key + "', outside the grammar"));
      }
    }

    const YAML::Node slots = entry[std::string(kSlotsKey)];
    if (!slots || !slots.IsSequence()) {
      // FR-012, FR-018, table C row 7: one entry names its slots.
      return std::unexpected(
          refusal(path, speaker, "holds no 'slots' sequence"));
    }
    if (slots.size() == 0) {
      // FR-012, FR-018, table C row 8: a group holds at least one slot.
      return std::unexpected(
          refusal(path, speaker, "holds an empty 'slots' sequence"));
    }

    ContextGroup group {name, {}};
    for (const auto& member : slots) {
      const auto position = slotOf(member);
      if (!position.has_value()) {
        // FR-018, table C row 9.
        return std::unexpected(
            refusal(path,
                    speaker,
                    "holds a 'slots' member that is no context position"));
      }
      if (*position >= contexts) {
        // FR-013, FR-018, table C row 10: the line names the value.
        return std::unexpected(
            refusal(path,
                    speaker,
                    "names context " + std::to_string(*position)
                        + ", outside the run's " + std::to_string(contexts)
                        + " contexts"));
      }
      if (claimed[*position]) {
        // FR-013, FR-018, table C row 11: the line names the slot.
        return std::unexpected(refusal(path,
                                       "",
                                       "context " + std::to_string(*position)
                                           + " stands named by two entries"));
      }
      claimed[*position] = true;
      group.slots.push_back(*position);
    }
    // A-6: the group keeps the document's order and its slots ascend.
    std::ranges::sort(group.slots);
    partition.push_back(std::move(group));
  }

  // FR-013, FR-018, table C row 12: the entries cover every context of the
  // instance, and the line names the first one left out.
  for (std::size_t position = 0; position < contexts; ++position) {
    if (!claimed[position]) {
      return std::unexpected(refusal(path,
                                     "",
                                     "context " + std::to_string(position)
                                         + " stands unnamed by every entry"));
    }
  }

  return partition;
}

}  // namespace detail
}  // namespace sg
