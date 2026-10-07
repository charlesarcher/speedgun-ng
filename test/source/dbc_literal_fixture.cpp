// D-01 fixture: this literal keeps the pre-rename spelling through the
// rename workflow, which makes the workflow's literal safety observable.
// The file names no library entity, so it includes no header.

auto main() -> int
{
  char const* const kept = "check_precondition";
  return kept == nullptr ? 1 : 0;
}
