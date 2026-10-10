#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"

/**
 * @file family.cpp
 * @brief The step from family records to instances: the registry
 * expansion `expandRegistry()` performs, the instance-name
 * construction, the suite and case derivation, the duplicate-instance
 * check, the family-size check, and the `createRange` and
 * `createDenseRange` builders.
 *
 * Expansion completes before the filter and the first run, so a run
 * carries no expansion work (FR-003, R-03). The instance list it
 * leaves behind groups stably by suite in first-appearance order, and
 * the runner walks that order (R-12).
 */

namespace sg
{

namespace
{

// The ceiling a grown range may not multiply past (FR-004).
constexpr std::int64_t kInt64Ceil = std::numeric_limits<std::int64_t>::max();

// The powers of `multiplier` that stand in the closed interval from
// `low` to `high`, appended in increasing order (FR-004).
auto addPowers(std::vector<std::int64_t>& values,
               const std::int64_t low,
               const std::int64_t high,
               const std::int64_t multiplier) -> void
{
  std::int64_t power = 1;
  while (power <= high) {
    if (power >= low) {
      values.push_back(power);
    }
    // The next multiply would leave the range of the type.
    if (power > kInt64Ceil / multiplier) {
      break;
    }
    power *= multiplier;
  }
}

// The powers of a negative interval, grown over the complement and then
// negated and reversed, the way AddRange mirrors a negative span at the
// cited revision (FR-004). The caller keeps `high` at or below -1, so
// the complement of either bound stays inside the type.
auto addNegatedPowers(std::vector<std::int64_t>& values,
                      const std::int64_t low,
                      const std::int64_t high,
                      const std::int64_t multiplier) -> void
{
  const auto start = static_cast<std::ptrdiff_t>(values.size());
  addPowers(values, -high, -low, multiplier);
  for (auto walk = values.begin() + start; walk != values.end(); ++walk) {
    *walk = -*walk;
  }
  std::reverse(values.begin() + start, values.end());
}

// `low`, every power of `multiplier` strictly between the bounds, and
// `high`, in increasing order (FR-004).
auto addRange(std::vector<std::int64_t>& values,
              const std::int64_t low,
              const std::int64_t high,
              const std::int64_t multiplier) -> void
{
  values.push_back(low);
  if (low == high) {
    return;
  }
  if (low + 1 == high) {
    values.push_back(high);
    return;
  }
  const std::int64_t innerLow = low + 1;
  const std::int64_t innerHigh = high - 1;
  if (innerLow < 0) {
    addNegatedPowers(
        values, innerLow, std::min(innerHigh, std::int64_t {-1}), multiplier);
  }
  // No power of a multiplier reaches zero, so a span that crosses it
  // carries zero on its own (FR-004).
  if (low < 0 && high >= 0) {
    values.push_back(0);
  }
  if (innerHigh > 0) {
    addPowers(
        values, std::max(innerLow, std::int64_t {1}), innerHigh, multiplier);
  }
  // The sweep above stops at `high - 1` and `low` stands at the head of the
  // list, so `high` is still missing here and joins it unconditionally
  // (FR-004).
  values.push_back(high);
}

}  // namespace

/**
 * @brief `createRange`: the argument list a range call accepts, the
 * two bounds and the powers of `multiplier` strictly between them
 * (FR-002, FR-004).
 *
 * `range`, `ranges` and `args` each take a list this builder returns
 * (FR-002).
 *
 * \pre `multiplier` is at least 2 and `low` stands at or below `high`
 *      (FR-006, R-15).
 * \post the returned `std::vector<std::int64_t>` holds `low`, every
 *       power of `multiplier` strictly between the bounds, and `high`,
 *       in increasing order, and a negative span follows `AddRange`
 *       (FR-004).
 * \invariant the returned list is never empty, because both bounds
 *            stand in it (FR-004).
 */
auto createRange(const std::int64_t low,
                 const std::int64_t high,
                 const std::int64_t multiplier) -> std::vector<std::int64_t>
{
  SG_REQUIRE(multiplier >= 2,
             "a range multiplier below 2 violates the bound multiplier >= 2 "
             "(FR-006)");
  SG_REQUIRE(low <= high,
             "a range low bound above its high bound violates the bound "
             "low <= high (FR-006)");
  std::vector<std::int64_t> values;
  addRange(values, low, high, multiplier);
  SG_ENSURE(!values.empty(), "the built list carries both bounds (FR-004)");
  SG_INVARIANT(!values.empty(), "both bounds stand in the built list (FR-004)");
  return values;
  // LCOV_EXCL_LINE : coverage exclusion (T048): gcov attaches this closing
  // brace a line record that no execution can advance. Verified with
  // `gcov -b -i` on the coverage tree: the record reports count 0 with no
  // block record at all, while the return line above reports a count of 18.
}  // LCOV_EXCL_LINE

/**
 * @brief `createDenseRange`: the argument list a dense range call
 * accepts, every value from `low` to `high` in steps of `step`
 * (FR-002, FR-005).
 *
 * \pre `step` is at least 1 and `low` stands at or below `high`
 *      (FR-006, R-15).
 * \post the returned `std::vector<std::int64_t>` holds `low`,
 *       `low + step`, and each further step that stays at or below
 *       `high`, in increasing order (FR-005).
 * \invariant the returned list is never empty, because `low` stands in
 *            it (FR-005).
 */
auto createDenseRange(const std::int64_t low,
                      const std::int64_t high,
                      const std::int64_t step) -> std::vector<std::int64_t>
{
  SG_REQUIRE(
      step >= 1,
      "a denseRange step below 1 violates the bound step >= 1 " "(FR-006)");
  SG_REQUIRE(low <= high,
             "a range low bound above its high bound violates the bound "
             "low <= high (FR-006)");
  std::vector<std::int64_t> values;
  for (std::int64_t value = low; value <= high; value += step) {
    values.push_back(value);
  }
  SG_ENSURE(!values.empty(), "the built list carries its low bound (FR-005)");
  SG_INVARIANT(!values.empty(), "`low` stands in the built list (FR-005)");
  return values;
  // LCOV_EXCL_LINE : coverage exclusion (T048): the same gcov line record as
  // in createRange, count 0 with no advanceable block.
}  // LCOV_EXCL_LINE

namespace detail
{

/// @brief The expanded instance list, in the order the runner walks it
/// (R-12).
auto instances() -> std::vector<Instance>&
{
  // The instance list is a container of its own beside the registry, so
  // the registry keeps owning the family records: the handle's pointer,
  // the duplicate-name scan, and every option setter still read the one
  // record they read before expansion, and the run path reaches the
  // instances through this accessor (R-01, R-02, R-03).
  static std::vector<Instance> list;
  return list;
}

namespace
{

/**
 * @brief The instance-name builder: the family name with one
 * `/`-joined segment per argument (`instanceName`).
 *
 * A segment carries the label of its position as `label:value` where
 * `argName` set one (E-04, FR-010).
 *
 * \pre the label count is zero or equal to the argument count, and
 *      that count equals the family arity (FR-006).
 * \post the name starts with the family name and adds one segment per
 *       argument, each segment the argument value in decimal, labeled
 *       where a nonempty label stands at that position (FR-010).
 * \invariant the family name is a prefix of every instance name of
 *            that family (FR-010).
 */
auto instanceName(const RegistryEntry& entry,
                  const std::vector<std::int64_t>& arguments) -> std::string
{
  SG_REQUIRE(entry.argNames.empty() ||
                 entry.argNames.size() == arguments.size(),
             "a label count unequal to the family arity violates the bound "
             "label count == family arity (FR-006)");
  std::string name = entry.name;
  for (std::size_t position = 0; position < arguments.size(); ++position) {
    name += '/';
    // An empty label leaves its segment unlabeled, as the cited
    // revision's BenchmarkInstance constructor skips the empty name and
    // its ':' (FR-010).
    if (position < entry.argNames.size() && !entry.argNames[position].empty()) {
      name += entry.argNames[position];
      name += ':';
    }
    name += std::to_string(arguments[position]);
  }
  SG_ENSURE(name.rfind(entry.name, 0) == 0,
            "the family name is a prefix of every instance name of that "
            "family (FR-010)");
  return name;
  // LCOV_EXCL_LINE : coverage exclusion (T048): the same gcov line record as
  // in createRange, count 0 with no advanceable block.
}  // LCOV_EXCL_LINE

/**
 * @brief The suite and case derivation of one instance name
 * (`deriveSuiteAndCase`), the pair FR-021 puts on the result (R-08).
 *
 * \pre the instance name starts with the family name (FR-010).
 * \post the suite is the family name up to its first `/`, the case is
 *       the instance name with the leading suite and its `/` removed,
 *       and an instance name equal to its suite keeps that name as its
 *       case (FR-021, R-08).
 * \invariant one suite and case pair names one instance, so the pair
 *            stays unique across the instance list (FR-021).
 */
auto deriveSuiteAndCase(const std::string& familyName, const std::string& name)
    -> std::pair<std::string, std::string>
{
  SG_REQUIRE(name.rfind(familyName, 0) == 0,
             "the instance name starts with the family name (FR-010)");
  std::string suite = familyName.substr(0, familyName.find('/'));
  std::string caseName =
      name.size() > suite.size() ? name.substr(suite.size() + 1) : name;
  return {std::move(suite), std::move(caseName)};
}

/**
 * @brief The family-size check of expansion (`checkFamilySize`), the
 * FR-007 warning (R-14).
 *
 * \pre the family's expansion has completed, so its instance count is
 *      known.
 * \post a family above `kMaxFamilySize` draws one warning naming the
 *       bound of 100 instances, keeps every instance, and the run
 *       continues (FR-007, R-14).
 * \invariant a family over the bound draws the warning once per
 *            expansion (FR-007).
 */
auto checkFamilySize(const std::string& familyName,
                     const std::size_t count) -> void
{
  if (count <= kMaxFamilySize) {
    return;
  }
  std::fprintf(stderr,
               "%s: the family expands to %zu instances, above the bound of "
               "%zu instances (kMaxFamilySize, FR-007); every instance is "
               "kept and the run continues\n",
               familyName.c_str(),
               count,
               kMaxFamilySize);
}

/**
 * @brief The duplicate-instance check of expansion
 * (`hasDuplicateInstance`), the FR-012 recoverable error (R-03).
 *
 * The scan reads the blocks already built, which the registry order
 * fills in registration order, so the first name a later instance
 * meets is always the earlier registration's (FR-012).
 *
 * \pre the name comes from the family record expansion is naming now.
 * \post on a clash the harness prints one standard-error line naming
 *       both instance names, keeps the earlier instance, drops the
 *       later one, runs every other instance, and leaves the exit
 *       status as H1 fixes it (FR-012, Q-7).
 * \invariant the instance list never holds two instances with one
 *            name (FR-012).
 */
auto hasDuplicateInstance(const std::vector<std::vector<Instance>>& blocks,
                          const std::string& name) -> bool
{
  for (const auto& block : blocks) {
    for (const auto& instance : block) {
      if (instance.name == name) {
        std::fprintf(stderr,
                     "duplicate instance name: the instance %s of a later "
                     "registration names the instance %s of an earlier one; "
                     "the earlier instance stays, the later is dropped, and "
                     "every other instance runs (FR-012)\n",
                     name.c_str(),
                     instance.name.c_str());
        return true;
      }
    }
  }
  return false;
}

}  // namespace

/**
 * @brief Expand every family record in the registry into its instance
 * set (FR-003, R-03).
 *
 * `speedgunMain` calls this once, before the filter selection and the
 * first run, and the runner walks what it leaves (R-03, R-12).
 *
 * \pre every family call of every registration has returned, and no
 *      run has started (FR-003, E-01).
 * \post the instance list holds one instance per argument tuple of
 *       every family record, grouped stably by suite in
 *       first-appearance order, and each instance carries its name,
 *       its suite, its case, and the arguments it owns; a later
 *       instance whose name an earlier one already holds is dropped
 *       from that list (FR-010, FR-012, R-02, R-12).
 * \invariant no run performs expansion work (FR-003).
 */
auto expandRegistry() -> void
{
  auto& list = instances();
  list.clear();

  // One block per family, the blocks keyed by suite in first-appearance
  // order and the instances inside a block in expansion order, so the
  // list the runner walks groups stably by suite (R-12).
  std::vector<std::string> suites;
  std::vector<std::vector<Instance>> blocks;
  for (const auto& entry : registry()) {
    const std::string suite = entry->name.substr(0, entry->name.find('/'));
    std::size_t slot = suites.size();
    const auto found = std::find(suites.begin(), suites.end(), suite);
    if (found != suites.end()) {
      slot = static_cast<std::size_t>(found - suites.begin());
    } else {
      suites.push_back(suite);
      blocks.emplace_back();
    }

    auto& block = blocks[slot];
    const std::size_t start = block.size();
    if (entry->args.empty()) {
      // A family that states no family call has one instance with zero
      // arguments (FR-008).
      if (!hasDuplicateInstance(blocks, entry->name)) {
        Instance& instance = block.emplace_back();
        instance.name = entry->name;
        instance.family = entry.get();
        const auto derived = deriveSuiteAndCase(entry->name, instance.name);
        instance.suite = derived.first;
        instance.caseName = derived.second;
      }
    } else {
      // One instance per argument list of the family record (FR-001,
      // FR-010): the family calls already grew the lists at
      // registration, so expansion names each list and hands it to an
      // instance that owns its storage (R-02, R-04).
      for (const auto& arguments : entry->args) {
        const std::string name = instanceName(*entry, arguments);
        if (hasDuplicateInstance(blocks, name)) {
          continue;
        }
        Instance& instance = block.emplace_back();
        instance.name = name;
        instance.arguments = arguments;
        instance.family = entry.get();
        const auto derived = deriveSuiteAndCase(entry->name, instance.name);
        instance.suite = derived.first;
        instance.caseName = derived.second;
      }
    }
    checkFamilySize(entry->name, block.size() - start);
  }

  for (auto& block : blocks) {
    for (auto& instance : block) {
      list.push_back(std::move(instance));
    }
  }
}

}  // namespace detail

}  // namespace sg
