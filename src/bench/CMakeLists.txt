add_executable(passgen-parse-bench passgen.c)
passgen_target_defaults(passgen-parse-bench)
target_link_libraries(passgen-parse-bench PUBLIC ${PASSGEN_LIB})

add_executable(passgen-random-bench "random.c")
passgen_target_defaults(passgen-random-bench)
target_link_libraries(passgen-random-bench PUBLIC ${PASSGEN_LIB})

add_executable(passgen-bench-new newbench.c newstack.c newrandom.c)
passgen_target_defaults(passgen-bench-new)
target_link_libraries(passgen-bench-new PUBLIC ${PASSGEN_LIB})
