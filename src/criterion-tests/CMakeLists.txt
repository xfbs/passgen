add_executable(passgen-criterion-tests test.c random.c)
target_link_libraries(passgen-criterion-tests criterion passgen-shared)
add_test(NAME passgen-criterion-test COMMAND passgen-criterion-tests)
