# custom testing framework.
add_library(passgen-test-lib STATIC "tests.c" "tests.h")
target_include_directories(passgen-test-lib PUBLIC ".")
passgen_target_defaults(passgen-test-lib)

# the testing framework puts back the allocator of the library after every
# test, so it uses the library, and brings it along.
target_link_libraries(passgen-test-lib PUBLIC ${PASSGEN_LIB})

# list of test sources.
set(TEST_LIST
    "alloc.c"
    "entropy.c"
    "error.c"
    "generate.c"
    "hashmap.c"
    "literal.c"
    "markov.c"
    "parser.c"
    "random.c"
    "regression.c"
    "siphash.c"
    "sizes.c"
    "stack.c"
    "token.c"
    "utf8.c"
    "vectors.c"
    "version.c"
    "wordlist.c"
)

# the testing framework wants a list of all test functions to execute, this step
# generates this list automagically from the test sources with the help of a
# little ruby script.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_list.c
  COMMAND ruby ${PROJECT_SOURCE_DIR}/scripts/generate_test_list.rb
          ${CMAKE_CURRENT_BINARY_DIR}/test_list.c ${TEST_LIST}
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/generate_test_list.rb ${TEST_LIST}
  COMMENT "Generating test list"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

add_executable(passgen-test ${CMAKE_CURRENT_BINARY_DIR}/test_list.c
                            ${TEST_LIST})
target_link_libraries(passgen-test PRIVATE passgen-test-lib)
passgen_target_defaults(passgen-test)

add_custom_target(test-lib passgen-test -v
    DEPENDS passgen-test USES_TERMINAL)

# a test which uses the library like something outside of this repository
# would: with only the public headers, and the shared library. this does not
# use passgen_target_defaults(), as that adds the headers for the internals.
if(LIBPASSGEN_SHARED)
    add_executable(passgen-test-public "public.c")
    target_link_libraries(passgen-test-public PRIVATE passgen-shared)
    target_compile_options(passgen-test-public PRIVATE -Wall -Wextra -g)
    target_compile_features(passgen-test-public PRIVATE c_std_99)

    # the same from C++, which only links if the headers declare everything
    # as extern "C".
    if(CMAKE_CXX_COMPILER)
        add_executable(passgen-test-public-cxx "public.cpp")
        target_link_libraries(passgen-test-public-cxx PRIVATE passgen-shared)
        target_compile_options(passgen-test-public-cxx PRIVATE -Wall -Wextra -g)
        target_compile_features(passgen-test-public-cxx PRIVATE cxx_std_11)
    endif()
endif()
