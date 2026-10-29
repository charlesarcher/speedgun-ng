// ============================================================================
// The noise tracer value parser (specs/036-osnoise-timerlat-tracers T007,
// US3).
//
// One translation unit owns the tracer beside the engine session the later
// phases land in the same files: the value grammar of
// contracts/cli-options.md, the standing cpu-list reader of the jitter
// probe, and the refusal shape the option module quotes.
// ============================================================================

#include <set>
#include <string>
#include <vector>

#include "detail/noise_tracer.hpp"

#include "detail/jitter_probe.hpp"
#include "detail/options.hpp"

namespace sg::detail
{

// The value grammar of contracts/cli-options.md (FR-002, FR-003, D-4,
// R-06): the head names the engine word osnoise or timerlat, then at most
// one optional cpus key follows, whose items the standing cpu-list reader
// reads whole, and the key stands once in the value. Every rejection
// names the token it read, quoted the way the option module quotes a
// value (FR-003).
auto parseOsNoiseValue(const std::string& value,
                       TracerEngine& engine,
                       std::vector<std::uint32_t>& cpus,
                       std::string& message) -> bool
{
  engine = TracerEngine::NONE;
  cpus.clear();
  const auto headEnd = value.find(',');
  const std::string head =
      headEnd == std::string::npos ? value : value.substr(0, headEnd);
  if (head == "osnoise") {
    engine = TracerEngine::OSNOISE;
  } else if (head == "timerlat") {
    engine = TracerEngine::TIMERLAT;
  } else {
    // An empty word refuses the same way: the value names an engine, so
    // the refusal reads the empty token (FR-002, R-06).
    message =
        "the engine '" + quotedText(head) + "' is not osnoise or timerlat";
    return false;
  }
  if (headEnd == std::string::npos) {
    return true;
  }

  // The key stands once in a value (FR-002): every item after the head
  // extends the one cpus list, a token naming the key a second time
  // stands the standing repeated-key refusal, and a bare item before
  // the key names no list the value asked for.
  std::set<std::string> seenKeys;
  std::size_t position = headEnd + 1;
  while (position <= value.size()) {
    const auto comma = value.find(',', position);
    const std::string token = value.substr(
        position,
        comma == std::string::npos ? std::string::npos : comma - position);
    const bool last = comma == std::string::npos;
    position = last ? value.size() + 1 : comma + 1;

    const auto equals = token.find('=');
    if (equals != std::string::npos) {
      const std::string key = token.substr(0, equals);
      if (key != "cpus") {
        message =
            "the key '" + quotedText(key) + "' is not one the value names";
        return false;
      }
      if (!seenKeys.insert(key).second) {
        message = "the key '" + quotedText(key) + "' stands twice in the value";
        return false;
      }
      // The key's item stands in the same token, so an empty item
      // reaches the cpu-list reader and its refusal reads the empty
      // token (FR-003).
      const std::string item = token.substr(equals + 1);
      if (!parseProbeCpuToken(item, cpus, message)) {
        return false;
      }
      continue;
    }
    if (seenKeys.empty()) {
      message =
          "the token '" + quotedText(token) + "' stands outside the cpus key";
      return false;
    }
    if (!parseProbeCpuToken(token, cpus, message)) {
      return false;
    }
  }

  // A cpu that stands twice in one list is an invalid value (FR-002,
  // FR-003); the whole value stands named beside the index that repeated,
  // the standing shape (FR-038).
  std::set<std::uint32_t> seenCpus;
  for (const auto cpu : cpus) {
    if (!seenCpus.insert(cpu).second) {
      message = "the cpu '" + std::to_string(cpu)
          + "' stands more than once in '" + quotedText(value) + "'";
      return false;
    }
  }
  return true;
}

}  // namespace sg::detail
