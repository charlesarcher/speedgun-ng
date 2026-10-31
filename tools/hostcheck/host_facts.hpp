#ifndef SG_TOOLS_HOSTCHECK_HOST_FACTS_HPP
#define SG_TOOLS_HOSTCHECK_HOST_FACTS_HPP

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "../../source/counters/detail/pmu.hpp"
#include "../../source/harness/detail/host_record.hpp"
#include "mapfile.hpp"

/**
 * @file host_facts.hpp
 * @brief The host facts one write reads: the identity the host answers, the
 * directory the mapfile of the tree root selects for it, and the layout the
 * run stands in (specs/031-host-validation; FR-007, FR-008, FR-012, plan
 * R-04).
 *
 * The reads stand behind the options struct, so the program answers it with
 * the host's own readers and a test scripts its own answers, and no test
 * reads CPUID or the installed layout (FR-031, FR-032, FR-035).
 */
namespace sg::tools
{

/**
 * @brief The reads one host-facts assembly asks for: the CPUID identity of
 * the host and the layout of the run (FR-007, FR-012).
 *
 * The program fills both with the host's own readers; a test fills both with
 * the answers it scripts, so the assembly runs on planted values
 * (FR-032, FR-035).
 *
 * \invariant no reader here reads a clock, and none stands asked before the
 *            write path asks for the facts (FR-033, FR-037)
 */
struct HostFactsOptions
{
  /// @brief The CPUID identity the host answers (FR-007).
  std::function<sg::counters::detail::PmuIdent()> pmuIdent;
  /// @brief The layout the standing topology read answers (FR-012).
  std::function<const sg::detail::MachineLayout&()> layout;
};

/**
 * @brief The facts of one tree root, the values the write path renders the
 * record from (FR-007, FR-008, FR-012, FR-014). A root whose mapfile no read
 * opens, a root whose mapfile names no row, and a root whose rows name a
 * pattern the matcher cannot compile each leave the directory unanswered, and
 * an identity without a directory names no CPU: the refusal of FR-010 answers
 * there, in the library that owns the lines.
 *
 * \pre `treeRoot` names the tree the run reads
 * \pre both readers of `reads` stand given, and the call asks each at most
 *      once (FR-002, FR-037)
 * \post the identity carries the four figures the CPUID read answers, the
 *       key `mapfileKey` renders for it, and the directory the rows of the
 *       root select, empty where the rows named none (FR-007, FR-008)
 * \post `mapfileReadable` stands false exactly where the mapfile stood
 *       absent, stood unreadable, or held a row whose pattern the matcher
 *       cannot compile, and true where the rows were weighed (FR-004,
 *       FR-008)
 * \post `layout` carries the layout the reader answers (FR-012)
 */
[[nodiscard]] inline auto hostFacts(const std::filesystem::path& treeRoot,
                                    const HostFactsOptions& reads)
    -> sg::detail::HostFacts
{
  const auto pmu = reads.pmuIdent();
  auto ident = sg::detail::HostIdent {};
  ident.vendor = pmu.vendor;
  ident.family = pmu.family;
  ident.model = pmu.model;
  if (pmu.stepping >= 0) {
    ident.stepping = pmu.stepping;
  }
  ident.key = sg::counters::detail::mapfileKey(pmu);
  auto directory = std::string {};
  auto rowsWeighed = true;
  const auto bytes = mapfileText(treeRoot);
  if (bytes.has_value()) {
    const auto selected = mapfileDirectory(*bytes, pmu);
    rowsWeighed = selected.has_value();
    directory = selected.value_or(std::string {});
  } else {
    rowsWeighed = false;
  }
  ident.directory = std::move(directory);
  auto facts = sg::detail::HostFacts {};
  facts.ident = std::move(ident);
  facts.layout = reads.layout();
  facts.mapfileReadable = rowsWeighed;
  return facts;
}

/**
 * @brief The seam the write path asks for, built over one options value: the
 * program hands the host's own readers, a test hands the readers it scripts,
 * and both answer one facts value per root (FR-037).
 *
 * \pre the options value outlives every call of the returned seam (E-02)
 * \post the returned seam answers `hostFacts` of the root it is handed, so
 *        the facts stand read at the root the parse named (FR-002, FR-037)
 */
[[nodiscard]] inline auto hostFactsSeam(const HostFactsOptions& reads)
    -> sg::detail::HostFactsSeam
{
  return [&reads](const std::filesystem::path& treeRoot)
  {
    return hostFacts(treeRoot, reads);
  };
}

}  // namespace sg::tools

#endif  // SG_TOOLS_HOSTCHECK_HOST_FACTS_HPP
