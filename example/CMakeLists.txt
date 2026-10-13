# The quick start program the root page quotes in full. It is a binary a
# reader builds and runs by hand, so it adds nothing to the report output the
# aggregate suite compares.
add_example(first_benchmark)
target_link_libraries(first_benchmark PRIVATE speedgun-ng::harness)

add_folders(Example)
