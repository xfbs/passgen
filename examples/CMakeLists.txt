# examples of using the library. they are built like a program outside of
# this repository would be, with only the public headers and the shared
# library, and run as tests (see the top-level CMakeLists.txt), so that they
# keep working.
add_executable(passgen-example-generate generate.c)
target_link_libraries(passgen-example-generate PRIVATE passgen-shared)
target_compile_options(passgen-example-generate PRIVATE -Wall -Wextra -g)
target_compile_features(passgen-example-generate PRIVATE c_std_99)
target_clangformat_setup(passgen-example-generate)
