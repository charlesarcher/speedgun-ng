#ifndef SG_TEST_DETAIL_GCOV_FLUSH_HPP
#define SG_TEST_DETAIL_GCOV_FLUSH_HPP

#include <csignal>
#include <cstdlib>

// libgcov writes its counters from the exit hooks a process reaches by
// returning from main. A breached child dies at the contract call instead, so
// the lines it ran never reach the coverage report. This handler writes the
// counters first and then dies by the same signal, so the parent still sees the
// death it asserts on. Writing from a signal handler is not async-signal-safe;
// it is accepted because the process is about to die and these children are
// single threaded.
// __gcov_dump is weak: a build without coverage instrumentation has no such
// symbol, its address compares null, and the handler stays out of the way.
#pragma weak __gcov_dump
extern "C" void
__gcov_dump();  // NOLINT(readability-identifier-naming) library symbol

namespace sg::test
{

inline auto dumpCountersThenDie(int const signal) noexcept -> void
{
  (void)std::signal(signal, SIG_DFL);
  if (&__gcov_dump != nullptr) {
    __gcov_dump();
  }
  (void)std::raise(signal);
  std::_Exit(128 + signal);
}

inline auto flushCountersOnAbort() -> void
{
  if (&__gcov_dump == nullptr) {
    return;
  }
  (void)std::signal(SIGABRT, &dumpCountersThenDie);
}

}  // namespace sg::test

#endif  // SG_TEST_DETAIL_GCOV_FLUSH_HPP
