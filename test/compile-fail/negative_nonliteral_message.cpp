// negative_nonliteral_message.cpp
// Negative test for FR-020: the message must be a string literal constant;
// runtime-evaluated message expressions are not supported.
//
// platforms: all
// expect: const char
//
// Actual diagnostics observed:
//   gcc:   error: invalid initialization of reference of type ‘const char (&)[]’ from expression of type ‘const char*’
//   clang: error: reference to incomplete type 'const char[]' could not bind to an lvalue of type 'const char *'

#include <speedgun-ng/dbc.hpp>

auto main() -> int
{
  const char* msg = "x";
  SG_REQUIRE(true, msg);
  return 0;
}
