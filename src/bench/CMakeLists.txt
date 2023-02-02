add_executable(passgen-bench bench.c stack.c random.c hashmap.c token.c parser.c utf8.c)
passgen_target_defaults(passgen-bench)
target_link_libraries(passgen-bench PUBLIC ${PASSGEN_LIB})

add_custom_target(bench
    COMMAND
    passgen-bench
    DEPENDS passgen-bench)
