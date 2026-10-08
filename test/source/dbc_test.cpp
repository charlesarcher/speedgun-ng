// ============================================================================
// TDD RED test suite for the DBC facility (feature 001).
//
// Written BEFORE include/speedgun-ng/dbc.hpp exists (task T003). The expected
// failure right now is a compile error:
//   "speedgun-ng/dbc.hpp: No such file or directory"
// Once the header lands (tasks T006-T010), this suite must pass under the
// default (enforce) developer semantic.
//
// The suite pins the public API contract the implementation must honor and
// encodes the acceptance criteria from spec.md:
//   (a) each primitive reports a violation with the correct kind (FR-002..007)
//   (b) the ViolationRecord carries kind/file/line/message/predicate (FR-021)
//   (c) the predicate is evaluated exactly once per check (FR-019)
//   (d) the build semantic selects the violation behavior (FR-013..015)
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>

#include "speedgun-ng/dbc.hpp"

#if defined(__unix__)
#  include <csignal>
#  include <cstring>

#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace
{

auto fail(char const* what) -> void
{
  std::fprintf(stderr, "DBC TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(bool cond, char const* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Sentinel exception the recording observer throws after storing the record.
// In the default (enforce) semantic the dispatch is NOT noexcept, so the throw
// propagates to the test caller (FR-009/FR-014); in a real program the uncaught
// throw would reach std::terminate. This is the test-only mechanism FR-009
// sanctions ("record the violation and throw an observable exception").
struct ViolationCaught
{
  sg::dbc::ViolationRecord record {};
};

// Instrumentation for the exactly-once predicate test (FR-019). A pure
// predicate cannot carry a side effect, so the test's *measurement probe* is
// side-effecting on purpose. The production purity rule (FR-019) binds
// contract predicates; a test probe stands outside it. The probe stays a
// named function because the count is the measurement and a contract may
// evaluate its predicate twice (T174).
int predicateEvaluations = 0;

// Named from a semantic-gated check alone, so an ignoring build elides the
// only reference and the warning set would see none (T174).
[[maybe_unused]] auto countingPredicate() -> bool
{
  ++predicateEvaluations;
  return true;
}

// A recording observer that stores the record into `out` and then throws it
// back to the caller.
auto recordInto(sg::dbc::ViolationRecord& out) -> sg::dbc::ViolationObserver
{
  return [&out](sg::dbc::ViolationRecord const& rec)
  {
    out = rec;
    throw ViolationCaught {rec};
  };
}

// ---- (a)/(b) per-primitive violation sites -------------------------------

auto violatePrecondition() -> void
{
  SG_REQUIRE(false, "pre: x > 0");
}

auto violatePostcondition() -> int
{
  int result = 41;
  SG_ENSURE(result == 42, "post: result == 42");
  return result;
}

auto violateLoopInvariant() -> void
{
  int i = -1;
  for (; i < 3; ++i) {
    SG_INVARIANT(i >= 0, "inv: i >= 0");
    break;
  }
}

// A class whose constructor leaves the invariant violated (checked at ctor
// exit, FR-006).
struct BadInvariantObject
{
  int value = -1;

  BadInvariantObject() { SG_INVARIANT(value >= 0, "inv: value >= 0"); }
};

auto violateAssertion() -> void
{
  SG_ASSERT(1 == 2, "assert: 1 == 2");
}

// Exactly-once probe site (a semantic-gated, satisfied check). The predicate
// counts its own evaluations and stays true across repeated calls, so a
// second evaluation shows in the caller's count. A named probe function is
// named from the gated check alone, which an ignoring build elides (T174).
auto gatedSatisfiedSite() -> void
{
  SG_REQUIRE(countingPredicate(), "positive");
}

// Runs `body` under a fresh recording observer; returns the captured record
// and whether the observer threw (i.e. the violation was delivered).
auto captureViolation(sg::dbc::ViolationRecord& rec, auto&& body) -> bool
{
  sg::dbc::setObserver(recordInto(rec));
  bool caught = false;
  try {
    body();
  } catch (ViolationCaught const&) {
    caught = true;
  }
  return caught;
}

#if SG_CONTRACTS_SEMANTIC == 2
template<typename T>
auto requirePositive(T arg) -> T
{
  SG_REQUIRE(arg > T {}, "template: arg > T{}");
  return arg;
}

struct ContractBase
{
  ContractBase() = default;
  ContractBase(ContractBase const&) = default;
  ContractBase(ContractBase&&) = default;
  auto operator=(ContractBase const&) -> ContractBase& = default;
  auto operator=(ContractBase&&) -> ContractBase& = default;
  virtual ~ContractBase() = default;

  virtual auto gated(int arg) -> void { SG_REQUIRE(arg > 0, "base: arg > 0"); }
};

struct ContractDerived : ContractBase
{
  // enforcement is per-body
  auto gated(int arg) -> void override { static_cast<void>(arg); }
};

struct PlacementHost
{
  PlacementHost()
  {
    // ctor exit (may exist)
    SG_INVARIANT(m_value > 0, "ctor-exit: value > 0");
  }

  PlacementHost(PlacementHost const&) = default;
  PlacementHost(PlacementHost&&) = default;
  auto operator=(PlacementHost const&) -> PlacementHost& = default;
  auto operator=(PlacementHost&&) -> PlacementHost& = default;

  ~PlacementHost()
  {
    // dtor entry
    SG_INVARIANT(m_value > 0, "dtor-entry: value > 0");
  }

  auto mutate() -> void
  {
    SG_INVARIANT(m_value > 0, "non-const-entry: value > 0");
    ++m_value;
    // non-const exit
    SG_INVARIANT(m_value > 0, "non-const-exit: value > 0");
  }

  auto breakAtExit() -> void
  {
    SG_INVARIANT(m_value > 0, "non-const-entry: value > 0");
    m_value = 0;
    // non-const exit
    SG_INVARIANT(m_value > 0, "non-const-exit: value > 0");
  }

  auto peek() const -> int
  {
    // const entry-only
    SG_INVARIANT(m_value > 0, "const-entry-only: value > 0");
    return m_value;
  }

  auto poison() -> void { m_value = 0; }

private:
  int m_value {1};
};
#endif  // SG_CONTRACTS_SEMANTIC == 2

}  // namespace

auto main() -> int
{
  using sg::dbc::Kind;

#if SG_CONTRACTS_SEMANTIC != 3
  sg::dbc::ViolationRecord rec {};

  // (a)/(b) precondition (FR-002/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(captureViolation(rec, [] { violatePrecondition(); }),
        "pre: violation delivered to the observer");
  check(rec.kind == Kind::PRECONDITION, "pre: kind == precondition");
  check(std::string(rec.message) == "pre: x > 0", "pre: message text");
  check(std::string(rec.predicateText) == "false", "pre: predicate text");
  check(rec.file != nullptr && rec.file[0] != '\0', "pre: file present");
  check(rec.line > 0, "pre: line present");

  // (a)/(b) postcondition over named result capture (FR-003/FR-004/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(captureViolation(rec, [] { (void)violatePostcondition(); }),
        "post: violation delivered to the observer");
  check(rec.kind == Kind::POSTCONDITION, "post: kind == postcondition");
  check(std::string(rec.message) == "post: result == 42", "post: message text");
  check(std::string(rec.predicateText) == "result == 42",
        "post: predicate text");

  // (a)/(b) loop invariant at iteration entry (FR-005/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(captureViolation(rec, [] { violateLoopInvariant(); }),
        "loop-inv: violation delivered to the observer");
  check(rec.kind == Kind::INVARIANT, "loop-inv: kind == invariant");
  check(std::string(rec.predicateText) == "i >= 0", "loop-inv: predicate text");

  // (a)/(b) class invariant at constructor exit (FR-006/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(captureViolation(rec, [] { (void)BadInvariantObject {}; }),
        "class-inv: violation delivered at ctor exit");
  check(rec.kind == Kind::INVARIANT, "class-inv: kind == invariant");
  check(std::string(rec.predicateText) == "value >= 0",
        "class-inv: predicate text");

  // (a)/(b) in-body assertion (FR-007/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(captureViolation(rec, [] { violateAssertion(); }),
        "assert: violation delivered to the observer");
  check(rec.kind == Kind::ASSERTION, "assert: kind == assertion");
  check(std::string(rec.predicateText) == "1 == 2", "assert: predicate text");
#endif  // SG_CONTRACTS_SEMANTIC != 3

  // (c) exactly-once predicate evaluation (FR-019). The probe site is
  // semantic-gated and satisfied, so the predicate is evaluated exactly once
  // in a checked build and not at all in ignore.
  predicateEvaluations = 0;
  gatedSatisfiedSite();
#if SG_CONTRACTS_SEMANTIC == 0
  check(predicateEvaluations == 0, "ignore: gated predicate not evaluated");
#else
  check(predicateEvaluations == 1, "checked: predicate evaluated exactly once");
#endif

  // (d) semantic behavior (FR-013..015). The suite asserts the branch matching
  // the *compiled* semantic. The dev build is `enforce`; `observe` is asserted
  // here (execution continues in-process). `quick_enforce` (no hook) and
  // `ignore` (elision) are proven by the trap fixture (T004/T005), the
  // consumer-release job (T016), and the semantics matrix (T019).
#if SG_CONTRACTS_SEMANTIC == 1
  {
    sg::dbc::ViolationRecord orec {};
    sg::dbc::setObserver([&orec](sg::dbc::ViolationRecord const& r)
                         { orec = r; });
    auto probe = []
    {
      int result = 0;
      SG_ENSURE(result > 0, "observe: result > 0");  // false; observe continues
      return 7;
    };
    int returned = probe();
    check(returned == 7,
          "observe: execution continued past the failed predicate");
    check(orec.kind == Kind::POSTCONDITION, "observe: record captured");
  }
#endif

  // ==========================================================================
  // US3 observer / response red tests (T020). Fork-based so parent survives
  // aborts. All fork groups guarded; on non-Unix print skip and continue.
  // (a)-(d) exercise current machinery (may pass); (e) is the deliberate TDD
  // red (re-entry guard absent until T021); (f) is QE-specific.
  // ==========================================================================

  sg::dbc::setObserver({});

#if defined(__unix__)
  struct ChildResult
  {
    int exitStatus = -1;
    int termSig = 0;
    std::string output;
    bool timedOut = false;
  };

  auto runInChild = [](auto&& childBody, int timeoutSec = 5) -> ChildResult
  {
    ChildResult r {};
    auto oldAlrm = signal(SIGALRM, [](int) {});
    int pipefd[2];
    if (pipe(pipefd) != 0) {
      fail("pipe failed");
    }
    pid_t pid = fork();
    if (pid < 0) {
      fail("fork failed");
    }
    if (pid == 0) {
      close(pipefd[0]);
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      close(pipefd[1]);
      if (timeoutSec > 0) {
        alarm(static_cast<unsigned>(timeoutSec));
      }
      childBody();
      _exit(0);
    }
    close(pipefd[1]);
    if (timeoutSec > 0) {
      alarm(static_cast<unsigned>(timeoutSec));
    }
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    alarm(0);
    signal(SIGALRM, oldAlrm);
    if (w < 0
        || (timeoutSec > 0 && WIFSIGNALED(status)
            && WTERMSIG(status) == SIGALRM))
    {
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      r.timedOut = true;
    }
    if (WIFSIGNALED(status)) {
      r.termSig = WTERMSIG(status);
    } else if (WIFEXITED(status)) {
      r.exitStatus = WEXITSTATUS(status);
    }
    char buf[8192];
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      r.output = buf;
    }
    close(pipefd[0]);
    return r;
  };
#endif  // __unix__

  // (a) default response under enforce: unix fork, aborts with SIGABRT,
  // structured stderr, not catchable by try/catch in child.
#if defined(__unix__)
  if (SG_CONTRACTS_SEMANTIC == 2) {
    auto res = runInChild(
        []
        {
          try {
            SG_REQUIRE(false, "default: must abort not catch");
          } catch (...) {
            std::fprintf(stderr, "CAUGHT-IN-CHILD\n");
            _exit(42);
          }
          _exit(99);
        },
        5);
    bool isSigabrt = (res.termSig == SIGABRT);
    bool hasStructured = res.output.find("[precondition]") != std::string::npos
        && res.output.find("default: must abort not catch") != std::string::npos
        && res.output.find("(predicate:") != std::string::npos
        && res.output.find(" at ") != std::string::npos;
    bool noCatchMarker =
        res.output.find("CAUGHT-IN-CHILD") == std::string::npos;
    check(isSigabrt, "(a) default response must SIGABRT");
    check(hasStructured, "(a) default diagnostic on stderr");
    check(noCatchMarker, "(a) not catchable");
  }
#else
  std::printf("(a) default-response fork test: SKIPPED (non-unix)\n");
#endif

#if SG_CONTRACTS_SEMANTIC == 2
  // (b) second setObserver replaces first
  {
    sg::dbc::ViolationRecord r1 {};
    sg::dbc::ViolationRecord r2 {};
    sg::dbc::setObserver(recordInto(r1));
    sg::dbc::setObserver(
        [&r2](sg::dbc::ViolationRecord const& recb)
        {
          r2 = recb;
          throw ViolationCaught {recb};
        });
    bool caught = false;
    try {
      SG_REQUIRE(false, "second-observer");
    } catch (ViolationCaught const&) {
      caught = true;
    }
    check(caught, "(b) second observer threw");
    check(std::string(r1.message ? r1.message : "") == "",
          "(b) first observer not called (replaced)");
    check(std::string(r2.message ? r2.message : "") == "second-observer",
          "(b) second replaced first");
  }
#endif

  // (c) observer-throw under enforce: unique exception, child dies by uncaught
  // exception (dispatch NOT noexcept), The response abort path is a separate
  // arm.
#if defined(__unix__)
  if (SG_CONTRACTS_SEMANTIC == 2) {
    struct UniqueExc : std::exception
    {
      const char* what() const noexcept override
      {
        return "unique-observer-exc";
      }
    };

    auto res = runInChild(
        []
        {
          sg::dbc::setObserver([](sg::dbc::ViolationRecord const&)
                               { throw UniqueExc {}; });
          SG_REQUIRE(false, "observer-throw");
          _exit(77);
        },
        5);
    // observer path: no default diagnostic emitted (report returns before
    // abort)
    bool noDefaultDiag =
        res.output.find("contract violation") == std::string::npos;
    bool died = (res.termSig != 0 || res.exitStatus != 0);
    // On uncaught, std::terminate typically aborts (SIGABRT or SIGSEGV), but
    // absence of the default diag proves we took the throw path not abort path.
    check(died, "(c) child died on observer throw");
    check(noDefaultDiag, "(c) died via exception (no default response diag)");
  }
#else
  std::printf("(c) observer-throw fork test: SKIPPED (non-unix)\n");
#endif

#if SG_CONTRACTS_SEMANTIC == 2
  // (d) violation inside noexcept function terminates via response; no
  // exception escapes the noexcept site.
  {
    auto noexceptViolator = []() noexcept
    { SG_REQUIRE(false, "noexcept-violation"); };
#  if defined(__unix__)
    if (SG_CONTRACTS_SEMANTIC == 2) {
      auto res = runInChild(
          [&]
          {
            noexceptViolator();
            _exit(0);
          },
          5);
      bool died = (res.termSig == SIGABRT || res.exitStatus != 0);
      check(died, "(d) noexcept site still terminates via response");
    }
#  else
    bool reached_after = false;
    try {
      noexcept_violator();
      reached_after = true;
    } catch (...) {
      check(false, "(d) exception escaped noexcept violation site");
    }
    (void)reached_after;
    std::printf("(d) noexcept-violation test: partial (non-unix)\n");
#  endif
  }
#endif

#if defined(__unix__)
  if (SG_CONTRACTS_SEMANTIC == 2) {
    std::printf("--- (e) re-entry recursive violation probe (TDD RED) ---\n");
    auto res = runInChild(
        []
        {
          sg::dbc::setObserver(
              [](sg::dbc::ViolationRecord const& record)
              {
                if (std::string(record.message ? record.message : "")
                        .find("reentry-outer")
                    != std::string::npos)
                {
                  SG_REQUIRE(false, "reentry-nested");
                }
              });
          SG_REQUIRE(false, "reentry-outer");
          _exit(0);
        },
        30);
    std::fprintf(
        stderr,
        "(e) re-entry result: sig=%d exit=%d timeout=%d out[:200]=%.200s\n",
        res.termSig,
        res.exitStatus,
        static_cast<int>(res.timedOut),
        res.output.c_str());
    check(!res.timedOut, "(e) nested violation did not recurse unboundedly");
    check(res.termSig == SIGABRT, "(e) still terminates via response");
  }
#else
  std::printf("(e) re-entry red probe: SKIPPED (non-unix)\n");
#endif

#if SG_CONTRACTS_SEMANTIC == 3
  {
    std::printf(
        "--- (f) quick_enforce gated trap (SG_CONTRACTS_SEMANTIC=3) ---\n");
#  if defined(__unix__)
    auto res = runInChild(
        []
        {
          SG_REQUIRE(false, "qe-gated-violation");
          std::printf("qe-marker-reached\n");
          std::fflush(stdout);
          _exit(0);
        },
        5);
    bool trapped = (res.termSig != 0 || res.exitStatus != 0);
    bool markerAbsent =
        res.output.find("qe-marker-reached") == std::string::npos;
    (void)markerAbsent;
    check(trapped, "(f) quick_enforce child trapped (no parent death)");
    std::printf("(f) parent survived quick_enforce trap in child\n");
#  else
    SG_REQUIRE(false, "qe-gated-violation-nonunix");
    std::printf("qe-marker-reached\n");
#  endif
  }
#endif  // SG_CONTRACTS_SEMANTIC == 3

#if SG_CONTRACTS_SEMANTIC == 2
  sg::dbc::setObserver({});

  {
    int const heldI = requirePositive(1);
    check(heldI == 1, "templates: holding int instantiation does not abort");
    unsigned const heldU = requirePositive(1U);
    check(heldU == 1U,
          "templates: holding unsigned instantiation does not abort");
#  if defined(__unix__)
    auto res = runInChild(
        []
        {
          (void)requirePositive(0);
          _exit(0);
        },
        5);
    check(res.termSig == SIGABRT, "templates: violating instantiation aborts");
#  else
    std::printf("templates: violating instantiation: SKIPPED (non-unix)\n");
#  endif
  }

  {
    sg::dbc::ViolationRecord vrec {};
    bool const derivedFired = captureViolation(vrec,
                                               []
                                               {
                                                 ContractDerived derived;
                                                 derived.gated(-1);
                                                 ContractBase& asBase = derived;
                                                 asBase.gated(-1);
                                               });
    check(!derivedFired,
          "virtual: derived override without restated contract does not fire");
    sg::dbc::setObserver({});
#  if defined(__unix__)
    auto res = runInChild(
        []
        {
          ContractBase base;
          base.gated(-1);
          _exit(0);
        },
        5);
    check(res.termSig == SIGABRT, "virtual: base body still aborts");
#  else
    std::printf("virtual: base abort: SKIPPED (non-unix)\n");
#  endif
  }

  {
    PlacementHost host;
    check(host.peek() == 1, "placement: ctor exit holding; const entry holds");
    host.mutate();
    check(host.peek() == 2, "placement: non-const exit holding");
#  if defined(__unix__)
    auto dtorRes = runInChild(
        []
        {
          PlacementHost poisoned;
          poisoned.poison();
        },
        5);
    check(dtorRes.termSig == SIGABRT, "placement: dtor entry aborts");
    check(dtorRes.output.find("dtor-entry: value > 0") != std::string::npos,
          "placement: dtor entry diagnostic");

    auto exitRes = runInChild(
        []
        {
          PlacementHost broken;
          broken.breakAtExit();
          _exit(0);
        },
        5);
    check(exitRes.termSig == SIGABRT, "placement: non-const exit aborts");
    check(exitRes.output.find("non-const-exit: value > 0") != std::string::npos,
          "placement: non-const exit diagnostic");

    auto constRes = runInChild(
        []
        {
          PlacementHost poisoned;
          poisoned.poison();
          (void)poisoned.peek();
          _exit(0);
        },
        5);
    check(constRes.termSig == SIGABRT,
          "placement: const entry-only aborts at entry");
    check(constRes.output.find("const-entry-only: value > 0")
              != std::string::npos,
          "placement: const entry-only diagnostic");
#  else
    std::printf("placement: abort cases: SKIPPED (non-unix)\n");
#  endif
  }
#endif  // SG_CONTRACTS_SEMANTIC == 2

  std::printf("dbc_test PASS (SG_CONTRACTS_SEMANTIC=%d)\n",
              static_cast<int>(SG_CONTRACTS_SEMANTIC));
  return 0;
}
