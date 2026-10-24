#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <simdjson.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/dbc.hpp"

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

// The same report over the values a record read: the first answer that
// carries an error names the field, and the rest are not looked at.
template<class... Parts>
auto allSettled(Parts&&... parts) -> std::expected<void, std::string>
{
  auto answer = std::expected<void, std::string> {};
  auto report = [&](auto& part) noexcept -> bool
  {
    if (part.has_value()) {
      return true;
    }
    answer = std::unexpected(part.error());
    return false;
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
    return std::unexpected(where + ": '" + *text
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
    result.contexts.push_back(sg::ContextDisclosure {
        .waitMode = *mode,
        .placement = *text,
        .applied = *flag,
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
        asked += " '" + std::string(std::string_view(value)) + "'";
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
        return std::unexpected(here(pointer) + ": " + missing("", key));
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
          return std::unexpected(here(pointer) + "/" + std::string(pair.key)
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
          return std::unexpected(here(pointer) + "/" + std::string(pair.key)
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
    return std::unexpected("the schema file '" + std::string(schemaPath)
                           + "' holds no JSON the parser reads: "
                           + simdjson::error_message(code));
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
    return std::unexpected("the file '" + path
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
    return std::unexpected("the file '" + path + "' carries schema version '"
                           + *versionText
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
