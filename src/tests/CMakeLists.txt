# custom testing framework.
add_library(passgen-test-lib STATIC "tests.c" "tests.h")
target_include_directories(passgen-test-lib PUBLIC ".")
passgen_target_defaults(passgen-test-lib)

# list of test sources.
set(TEST_LIST
    "array.c"
    "parser.c"
    "random.c"
    "token.c"
    "generate.c"
    "utf8.c"
    "wordlist.c"
    "stack.c"
    "markov.c"
    "hashmap.c")

# the testing framework wants a list of all test functions to execute, this step
# generates this list automagically from the test sources with the help of a
# little ruby script.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_list.c
  COMMAND ${PROJECT_SOURCE_DIR}/scripts/generate_test_list.rb
          ${CMAKE_CURRENT_BINARY_DIR}/test_list.c ${TEST_LIST}
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/generate_test_list.rb ${TEST_LIST}
  COMMENT "Generating test list"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

add_executable(passgen-test ${CMAKE_CURRENT_BINARY_DIR}/test_list.c
                            ${TEST_LIST})
target_link_libraries(passgen-test PRIVATE ${PASSGEN_LIB} passgen-test-lib)
passgen_target_defaults(passgen-test)

add_test(NAME passgen-test COMMAND passgen-test)
