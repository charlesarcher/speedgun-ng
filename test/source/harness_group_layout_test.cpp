// ============================================================================
// TDD test for the group layout reader (specs/039-context-groups T007 and
// T008; FR-012 to FR-018, FR-021, B-1 and A-1 to A-6 of the layout
// contract).
//
// The reader is one file read: a path and a context count in, a partition or
// one refusal line out. No scenario installs a machine shape, reads the host
// topology, starts a thread or reads a clock, so this file carries no
// topology seam at all (FR-036, R-5).
//
// Each case writes one document to a private path under the system temp
// directory, named by this process number, and removes it. Every refusal
// answer carries no partition (A-1) and states one line naming the path, and
// the entry the fault stands in (FR-018, FR-021, A-3). Every accept case
// asserts the names, the ascending slots and the document order the read
// answers (FR-012 to FR-015, A-6).
//
// Table C of contracts/group-layout.md stands covered row by row:
//   unreadable path                     absent path, spelled with `$HOME`
//   unparsable document                 the reader's own reason
//   root mapping with no `groups`       plus the empty document
//   root node that is a sequence        one case
//   empty `groups` sequence             one case
//   entry that is not a mapping         the index names it (A-3)
//   entry with no `slots`               plus a `slots` value that is no list
//   empty `slots` sequence              one case
//   a `slots` member of the wrong kind  a text, part of a text, and a
//                                       nested member
//   member outside the context range    the line names the value
//   a repeated slot                     the line names the slot
//   a context no entry names            the line names the slot
//   the key `cpus`                      the reason this release groups by slots
//   any key outside the grammar         one at an entry, one at the root
//   repeated group name                 the line names the name
//   a `name` outside the set            long, bad character, empty, nested,
//                                       and one the reader trimmed nowhere
//   a second document (FR-017)          the reason, before any entry reads
// ============================================================================

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>

#include "detail/internal.hpp"

namespace
{

using Partition = std::vector<sg::detail::ContextGroup>;
using Answer = std::expected<Partition, std::string>;
using Shape = std::vector<std::pair<std::string, std::vector<std::size_t>>>;

constexpr std::size_t kOne = 1;
constexpr std::size_t kTwo = 2;
constexpr std::size_t kFour = 4;

// The 33 letters stand one past the length FR-015 allows; the 32 letters
// stand at it.
const std::string kTooLong(33, 'a');
const std::string kAtLimit(32, 'a');

auto fail(const std::string& what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what.c_str());
  std::exit(1);
}

auto check(bool cond, const std::string& what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The private path of one case: the system temp directory, this process
// number and the case label, so two runs never share one document.
auto layoutPath(const std::string& label) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_group_layout_" + std::to_string(static_cast<long>(getpid()))
         + "_" + label + ".yaml");
}

auto writeLayout(const std::string& label,
                 const std::string& text) -> std::filesystem::path
{
  const auto path = layoutPath(label);
  std::ofstream file(path);
  file << text;
  file.close();
  return path;
}

auto readLayout(const std::filesystem::path& path,
                std::size_t contexts) -> Answer
{
  return sg::detail::parseGroupLayout(path.string(), contexts);
}

// The partition an answer carries, spelled for a failure message.
auto spelling(const Answer& answer) -> std::string
{
  if (!answer.has_value()) {
    return "no partition";
  }
  std::string text;
  for (const auto& group : *answer) {
    text += group.name + "=[";
    for (const auto slot : group.slots) {
      text += std::to_string(slot) + ",";
    }
    text += "] ";
  }
  return text;
}

// An accepted document: the partition carries the expected groups in the
// document's own order, each name as the entry spells it or as `g` plus the
// index, and each slot list ascending (FR-012 to FR-015, A-2, A-6).
auto checkPartition(const std::string& label,
                    const std::string& text,
                    std::size_t contexts,
                    const Shape& expected) -> void
{
  const auto path = writeLayout(label, text);
  const auto answer = readLayout(path, contexts);
  check(answer.has_value(),
        "the document stands accepted (FR-012, A-2): " + label + " answers "
            + (answer.has_value() ? spelling(answer) : answer.error()));
  if (!answer.has_value()) {
    return;
  }
  check(answer->size() == expected.size(),
        "one group per entry (FR-012): " + label + " answers "
            + spelling(answer));
  const auto count =
      answer->size() < expected.size() ? answer->size() : expected.size();
  for (std::size_t index = 0; index < count; ++index) {
    const auto& group = (*answer)[index];
    const auto& [name, slots] = expected[index];
    check(group.name == name,
          "group " + std::to_string(index) + " of " + label + " names '" + name
              + "' (FR-015, A-6): " + spelling(answer));
    check(
        group.slots == slots,
        "group " + std::to_string(index) + " of " + label
            + " holds its slots ascending (FR-013, A-6): " + spelling(answer));
  }
  std::filesystem::remove(path);
}

// A refused document: the answer carries no partition (A-1) and states one
// line naming the path (FR-018, FR-021) and everything else the fault row
// names (A-3).
auto checkFaultLine(const std::string& label,
                    const std::filesystem::path& path,
                    const Answer& answer,
                    const std::vector<std::string>& needles) -> void
{
  check(!answer.has_value(),
        "the document stands refused (FR-018): " + label + " answers "
            + spelling(answer));
  if (answer.has_value()) {
    return;
  }
  const auto& line = answer.error();
  check(line.find('\n') == std::string::npos,
        "the refusal states one line (FR-018, FR-021): " + line);
  check(line.find(path.string()) != std::string::npos,
        "the refusal names the layout path (FR-021): " + line);
  for (const auto& needle : needles) {
    check(line.find(needle) != std::string::npos,
          "the refusal of " + label + " names [" + needle
              + "] (FR-021, A-3): " + line);
  }
}

auto checkFault(const std::string& label,
                const std::string& text,
                std::size_t contexts,
                const std::vector<std::string>& needles) -> void
{
  const auto path = writeLayout(label, text);
  checkFaultLine(label, path, readLayout(path, contexts), needles);
  std::filesystem::remove(path);
}

auto run() -> int
{
  // ---- The accept set (FR-012 to FR-015, FR-021, A-2, A-6) ----

  // One entry per group, each named: the grammar's basic shape, the partition
  // in the document's order with the names the file spells (FR-012, FR-015).
  checkPartition("explicit-names",
                 "groups:\n"
                 "  - name: fast\n"
                 "    slots: [0, 1]\n"
                 "  - name: slow\n"
                 "    slots: [2, 3]\n",
                 kFour, {{"fast", {0, 1}}, {"slow", {2, 3}}});

  // An entry that names none takes `g` plus its zero-based document index
  // (FR-015, D-4).
  checkPartition("default-names",
                 "groups:\n"
                 "  - slots: [0, 2]\n"
                 "  - slots: [1, 3]\n",
                 kFour, {{"g0", {0, 2}}, {"g1", {1, 3}}});

  // One entry covering every context stands legal and names the run's one
  // group (FR-012, D-9).
  checkPartition("one-group-every-context",
                 "groups:\n"
                 "  - name: all\n"
                 "    slots: [0, 1, 2, 3]\n",
                 kFour, {{"all", {0, 1, 2, 3}}});

  // A slot list the file wrote out of order comes back ascending, and the
  // groups keep the document's order (FR-013, A-6).
  checkPartition("unsorted-slots",
                 "groups:\n"
                 "  - name: even\n"
                 "    slots: [2, 0]\n"
                 "  - name: odd\n"
                 "    slots: [3, 1]\n",
                 kFour, {{"even", {0, 2}}, {"odd", {1, 3}}});

  // Two names that differ by case stand distinct: the reader folds no case
  // (FR-015, A-4).
  checkPartition("case-distinct-names",
                 "groups:\n"
                 "  - name: fast\n"
                 "    slots: [0]\n"
                 "  - name: Fast\n"
                 "    slots: [1]\n",
                 kTwo, {{"fast", {0}}, {"Fast", {1}}});

  // The characters FR-015 allows, and its upper length, stand accepted.
  checkPartition("name-at-the-limit",
                 "groups:\n"
                 "  - name: Ab9._-\n"
                 "    slots: [0]\n"
                 "  - name: " +
                     kAtLimit + "\n"
                     "    slots: [1]\n",
                 kTwo, {{"Ab9._-", {0}}, {kAtLimit, {1}}});

  // Above one context the read answers: one context, one entry, one slot
  // (B-1, FR-013).
  checkPartition(
      "lone-context", "groups:\n  - slots: [0]\n", kOne, {{"g0", {0}}});

  // ---- The fault rows of table C (FR-018, FR-021, A-1, A-3) ----

  // Row 1, an unreadable path. The path spells a shell variable the reader
  // expands nowhere (FR-018, A-4).
  {
    const auto absent =
        std::filesystem::path("$HOME") / "absent-group-layout.yaml";
    checkFaultLine(
        "absent-path", absent, readLayout(absent, kFour), {"cannot open"});
  }

  // Row 2, a document the reader rejects: the reader's own reason travels
  // beside the path.
  checkFault("unparsable-document",
             "groups: [\n",
             kFour,
             {"end of sequence flow not found"});

  // Row 3, a root mapping with no `groups` key, the `groups` value that is no
  // sequence, and the empty document that names no key at all.
  checkFault("root-without-groups", "mode: fast\n", kFour, {"groups"});
  checkFault("groups-not-a-sequence", "groups: 5\n", kFour, {"groups"});
  checkFault("empty-document", "", kFour, {"groups"});

  // Row 4, a root node that is a sequence.
  checkFault("root-sequence", "- slots: [0]\n", kOne, {"groups"});

  // Row 5, an empty `groups` sequence.
  checkFault("empty-groups", "groups: []\n", kFour, {"empty"});

  // Row 6, an entry that is not a mapping: the index names it, the entry
  // having no name (A-3).
  checkFault("entry-not-a-mapping",
             "groups:\n  - 42\n",
             kOne,
             {"entry 0", "no mapping"});

  // Row 7, an entry with no `slots` key, and the `slots` value that is no
  // sequence: each leaves the entry holding no slots.
  checkFault("entry-without-slots",
             "groups:\n  - name: slow\n",
             kOne,
             {"entry 'slow'", "slots"});
  checkFault("slots-not-a-sequence",
             "groups:\n  - name: slow\n    slots: 5\n",
             kOne,
             {"entry 'slow'", "slots"});

  // Row 8, an empty `slots` sequence.
  checkFault("empty-slots",
             "groups:\n  - name: slow\n    slots: []\n",
             kOne,
             {"entry 'slow'", "empty"});

  // Row 9, a `slots` member of the wrong kind: a text that is no number, a
  // text a number only partly fills, and a member that is no text. The partly
  // numeric text parses as a position and leaves characters behind, so it
  // names none.
  checkFault("bad-slot-text",
             "groups:\n  - name: mixed\n    slots: [0, x]\n",
             kFour,
             {"entry 'mixed'", "no context position"});
  checkFault("bad-slot-partial",
             "groups:\n  - name: mixed\n    slots: [0, 1x]\n",
             kFour,
             {"entry 'mixed'", "no context position"});
  checkFault("bad-slot-kind",
             "groups:\n  - name: mixed\n    slots: [[1], 0]\n",
             kFour,
             {"entry 'mixed'", "no context position"});

  // Row 10, a member outside the context range: the line names the value.
  checkFault("slot-out-of-range",
             "groups:\n  - name: wide\n    slots: [0, 5]\n",
             kTwo,
             {"entry 'wide'", "context 5"});

  // Row 11, a repeated slot: the line names the slot.
  checkFault("repeated-slot",
             "groups:\n"
             "  - name: a\n"
             "    slots: [0, 1]\n"
             "  - name: b\n"
             "    slots: [1, 2]\n",
             3, {"context 1"});

  // Row 12, a context no entry names: the line names the slot.
  checkFault("unnamed-context",
             "groups:\n"
             "  - name: a\n"
             "    slots: [0, 1]\n"
             "  - name: b\n"
             "    slots: [2]\n",
             kFour, {"context 3"});

  // Row 13, the key `cpus`: the line names the key, the entry and the reason
  // this release groups by slots (FR-014, D-6).
  checkFault("cpus-key",
             "groups:\n"
             "  - name: fast\n"
             "    cpus: [3]\n"
             "    slots: [0, 1]\n",
             kFour, {"entry 'fast'", "'cpus'", "slots"});

  // Row 14, a key outside the grammar, at an entry and at the root.
  checkFault("foreign-entry-key",
             "groups:\n"
             "  - name: fast\n"
             "    slots: [0, 1]\n"
             "    nodes: [0]\n",
             kFour, {"entry 'fast'", "'nodes'"});
  checkFault("foreign-root-key",
             "groups:\n  - name: all\n    slots: [0, 1]\nmode: fast\n",
             kTwo,
             {"'mode'"});

  // Row 15, a repeated group name: the line names the name.
  checkFault("repeated-name",
             "groups:\n"
             "  - name: fast\n"
             "    slots: [0]\n"
             "  - name: fast\n"
             "    slots: [1]\n",
             kTwo, {"'fast'", "repeated"});

  // Row 16, a `name` outside the characters or the length: one letter past
  // the bound, a letter outside the set, and an entry that names nothing.
  checkFault("too-long-name",
             "groups:\n  - name: " + kTooLong + "\n    slots: [0]\n",
             kOne,
             {kTooLong, "length"});
  checkFault("bad-char-name",
             "groups:\n  - name: fast/slow\n    slots: [0]\n",
             kOne,
             {"fast/slow", "characters"});
  checkFault("empty-name",
             "groups:\n  - name:\n    slots: [0]\n",
             kOne,
             {"entry 0", "characters"});

  // A nested `name` value carries no text, and a quoted text keeps the spaces
  // around it: the reader trims nothing (FR-015, FR-017, A-4).
  checkFault("nested-name",
             "groups:\n  - name:\n      inner: 1\n    slots: [0]\n",
             kOne,
             {"entry 0", "characters"});
  checkFault("untrimmed-name",
             "groups:\n  - name: \" fast \"\n    slots: [0]\n",
             kOne,
             {"' fast '", "characters"});

  // FR-017 bars a second document: the reader answers the path and its reason
  // before any entry reads.
  checkFault("second-document",
             "groups:\n  - slots: [0]\n---\ngroups:\n  - slots: [1]\n",
             kTwo,
             {"second document"});

  std::printf("harness_group_layout_test PASS\n");
  return 0;
}

}  // namespace

auto main() -> int
{
  try {
    return run();
  } catch (...) {
    std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: unhandled exception\n");
    return 1;
  }
}
