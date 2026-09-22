// Downstream consumer test project (FR-026, SC-005).
// Driven by the downstream-consumer CI job: install speedgun-ng to a
// prefix, configure, build, and run this program against it. This
// file must never name the vendored topology library; that absence
// is the point.
//
// It uses the public API exactly as the README instructs: the
// exported class from <speedgun-ng/speedgun-ng.hpp>. Constructing it
// and calling name() pulls a real symbol out of the library, so the
// link is exercised on every machine.

#include <speedgun-ng/speedgun-ng.hpp>

auto main() -> int
{
  exported_class const library;
  return library.name() == nullptr ? 1 : 0;
}
