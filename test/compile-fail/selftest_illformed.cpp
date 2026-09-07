// selftest_illformed.cpp
// Self-test TU for the negative-compile harness (T011).
// Must be caught as a compile failure with a diagnostic containing the expect string.
// This proves the harness itself works before any real negative TUs are added (T012).
//
// platforms: all
// expect: declared

int main() {
    // undeclared identifier on purpose
    foo = 42;  // 'foo' was not declared / use of undeclared identifier 'foo'
    return 0;
}
