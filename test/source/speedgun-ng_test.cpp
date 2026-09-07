#include <cstdio>
#include <cstdlib>
#include <string>

#include "speedgun-ng/speedgun-ng.hpp"

#include "speedgun-ng/dbc.hpp"

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

struct violation_caught
{
  sg::dbc::ViolationRecord record {};
};

auto record_into(sg::dbc::ViolationRecord& out) -> sg::dbc::violation_observer
{
  return [&out](sg::dbc::ViolationRecord const& rec)
  {
    out = rec;
    throw violation_caught {rec};
  };
}

auto capture_violation(sg::dbc::ViolationRecord& rec, auto&& body) -> bool
{
  sg::dbc::set_observer(record_into(rec));
  bool caught = false;
  try {
    body();
  } catch (violation_caught const&) {
    caught = true;
  }
  sg::dbc::set_observer({});
  return caught;
}

// Test-only helpers that violate the same contract kinds exported_class
// enforces (class invariant / named-result postcondition). The production
// members cannot be driven into violation without a public seam.
auto violate_class_invariant() -> void
{
  std::string stored {};
  SG_INVARIANT(!stored.empty(), "inv: stored name is non-empty");
}

auto violate_name_postcondition() -> char const*
{
  char const* const result = "";
  SG_ENSURE(std::string(result) == "speedgun-ng",
            "post: name() returns the project name");
  return result;
}

auto run() -> int
{
  using sg::dbc::Kind;

  // PASS-side: contracts hold; construct and name() return the project name.
  {
    auto const exported = exported_class {};
    check(std::string("speedgun-ng") == exported.name(),
          "pass: name() returns the project name");
  }

  // FAIL-side: observer-capture of an intentional invariant violation.
  sg::dbc::ViolationRecord rec {};
  check(capture_violation(rec, [] { violate_class_invariant(); }),
        "fail: invariant violation delivered to the observer");
  check(rec.kind == Kind::invariant, "fail: kind == invariant");
  check(std::string(rec.message) == "inv: stored name is non-empty",
        "fail: invariant message text");
  check(std::string(rec.predicateText) == "!stored.empty()",
        "fail: invariant predicate text");
  check(rec.file != nullptr && rec.file[0] != '\0', "fail: file present");
  check(rec.line > 0, "fail: line present");

  // FAIL-side: observer-capture of an intentional postcondition violation
  // over a named result capture (the name() ENSURE shape).
  rec = sg::dbc::ViolationRecord {};
  check(capture_violation(rec, [] { (void)violate_name_postcondition(); }),
        "fail: postcondition violation delivered to the observer");
  check(rec.kind == Kind::postcondition, "fail: kind == postcondition");
  check(std::string(rec.message) == "post: name() returns the project name",
        "fail: postcondition message text");
  check(std::string(rec.predicateText)
            == "std::string(result) == \"speedgun-ng\"",
        "fail: postcondition predicate text");

  std::printf("speedgun-ng_test PASS\n");
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
