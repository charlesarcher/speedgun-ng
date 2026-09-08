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
struct violation_caught
{
  sg::dbc::ViolationRecord record {};
};

// Instrumentation for the exactly-once predicate test (FR-019). A pure
// predicate cannot carry a side effect, so the test's *measurement probe* is
// side-effecting on purpose; the production purity rule (FR-019) applies to
// contract predicates, not to test probes.
int predicate_evaluations = 0;

auto counting_predicate() -> bool
{
  ++predicate_evaluations;
  return true;
}

// A recording observer that stores the record into `out` and then throws it
// back to the caller.
auto record_into(sg::dbc::ViolationRecord& out) -> sg::dbc::violation_observer
{
  return [&out](sg::dbc::ViolationRecord const& rec)
  {
    out = rec;
    throw violation_caught {rec};
  };
}

// ---- (a)/(b) per-primitive violation sites -------------------------------

auto violate_precondition() -> void
{
  SG_REQUIRE(false, "pre: x > 0");
}

auto violate_postcondition() -> int
{
  int result = 41;
  SG_ENSURE(result == 42, "post: result == 42");
  return result;
}

auto violate_loop_invariant() -> void
{
  int i = -1;
  for (; i < 3; ++i) {
    SG_INVARIANT(i >= 0, "inv: i >= 0");
    break;
  }
}

// A class whose constructor leaves the invariant violated (checked at ctor
// exit, FR-006).
struct bad_invariant_object
{
  int value = -1;

  bad_invariant_object() { SG_INVARIANT(value >= 0, "inv: value >= 0"); }
};

auto violate_assertion() -> void
{
  SG_ASSERT(1 == 2, "assert: 1 == 2");
}

// Exactly-once probe site (a semantic-gated, satisfied check).
auto gated_satisfied_site() -> void
{
  SG_REQUIRE(counting_predicate(), "positive");
}

// Runs `body` under a fresh recording observer; returns the captured record
// and whether the observer actually threw (i.e. the violation was delivered).
auto capture_violation(sg::dbc::ViolationRecord& rec, auto&& body) -> bool
{
  sg::dbc::set_observer(record_into(rec));
  bool caught = false;
  try {
    body();
  } catch (violation_caught const&) {
    caught = true;
  }
  return caught;
}

#if SG_CONTRACTS_SEMANTIC == 2
template<typename T>
auto require_positive(T arg) -> T
{
  SG_REQUIRE(arg > T {}, "template: arg > T{}");
  return arg;
}

struct contract_base
{
  contract_base() = default;
  contract_base(contract_base const&) = default;
  contract_base(contract_base&&) = default;
  auto operator=(contract_base const&) -> contract_base& = default;
  auto operator=(contract_base&&) -> contract_base& = default;
  virtual ~contract_base() = default;

  virtual auto gated(int arg) -> void { SG_REQUIRE(arg > 0, "base: arg > 0"); }
};

struct contract_derived : contract_base
{
  // enforcement is per-body
  auto gated(int arg) -> void override { static_cast<void>(arg); }
};

struct placement_host
{
  placement_host()
  {
    // ctor exit (may exist)
    SG_INVARIANT(m_value > 0, "ctor-exit: value > 0");
  }

  placement_host(placement_host const&) = default;
  placement_host(placement_host&&) = default;
  auto operator=(placement_host const&) -> placement_host& = default;
  auto operator=(placement_host&&) -> placement_host& = default;

  ~placement_host()
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

  auto break_at_exit() -> void
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
  sg::dbc::ViolationRecord rec {};

#if SG_CONTRACTS_SEMANTIC != 3
  // (a)/(b) precondition (FR-002/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(capture_violation(rec, [] { violate_precondition(); }),
        "pre: violation delivered to the observer");
  check(rec.kind == Kind::precondition, "pre: kind == precondition");
  check(std::string(rec.message) == "pre: x > 0", "pre: message text");
  check(std::string(rec.predicateText) == "false", "pre: predicate text");
  check(rec.file != nullptr && rec.file[0] != '\0', "pre: file present");
  check(rec.line > 0, "pre: line present");

  // (a)/(b) postcondition over named result capture (FR-003/FR-004/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(capture_violation(rec, [] { (void)violate_postcondition(); }),
        "post: violation delivered to the observer");
  check(rec.kind == Kind::postcondition, "post: kind == postcondition");
  check(std::string(rec.message) == "post: result == 42", "post: message text");
  check(std::string(rec.predicateText) == "result == 42",
        "post: predicate text");

  // (a)/(b) loop invariant at iteration entry (FR-005/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(capture_violation(rec, [] { violate_loop_invariant(); }),
        "loop-inv: violation delivered to the observer");
  check(rec.kind == Kind::invariant, "loop-inv: kind == invariant");
  check(std::string(rec.predicateText) == "i >= 0", "loop-inv: predicate text");

  // (a)/(b) class invariant at constructor exit (FR-006/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(capture_violation(rec, [] { (void)bad_invariant_object {}; }),
        "class-inv: violation delivered at ctor exit");
  check(rec.kind == Kind::invariant, "class-inv: kind == invariant");
  check(std::string(rec.predicateText) == "value >= 0",
        "class-inv: predicate text");

  // (a)/(b) in-body assertion (FR-007/FR-021)
  rec = sg::dbc::ViolationRecord {};
  check(capture_violation(rec, [] { violate_assertion(); }),
        "assert: violation delivered to the observer");
  check(rec.kind == Kind::assertion, "assert: kind == assertion");
  check(std::string(rec.predicateText) == "1 == 2", "assert: predicate text");
#endif  // SG_CONTRACTS_SEMANTIC != 3

  // (c) exactly-once predicate evaluation (FR-019). The probe site is
  // semantic-gated and satisfied, so the predicate is evaluated exactly once
  // in a checked build and not at all in ignore.
  predicate_evaluations = 0;
  gated_satisfied_site();
#if SG_CONTRACTS_SEMANTIC == 0
  check(predicate_evaluations == 0, "ignore: gated predicate not evaluated");
#else
  check(predicate_evaluations == 1,
        "checked: predicate evaluated exactly once");
#endif

  // (d) semantic behavior (FR-013..015). The suite asserts the branch matching
  // the *compiled* semantic. The dev build is `enforce`; `observe` is asserted
  // here (execution continues in-process). `quick_enforce` (no hook) and
  // `ignore` (elision) are proven by the trap fixture (T004/T005), the
  // consumer-release job (T016), and the semantics matrix (T019).
#if SG_CONTRACTS_SEMANTIC == 1
  {
    sg::dbc::ViolationRecord orec {};
    sg::dbc::set_observer([&orec](sg::dbc::ViolationRecord const& r)
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
    check(orec.kind == Kind::postcondition, "observe: record captured");
  }
#endif

  // ==========================================================================
  // US3 observer / response red tests (T020). Fork-based so parent survives
  // aborts. All fork groups guarded; on non-Unix print skip and continue.
  // (a)-(d) exercise current machinery (may pass); (e) is the deliberate TDD
  // red (re-entry guard absent until T021); (f) is QE-specific.
  // ==========================================================================

  sg::dbc::set_observer({});

#if defined(__unix__)
  struct child_result
  {
    int exit_status = -1;
    int term_sig = 0;
    std::string output;
    bool timed_out = false;
  };

  auto run_in_child = [](auto&& child_body, int timeout_sec = 5) -> child_result
  {
    child_result r {};
    auto old_alrm = signal(SIGALRM, [](int) {});
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
      if (timeout_sec > 0) {
        alarm(static_cast<unsigned>(timeout_sec));
      }
      child_body();
      _exit(0);
    }
    close(pipefd[1]);
    if (timeout_sec > 0) {
      alarm(static_cast<unsigned>(timeout_sec));
    }
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    alarm(0);
    signal(SIGALRM, old_alrm);
    if (w < 0
        || (timeout_sec > 0 && WIFSIGNALED(status)
            && WTERMSIG(status) == SIGALRM))
    {
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      r.timed_out = true;
    }
    if (WIFSIGNALED(status)) {
      r.term_sig = WTERMSIG(status);
    } else if (WIFEXITED(status)) {
      r.exit_status = WEXITSTATUS(status);
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
    auto res = run_in_child(
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
    bool is_sigabrt = (res.term_sig == SIGABRT);
    bool has_structured = res.output.find("[precondition]") != std::string::npos
        && res.output.find("default: must abort not catch") != std::string::npos
        && res.output.find("(predicate:") != std::string::npos
        && res.output.find(" at ") != std::string::npos;
    bool no_catch_marker =
        res.output.find("CAUGHT-IN-CHILD") == std::string::npos;
    check(is_sigabrt, "(a) default response must SIGABRT");
    check(has_structured, "(a) default diagnostic on stderr");
    check(no_catch_marker, "(a) not catchable");
  }
#else
  std::printf("(a) default-response fork test: SKIPPED (non-unix)\n");
#endif

#if SG_CONTRACTS_SEMANTIC == 2
  // (b) second set_observer replaces first
  {
    sg::dbc::ViolationRecord r1 {};
    sg::dbc::ViolationRecord r2 {};
    sg::dbc::set_observer(record_into(r1));
    sg::dbc::set_observer(
        [&r2](sg::dbc::ViolationRecord const& recb)
        {
          r2 = recb;
          throw violation_caught {recb};
        });
    bool caught = false;
    try {
      SG_REQUIRE(false, "second-observer");
    } catch (violation_caught const&) {
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
  // exception (dispatch NOT noexcept), not via the response abort path.
#if defined(__unix__)
  if (SG_CONTRACTS_SEMANTIC == 2) {
    struct unique_exc : std::exception
    {
      const char* what() const noexcept override
      {
        return "unique-observer-exc";
      }
    };

    auto res = run_in_child(
        []
        {
          sg::dbc::set_observer([](sg::dbc::ViolationRecord const&)
                                { throw unique_exc {}; });
          SG_REQUIRE(false, "observer-throw");
          _exit(77);
        },
        5);
    // observer path: no default diagnostic emitted (report returns before
    // abort)
    bool no_default_diag =
        res.output.find("contract violation") == std::string::npos;
    bool died = (res.term_sig != 0 || res.exit_status != 0);
    // On uncaught, std::terminate typically aborts (SIGABRT or SIGSEGV), but
    // absence of the default diag proves we took the throw path not abort path.
    check(died, "(c) child died on observer throw");
    check(no_default_diag, "(c) died via exception (no default response diag)");
  }
#else
  std::printf("(c) observer-throw fork test: SKIPPED (non-unix)\n");
#endif

#if SG_CONTRACTS_SEMANTIC == 2
  // (d) violation inside noexcept function terminates via response; no
  // exception escapes the noexcept site.
  {
    auto noexcept_violator = []() noexcept
    { SG_REQUIRE(false, "noexcept-violation"); };
#  if defined(__unix__)
    if (SG_CONTRACTS_SEMANTIC == 2) {
      auto res = run_in_child(
          [&]
          {
            noexcept_violator();
            _exit(0);
          },
          5);
      bool died = (res.term_sig == SIGABRT || res.exit_status != 0);
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
    auto res = run_in_child(
        []
        {
          sg::dbc::set_observer(
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
        res.term_sig,
        res.exit_status,
        static_cast<int>(res.timed_out),
        res.output.c_str());
    check(!res.timed_out, "(e) nested violation did not recurse unboundedly");
    check(res.term_sig == SIGABRT, "(e) still terminates via response");
  }
#else
  std::printf("(e) re-entry red probe: SKIPPED (non-unix)\n");
#endif

#if SG_CONTRACTS_SEMANTIC == 3
  {
    std::printf(
        "--- (f) quick_enforce gated trap (SG_CONTRACTS_SEMANTIC=3) ---\n");
#  if defined(__unix__)
    auto res = run_in_child(
        []
        {
          SG_REQUIRE(false, "qe-gated-violation");
          std::printf("qe-marker-reached\n");
          std::fflush(stdout);
          _exit(0);
        },
        5);
    bool trapped = (res.term_sig != 0 || res.exit_status != 0);
    bool marker_absent =
        res.output.find("qe-marker-reached") == std::string::npos;
    (void)marker_absent;
    check(trapped, "(f) quick_enforce child trapped (no parent death)");
    std::printf("(f) parent survived quick_enforce trap in child\n");
#  else
    SG_REQUIRE(false, "qe-gated-violation-nonunix");
    std::printf("qe-marker-reached\n");
#  endif
  }
#endif  // SG_CONTRACTS_SEMANTIC == 3

#if SG_CONTRACTS_SEMANTIC == 2
  sg::dbc::set_observer({});

  {
    int const held_i = require_positive(1);
    check(held_i == 1, "templates: holding int instantiation does not abort");
    unsigned const held_u = require_positive(1U);
    check(held_u == 1U,
          "templates: holding unsigned instantiation does not abort");
#  if defined(__unix__)
    auto res = run_in_child(
        []
        {
          (void)require_positive(0);
          _exit(0);
        },
        5);
    check(res.term_sig == SIGABRT, "templates: violating instantiation aborts");
#  else
    std::printf("templates: violating instantiation: SKIPPED (non-unix)\n");
#  endif
  }

  {
    sg::dbc::ViolationRecord vrec {};
    bool const derived_fired = capture_violation(vrec,
                                                 []
                                                 {
                                                   contract_derived derived;
                                                   derived.gated(-1);
                                                   contract_base& as_base =
                                                       derived;
                                                   as_base.gated(-1);
                                                 });
    check(!derived_fired,
          "virtual: derived override without restated contract does not fire");
    sg::dbc::set_observer({});
#  if defined(__unix__)
    auto res = run_in_child(
        []
        {
          contract_base base;
          base.gated(-1);
          _exit(0);
        },
        5);
    check(res.term_sig == SIGABRT, "virtual: base body still aborts");
#  else
    std::printf("virtual: base abort: SKIPPED (non-unix)\n");
#  endif
  }

  {
    placement_host host;
    check(host.peek() == 1, "placement: ctor exit holding; const entry holds");
    host.mutate();
    check(host.peek() == 2, "placement: non-const exit holding");
#  if defined(__unix__)
    auto dtor_res = run_in_child(
        []
        {
          placement_host poisoned;
          poisoned.poison();
        },
        5);
    check(dtor_res.term_sig == SIGABRT, "placement: dtor entry aborts");
    check(dtor_res.output.find("dtor-entry: value > 0") != std::string::npos,
          "placement: dtor entry diagnostic");

    auto exit_res = run_in_child(
        []
        {
          placement_host broken;
          broken.break_at_exit();
          _exit(0);
        },
        5);
    check(exit_res.term_sig == SIGABRT, "placement: non-const exit aborts");
    check(
        exit_res.output.find("non-const-exit: value > 0") != std::string::npos,
        "placement: non-const exit diagnostic");

    auto const_res = run_in_child(
        []
        {
          placement_host poisoned;
          poisoned.poison();
          (void)poisoned.peek();
          _exit(0);
        },
        5);
    check(const_res.term_sig == SIGABRT,
          "placement: const entry-only aborts at entry");
    check(const_res.output.find("const-entry-only: value > 0")
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
