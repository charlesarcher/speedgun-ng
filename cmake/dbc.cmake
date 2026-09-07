# ---- Contract evaluation semantic (feature 001, DBC) ----
#
# speedgun-ng_CONTRACTS selects how contract violations are handled and
# how much contract code is emitted (FR-011). It is fully independent of
# NDEBUG and of the optimization level (FR-017): changing the build type
# or the standard assertion macro never changes contract evaluation, and
# changing this switch never changes standard assert behavior.
#
# The semantic is exposed to the contract header (include/speedgun-ng/
# dbc.hpp) as a single numeric compile definition so the macro layer can
# branch with plain preprocessor equality tests:
#
#   SG_CONTRACTS_SEMANTIC  0=ignore  1=observe  2=enforce  3=quick_enforce
#
# "ignore" elides all semantic-gated contract macros to nothing (zero
# code, FR-012/FR-018/FR-037). Always-on macros (FR-036) are compiled in
# every configuration regardless of this value and force the default
# (checked-build) violation response.
#
# The developer/CI default is "enforce" (FR-016); the dedicated
# consumer-release CI job configures "ignore" explicitly (SC-002).

set(
    speedgun-ng_CONTRACTS "enforce"
    CACHE STRING
    "DBC evaluation semantic: ignore, observe, enforce, or quick_enforce"
)
set_property(
    CACHE speedgun-ng_CONTRACTS PROPERTY STRINGS
    ignore observe enforce quick_enforce
)

if(speedgun-ng_CONTRACTS STREQUAL "ignore")
  set(_dbc_semantic 0)
elseif(speedgun-ng_CONTRACTS STREQUAL "observe")
  set(_dbc_semantic 1)
elseif(speedgun-ng_CONTRACTS STREQUAL "enforce")
  set(_dbc_semantic 2)
elseif(speedgun-ng_CONTRACTS STREQUAL "quick_enforce")
  set(_dbc_semantic 3)
else()
  message(
      FATAL_ERROR
      "speedgun-ng_CONTRACTS: unknown semantic "
      "'${speedgun-ng_CONTRACTS}' (expected ignore, observe, enforce, "
      "or quick_enforce)"
  )
endif()

target_compile_definitions(
    speedgun-ng_speedgun-ng PUBLIC SG_CONTRACTS_SEMANTIC=${_dbc_semantic}
)
