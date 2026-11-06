// ============================================================================
// The two counter sources the `hwlat` method reads
// (specs/048-hwlat-probe T007, T008, T009, T010): the vendor gate of
// `proc/cpuinfo`, the register node `dev/cpu/<cpu>/msr` with its
// system-management-interrupt counter, and the `NMI` count of one cpu in
// the `proc/interrupts` text (FR-007, FR-009, FR-010, FR-011, D-3, D-4,
// P-1, P-2). This unit owns the only kernel interface reads of the
// feature. No path here loads a module, changes a module parameter, or
// opens a node for write (FR-008, D-3).
// ============================================================================

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "detail/latency_counters.hpp"

#include <fcntl.h>
#include <unistd.h>

#include "../detail/proc_root.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// The one vendor text whose part documents the counter (FR-010, D-4, P-4).
// The gate names no other vendor and no second counter, so a part that
// states none gets no substitute (D-4, FR-008).
constexpr std::string_view kSmiVendorId = "GenuineIntel";

// The key the cpuinfo text carries the vendor under (FR-010).
constexpr std::string_view kVendorKey = "vendor_id";

// The label of the row the non-maskable-interrupt count stands on (FR-009).
constexpr std::string_view kNmiLabel = "NMI";

// The blanks the two kernel files carry around each field.
constexpr std::string_view kBlanks = " \t\r";

// The node path of one cpu, the path spec D-3 names and the seam resolves
// under the file root (FR-001, FR-007).
auto nodePathOf(std::uint32_t cpu) -> std::string
{
  return "dev/cpu/" + std::to_string(cpu) + "/msr";
}

// One file under the file root, read whole: the shape `readTracerFile`
// stands in the tracer unit, since `readUnderRoot` stands undeclared in
// every header. A file that stands missing or unreadable answers nothing,
// and a read that cannot carry the file to its end answers nothing, the
// not-available shape (FR-001, FR-011).
auto readRootFile(const std::string& relative) -> std::optional<std::string>
{
  std::ifstream file(procRootPath(relative));
  if (!file) {
    return std::nullopt;
  }
  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line;
    text += '\n';
  }
  if (!file.eof()) {
    return std::nullopt;
  }
  return text;
}

// One text with the blanks at either end dropped: a text that holds
// nothing beside blanks answers the empty view.
auto trimmedOf(std::string_view text) -> std::string_view
{
  const auto begin = text.find_first_not_of(kBlanks);
  if (begin == std::string_view::npos) {
    return {};
  }
  const auto end = text.find_last_not_of(kBlanks);
  return text.substr(begin, end - begin + 1);
}

// The value one kernel text states for one key: the text behind the colon
// of the key's line, the shape `vendor_id\t: <text>` carries (FR-010). A
// text that states no such line, and a line that states no value, each
// answer nothing.
auto valueOfKey(const std::string& text,
                std::string_view key) -> std::optional<std::string>
{
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    const auto colon = line.find(':');
    const auto head = trimmedOf(std::string_view(line).substr(0, colon));
    if (head != key) {
      continue;
    }
    return std::string(trimmedOf(std::string_view(line).substr(colon + 1)));
  }
  return std::nullopt;
}

// The row one kernel text carries under one label: the line whose text
// before the colon reads the label, the alignment the file gives each
// label (FR-009). A line that carries no colon names no row, and a text
// that holds no such line answers false.
auto rowOfLabel(const std::string& text,
                std::string_view label,
                std::string& row) -> bool
{
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    if (trimmedOf(std::string_view(line).substr(0, colon)) != label) {
      continue;
    }
    row = line;
    return true;
  }
  return false;
}

// One unsigned figure off one token: a token that parses as no number, or
// that carries text beside the number, answers nothing (FR-011).
auto figureOfToken(std::string_view token) -> std::optional<std::uint64_t>
{
  std::uint64_t figure = 0;
  const char* begin = token.data();
  const char* end = begin + token.size();
  const auto result = std::from_chars(begin, end, figure);
  if (result.ec != std::errc {} || result.ptr != end) {
    return std::nullopt;
  }
  return figure;
}

// The figure of one column of one row: the tokens behind the colon stand
// in the file's own order, the first naming column zero (FR-009). A row
// that names fewer columns than the cpu asks for answers nothing (FR-011).
auto figureAtColumn(const std::string& row,
                    std::uint32_t cpu) -> std::optional<std::uint64_t>
{
  std::istringstream fields(row.substr(row.find(':') + 1));
  std::string token;
  for (std::uint32_t column = 0; column <= cpu; ++column) {
    if (!(fields >> token)) {
      return std::nullopt;
    }
  }
  return figureOfToken(token);
}

// The figure the counter bytes carry: the bytes stand least significant
// first, the form the node answers (FR-007).
auto figureOfBytes(const std::array<std::byte, kSmiCountBytes>& bytes)
    -> std::uint64_t
{
  std::uint64_t figure = 0;
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    figure |= std::to_integer<std::uint64_t>(bytes.at(index)) << (8U * index);
  }
  return figure;
}

// A contract-only helper: a supported answer stands a cpuinfo text that
// names both the vendor key and the vendor word (FR-010, D-4).
[[maybe_unused]] auto namesVendorText(const std::optional<std::string>& text)
    -> bool
{
  return text.has_value() && text->find(kVendorKey) != std::string::npos
      && text->find(kSmiVendorId) != std::string::npos;
}

// A contract-only helper: an answered figure stands among the tokens of
// the row the reader read (FR-009).
[[maybe_unused]] auto rowNamesFigure(
    const std::string& row, std::optional<std::uint64_t> figure) -> bool
{
  return !figure.has_value()
      || row.find(std::to_string(*figure)) != std::string::npos;
}

}  // namespace

auto smiSourceSupported() -> bool
{
  const auto text = readRootFile("proc/cpuinfo");
  std::optional<std::string> vendor;
  if (text.has_value()) {
    vendor = valueOfKey(*text, kVendorKey);
  }
  const bool answer = vendor.has_value() && *vendor == kSmiVendorId;
  SG_ENSURE(!answer || namesVendorText(text),
            "smiSourceSupported answers supported on a cpuinfo text that "
            "names the vendor key beside the vendor word");
  return answer;
}

auto openMsrNode(const std::uint32_t cpu) -> std::optional<int>
{
  const auto node = procRootPath(nodePathOf(cpu));
  const int descriptor = ::open(node.c_str(), O_RDONLY | O_CLOEXEC);
  std::optional<int> answer;
  if (descriptor >= 0) {
    answer = descriptor;
  }
  SG_ENSURE(!answer.has_value() || *answer >= 0,
            "openMsrNode answers no descriptor, or a descriptor the open "
            "stood behind");
  return answer;
}

auto closeMsrNode(const int descriptor) -> void
{
  SG_REQUIRE(descriptor >= 0, "closeMsrNode reaches a descriptor that is none");
  static_cast<void>(::close(descriptor));
}

auto smiCountOf(const int descriptor) -> std::optional<std::uint64_t>
{
  SG_REQUIRE(descriptor >= 0, "smiCountOf reaches a descriptor that is none");
  std::array<std::byte, kSmiCountBytes> bytes {};
  const auto read = ::pread(descriptor,
                            bytes.data(),
                            kSmiCountBytes,
                            static_cast<off_t>(kSmiCountAddress));
  const bool stood = read == static_cast<ssize_t>(kSmiCountBytes);
  std::optional<std::uint64_t> answer;
  if (stood) {
    answer = figureOfBytes(bytes);
  }
  SG_ENSURE(answer.has_value() == stood,
            "smiCountOf answers a figure on one read of the counter width, "
            "and nothing beside a short or failed read");
  return answer;
}

auto nmiCountOfText(const std::string& text,
                    const std::uint32_t cpu) -> std::optional<std::uint64_t>
{
  SG_REQUIRE(text.find('\0') == std::string::npos,
             "nmiCountOfText reaches a text that is not one whole text");
  std::string row;
  std::optional<std::uint64_t> answer;
  if (rowOfLabel(text, kNmiLabel, row)) {
    answer = figureAtColumn(row, cpu);
  }
  SG_ENSURE(rowNamesFigure(row, answer),
            "nmiCountOfText answers a figure the NMI row names in the "
            "column of the cpu");
  return answer;
}

}  // namespace sg::detail
