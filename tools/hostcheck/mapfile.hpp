#ifndef SG_TOOLS_HOSTCHECK_MAPFILE_HPP
#define SG_TOOLS_HOSTCHECK_MAPFILE_HPP

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "../../source/counters/detail/pmu.hpp"
#include "../../source/harness/detail/host_record_check.hpp"
#include "../../source/harness/detail/internal.hpp"

/**
 * @brief The mapfile of one tree, the reader that opens it, and the chooser the
 * reader program builds from it (specs/031-host-validation; FR-008, FR-025,
 * plan R-04, R-06).
 *
 * The two programs under `tools/hostcheck/` share this header, because the one
 * mapfile matcher lives in the counters library and the harness library takes
 * no dependency on it: a program that links both may open the mapfile and ask
 * that matcher, and the library reads the answer through the chooser it is
 * handed (FR-008, FR-025).
 */
namespace sg::tools
{

/**
 * @brief The bytes of one tree's mapfile, the one open both programs share: the
 * read answers what it found, and its error names the path it could not read
 * (FR-008, FR-025).
 *
 * \pre `treeRoot` names the tree the run reads
 */
[[nodiscard]] inline auto mapfileText(const std::filesystem::path& treeRoot)
    -> std::expected<std::string, std::string>
{
  return sg::detail::readTextFile(
      (treeRoot
       / std::filesystem::path(std::string(sg::detail::kMapfileRelPath)))
          .string());
}

/**
 * @brief The identity as the matcher reads it: the four figures the record
 * carries, in the spelling `PmuIdent` states, the stepping the host left
 * unanswered standing the negative figure that spells no stepping class
 * (FR-007, FR-008).
 */
[[nodiscard]] inline auto pmuIdentOf(const sg::detail::HostIdent& ident)
    -> sg::counters::detail::PmuIdent
{
  auto pmu = sg::counters::detail::PmuIdent {};
  pmu.vendor = ident.vendor;
  pmu.family = ident.family;
  pmu.model = ident.model;
  pmu.stepping = ident.stepping.value_or(-1);
  return pmu;
}

/**
 * @brief The directory one tree's rows select for one identity, the matcher's
 * answer read whole: the empty text where no row names a directory, and no
 * answer at all where a row names a pattern the matcher cannot compile
 * (FR-004, FR-008).
 *
 * The matcher compiles the first column of every row it weighs, so a row a
 * person has edited can carry a pattern that throws. Both programs ask here,
 * so the throw stays inside the boundary that reads the tree and both answer
 * one unreadable mapfile the same way (FR-025, plan R-04, R-06).
 *
 * \param bytes the text one tree's mapfile holds
 * \param id the identity the run reads
 * \return the selected directory, or `std::nullopt` where a row names a
 *         pattern the matcher cannot compile
 */
[[nodiscard]] inline auto mapfileDirectory(
    std::string_view bytes,
    const sg::counters::detail::PmuIdent& id) -> std::optional<std::string>
{
  try {
    auto mapfile = std::istringstream(std::string(bytes));
    return sg::counters::detail::pmuSelectDirectory(mapfile, id);
  } catch (const std::regex_error&) {
    return std::nullopt;
  }
}

/**
 * @brief The chooser the reader hands its check: the mapfile of one tree
 * answers rule 4 for every identity the run meets (E-27, FR-008, FR-025, plan
 * R-06).
 *
 * \pre `treeRoot` names the tree the run reads
 * \post the rows stand read at the first question and kept for the rest, so a
 *       run of records opens the mapfile once and a run with no record opens
 *       nothing (plan R-06)
 * \post the identity whose rows name no directory answers NO_ROW, and the
 *       identity whose rows name one answers FOUND with that directory beside
 *       it (FR-008, FR-025)
 * \post a mapfile the read cannot open, and a mapfile whose rows hold a
 *       pattern the matcher cannot compile, answer UNREADABLE_MAPFILE for every
 *       identity the run meets, the answer that ends the run (FR-004, FR-025)
 */
[[nodiscard]] inline auto makeDirectoryChooser(
    const std::filesystem::path& treeRoot) -> sg::detail::DirectoryChooser
{
  auto reading = std::make_shared<
      std::optional<std::expected<std::string, std::string>>>();
  return [treeRoot, reading](
             const sg::detail::HostIdent& ident) -> sg::detail::DirectoryChoice
  {
    if (!reading->has_value()) {
      // One open behind the run: the first question reads the rows and every
      // later question reads the answer kept here, the answer this run never
      // reads twice (plan R-06).
      reading->emplace(mapfileText(treeRoot));
    }
    const auto& bytes = reading->value();
    if (!bytes.has_value()) {
      // A mapfile no read opens answers the run itself: rule 4 stands
      // unanswered for every identity of the listing, so no record of that run
      // reads a line and the reader names this path in its refusal (FR-004,
      // FR-025).
      return sg::detail::DirectoryChoice {
          .answer = sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
          .directory = {},
      };
    }
    const auto selected = mapfileDirectory(bytes.value(), pmuIdentOf(ident));
    if (!selected.has_value()) {
      // A row whose pattern the matcher cannot compile names no catalog for any
      // identity, so the rows stand unreadable to this run and the reader names
      // this mapfile in the refusal that ends the run (FR-004, FR-025).
      return sg::detail::DirectoryChoice {
          .answer = sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
          .directory = {},
      };
    }
    const auto& directory = selected.value();
    if (directory.empty()) {
      return sg::detail::DirectoryChoice {
          .answer = sg::detail::DirectoryAnswer::NO_ROW,
          .directory = {},
      };
    }
    return sg::detail::DirectoryChoice {
        .answer = sg::detail::DirectoryAnswer::FOUND,
        .directory = directory,
    };
  };
}

}  // namespace sg::tools

#endif  // SG_TOOLS_HOSTCHECK_MAPFILE_HPP
