// ============================================================================
// TDD test for the group name of one context's disclosure record
// (specs/039-context-groups T001; FR-005, FR-031, A-1 of the partition
// contract).
//
// Wave 1 reads the public record alone: the aggregate the header owns, the
// empty-group answer, and the four fields a filled record carries. No run
// starts here, so no provider registers, no thread starts, and no clock
// reads. Wave 5 adds the console segment and the report key to this
// executable (FR-028, FR-029).
//
// The record stays an aggregate and `group` stands last, so the positional
// form below is the order proof: a member written elsewhere stops this file
// from compiling (FR-005).
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include <type_traits>

#include "speedgun-ng/benchmark.hpp"

namespace
{

auto fail(char const* what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(bool cond, char const* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The record stays an aggregate, and the group name is a `std::string`
// spelled `group` with no prefix (FR-005, N-12).
static_assert(std::is_aggregate_v<sg::ContextDisclosure>);
static_assert(
    std::is_same_v<decltype(sg::ContextDisclosure::group), std::string>);

auto run() -> int
{
  // ---- The disclosure block (specs/039 T001; FR-005, FR-031, A-1) ----

  // A record nobody filled names no group: the shape of a run that carries
  // no partition, and of a report document that carries no `group` key
  // (FR-004, FR-031).
  {
    const sg::ContextDisclosure ungrouped {};
    check(ungrouped.group.empty(),
          "disclosure: a default record names no group");
    check(ungrouped.waitMode == sg::WaitMode::BLOCK,
          "disclosure: a default record waits by blocking");
  }

  // A record built from the three standing fields names no group, the
  // shape every ungrouped run keeps (FR-004, FR-033).
  {
    const sg::ContextDisclosure three {sg::WaitMode::SPIN, "cpu@3", true};
    check(three.group.empty(),
          "disclosure: a three-field record names no group");
    check(three.waitMode == sg::WaitMode::SPIN,
          "disclosure: a three-field record keeps its wait mode");
  }

  // A filled record carries the name beside the wait mode, the placement
  // text and the applied flag, and the positional form proves the member
  // stands last (FR-005).
  {
    const sg::ContextDisclosure filled {
        sg::WaitMode::BLOCK, "NO_PIN", false, "l3-1"};
    check(filled.waitMode == sg::WaitMode::BLOCK,
          "disclosure: the filled record keeps its wait mode");
    check(filled.placement == "NO_PIN",
          "disclosure: the filled record keeps its placement text");
    check(!filled.applied,
          "disclosure: the filled record keeps its applied flag");
    check(filled.group == "l3-1",
          "disclosure: the filled record carries the group name");
  }

  // The designated form spells the member by its public name, the shape
  // the report reader writes for a document that carries the key (FR-005,
  // FR-031).
  {
    const sg::ContextDisclosure named {
        .waitMode = sg::WaitMode::SPIN,
        .placement = "cpu@7",
        .applied = true,
        .group = "node-0",
    };
    check(named.waitMode == sg::WaitMode::SPIN && named.placement == "cpu@7"
              && named.applied && named.group == "node-0",
          "disclosure: the designated form carries the four fields");
  }

  std::printf("harness_group_report_test PASS\n");
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
