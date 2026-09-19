add_custom_target(
    prose-lint
    COMMAND "${CMAKE_COMMAND}"
    -D "PROSE_MODE=range"
    -P "${PROJECT_SOURCE_DIR}/cmake/prose-lint.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Checking prose and commit messages"
    VERBATIM
)

add_custom_target(
    prose-lint-fixtures
    COMMAND
      python3
      "${PROJECT_SOURCE_DIR}/test/prose-gate-fixture/run_prose_gate_fixtures.py"
      --check both
      --out "${CMAKE_BINARY_DIR}/prose-gate-fixtures"
      "${PROJECT_SOURCE_DIR}/test/prose-gate-fixture"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Running the prose gate fixtures"
    VERBATIM
)
