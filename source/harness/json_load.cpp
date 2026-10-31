#include <array>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <expected>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <simdjson.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/hunt.hpp"

namespace sg
{
namespace
{

using Element = simdjson::dom::element;
using JsonArray = simdjson::dom::array;

auto kindName(Element value) -> const char*
{
  if (value.is_null()) {
    return "null";
  }
  if (value.is_bool()) {
    return "boolean";
  }
  if (value.is_number()) {
    return "number";
  }
  if (value.is_string()) {
    return "string";
  }
  if (value.is_array()) {
    return "array";
  }
  return "object";
}

auto wrongKind(const std::string& where,
               Element value,
               std::string_view want) -> std::string
{
  return where + ": expected " + std::string(want) + ", found "
      + kindName(value);
}

auto missing(const std::string& where, std::string_view key) -> std::string
{
  return where + ": the key '" + std::string(key) + "' stands missing";
}

auto field(Element object,
           const std::string& where,
           std::string_view key) -> std::expected<Element, std::string>
{
  auto found = object[key];
  if (found.has_value()) {
    return found.value();
  }
  if (found.error() == simdjson::NO_SUCH_FIELD) {
    return std::unexpected(missing(where, key));
  }
  return std::unexpected(where + ": " + simdjson::error_message(found.error()));
}

// One key that stands optional: the four keys of the pass disclosure and the
// pass index of a metric column arrived after the format was published, and
// the comparison of specs/026 FR-051 reads documents stored before it, so a
// key that stands absent reads as nothing and leaves its member at the
// default of a run that collected no pass. A key that stands present keeps
// its own kind and its own refusal (specs/024 FR-038).
auto optionalField(Element object,
                   std::string_view key) noexcept -> std::optional<Element>
{
  auto found = object[key];
  if (!found.has_value()) {
    return std::nullopt;
  }
  return *found;
}

// The keys a record must hold, reported once: the first key that stands
// missing names the record and itself (FR-024).
auto settled(const std::vector<
             std::pair<std::string, std::expected<Element, std::string>>>&
                 parts) -> std::expected<void, std::string>
{
  for (const auto& part : parts) {
    if (!part.second.has_value()) {
      return std::unexpected(part.second.error());
    }
  }
  return {};
}

// The record of the first answer that carries an error, held by a plain
// function: a coverage trace drawn from a template body leaves a cold block
// inside a generic lambda out of its counts, while the same shape in
// `settled` above carries its count (specs/023 FR-024).
auto keepFirstError(std::expected<void, std::string>& answer,
                    const std::string& error) noexcept -> void
{
  answer = std::unexpected(error);
}

// The same report over the values a record read: the first answer that
// carries an error names the field, and the rest are not looked at.
template<class... Parts>
auto allSettled(Parts&&... parts) -> std::expected<void, std::string>
{
  auto answer = std::expected<void, std::string> {};
  auto report = [&](auto& part) noexcept -> bool
  {
    return part.has_value() ? true
                            : (keepFirstError(answer, part.error()), false);
  };
  (void)((report(parts) && ...) && true);
  return answer;
}

auto asString(Element value, const std::string& where)
    -> std::expected<std::string, std::string>
{
  std::string_view text;
  if (const auto code = value.get_string().get(text); code != simdjson::SUCCESS)
  {
    return std::unexpected(wrongKind(where, value, "a string"));
  }
  return std::string(text);
}

auto asDouble(Element value,
              const std::string& where) -> std::expected<double, std::string>
{
  double number = 0.0;
  if (const auto code = value.get_double().get(number);
      code != simdjson::SUCCESS)
  {
    return std::unexpected(wrongKind(where, value, "a number"));
  }
  return number;
}

auto asUnsigned(Element value, const std::string& where)
    -> std::expected<std::uint64_t, std::string>
{
  std::int64_t number = 0;
  if (const auto code = value.get_int64().get(number);
      code != simdjson::SUCCESS)
  {
    return std::unexpected(wrongKind(where, value, "an integer"));
  }
  if (number < 0) {
    return std::unexpected(where + ": a count stands negative");
  }
  return static_cast<std::uint64_t>(number);
}

auto asBool(Element value,
            const std::string& where) -> std::expected<bool, std::string>
{
  bool flag = false;
  if (const auto code = value.get_bool().get(flag); code != simdjson::SUCCESS) {
    return std::unexpected(wrongKind(where, value, "a boolean"));
  }
  return flag;
}

auto asArray(Element value,
             const std::string& where) -> std::expected<JsonArray, std::string>
{
  JsonArray items;
  if (const auto code = value.get_array().get(items); code != simdjson::SUCCESS)
  {
    return std::unexpected(wrongKind(where, value, "an array"));
  }
  return items;
}

// One word of a closed enumeration, or the error naming the word the
// document held (FR-024).
template<class Enum>
auto asEnum(const auto& words,
            Element value,
            const std::string& where) -> std::expected<Enum, std::string>
{
  auto text = asString(value, where);
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  const auto found = detail::enumOf<Enum>(words, *text);
  if (!found.has_value()) {
    return std::unexpected(where + ": '" + detail::quotedText(*text)
                           + "' stands outside the names the schema lists");
  }
  return *found;
}

auto readStatistics(Element value, const std::string& where)
    -> std::expected<sg::Statistics, std::string>
{
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts;
  for (const auto* key : {"samples",
                          "mean",
                          "median",
                          "sampleStdDev",
                          "coefficientOfVariation",
                          "min",
                          "max"})
  {
    parts.emplace_back(key, field(value, where, key));
  }
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto samples = asUnsigned(parts[0].second.value(), where + "/samples");
  auto mean = asDouble(parts[1].second.value(), where + "/mean");
  auto median = asDouble(parts[2].second.value(), where + "/median");
  auto deviation = asDouble(parts[3].second.value(), where + "/sampleStdDev");
  auto variation =
      asDouble(parts[4].second.value(), where + "/coefficientOfVariation");
  auto low = asDouble(parts[5].second.value(), where + "/min");
  auto high = asDouble(parts[6].second.value(), where + "/max");
  auto settledValues =
      allSettled(samples, mean, median, deviation, variation, low, high);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  return sg::Statistics {
      .samples = *samples,
      .mean = *mean,
      .median = *median,
      .sampleStdDev = *deviation,
      .coefficientOfVariation = *variation,
      .min = *low,
      .max = *high,
  };
}

// A summary the document holds as `null` answers the empty record, the
// value whose sample count no row reaches, which is the pair the writer
// turns back into `null` (FR-025, FR-026).
auto readStatisticsOrNull(Element value, const std::string& where)
    -> std::expected<sg::Statistics, std::string>
{
  if (value.is_null()) {
    return sg::Statistics {};
  }
  return readStatistics(value, where);
}

auto readPercentileCell(Element value, const std::string& where)
    -> std::expected<sg::PercentileValue, std::string>
{
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"available", field(value, where, "available")},
          {"value", field(value, where, "value")},
      };
  if (!settled(parts).has_value()) {
    return std::unexpected(settled(parts).error());
  }
  auto flag = asBool(parts[0].second.value(), where + "/available");
  if (!flag.has_value()) {
    return std::unexpected(flag.error());
  }
  sg::PercentileValue cell;
  cell.available = *flag;
  const Element number = parts[1].second.value();
  if (!number.is_null()) {
    auto answer = asDouble(number, where + "/value");
    if (!answer.has_value()) {
      return std::unexpected(answer.error());
    }
    cell.value = *answer;
  }
  return cell;
}

auto readPercentileColumns(Element value, const std::string& where)
    -> std::expected<std::vector<std::vector<sg::PercentileValue>>, std::string>
{
  auto columns = asArray(value, where);
  if (!columns.has_value()) {
    return std::unexpected(columns.error());
  }
  std::vector<std::vector<sg::PercentileValue>> read;
  std::size_t position = 0;
  for (const auto column : *columns) {
    const std::string here = where + "/" + std::to_string(position);
    auto points = asArray(column, here);
    if (!points.has_value()) {
      return std::unexpected(points.error());
    }
    std::vector<sg::PercentileValue> columnRead;
    std::size_t point = 0;
    for (const auto cell : *points) {
      auto answer =
          readPercentileCell(cell, here + "/" + std::to_string(point));
      if (!answer.has_value()) {
        return std::unexpected(answer.error());
      }
      columnRead.push_back(*answer);
      ++point;
    }
    read.push_back(std::move(columnRead));
    ++position;
  }
  return read;
}

auto readRawColumns(Element value, const std::string& where)
    -> std::expected<RowRaw, std::string>
{
  auto columns = field(value, where, "columns");
  if (!columns.has_value()) {
    return std::unexpected(columns.error());
  }
  auto arrays = asArray(*columns, where + "/columns");
  if (!arrays.has_value()) {
    return std::unexpected(arrays.error());
  }
  RowRaw raw;
  std::size_t position = 0;
  for (const auto column : *arrays) {
    const std::string here = where + "/columns/" + std::to_string(position);
    auto values = asArray(column, here);
    if (!values.has_value()) {
      return std::unexpected(values.error());
    }
    std::vector<double> read;
    for (const auto item : *values) {
      auto number = asDouble(item, here);
      if (!number.has_value()) {
        return std::unexpected(number.error());
      }
      read.push_back(*number);
    }
    raw.columns.push_back(std::move(read));
    ++position;
  }
  return raw;
}

auto readMetricValue(Element value, const std::string& where)
    -> std::expected<sg::MetricValue, std::string>
{
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"label", field(value, where, "label")},
          {"value", field(value, where, "value")},
          {"runningRatio", field(value, where, "runningRatio")},
          {"scaled", field(value, where, "scaled")},
          {"availability", field(value, where, "availability")},
      };
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto label = asString(parts[0].second.value(), where + "/label");
  auto answer = asDouble(parts[1].second.value(), where + "/value");
  auto running = asDouble(parts[2].second.value(), where + "/runningRatio");
  auto flag = asBool(parts[3].second.value(), where + "/scaled");
  auto state = asEnum<sg::counters::Availability>(detail::kAvailabilityWords,
                                                  parts[4].second.value(),
                                                  where + "/availability");
  auto settledValues = allSettled(label, answer, running, flag, state);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  std::optional<std::size_t> passIndex;
  const auto pass = optionalField(value, "pass");
  if (pass.has_value() && !pass->is_null()) {
    auto count = asUnsigned(*pass, where + "/pass");
    if (!count.has_value()) {
      return std::unexpected(count.error());
    }
    passIndex = static_cast<std::size_t>(*count);
  }
  // The label of a metric column stands in the entry's `metricLabels`,
  // which the entry reads beside these values; the row repeats the label
  // for a reader of the file alone, so the value keeps no copy of it.
  return sg::MetricValue {
      .value = *answer,
      .runningRatio = *running,
      .scaled = *flag,
      .availability = *state,
      // specs/024 E-08, FR-019: the pass the document names beside the
      // value, the empty reading a one-pass run gives.
      .pass = passIndex,
  };
}

auto readFit(Element value,
             const std::string& where) -> std::expected<sg::FitRow, std::string>
{
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"quantity", field(value, where, "quantity")},
          {"curve", field(value, where, "curve")},
          {"coefficient", field(value, where, "coefficient")},
          {"rmsPercent", field(value, where, "rmsPercent")},
          {"scalesWithUnit", field(value, where, "scalesWithUnit")},
          {"reason", field(value, where, "reason")},
      };
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto quantity = asString(parts[0].second.value(), where + "/quantity");
  auto curve = asString(parts[1].second.value(), where + "/curve");
  auto coefficient = asDouble(parts[2].second.value(), where + "/coefficient");
  auto rms = asDouble(parts[3].second.value(), where + "/rmsPercent");
  auto scales = asBool(parts[4].second.value(), where + "/scalesWithUnit");
  auto settledValues = allSettled(quantity, curve, coefficient, rms, scales);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  sg::FitRow fit {
      .quantity = *quantity,
      .curve = *curve,
      .coefficient = *coefficient,
      .rmsPercent = *rms,
      .scalesWithUnit = *scales,
      .reason = {},
  };
  const Element reason = parts[5].second.value();
  if (!reason.is_null()) {
    auto text = asString(reason, where + "/reason");
    if (!text.has_value()) {
      return std::unexpected(text.error());
    }
    fit.reason = *text;
  }
  return fit;
}

// One per-cpu stats object of a jitter record: the cpu index and the
// sixteen fields in the E-03 order, plus the kept gap records when the
// document carries them (E-03, E-10). The reader answers the parsed cpu
// and stats so the per-entry read fills the result's carrier with them
// (E-08, FR-027); the run-level read lets them go. Each key is read and
// kind-checked in order, so a wrong kind names itself before a later
// key's absence does (FR-029).
auto readJitterCpu(Element value, const std::string& where)
    -> std::expected<std::pair<std::uint32_t, sg::JitterStats>, std::string>
{
  std::pair<std::uint32_t, sg::JitterStats> read {};
  std::uint64_t cpuValue = 0;
  const auto readUnsignedKey =
      [&](std::string_view key,
          std::uint64_t* into) -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto number = asUnsigned(*found, where + "/" + std::string(key));
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    *into = *number;
    return {};
  };
  const auto readRealKey = [&](std::string_view key,
                               double* into) -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto number = asDouble(*found, where + "/" + std::string(key));
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    *into = *number;
    return {};
  };
  auto settledValues =
      allSettled(readUnsignedKey("cpu", &cpuValue),
                 readUnsignedKey("gapCount", &read.second.gapCount),
                 readRealKey("gapsPerSecond", &read.second.gapsPerSecond),
                 readUnsignedKey("minNs", &read.second.minNs),
                 readUnsignedKey("medianNs", &read.second.medianNs),
                 readRealKey("meanNs", &read.second.meanNs),
                 readUnsignedKey("p90Ns", &read.second.p90Ns),
                 readUnsignedKey("p99Ns", &read.second.p99Ns),
                 readUnsignedKey("p999Ns", &read.second.p999Ns),
                 readUnsignedKey("p9999Ns", &read.second.p9999Ns),
                 readUnsignedKey("maxNs", &read.second.maxNs),
                 readUnsignedKey("totalNs", &read.second.totalNs),
                 readRealKey("totalPercent", &read.second.totalPercent),
                 readUnsignedKey("runtimeNs", &read.second.runtimeNs),
                 readUnsignedKey("thresholdNs", &read.second.thresholdNs),
                 readUnsignedKey("loopPeriodNs", &read.second.loopPeriodNs),
                 readUnsignedKey("droppedGaps", &read.second.droppedGaps));
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  if (const auto gaps = field(value, where, "gaps"); gaps.has_value()) {
    auto items = asArray(*gaps, where + "/gaps");
    if (!items.has_value()) {
      return std::unexpected(items.error());
    }
    std::size_t index = 0;
    for (const auto item : *items) {
      const std::string here = where + "/gaps/" + std::to_string(index);
      auto stamp = field(item, here, "stamp");
      if (!stamp.has_value()) {
        return std::unexpected(stamp.error());
      }
      auto stampValue = asUnsigned(*stamp, here + "/stamp");
      if (!stampValue.has_value()) {
        return std::unexpected(stampValue.error());
      }
      auto gap = field(item, here, "gapNs");
      if (!gap.has_value()) {
        return std::unexpected(gap.error());
      }
      auto gapValue = asUnsigned(*gap, here + "/gapNs");
      if (!gapValue.has_value()) {
        return std::unexpected(gapValue.error());
      }
      ++index;
    }
  }
  read.first = static_cast<std::uint32_t>(cpuValue);
  return read;
}

// One count the document may stand `null`: an attribution figure the
// pre-check could not read, the file form of the console's
// `not available` (specs/034 FR-017, FR-024).
auto readNullableCount(Element value, const std::string& where)
    -> std::expected<std::optional<std::uint64_t>, std::string>
{
  if (value.is_null()) {
    return std::optional<std::uint64_t> {};
  }
  auto number = asUnsigned(value, where);
  if (!number.has_value()) {
    return std::unexpected(number.error());
  }
  return std::optional<std::uint64_t> {*number};
}

// One text the document may stand `null`: a host setting the host does
// not state (specs/034 FR-017, FR-024). A null answers an empty optional,
// the reading of a setting the run disclosed as unavailable.
auto readNullableText(Element value, const std::string& where)
    -> std::expected<std::optional<std::string>, std::string>
{
  if (value.is_null()) {
    return std::optional<std::string> {};
  }
  auto text = asString(value, where);
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  return std::optional<std::string> {std::move(*text)};
}

// One per-cpu attribution object of a pre-check record: the interrupt and
// softirq deltas and the scheduler deltas, each an absent-figure `null`
// or a count (specs/034 FR-018, E-04). The softirq kinds are the
// document's own, so the reader carries them in the order the document
// states them (FR-029).
auto readPrecheckAttribution(Element value, const std::string& where)
    -> std::expected<sg::PrecheckAttribution, std::string>
{
  sg::PrecheckAttribution read {};
  const auto readFigure = [&](std::string_view key,
                              std::optional<std::uint64_t>* into)
      -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto figure = readNullableCount(*found, where + "/" + std::string(key));
    if (!figure.has_value()) {
      return std::unexpected(figure.error());
    }
    *into = std::move(*figure);
    return {};
  };
  auto settledValues = allSettled(readFigure("interrupts", &read.interrupts),
                                  readFigure("switchDelta", &read.switches),
                                  readFigure("scheduleDelta", &read.schedules));
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  auto softirqs = field(value, where, "softirqs");
  if (!softirqs.has_value()) {
    return std::unexpected(softirqs.error());
  }
  if (!softirqs->is_object()) {
    return std::unexpected(
        wrongKind(where + "/softirqs", *softirqs, "an object"));
  }
  for (const auto pair : softirqs->get_object()) {
    const std::string here = where + "/softirqs/" + std::string(pair.key);
    auto figure = readNullableCount(pair.value, here);
    if (!figure.has_value()) {
      return std::unexpected(figure.error());
    }
    read.softirqs.emplace_back(std::string(pair.key), *figure);
  }
  return read;
}

// One per-cpu stats object of a pre-check record: the sixteen fields in
// the E-03 order, no cpu of its own and no kept gap records, the shape
// the pre-check writes (specs/034 FR-021, E-03).
auto readPrecheckStats(Element value, const std::string& where)
    -> std::expected<sg::JitterStats, std::string>
{
  sg::JitterStats read {};
  const auto readUnsignedKey =
      [&](std::string_view key,
          std::uint64_t* into) -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto number = asUnsigned(*found, where + "/" + std::string(key));
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    *into = *number;
    return {};
  };
  const auto readRealKey = [&](std::string_view key,
                               double* into) -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto number = asDouble(*found, where + "/" + std::string(key));
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    *into = *number;
    return {};
  };
  auto settledValues =
      allSettled(readUnsignedKey("gapCount", &read.gapCount),
                 readRealKey("gapsPerSecond", &read.gapsPerSecond),
                 readUnsignedKey("minNs", &read.minNs),
                 readUnsignedKey("medianNs", &read.medianNs),
                 readRealKey("meanNs", &read.meanNs),
                 readUnsignedKey("p90Ns", &read.p90Ns),
                 readUnsignedKey("p99Ns", &read.p99Ns),
                 readUnsignedKey("p999Ns", &read.p999Ns),
                 readUnsignedKey("p9999Ns", &read.p9999Ns),
                 readUnsignedKey("maxNs", &read.maxNs),
                 readUnsignedKey("totalNs", &read.totalNs),
                 readRealKey("totalPercent", &read.totalPercent),
                 readUnsignedKey("runtimeNs", &read.runtimeNs),
                 readUnsignedKey("thresholdNs", &read.thresholdNs),
                 readUnsignedKey("loopPeriodNs", &read.loopPeriodNs),
                 readUnsignedKey("droppedGaps", &read.droppedGaps));
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  return read;
}

// One per-cpu entry of a pre-check record: the cpu, its role, its stats
// and its attribution deltas (specs/034 FR-021, E-04). The role names the
// planned set, so a text no role states refuses.
auto readPrecheckCpu(Element value, const std::string& where)
    -> std::expected<sg::PrecheckCpuRecord, std::string>
{
  sg::PrecheckCpuRecord read {};
  auto cpu = field(value, where, "cpu");
  if (!cpu.has_value()) {
    return std::unexpected(cpu.error());
  }
  auto cpuValue = asUnsigned(*cpu, where + "/cpu");
  if (!cpuValue.has_value()) {
    return std::unexpected(cpuValue.error());
  }
  auto role = field(value, where, "role");
  if (!role.has_value()) {
    return std::unexpected(role.error());
  }
  auto roleText = asString(*role, where + "/role");
  if (!roleText.has_value()) {
    return std::unexpected(roleText.error());
  }
  if (*roleText == "planned") {
    read.planned = true;
  } else if (*roleText != "candidate") {
    return std::unexpected(where + "/role: '" + detail::quotedText(*roleText)
                           + "' names no pre-check role");
  }
  auto stats = field(value, where, "stats");
  if (!stats.has_value()) {
    return std::unexpected(stats.error());
  }
  auto statsRead = readPrecheckStats(*stats, where + "/stats");
  if (!statsRead.has_value()) {
    return std::unexpected(statsRead.error());
  }
  read.stats = *statsRead;
  auto attribution = field(value, where, "attribution");
  if (!attribution.has_value()) {
    return std::unexpected(attribution.error());
  }
  auto deltas = readPrecheckAttribution(*attribution, where + "/attribution");
  if (!deltas.has_value()) {
    return std::unexpected(deltas.error());
  }
  read.attribution = *deltas;
  read.cpu = static_cast<std::uint32_t>(*cpuValue);
  return read;
}

// One host settings object of a pre-check record: the three kernel
// settings, the per-cpu governors and the per-cpu irq affinity lists,
// each a text the host states or a `null` it does not (specs/034 FR-018).
// A governor key names its cpu, and an affinity text names its list, so
// the reader parses the list it carries.
auto readPrecheckHost(Element value, const std::string& where)
    -> std::expected<sg::PrecheckHostSettings, std::string>
{
  sg::PrecheckHostSettings read {};
  const auto readTextKey =
      [&](std::string_view key,
          std::optional<std::string>* into) -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto text = readNullableText(*found, where + "/" + std::string(key));
    if (!text.has_value()) {
      return std::unexpected(text.error());
    }
    *into = std::move(*text);
    return {};
  };
  auto settledValues = allSettled(readTextKey("isolcpus", &read.isolcpus),
                                  readTextKey("nohz_full", &read.nohzFull),
                                  readTextKey("rcu_nocbs", &read.rcuNocbs));
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  // A key of either per-cpu object names its cpu, so the reader parses
  // the key text and refuses a key no cpu states (FR-029).
  const auto readCpuKey =
      [](std::string_view key,
         const std::string& here) -> std::expected<std::uint32_t, std::string>
  {
    std::uint32_t cpu = 0;
    const auto* const end = key.data() + key.size();
    const auto [stop, code] = std::from_chars(key.data(), end, cpu);
    if (code != std::errc {} || stop != end) {
      return std::unexpected(here + ": a key names no cpu");
    }
    return cpu;
  };
  const auto readGovernors = [&]() -> std::expected<void, std::string>
  {
    auto object = field(value, where, "governors");
    if (!object.has_value()) {
      return std::unexpected(object.error());
    }
    if (!object->is_object()) {
      return std::unexpected(
          wrongKind(where + "/governors", *object, "an object"));
    }
    for (const auto pair : object->get_object()) {
      const std::string here = where + "/governors/" + std::string(pair.key);
      auto cpu = readCpuKey(pair.key, here);
      if (!cpu.has_value()) {
        return std::unexpected(cpu.error());
      }
      auto governor = readNullableText(pair.value, here);
      if (!governor.has_value()) {
        return std::unexpected(governor.error());
      }
      read.governors.emplace_back(*cpu, std::move(*governor));
    }
    return {};
  };
  auto governors = readGovernors();
  if (!governors.has_value()) {
    return std::unexpected(governors.error());
  }
  const auto readAffinity = [&]() -> std::expected<void, std::string>
  {
    auto object = field(value, where, "irqAffinity");
    if (!object.has_value()) {
      return std::unexpected(object.error());
    }
    if (!object->is_object()) {
      return std::unexpected(
          wrongKind(where + "/irqAffinity", *object, "an object"));
    }
    for (const auto pair : object->get_object()) {
      const std::string here = where + "/irqAffinity/" + std::string(pair.key);
      auto cpu = readCpuKey(pair.key, here);
      if (!cpu.has_value()) {
        return std::unexpected(cpu.error());
      }
      auto text = readNullableText(pair.value, here);
      if (!text.has_value()) {
        return std::unexpected(text.error());
      }
      std::vector<std::uint32_t> irqs;
      if (text->has_value()) {
        const std::string& list = **text;
        std::size_t start = 0;
        while (start < list.size()) {
          const std::size_t stop = list.find(',', start);
          const std::string_view item = std::string_view(list).substr(
              start,
              stop == std::string::npos ? std::string::npos : stop - start);
          std::uint32_t irq = 0;
          const auto* const itemEnd = item.data() + item.size();
          const auto [itemStop, itemCode] =
              std::from_chars(item.data(), itemEnd, irq);
          if (itemCode != std::errc {} || itemStop != itemEnd) {
            return std::unexpected(here + ": '" + std::string(item)
                                   + "' names no irq");
          }
          irqs.push_back(irq);
          if (stop == std::string::npos) {
            break;
          }
          start = stop + 1;
        }
      }
      read.irqAffinity.emplace_back(*cpu, std::move(irqs));
    }
    return {};
  };
  auto affinity = readAffinity();
  if (!affinity.has_value()) {
    return std::unexpected(affinity.error());
  }
  return read;
}

// One per-cpu entry of a tracer record: the cpu and the five figures,
// each an absent-figure `null` or a count, the standing per-figure
// optional path of the attribution read (specs/036 FR-014, E-17).
auto readPrecheckTracerCpu(Element value, const std::string& where)
    -> std::expected<sg::PrecheckTracerCpuRecord, std::string>
{
  sg::PrecheckTracerCpuRecord read {};
  auto cpu = field(value, where, "cpu");
  if (!cpu.has_value()) {
    return std::unexpected(cpu.error());
  }
  auto cpuValue = asUnsigned(*cpu, where + "/cpu");
  if (!cpuValue.has_value()) {
    return std::unexpected(cpuValue.error());
  }
  read.cpu = static_cast<std::uint32_t>(*cpuValue);
  const auto readFigure = [&](std::string_view key,
                              std::optional<std::uint64_t>* into)
      -> std::expected<void, std::string>
  {
    auto found = field(value, where, key);
    if (!found.has_value()) {
      return std::unexpected(found.error());
    }
    auto figure = readNullableCount(*found, where + "/" + std::string(key));
    if (!figure.has_value()) {
      return std::unexpected(figure.error());
    }
    *into = std::move(*figure);
    return {};
  };
  auto settledValues = allSettled(readFigure("nmi", &read.nmi),
                                  readFigure("irq", &read.irq),
                                  readFigure("softirq", &read.softirq),
                                  readFigure("thread", &read.thread),
                                  readFigure("hardware", &read.hardware));
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  return read;
}

// The optional tracer record of a pre-check record (specs/036 FR-014,
// E-20): the engine word of the closed table, then one entry per tracer
// cpu. The engine reads through the standing `asEnum` reader the way the
// jitter `mode` reads, so a word outside the list refuses and names
// itself, and the record carries the word back.
auto readPrecheckTracer(Element value, const std::string& where)
    -> std::expected<sg::PrecheckTracerRecord, std::string>
{
  sg::PrecheckTracerRecord read {};
  auto engine = field(value, where, "engine");
  if (!engine.has_value()) {
    return std::unexpected(engine.error());
  }
  auto engineValue = asEnum<sg::TracerEngine>(
      detail::kTracerEngineWords, *engine, where + "/engine");
  if (!engineValue.has_value()) {
    return std::unexpected(engineValue.error());
  }
  read.engine = std::string(detail::enumWord(detail::kTracerEngineWords,
                                             std::to_underlying(*engineValue)));
  auto cpus = field(value, where, "cpus");
  if (!cpus.has_value()) {
    return std::unexpected(cpus.error());
  }
  auto cpuItems = asArray(*cpus, where + "/cpus");
  if (!cpuItems.has_value()) {
    return std::unexpected(cpuItems.error());
  }
  std::size_t index = 0;
  for (const auto item : *cpuItems) {
    auto entry =
        readPrecheckTracerCpu(item, where + "/cpus/" + std::to_string(index));
    if (!entry.has_value()) {
      return std::unexpected(entry.error());
    }
    read.cpus.push_back(std::move(*entry));
    ++index;
  }
  return read;
}

// The run-level pre-check record: the five keys the writer stands in
// order, one entry per measured cpu with its role and figures, the host
// settings, and the skipped groups (specs/034 FR-021, FR-024). Each key
// is read and kind-checked in order, so a wrong kind names itself before
// a later key's absence does (FR-029).
auto readPrecheckRecord(Element value, const std::string& where)
    -> std::expected<sg::PrecheckRecord, std::string>
{
  sg::PrecheckRecord read {};
  auto serial = field(value, where, "serial");
  if (!serial.has_value()) {
    return std::unexpected(serial.error());
  }
  auto serialFlag = asBool(*serial, where + "/serial");
  if (!serialFlag.has_value()) {
    return std::unexpected(serialFlag.error());
  }
  read.serial = *serialFlag;
  auto window = field(value, where, "windowMs");
  if (!window.has_value()) {
    return std::unexpected(window.error());
  }
  auto windowValue = asUnsigned(*window, where + "/windowMs");
  if (!windowValue.has_value()) {
    return std::unexpected(windowValue.error());
  }
  read.windowMs = static_cast<std::uint32_t>(*windowValue);
  auto cpus = field(value, where, "cpus");
  if (!cpus.has_value()) {
    return std::unexpected(cpus.error());
  }
  auto cpuItems = asArray(*cpus, where + "/cpus");
  if (!cpuItems.has_value()) {
    return std::unexpected(cpuItems.error());
  }
  std::size_t index = 0;
  for (const auto item : *cpuItems) {
    auto readEntry =
        readPrecheckCpu(item, where + "/cpus/" + std::to_string(index));
    if (!readEntry.has_value()) {
      return std::unexpected(readEntry.error());
    }
    read.cpus.push_back(std::move(*readEntry));
    ++index;
  }
  auto host = field(value, where, "hostSettings");
  if (!host.has_value()) {
    return std::unexpected(host.error());
  }
  auto hostRead = readPrecheckHost(*host, where + "/hostSettings");
  if (!hostRead.has_value()) {
    return std::unexpected(hostRead.error());
  }
  read.host = *hostRead;
  auto skipped = field(value, where, "skippedGroups");
  if (!skipped.has_value()) {
    return std::unexpected(skipped.error());
  }
  auto skippedItems = asArray(*skipped, where + "/skippedGroups");
  if (!skippedItems.has_value()) {
    return std::unexpected(skippedItems.error());
  }
  index = 0;
  for (const auto item : *skippedItems) {
    auto group =
        asUnsigned(item, where + "/skippedGroups/" + std::to_string(index));
    if (!group.has_value()) {
      return std::unexpected(group.error());
    }
    read.skippedGroups.push_back(static_cast<std::uint32_t>(*group));
    ++index;
  }
  // The optional tracer record of specs/036 (FR-014, E-20), read through
  // the standing `optionalField` path the way the root `hunt` reads: a
  // document written before the key, and a run whose tracer stands not
  // available, carry no key and leave the member empty (FR-015, R-11). A
  // key that stands present keeps its own kind and its own refusal.
  if (const auto tracer = optionalField(value, "tracer"); tracer.has_value()) {
    auto record = readPrecheckTracer(*tracer, where + "/tracer");
    if (!record.has_value()) {
      return std::unexpected(record.error());
    }
    read.tracer = std::move(*record);
  }
  return read;
}

// specs/037-execution-graphs FR-023, D-5: one node of the record. The three
// keys of a `nodes` entry stand in the order the schema's `x-order` states,
// and each is read and kind-checked in that order, so a wrong kind names
// itself before a later key's absence does (FR-029).
auto readGraphNode(Element value, const std::string& where)
    -> std::expected<sg::GraphNodeRecord, std::string>
{
  auto name = field(value, where, "name");
  if (!name.has_value()) {
    return std::unexpected(name.error());
  }
  auto nameText = asString(*name, where + "/name");
  if (!nameText.has_value()) {
    return std::unexpected(nameText.error());
  }
  auto slot = field(value, where, "slot");
  if (!slot.has_value()) {
    return std::unexpected(slot.error());
  }
  auto slotValue = asUnsigned(*slot, where + "/slot");
  if (!slotValue.has_value()) {
    return std::unexpected(slotValue.error());
  }
  auto outcome = field(value, where, "outcome");
  if (!outcome.has_value()) {
    return std::unexpected(outcome.error());
  }
  auto outcomeText = asString(*outcome, where + "/outcome");
  if (!outcomeText.has_value()) {
    return std::unexpected(outcomeText.error());
  }
  return sg::GraphNodeRecord {
      .name = std::move(*nameText),
      .slot = static_cast<std::uint32_t>(*slotValue),
      .outcome = std::move(*outcomeText),
  };
}

// One dependency pair of the record: the two keys of an `edges` entry, the
// pair of node names the graph file states (FR-014, FR-023).
auto readGraphEdge(Element value, const std::string& where)
    -> std::expected<sg::GraphEdgeRecord, std::string>
{
  auto from = field(value, where, "from");
  if (!from.has_value()) {
    return std::unexpected(from.error());
  }
  auto fromText = asString(*from, where + "/from");
  if (!fromText.has_value()) {
    return std::unexpected(fromText.error());
  }
  auto to = field(value, where, "to");
  if (!to.has_value()) {
    return std::unexpected(to.error());
  }
  auto toText = asString(*to, where + "/to");
  if (!toText.has_value()) {
    return std::unexpected(toText.error());
  }
  return sg::GraphEdgeRecord {.from = std::move(*fromText),
                              .to = std::move(*toText)};
}

// One held node of the record: the two keys of a `held` entry, the node and
// the failed node that gates it (FR-020, FR-023).
auto readGraphHeld(Element value, const std::string& where)
    -> std::expected<sg::GraphHeldRecord, std::string>
{
  auto node = field(value, where, "node");
  if (!node.has_value()) {
    return std::unexpected(node.error());
  }
  auto nodeText = asString(*node, where + "/node");
  if (!nodeText.has_value()) {
    return std::unexpected(nodeText.error());
  }
  auto reason = field(value, where, "reason");
  if (!reason.has_value()) {
    return std::unexpected(reason.error());
  }
  auto reasonText = asString(*reason, where + "/reason");
  if (!reasonText.has_value()) {
    return std::unexpected(reasonText.error());
  }
  return sg::GraphHeldRecord {.node = std::move(*nodeText),
                              .reason = std::move(*reasonText)};
}

// One `runs` entry: the five keys of the entry in the order the schema's
// `x-order` states, the run named by its benchmark and its four arrays
// keeping the order the writer wrote them (specs/037 FR-023, D-5).
auto readGraphRun(Element value, const std::string& where)
    -> std::expected<sg::GraphRunRecord, std::string>
{
  sg::GraphRunRecord read {};
  auto benchmark = field(value, where, "benchmark");
  if (!benchmark.has_value()) {
    return std::unexpected(benchmark.error());
  }
  auto benchmarkText = asString(*benchmark, where + "/benchmark");
  if (!benchmarkText.has_value()) {
    return std::unexpected(benchmarkText.error());
  }
  read.benchmark = std::move(*benchmarkText);
  auto nodes = field(value, where, "nodes");
  if (!nodes.has_value()) {
    return std::unexpected(nodes.error());
  }
  auto nodeItems = asArray(*nodes, where + "/nodes");
  if (!nodeItems.has_value()) {
    return std::unexpected(nodeItems.error());
  }
  std::size_t index = 0;
  for (const auto item : *nodeItems) {
    auto entry = readGraphNode(item, where + "/nodes/" + std::to_string(index));
    if (!entry.has_value()) {
      return std::unexpected(entry.error());
    }
    read.nodes.push_back(std::move(*entry));
    ++index;
  }
  auto edges = field(value, where, "edges");
  if (!edges.has_value()) {
    return std::unexpected(edges.error());
  }
  auto edgeItems = asArray(*edges, where + "/edges");
  if (!edgeItems.has_value()) {
    return std::unexpected(edgeItems.error());
  }
  index = 0;
  for (const auto item : *edgeItems) {
    auto entry = readGraphEdge(item, where + "/edges/" + std::to_string(index));
    if (!entry.has_value()) {
      return std::unexpected(entry.error());
    }
    read.edges.push_back(std::move(*entry));
    ++index;
  }
  auto held = field(value, where, "held");
  if (!held.has_value()) {
    return std::unexpected(held.error());
  }
  auto heldItems = asArray(*held, where + "/held");
  if (!heldItems.has_value()) {
    return std::unexpected(heldItems.error());
  }
  index = 0;
  for (const auto item : *heldItems) {
    auto entry = readGraphHeld(item, where + "/held/" + std::to_string(index));
    if (!entry.has_value()) {
      return std::unexpected(entry.error());
    }
    read.held.push_back(std::move(*entry));
    ++index;
  }
  auto stopped = field(value, where, "stopped");
  if (!stopped.has_value()) {
    return std::unexpected(stopped.error());
  }
  auto stoppedItems = asArray(*stopped, where + "/stopped");
  if (!stoppedItems.has_value()) {
    return std::unexpected(stoppedItems.error());
  }
  index = 0;
  for (const auto item : *stoppedItems) {
    auto name = asString(item, where + "/stopped/" + std::to_string(index));
    if (!name.has_value()) {
      return std::unexpected(name.error());
    }
    read.stopped.push_back(std::move(*name));
    ++index;
  }
  return read;
}

// The run-level graph record of specs/037: the object's one key `runs`, an
// array with one entry per threaded run in the batch's order (FR-023, D-5).
// The reader checks the record and lets the values go, the shape the
// pre-check record read takes; each key is read and kind-checked in order,
// so a wrong kind names itself before a later key's absence does (FR-029).
auto readGraphRecord(Element value, const std::string& where)
    -> std::expected<sg::GraphRecord, std::string>
{
  sg::GraphRecord read {};
  auto runs = field(value, where, "runs");
  if (!runs.has_value()) {
    return std::unexpected(runs.error());
  }
  auto runItems = asArray(*runs, where + "/runs");
  if (!runItems.has_value()) {
    return std::unexpected(runItems.error());
  }
  std::size_t index = 0;
  for (const auto item : *runItems) {
    auto entry = readGraphRun(item, where + "/runs/" + std::to_string(index));
    if (!entry.has_value()) {
      return std::unexpected(entry.error());
    }
    read.runs.push_back(std::move(*entry));
    ++index;
  }
  return read;
}

// The run-level jitter record: the seven keys of E-07 in the kinds the
// schema states. The reader checks the record and lets the values go,
// the shape the noise warning read uses (E-07, E-08, FR-029); each key
// is read and kind-checked in order, so a wrong kind names itself
// before a later key's absence does (FR-029).
auto readJitterRecord(Element value, const std::string& where)
    -> std::expected<std::vector<sg::JitterEntryFold>, std::string>
{
  const auto readKey =
      [&](std::string_view key) -> std::expected<Element, std::string>
  { return field(value, where, key); };
  auto mode = readKey("mode").and_then(
      [&](Element found)
      {
        return asEnum<sg::JitterProbeMode>(
            detail::kJitterModeWords, found, where + "/mode");
      });
  if (!mode.has_value()) {
    return std::unexpected(mode.error());
  }
  auto threshold =
      readKey("thresholdNs")
          .and_then([&](Element found)
                    { return asUnsigned(found, where + "/thresholdNs"); });
  if (!threshold.has_value()) {
    return std::unexpected(threshold.error());
  }
  auto period =
      readKey("loopPeriodNs")
          .and_then([&](Element found)
                    { return asUnsigned(found, where + "/loopPeriodNs"); });
  if (!period.has_value()) {
    return std::unexpected(period.error());
  }
  auto cpus = readKey("cpus").and_then(
      [&](Element found) { return asArray(found, where + "/cpus"); });
  if (!cpus.has_value()) {
    return std::unexpected(cpus.error());
  }
  for (const auto item : *cpus) {
    auto number = asUnsigned(item, where + "/cpus");
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
  }
  auto stats = readKey("stats").and_then(
      [&](Element found) { return asArray(found, where + "/stats"); });
  if (!stats.has_value()) {
    return std::unexpected(stats.error());
  }
  std::vector<sg::JitterEntryFold> folds;
  std::size_t index = 0;
  for (const auto item : *stats) {
    auto read = readJitterCpu(item, where + "/stats/" + std::to_string(index));
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    // E-07: the record reads back into the carrier a `jitter:run:`
    // comparison rule reads, the same shape the per-entry stats ride
    // (FR-031).
    folds.push_back({.cpu = read->first, .stats = read->second});
    ++index;
  }
  auto dropped =
      readKey("droppedGaps")
          .and_then([&](Element found)
                    { return asUnsigned(found, where + "/droppedGaps"); });
  if (!dropped.has_value()) {
    return std::unexpected(dropped.error());
  }
  auto disclosure =
      readKey("disclosure")
          .and_then([&](Element found)
                    { return asString(found, where + "/disclosure"); });
  if (!disclosure.has_value()) {
    return std::unexpected(disclosure.error());
  }
  return folds;
}

auto readRow(Element value,
             const std::string& where,
             sg::ResultRow& row,
             RowRaw& raw,
             std::vector<RowRaw>& contextRaw,
             std::vector<std::vector<std::vector<sg::PercentileValue>>>&
                 contextPercentiles) -> std::expected<void, std::string>
{
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts;
  for (const auto* key : {"kind",
                          "label",
                          "iterations",
                          "timePerIterationNs",
                          "cpuTimePerIterationNs",
                          "endpointSamples",
                          "pauseSamplesFolded",
                          "timeAvailability",
                          "metrics",
                          "counters",
                          "counterDisclosure",
                          "fit",
                          "overheadFloorNs",
                          "percentiles",
                          "chunkCount",
                          "chunkIterations",
                          "timeStats",
                          "metricStats"})
  {
    parts.emplace_back(key, field(value, where, key));
  }
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  const auto at = [&parts](std::size_t index) noexcept -> Element
  { return parts[index].second.value(); };
  auto kind =
      asEnum<sg::RowKind>(detail::kRowKindWords, at(0), where + "/kind");
  auto iterations = asUnsigned(at(2), where + "/iterations");
  auto time = asDouble(at(3), where + "/timePerIterationNs");
  auto cpuTime = asDouble(at(4), where + "/cpuTimePerIterationNs");
  auto samples = asUnsigned(at(5), where + "/endpointSamples");
  auto folded = asBool(at(6), where + "/pauseSamplesFolded");
  auto availability = asEnum<sg::counters::Availability>(
      detail::kAvailabilityWords, at(7), where + "/timeAvailability");
  auto settledValues = allSettled(
      kind, iterations, time, cpuTime, samples, folded, availability);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  row.kind = *kind;
  row.iterations = *iterations;
  row.timePerIterationNs = *time;
  row.cpuTimePerIterationNs = *cpuTime;
  row.endpointSamples = *samples;
  row.pauseSamplesFolded = *folded;
  row.timeAvailability = *availability;
  if (!at(1).is_null()) {
    auto text = asString(at(1), where + "/label");
    if (!text.has_value()) {
      return std::unexpected(text.error());
    }
    row.label = *text;
  }
  auto metrics = asArray(at(8), where + "/metrics");
  if (!metrics.has_value()) {
    return std::unexpected(metrics.error());
  }
  std::size_t metricIndex = 0;
  for (const auto item : *metrics) {
    auto metric = readMetricValue(
        item, where + "/metrics/" + std::to_string(metricIndex));
    if (!metric.has_value()) {
      return std::unexpected(metric.error());
    }
    row.metrics.push_back(*metric);
    ++metricIndex;
  }
  auto counters = asArray(at(9), where + "/counters");
  if (!counters.has_value()) {
    return std::unexpected(counters.error());
  }
  for (const auto item : *counters) {
    auto number = asDouble(item, where + "/counters");
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    row.counters.push_back(*number);
  }
  if (!at(10).is_null()) {
    auto text = asString(at(10), where + "/counterDisclosure");
    if (!text.has_value()) {
      return std::unexpected(text.error());
    }
    row.counterDisclosure = *text;
  }
  if (!at(11).is_null()) {
    auto fit = readFit(at(11), where + "/fit");
    if (!fit.has_value()) {
      return std::unexpected(fit.error());
    }
    row.fit = *fit;
  }
  auto floor = asDouble(at(12), where + "/overheadFloorNs");
  if (!floor.has_value()) {
    return std::unexpected(floor.error());
  }
  row.overheadFloorNs = *floor;
  auto percentiles = readPercentileColumns(at(13), where + "/percentiles");
  if (!percentiles.has_value()) {
    return std::unexpected(percentiles.error());
  }
  row.percentiles = std::move(*percentiles);
  auto chunkCount = asUnsigned(at(14), where + "/chunkCount");
  auto chunkIterations = asUnsigned(at(15), where + "/chunkIterations");
  auto settledChunks = allSettled(chunkCount, chunkIterations);
  if (!settledChunks.has_value()) {
    return std::unexpected(settledChunks.error());
  }
  row.chunkCount = *chunkCount;
  row.chunkIterations = *chunkIterations;
  auto timeStats = readStatisticsOrNull(at(16), where + "/timeStats");
  if (!timeStats.has_value()) {
    return std::unexpected(timeStats.error());
  }
  row.timeStats = *timeStats;
  auto metricStats = asArray(at(17), where + "/metricStats");
  if (!metricStats.has_value()) {
    return std::unexpected(metricStats.error());
  }
  for (const auto item : *metricStats) {
    auto stat = readStatisticsOrNull(item, where + "/metricStats");
    if (!stat.has_value()) {
      return std::unexpected(stat.error());
    }
    row.metricStats.push_back(*stat);
  }
  if (const auto context = field(value, where, "contextPercentiles");
      context.has_value())
  {
    auto items = asArray(*context, where + "/contextPercentiles");
    if (!items.has_value()) {
      return std::unexpected(items.error());
    }
    // One entry of the flattened field per context of this row, the
    // order the writer laid the blocks down in.
    for (const auto item : *items) {
      auto read = readPercentileColumns(item, where + "/contextPercentiles");
      if (!read.has_value()) {
        return std::unexpected(read.error());
      }
      contextPercentiles.push_back(std::move(*read));
    }
  }
  if (const auto contexts = field(value, where, "contextRaw");
      contexts.has_value())
  {
    auto items = asArray(*contexts, where + "/contextRaw");
    if (!items.has_value()) {
      return std::unexpected(items.error());
    }
    for (const auto item : *items) {
      auto columns = readRawColumns(item, where + "/contextRaw");
      if (!columns.has_value()) {
        return std::unexpected(columns.error());
      }
      contextRaw.push_back(*columns);
    }
  }
  if (const auto rawKey = field(value, where, "raw"); rawKey.has_value()) {
    auto columns = readRawColumns(*rawKey, where + "/raw");
    if (!columns.has_value()) {
      return std::unexpected(columns.error());
    }
    raw = *columns;
    // FR-024: a chunk column whose size disagrees with the chunk count
    // of its row is a document the reader refuses.
    for (const auto& column : raw.columns) {
      if (column.size() != row.chunkCount) {
        return std::unexpected(where + "/raw: a chunk column holds "
                               + std::to_string(column.size())
                               + " values beside a chunk count of "
                               + std::to_string(row.chunkCount));
      }
    }
  }
  return {};
}

// The hunt word tables of specs/029 (contracts/hunt-json.md). The document
// spells `norm` and `rank` with the lower-case words the option takes and
// `skipped[].kind` with the words the console prints for a refusal, so no
// table here is `kAvailabilityWords`, which spells the enumerator. The
// writer's three tables stand inside `json_report.cpp`, so the reader keeps
// its own copy beside its own keys; each is indexed by its own enumeration
// and tied to its last enumerator, the shape the tables of the header take
// (E-02, E-06, FR-050).
constexpr std::array<std::string_view, 3> kHuntNormWords {
    "instr",
    "cycle",
    "iter",
};
static_assert(kHuntNormWords.size()
              == std::to_underlying(hunt::HuntNorm::ITER) + 1);

constexpr std::array<std::string_view, 4> kHuntRankWords {
    "rate",
    "cv",
    "sensitivity",
    "delta",
};
static_assert(kHuntRankWords.size()
              == std::to_underlying(hunt::HuntRank::DELTA) + 1);

constexpr std::array<std::string_view, 5> kSkippedKindWords {
    "permission-blocked",
    "not-encodable",
    "absent",
    "scope-refused",
    "gap",
};
static_assert(kSkippedKindWords.size()
              == std::to_underlying(hunt::SkippedKind::GAP) + 1);

// The bound of the run record, in the spelling the document carries: the
// amount with the unit the caller named it in, `120s` or `8p`. The record
// holds the two figures apart, so a text that names no unit or spells no
// amount is a document the reader refuses by its pointer (FR-012, FR-051).
auto asBudget(Element value, const std::string& where)
    -> std::expected<hunt::HuntBudget, std::string>
{
  auto text = asString(value, where);
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  const char unit = text->empty() ? '\0' : text->back();
  if (unit != 's' && unit != 'p') {
    return std::unexpected(where + ": '" + *text
                           + "' states no budget unit the option takes");
  }
  std::uint64_t amount = 0;
  const auto* const end = text->data() + text->size() - 1;
  const auto [stop, code] = std::from_chars(text->data(), end, amount);
  if (code != std::errc {} || stop != end) {
    return std::unexpected(where + ": '" + *text + "' states no budget amount");
  }
  return hunt::HuntBudget {
      .kind = unit == 'p' ? hunt::HuntBudget::Kind::PASSES
                          : hunt::HuntBudget::Kind::SECONDS,
      .amount = amount,
  };
}

// specs/033-hunter-budgets FR-001, FR-021, N-01: the `e` clause text of the
// sample budget, in the spelling the document carries. The record holds whole
// nanoseconds, so the reader splits the seconds at the point and moves each
// fraction digit into the count, the split `detail::huntSampleClauseText`
// makes in the other direction. A text that names no unit, states no amount,
// or states a figure outside the bound the option carries is a document the
// reader refuses by its pointer, the way `asBudget` refuses the two-stage
// bound.
auto asSampleBudget(Element value, const std::string& where)
    -> std::expected<std::uint64_t, std::string>
{
  constexpr std::uint64_t kNsPerSecond = 1'000'000'000ULL;
  auto text = asString(value, where);
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  if (text->empty() || text->back() != 'e') {
    return std::unexpected(where + ": '" + *text
                           + "' states no sample budget unit");
  }
  const std::string_view amount {text->data(), text->size() - 1};
  const auto point = amount.find('.');
  const auto* const digits = amount.data();
  const auto* const wholeEnd =
      digits + (point == std::string_view::npos ? amount.size() : point);
  auto seconds = std::uint64_t {0};
  const auto [stop, code] = std::from_chars(digits, wholeEnd, seconds);
  if (code != std::errc {} || stop != wholeEnd
      || seconds > static_cast<std::uint64_t>(detail::kMaxWholeSeconds))
  {
    return std::unexpected(where + ": '" + *text
                           + "' states no sample budget amount");
  }
  auto budgetNs = seconds * kNsPerSecond;
  auto scale = kNsPerSecond;
  const auto fraction =
      point == std::string_view::npos ? amount.size() : point + 1;
  for (std::size_t at = fraction; at < amount.size(); ++at) {
    const char digit = amount[at];
    if (digit < '0' || digit > '9' || scale == 1) {
      return std::unexpected(where + ": '" + *text
                             + "' states no sample budget amount");
    }
    scale /= 10;
    budgetNs += static_cast<std::uint64_t>(digit - '0') * scale;
  }
  return budgetNs;
}

// specs/033-hunter-budgets FR-003, FR-021, N-02: the `g` clause text of the
// multiplex cap, the whole count and the letter `g` (`32g`). A text that names
// no unit or states no whole count is a document the reader refuses by its
// pointer.
auto asMultiplexCap(Element value, const std::string& where)
    -> std::expected<std::size_t, std::string>
{
  auto text = asString(value, where);
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  if (text->empty() || text->back() != 'g') {
    return std::unexpected(where + ": '" + *text
                           + "' states no multiplex cap unit");
  }
  std::uint64_t count = 0;
  const auto* const end = text->data() + text->size() - 1;
  const auto [stop, code] = std::from_chars(text->data(), end, count);
  if (code != std::errc {} || stop != end) {
    return std::unexpected(where + ": '" + *text
                           + "' states no whole multiplex cap count");
  }
  return static_cast<std::size_t>(count);
}

// One object of the closed `hunt.stages` node: the range of planned
// addresses one stage measured, and the repeats it gave them (E-08, FR-050).
auto readStageRange(Element value, const std::string& where)
    -> std::expected<hunt::HuntStageRange, std::string>
{
  if (!value.is_object()) {
    return std::unexpected(wrongKind(where, value, "an object"));
  }
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"first", field(value, where, "first")},
          {"count", field(value, where, "count")},
          {"repeats", field(value, where, "repeats")},
      };
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto first = asUnsigned(parts[0].second.value(), where + "/first");
  auto count = asUnsigned(parts[1].second.value(), where + "/count");
  auto repeats = asUnsigned(parts[2].second.value(), where + "/repeats");
  auto settledValues = allSettled(first, count, repeats);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  return hunt::HuntStageRange {
      .first = static_cast<std::size_t>(*first),
      .count = static_cast<std::size_t>(*count),
      .repeats = static_cast<std::size_t>(*repeats),
  };
}

// One object of the closed `hunt.entries` node: one column of the rank table
// (E-06, FR-050). The sample count and the repeated mark stay out of the
// document (D-17), so the record keeps the two defaults a rank the console
// prints unmarked carries.
auto readRankedEvent(Element value, const std::string& where)
    -> std::expected<hunt::RankedEvent, std::string>
{
  if (!value.is_object()) {
    return std::unexpected(wrongKind(where, value, "an object"));
  }
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"address", field(value, where, "address")},
          {"rate", field(value, where, "rate")},
          {"cv", field(value, where, "cv")},
          {"noisy", field(value, where, "noisy")},
          {"multiplexed", field(value, where, "multiplexed")},
          {"rank", field(value, where, "rank")},
      };
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto address = asString(parts[0].second.value(), where + "/address");
  auto rate = asDouble(parts[1].second.value(), where + "/rate");
  auto variation = asDouble(parts[2].second.value(), where + "/cv");
  auto noisy = asBool(parts[3].second.value(), where + "/noisy");
  auto multiplexed = asBool(parts[4].second.value(), where + "/multiplexed");
  auto settledValues = allSettled(address, rate, variation, noisy, multiplexed);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  hunt::RankedEvent event {
      .address = *address,
      .rate = *rate,
      .cv = *variation,
      .samples = 0,
      .noisy = *noisy,
      .repeated = false,
      .multiplexed = *multiplexed,
      .rank = std::nullopt,
      .sensitivityCorrelation = std::nullopt,
      .sensitivitySlope = std::nullopt,
      .delta = std::nullopt,
  };
  // A noisy event, a single-sample event and an event outside the top N
  // carry no rank, which the document writes `null` (D-7).
  const Element rank = parts[5].second.value();
  if (!rank.is_null()) {
    auto number = asUnsigned(rank, where + "/rank");
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    event.rank = static_cast<std::size_t>(*number);
  }
  // The three rank figures of specs/040 (FR-030, B-9): the writer stands
  // each key in every hunt document at minor 1.9, `null` for a figure the
  // run did not measure. A document an older build wrote carries none of
  // them, so an absent key leaves its member empty, the reading of a run
  // that stamped no figure, and a key that stands present keeps its own
  // kind and its own refusal (FR-031).
  const auto readFigure =
      [&](std::string_view key,
          std::optional<double>* into) -> std::expected<void, std::string>
  {
    const auto found = optionalField(value, key);
    if (!found.has_value() || found->is_null()) {
      return {};
    }
    auto number = asDouble(*found, where + "/" + std::string(key));
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    *into = *number;
    return {};
  };
  auto settledFigures =
      allSettled(readFigure("correlation", &event.sensitivityCorrelation),
                 readFigure("slope", &event.sensitivitySlope),
                 readFigure("delta", &event.delta));
  if (!settledFigures.has_value()) {
    return std::unexpected(settledFigures.error());
  }
  return event;
}

// One object of the closed `hunt.skipped` node: one event the run refused,
// with the kind of the refusal spelled as the console spells it (E-06,
// FR-050).
auto readSkippedEvent(Element value, const std::string& where)
    -> std::expected<hunt::SkippedEvent, std::string>
{
  if (!value.is_object()) {
    return std::unexpected(wrongKind(where, value, "an object"));
  }
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"address", field(value, where, "address")},
          {"kind", field(value, where, "kind")},
      };
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto address = asString(parts[0].second.value(), where + "/address");
  auto kind = asEnum<hunt::SkippedKind>(
      kSkippedKindWords, parts[1].second.value(), where + "/kind");
  auto settledValues = allSettled(address, kind);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  return hunt::SkippedEvent {
      .address = *address,
      .kind = *kind,
  };
}

// The per-entry hunt object of specs/029 (contracts/hunt-json.md). Every key
// of the closed node stands in the document of a hunt entry, so a key that
// stands missing is a refusal naming the pointer, and a value the run left
// unset stands `null` rather than away (FR-051).
auto readEntryHunt(Element value, const std::string& where)
    -> std::expected<hunt::EntryHunt, std::string>
{
  if (!value.is_object()) {
    return std::unexpected(wrongKind(where, value, "an object"));
  }
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts {
          {"method", field(value, where, "method")},
          {"pass_count", field(value, where, "pass_count")},
          {"top", field(value, where, "top")},
          {"retried_passes", field(value, where, "retried_passes")},
          {"entries", field(value, where, "entries")},
          {"skipped", field(value, where, "skipped")},
      };
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  auto method = asEnum<sg::MetricMethod>(
      detail::kMetricMethodWords, parts[0].second.value(), where + "/method");
  auto passCount = asUnsigned(parts[1].second.value(), where + "/pass_count");
  auto top = asUnsigned(parts[2].second.value(), where + "/top");
  auto retried = asUnsigned(parts[3].second.value(), where + "/retried_passes");
  auto settledValues = allSettled(method, passCount, top, retried);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  hunt::EntryHunt record {
      .method = *method,
      .passCount = static_cast<std::size_t>(*passCount),
      .top = static_cast<std::size_t>(*top),
      .entries = {},
      .skipped = {},
      .retriedPasses = static_cast<std::size_t>(*retried),
  };
  auto entries = asArray(parts[4].second.value(), where + "/entries");
  if (!entries.has_value()) {
    return std::unexpected(entries.error());
  }
  std::size_t index = 0;
  for (const auto item : *entries) {
    auto read =
        readRankedEvent(item, where + "/entries/" + std::to_string(index));
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    record.entries.push_back(*read);
    ++index;
  }
  auto skipped = asArray(parts[5].second.value(), where + "/skipped");
  if (!skipped.has_value()) {
    return std::unexpected(skipped.error());
  }
  index = 0;
  for (const auto item : *skipped) {
    auto read =
        readSkippedEvent(item, where + "/skipped/" + std::to_string(index));
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    record.skipped.push_back(*read);
    ++index;
  }
  return record;
}

// The run-level hunt object of specs/029 (contracts/hunt-json.md): the
// settled option values beside the plan the run ran (E-08, PD-10). As in the
// entry record, every key of the closed node is required.
auto readHunt(Element value, const std::string& where)
    -> std::expected<hunt::RunHunt, std::string>
{
  if (!value.is_object()) {
    return std::unexpected(wrongKind(where, value, "an object"));
  }
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts;
  for (const auto* key : {"selection",
                          "norm",
                          "rank",
                          "top",
                          "noisePercent",
                          "repeats",
                          "seed",
                          "budget",
                          "passes",
                          "method",
                          "events",
                          "stages",
                          "estimate_ns",
                          "planned_events",
                          "ran_events",
                          "stage_two_events"})
  {
    parts.emplace_back(key, field(value, where, key));
  }
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  const auto at = [&parts](std::size_t index) noexcept -> Element
  { return parts[index].second.value(); };
  auto selection = asString(at(0), where + "/selection");
  auto norm = asEnum<hunt::HuntNorm>(kHuntNormWords, at(1), where + "/norm");
  auto rank = asEnum<hunt::HuntRank>(kHuntRankWords, at(2), where + "/rank");
  auto top = asUnsigned(at(3), where + "/top");
  auto noise = asDouble(at(4), where + "/noisePercent");
  auto repeats = asUnsigned(at(5), where + "/repeats");
  auto seed = asUnsigned(at(6), where + "/seed");
  auto passes = asUnsigned(at(8), where + "/passes");
  auto method = asEnum<sg::MetricMethod>(
      detail::kMetricMethodWords, at(9), where + "/method");
  // `planned_events` is the size of `events` restated (D-17), so its kind is
  // checked here and the addresses themselves are kept once, below.
  auto planned = asUnsigned(at(13), where + "/planned_events");
  auto ran = asUnsigned(at(14), where + "/ran_events");
  auto stageTwo = asUnsigned(at(15), where + "/stage_two_events");
  auto settledValues = allSettled(selection,
                                  norm,
                                  rank,
                                  top,
                                  noise,
                                  repeats,
                                  seed,
                                  passes,
                                  method,
                                  planned,
                                  ran,
                                  stageTwo);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  hunt::RunHunt record;
  record.selection = *selection;
  record.options.norm = *norm;
  record.options.rank = *rank;
  record.options.top = static_cast<std::size_t>(*top);
  record.options.noise = *noise;
  record.options.repeats = static_cast<std::size_t>(*repeats);
  record.options.seed = *seed;
  record.passes = static_cast<std::size_t>(*passes);
  record.method = *method;
  record.ranEvents = static_cast<std::size_t>(*ran);
  record.stageTwoEvents = static_cast<std::size_t>(*stageTwo);
  // The two values a run may leave unset stand `null`: the budget of a run
  // that named no bound and the estimate of a plan that calibrated nothing.
  if (!at(7).is_null()) {
    auto budget = asBudget(at(7), where + "/budget");
    if (!budget.has_value()) {
      return std::unexpected(budget.error());
    }
    record.options.budget = *budget;
  }
  if (!at(12).is_null()) {
    auto estimate = asUnsigned(at(12), where + "/estimate_ns");
    if (!estimate.has_value()) {
      return std::unexpected(estimate.error());
    }
    record.estimateNs = *estimate;
  }
  auto events = asArray(at(10), where + "/events");
  if (!events.has_value()) {
    return std::unexpected(events.error());
  }
  for (const auto item : *events) {
    auto address = asString(item, where + "/events");
    if (!address.has_value()) {
      return std::unexpected(address.error());
    }
    record.plannedAddresses.push_back(*address);
  }
  auto stages = asArray(at(11), where + "/stages");
  if (!stages.has_value()) {
    return std::unexpected(stages.error());
  }
  std::size_t index = 0;
  for (const auto item : *stages) {
    auto stage =
        readStageRange(item, where + "/stages/" + std::to_string(index));
    if (!stage.has_value()) {
      return std::unexpected(stage.error());
    }
    record.stages.push_back(*stage);
    ++index;
  }
  // specs/033-hunter-budgets FR-020 to FR-022: the four keys the budgets add
  // stand optional, so a document written before schema 1.7 carries none of
  // them and the four fields stay unset (FR-021). A key that stands present
  // keeps its own kind and its own refusal.
  if (const auto key = optionalField(value, "sample_budget"); key.has_value()) {
    auto sample = asSampleBudget(*key, where + "/sample_budget");
    if (!sample.has_value()) {
      return std::unexpected(sample.error());
    }
    record.options.sampleBudgetNs = *sample;
  }
  if (const auto key = optionalField(value, "multiplex_cap"); key.has_value()) {
    auto cap = asMultiplexCap(*key, where + "/multiplex_cap");
    if (!cap.has_value()) {
      return std::unexpected(cap.error());
    }
    record.options.multiplexCap = *cap;
  }
  if (const auto key = optionalField(value, "derived_repeats"); key.has_value())
  {
    auto derived = asUnsigned(*key, where + "/derived_repeats");
    if (!derived.has_value()) {
      return std::unexpected(derived.error());
    }
    record.sample.derivedRepeats = static_cast<std::size_t>(*derived);
  }
  if (const auto key = optionalField(value, "mux_groups"); key.has_value()) {
    auto sizes = asArray(*key, where + "/mux_groups");
    if (!sizes.has_value()) {
      return std::unexpected(sizes.error());
    }
    std::size_t group = 0;
    for (const auto item : *sizes) {
      auto size =
          asUnsigned(item, where + "/mux_groups/" + std::to_string(group));
      if (!size.has_value()) {
        return std::unexpected(size.error());
      }
      record.muxGroups.push_back(static_cast<std::size_t>(*size));
      ++group;
    }
  }
  return record;
}

auto readEntry(Element value,
               const std::string& where,
               BenchmarkResult& result,
               std::vector<RowRaw>& rowRaw,
               std::vector<std::vector<RowRaw>>& contextRaw,
               std::optional<NoiseWarning>& warning)
    -> std::expected<void, std::string>
{
  std::vector<std::pair<std::string, std::expected<Element, std::string>>>
      parts;
  for (const auto* key : {"name",
                          "suite",
                          "case",
                          "outcome",
                          "reason",
                          "hideRepetitionRows",
                          "timeUnit",
                          "primaryIsCpu",
                          "pauseSamplesFolded",
                          "percentilePoints",
                          "contexts",
                          "rows",
                          "metricLabels",
                          "counterLabels",
                          "counterStats",
                          "statisticValues"})
  {
    parts.emplace_back(key, field(value, where, key));
  }
  auto keys = settled(parts);
  if (!keys.has_value()) {
    return std::unexpected(keys.error());
  }
  const auto at = [&parts](std::size_t index) noexcept -> Element
  { return parts[index].second.value(); };
  auto name = asString(at(0), where + "/name");
  auto suite = asString(at(1), where + "/suite");
  auto caseName = asString(at(2), where + "/case");
  auto outcome =
      asEnum<sg::RunOutcome>(detail::kOutcomeWords, at(3), where + "/outcome");
  auto hide = asBool(at(5), where + "/hideRepetitionRows");
  auto timeUnit =
      asEnum<sg::TimeUnit>(detail::kTimeUnitWords, at(6), where + "/timeUnit");
  auto primaryIsCpu = asBool(at(7), where + "/primaryIsCpu");
  auto folded = asBool(at(8), where + "/pauseSamplesFolded");
  auto settledValues = allSettled(
      name, suite, caseName, outcome, hide, timeUnit, primaryIsCpu, folded);
  if (!settledValues.has_value()) {
    return std::unexpected(settledValues.error());
  }
  result.name = *name;
  result.suite = *suite;
  result.caseName = *caseName;
  result.outcome = *outcome;
  result.hideRepetitionRows = *hide;
  result.timeUnit = *timeUnit;
  result.primaryIsCpu = *primaryIsCpu;
  result.pauseSamplesFolded = *folded;
  if (!at(4).is_null()) {
    auto text = asString(at(4), where + "/reason");
    if (!text.has_value()) {
      return std::unexpected(text.error());
    }
    result.reason = *text;
  }
  auto points = asArray(at(9), where + "/percentilePoints");
  if (!points.has_value()) {
    return std::unexpected(points.error());
  }
  for (const auto item : *points) {
    auto number = asDouble(item, where + "/percentilePoints");
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    result.percentilePoints.push_back(*number);
  }
  auto contexts = asArray(at(10), where + "/contexts");
  if (!contexts.has_value()) {
    return std::unexpected(contexts.error());
  }
  std::size_t position = 0;
  for (const auto item : *contexts) {
    const std::string here = where + "/contexts/" + std::to_string(position);
    auto waitMode = field(item, here, "waitMode");
    auto placement = field(item, here, "placement");
    auto applied = field(item, here, "applied");
    auto settledContext = allSettled(waitMode, placement, applied);
    if (!settledContext.has_value()) {
      return std::unexpected(settledContext.error());
    }
    auto mode = asEnum<sg::WaitMode>(
        detail::kWaitModeWords, *waitMode, here + "/waitMode");
    auto text = asString(*placement, here + "/placement");
    auto flag = asBool(*applied, here + "/applied");
    auto settledFields = allSettled(mode, text, flag);
    if (!settledFields.has_value()) {
      return std::unexpected(settledFields.error());
    }
    // specs/039 FR-031: the name of the group that holds this context stands
    // optional, so the document of a run that carries no partition, and every
    // document written before the key, loads as the record naming no group. A
    // name that stands present keeps its own kind and its own refusal
    // (specs/024 FR-038).
    auto group = std::string();
    if (const auto named = optionalField(item, "group"); named.has_value()) {
      auto spelled = asString(*named, here + "/group");
      if (!spelled.has_value()) {
        return std::unexpected(spelled.error());
      }
      group = *spelled;
    }
    result.contexts.push_back(sg::ContextDisclosure {
        .waitMode = *mode,
        .placement = *text,
        .applied = *flag,
        .group = group,
    });
    ++position;
  }
  if (const auto noise = field(value, where, "noiseWarning");
      noise.has_value() && !noise->is_null())
  {
    // The three fields of the warning are the account the console line
    // prints and the document carries; the result value holds no field
    // for them, so the reader checks the record and stores it beside the
    // result it warns of (FR-017, FR-070, E-06, E-10).
    auto spinning = field(*noise, where + "/noiseWarning", "spinning");
    auto runnable = field(*noise, where + "/noiseWarning", "runnableCpus");
    auto shares = field(*noise, where + "/noiseWarning", "sharesOneCpu");
    auto settledNoise = allSettled(spinning, runnable, shares);
    if (!settledNoise.has_value()) {
      return std::unexpected(settledNoise.error());
    }
    auto spinningCount =
        asUnsigned(*spinning, where + "/noiseWarning/spinning");
    auto runnableCount =
        asUnsigned(*runnable, where + "/noiseWarning/runnableCpus");
    auto oneCpu = asBool(*shares, where + "/noiseWarning/sharesOneCpu");
    auto settledRecord = allSettled(spinningCount, runnableCount, oneCpu);
    if (!settledRecord.has_value()) {
      return std::unexpected(settledRecord.error());
    }
    warning = NoiseWarning {
        .spinning = *spinningCount,
        .runnableCpus = *runnableCount,
        .sharesOneCpu = *oneCpu,
    };
  }
  if (const auto jitter = field(value, where, "jitter");
      jitter.has_value() && !jitter->is_null())
  {
    // E-08: the object form carries `stats`, one object per probe cpu
    // over the entry's windows; the reader checks the record and lets
    // the values go (FR-027, E-08).
    const std::string here = where + "/jitter";
    auto stats = field(*jitter, here, "stats");
    if (!stats.has_value()) {
      return std::unexpected(stats.error());
    }
    auto items = asArray(*stats, here + "/stats");
    if (!items.has_value()) {
      return std::unexpected(items.error());
    }
    std::size_t index = 0;
    for (const auto item : *items) {
      auto read = readJitterCpu(item, here + "/stats/" + std::to_string(index));
      if (!read.has_value()) {
        return std::unexpected(read.error());
      }
      // E-08, FR-029: the object reads back into the result's carrier,
      // the value the round trip compares equal (SC-001).
      result.jitterFolds.push_back(sg::JitterEntryFold {
          .cpu = read->first,
          .stats = read->second,
      });
      ++index;
    }
  }
  auto rows = asArray(at(11), where + "/rows");
  if (!rows.has_value()) {
    return std::unexpected(rows.error());
  }
  position = 0;
  for (const auto item : *rows) {
    const std::string here = where + "/rows/" + std::to_string(position);
    result.rows.emplace_back();
    rowRaw.emplace_back();
    contextRaw.emplace_back();
    auto read = readRow(item,
                        here,
                        result.rows.back(),
                        rowRaw.back(),
                        contextRaw.back(),
                        result.contextPercentiles);
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    ++position;
  }
  for (const std::pair<std::string_view, Element> labels :
       {std::pair {"metricLabels", at(12)},
        std::pair {"counterLabels", at(13)}})
  {
    auto items =
        asArray(labels.second, where + "/" + std::string(labels.first));
    if (!items.has_value()) {
      return std::unexpected(items.error());
    }
    auto& target = labels.first == "metricLabels" ? result.metricLabels
                                                  : result.counterLabels;
    for (const auto item : *items) {
      auto text = asString(item, where + "/" + std::string(labels.first));
      if (!text.has_value()) {
        return std::unexpected(text.error());
      }
      target.push_back(*text);
    }
  }
  auto stats = asArray(at(14), where + "/counterStats");
  if (!stats.has_value()) {
    return std::unexpected(stats.error());
  }
  std::size_t statIndex = 0;
  for (const auto item : *stats) {
    auto stat = readStatistics(
        item, where + "/counterStats/" + std::to_string(statIndex));
    if (!stat.has_value()) {
      return std::unexpected(stat.error());
    }
    result.counterStats.push_back(*stat);
    ++statIndex;
  }
  auto values = asArray(at(15), where + "/statisticValues");
  if (!values.has_value()) {
    return std::unexpected(values.error());
  }
  std::size_t valueIndex = 0;
  for (const auto item : *values) {
    const std::string here =
        where + "/statisticValues/" + std::to_string(valueIndex);
    auto label = field(item, here, "name");
    auto unit = field(item, here, "unit");
    auto number = field(item, here, "value");
    auto settledValue = allSettled(label, unit, number);
    if (!settledValue.has_value()) {
      return std::unexpected(settledValue.error());
    }
    auto text = asString(*label, here + "/name");
    auto readUnit =
        asEnum<sg::StatUnit>(detail::kStatUnitWords, *unit, here + "/unit");
    auto answer = asDouble(*number, here + "/value");
    auto settledFields = allSettled(text, readUnit, answer);
    if (!settledFields.has_value()) {
      return std::unexpected(settledFields.error());
    }
    result.statisticValues.push_back(sg::StatisticValue {
        .name = *text,
        .unit = *readUnit,
        .value = *answer,
    });
    ++valueIndex;
  }
  // specs/024 FR-038: the pass disclosure the writer puts after the
  // statistics. The warning enters no field: FR-018 answers it from the
  // spread at every reader, and the document keeps the answer explicit for
  // a reader of the file alone, so the read checks the two agree. A key the
  // writer of before specs/024 never wrote leaves its member at the default
  // it holds, which is the disclosure of a run that collected no pass.
  const auto declared = optionalField(value, "metricMethod");
  if (declared.has_value()) {
    auto method = asEnum<sg::MetricMethod>(
        detail::kMetricMethodWords, *declared, where + "/metricMethod");
    if (!method.has_value()) {
      return std::unexpected(method.error());
    }
    result.metricMethod = *method;
  }
  const auto counted = optionalField(value, "passCount");
  if (counted.has_value()) {
    auto passes = asUnsigned(*counted, where + "/passCount");
    if (!passes.has_value()) {
      return std::unexpected(passes.error());
    }
    result.passCount = static_cast<std::size_t>(*passes);
  }
  std::optional<double> spread;
  const auto spreadField = optionalField(value, "passSpread");
  if (spreadField.has_value() && !spreadField->is_null()) {
    auto number = asDouble(*spreadField, where + "/passSpread");
    if (!number.has_value()) {
      return std::unexpected(number.error());
    }
    spread = *number;
  }
  const auto warnedField = optionalField(value, "spreadWarning");
  if (warnedField.has_value()) {
    auto warned = asBool(*warnedField, where + "/spreadWarning");
    if (!warned.has_value()) {
      return std::unexpected(warned.error());
    }
    if (*warned != detail::spreadWarned(spread)) {
      return std::unexpected(where
                             + "/spreadWarning: the document's warning "
                               "disagrees with its pass spread");
    }
  }
  result.passSpread = spread;
  // specs/029 FR-051: the hunt object the writer appends beside these keys.
  // A document written before the hunt keys, and a run of this build that
  // carried no `--hunt`, write no key at all: the member then stays empty,
  // which is the hunt-less reading, and the rest of the entry reads as it
  // did. A key that stands present keeps its own kind and its own refusal.
  const auto hunted = optionalField(value, "hunt");
  if (hunted.has_value()) {
    auto record = readEntryHunt(*hunted, where + "/hunt");
    if (!record.has_value()) {
      return std::unexpected(record.error());
    }
    result.hunt = *record;
  }
  return {};
}

// One node of the schema file: the kinds, the key set, the key order and
// the one value this position of the document must hold.
struct SchemaNode
{
  std::vector<std::string> types;
  std::vector<std::string> required;
  std::vector<std::string> order;
  std::vector<std::pair<std::string, std::shared_ptr<SchemaNode>>> properties;
  std::shared_ptr<SchemaNode> items;
  std::vector<std::string> enums;
  bool closed = false;
};

auto parseSchema(Element value, const std::string& where)
    -> std::expected<SchemaNode, std::string>
{
  if (!value.is_object()) {
    return std::unexpected(wrongKind(where, value, "an object"));
  }
  SchemaNode node;
  if (const auto type = field(value, where, "type"); type.has_value()) {
    if (type->is_string()) {
      node.types.push_back(std::string(std::string_view(*type)));
    } else {
      auto kinds = asArray(*type, where + "/type");
      if (!kinds.has_value()) {
        return std::unexpected(kinds.error());
      }
      for (const auto kind : *kinds) {
        auto text = asString(kind, where + "/type");
        if (!text.has_value()) {
          return std::unexpected(text.error());
        }
        node.types.push_back(*text);
      }
    }
  }
  for (const auto* key : {"required", "x-order"}) {
    if (const auto list = field(value, where, key); list.has_value()) {
      auto names = asArray(*list, where + "/" + std::string(key));
      if (!names.has_value()) {
        return std::unexpected(names.error());
      }
      auto& target =
          key == std::string_view("x-order") ? node.order : node.required;
      for (const auto name : *names) {
        auto text = asString(name, where + "/" + std::string(key));
        if (!text.has_value()) {
          return std::unexpected(text.error());
        }
        target.push_back(*text);
      }
    }
  }
  if (const auto closed = field(value, where, "additionalProperties");
      closed.has_value())
  {
    bool flag = false;
    if (const auto code = closed->get_bool().get(flag);
        code == simdjson::SUCCESS)
    {
      node.closed = !flag;
    }
  }
  if (const auto list = field(value, where, "enum"); list.has_value()) {
    auto words = asArray(*list, where + "/enum");
    if (!words.has_value()) {
      return std::unexpected(words.error());
    }
    for (const auto word : *words) {
      auto text = asString(word, where + "/enum");
      if (!text.has_value()) {
        return std::unexpected(text.error());
      }
      node.enums.push_back(*text);
    }
  }
  if (const auto properties = field(value, where, "properties");
      properties.has_value())
  {
    auto object = properties->get_object();
    if (!object.has_value()) {
      return std::unexpected(
          wrongKind(where + "/properties", *properties, "an object"));
    }
    for (const auto pair : *object) {
      auto child = parseSchema(pair.value,
                               where + "/properties/" + std::string(pair.key));
      if (!child.has_value()) {
        return std::unexpected(child.error());
      }
      node.properties.emplace_back(std::string(pair.key),
                                   std::make_shared<SchemaNode>(*child));
    }
  }
  if (const auto items = field(value, where, "items"); items.has_value()) {
    auto child = parseSchema(*items, where + "/items");
    if (!child.has_value()) {
      return std::unexpected(child.error());
    }
    node.items = std::make_shared<SchemaNode>(*child);
  }
  return node;
}

auto propertyOf(const SchemaNode& node,
                std::string_view key) noexcept -> const SchemaNode*
{
  for (const auto& property : node.properties) {
    if (property.first == key) {
      return property.second.get();
    }
  }
  return nullptr;
}

auto kindMatches(const SchemaNode& node, Element value) noexcept -> bool
{
  for (const auto& type : node.types) {
    const bool holds = type == "object" ? value.is_object()
        : type == "array"               ? value.is_array()
        : type == "string"              ? value.is_string()
        : type == "number"              ? value.is_number()
        : type == "integer"             ? value.is_int64()
        : type == "boolean"             ? value.is_bool()
                                        : value.is_null();
    if (holds) {
      return true;
    }
  }
  return false;
}

auto here(const std::string& pointer) -> std::string
{
  return pointer.empty() ? "the document root" : pointer;
}

// The value one schema node asks for at one position of the document,
// reported by the JSON pointer of the first disagreement (FR-008).
auto checkNode(const SchemaNode& node,
               Element value,
               const std::string& pointer) -> std::expected<void, std::string>
{
  if (!node.types.empty() && !kindMatches(node, value)) {
    std::string wants;
    for (const auto& type : node.types) {
      wants += wants.empty() ? type : " or " + type;
    }
    return std::unexpected(here(pointer) + ": the schema asks for " + wants
                           + ", the document holds " + kindName(value));
  }
  if (!node.enums.empty()) {
    const bool holds = value.is_string()
        && std::find(node.enums.begin(),
                     node.enums.end(),
                     std::string(std::string_view(value)))
            != node.enums.end();
    if (!holds) {
      auto asked = std::string(": the document holds a value");
      if (value.is_string()) {
        asked += " '" + detail::quotedText(std::string_view(value)) + "'";
      } else {
        asked += std::string(" of kind ") + kindName(value);
      }
      return std::unexpected(here(pointer) + asked
                             + " outside the list the schema states");
    }
  }
  simdjson::dom::object object;
  if (const auto code = value.get_object().get(object);
      code == simdjson::SUCCESS)
  {
    for (const auto& key : node.required) {
      if (!object[key].has_value()) {
        return std::unexpected(missing(here(pointer), key));
      }
    }
    std::size_t previous = 0;
    for (const auto pair : object) {
      const auto* child = propertyOf(node, pair.key);
      if (child == nullptr) {
        // A schema that leaves the object open takes a key a newer minor
        // added; a closed object names the key the schema does not list
        // (FR-008, FR-010).
        if (node.closed) {
          return std::unexpected(here(pointer) + "/"
                                 + detail::quotedText(std::string(pair.key))
                                 + ": the schema lists no key of that name");
        }
        continue;
      }
      const auto position = std::find(
          node.order.begin(), node.order.end(), std::string(pair.key));
      if (position != node.order.end()) {
        const auto index =
            static_cast<std::size_t>(position - node.order.begin());
        if (index < previous) {
          return std::unexpected(
              here(pointer) + "/"
              + detail::quotedText(std::string(pair.key))
              + ": the key stands out of the order the"
                " schema states");
        }
        previous = index + 1;
      }
      auto read =
          checkNode(*child, pair.value, pointer + "/" + std::string(pair.key));
      if (!read.has_value()) {
        return std::unexpected(read.error());
      }
    }
    return {};
  }
  if (node.items != nullptr) {
    simdjson::dom::array items;
    if (const auto code = value.get_array().get(items);
        code == simdjson::SUCCESS)
    {
      std::size_t index = 0;
      for (const auto item : items) {
        auto read =
            checkNode(*node.items, item, pointer + "/" + std::to_string(index));
        if (!read.has_value()) {
          return std::unexpected(read.error());
        }
        ++index;
      }
      return {};
    }
  }
  return {};
}

// FR-017: the first key that stands twice in one object, nothing when
// none does: the parser's lookup keeps the last of a duplicated key, so
// the count reads the pairs the tape carries, duplicates and all, and a
// hand-edited document or schema that carries one key twice stands
// refused before the walk.
auto firstDuplicateKey(const simdjson::dom::element& root)
    -> std::optional<std::string>
{
  simdjson::dom::object object;
  if (root.get(object) != simdjson::SUCCESS) {
    return std::nullopt;
  }
  std::set<std::string_view> seen;
  for (auto [key, value] : object) {
    if (!seen.insert(key).second) {
      return std::string(key);
    }
  }
  return std::nullopt;
}

}  // namespace

namespace detail
{

auto checkReport(std::string_view schemaPath,
                 std::string_view document) -> std::expected<void, std::string>
{
  auto schemaText = detail::readTextFile(std::string(schemaPath));
  if (!schemaText.has_value()) {
    return std::unexpected(schemaText.error());
  }
  simdjson::dom::parser schemaParser;
  auto schemaDocument = schemaParser.parse(
      simdjson::padded_string(std::string_view(*schemaText)));
  simdjson::dom::element schemaRoot;
  if (auto code = schemaDocument.get(schemaRoot); code != simdjson::SUCCESS) {
    return std::unexpected(
        "the schema file '" + detail::quotedText(std::string(schemaPath))
        + "' holds no JSON the parser reads: " + simdjson::error_message(code));
  }
  if (const auto duplicate = firstDuplicateKey(schemaRoot);
      duplicate.has_value())
  {
    return std::unexpected("/" + *duplicate
                           + ": the key stands twice in the schema object");
  }
  auto root = parseSchema(schemaRoot, "");
  if (!root.has_value()) {
    return std::unexpected(root.error());
  }
  simdjson::dom::parser documentParser;
  auto parsed =
      documentParser.parse(simdjson::padded_string(std::string_view(document)));
  simdjson::dom::element documentRoot;
  if (auto code = parsed.get(documentRoot); code != simdjson::SUCCESS) {
    return std::unexpected(
        std::string("the written document holds no JSON the parser reads: ")
        + simdjson::error_message(code));
  }
  if (const auto duplicate = firstDuplicateKey(documentRoot);
      duplicate.has_value())
  {
    return std::unexpected("/" + *duplicate
                           + ": the key stands twice in the object");
  }
  return checkNode(*root, documentRoot, "");
}

}  // namespace detail

auto loadReport(const std::string& path)
    -> std::expected<ReportData, std::string>
{
  SG_REQUIRE(!path.empty(), "loadReport reaches an empty path");
  auto text = detail::readTextFile(path);
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  simdjson::dom::parser parser;
  auto parsed = parser.parse(simdjson::padded_string(std::string_view(*text)));
  simdjson::dom::element root;
  if (auto code = parsed.get(root); code != simdjson::SUCCESS) {
    return std::unexpected("the file '" + detail::quotedText(path)
                           + "' holds no JSON the parser reads: "
                           + simdjson::error_message(code));
  }
  auto version = field(root, "", "schemaVersion");
  if (!version.has_value()) {
    return std::unexpected(version.error());
  }
  auto versionText = asString(*version, "/schemaVersion");
  if (!versionText.has_value()) {
    return std::unexpected(versionText.error());
  }
  // The reader knows one major: a later minor of it stays readable, since
  // the writer moves the minor for a key it adds inside major 1, while a
  // text that states no major is no version this build can read
  // (specs/023 FR-010, FR-023, specs/026 D-13).
  if (detail::schemaMajor(*versionText) != std::string_view("1")) {
    return std::unexpected("the file '" + detail::quotedText(path)
                           + "' carries schema version '"
                           + detail::quotedText(*versionText)
                           + "', which this build cannot read: the reader "
                             "knows version 1");
  }
  ReportData data;
  if (const auto jitter = field(root, "", "jitter"); jitter.has_value()) {
    auto read = readJitterRecord(*jitter, "/jitter");
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    data.runJitterStats = std::move(*read);
  }
  // specs/029 FR-051, FR-031 (PD-10): the run-level hunt object, the key the
  // writer closes the document with. It is read before the benchmarks array
  // so the run record stands settled before its entries are. A document
  // written before specs/029, and a run of this build that carried no
  // `--hunt`, write no key: the member stays empty, which is the reading of a
  // run that carried no hunt (I-8), and the rest of the document reads as it
  // did. A key that stands present keeps its own kind and its own refusal.
  if (const auto hunted = optionalField(root, "hunt"); hunted.has_value()) {
    auto record = readHunt(*hunted, "/hunt");
    if (!record.has_value()) {
      return std::unexpected(record.error());
    }
    data.hunt = *record;
  }
  // specs/034 FR-024, D-8: the root pre-check object, the key the writer
  // stands after `jitter` and before `benchmarks`. A run without the
  // pre-check, and every document the builds before specs/034 wrote,
  // carries no key: the member stays empty, which is the reading of a run
  // that measured no pre-check, and the rest of the document reads as it
  // did (I-8). A key that stands present keeps its own kind and its own
  // refusal, so a malformed record names itself before the benchmarks
  // array reads.
  if (const auto precheck = optionalField(root, "precheck");
      precheck.has_value())
  {
    auto record = readPrecheckRecord(*precheck, "/precheck");
    if (!record.has_value()) {
      return std::unexpected(record.error());
    }
    data.precheck = std::move(*record);
  }
  // specs/037 FR-023, D-5: the root graph object, the key the writer stands
  // after `precheck` and before `benchmarks`. A run with no `--graph`, and
  // every document the builds before specs/037 wrote, carries no key: the
  // member stays empty, which is the reading of a run that gated no graph
  // (FR-006, FR-023). A key that stands present keeps its own kind and its
  // own refusal, so a malformed record names itself before the benchmarks
  // array reads.
  if (const auto graph = optionalField(root, "graph"); graph.has_value()) {
    auto record = readGraphRecord(*graph, "/graph");
    if (!record.has_value()) {
      return std::unexpected(record.error());
    }
    data.graph = std::move(*record);
  }
  auto entries = field(root, "", "benchmarks");
  if (!entries.has_value()) {
    return std::unexpected(entries.error());
  }
  auto items = asArray(*entries, "/benchmarks");
  if (!items.has_value()) {
    return std::unexpected(items.error());
  }
  data.schemaVersion = *versionText;
  if (const auto header = field(root, "", "speedgun"); header.has_value()) {
    // A header text the reader only ever prints stays lenient: the object
    // or the key may stand absent and answer an empty text with no fault,
    // while a value of the wrong kind names the pointer it stands at
    // (FR-070, E-07).
    simdjson::dom::object headerObject;
    if (const auto code = header->get_object().get(headerObject);
        code != simdjson::SUCCESS)
    {
      return std::unexpected(wrongKind("/speedgun", *header, "an object"));
    }
    if (const auto versionKey = field(*header, "/speedgun", "version");
        versionKey.has_value())
    {
      auto headerText = asString(*versionKey, "/speedgun/version");
      if (!headerText.has_value()) {
        return std::unexpected(headerText.error());
      }
      data.speedgunVersion = *headerText;
    }
  }
  std::size_t index = 0;
  for (const auto item : *items) {
    data.results.emplace_back();
    data.rowRaw.emplace_back();
    data.contextRaw.emplace_back();
    data.noiseWarnings.emplace_back();
    auto read = readEntry(item,
                          "/benchmarks/" + std::to_string(index),
                          data.results.back(),
                          data.rowRaw.back(),
                          data.contextRaw.back(),
                          data.noiseWarnings.back());
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    ++index;
  }
  SG_ENSURE(data.results.size() == data.rowRaw.size()
                && data.results.size() == data.contextRaw.size()
                && data.results.size() == data.noiseWarnings.size(),
            "loadReport answers a value whose raw fields and warning "
            "entries answer results");
  return data;
}

}  // namespace sg
