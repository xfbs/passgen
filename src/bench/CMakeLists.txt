add_executable(passgen-bench
    bench.c
    stack.c
    random.c
    hashmap.c
    token.c
    parser.c
    utf8.c
    wordlist.c
    generate.c
    master.c)
passgen_target_defaults(passgen-bench)
target_link_libraries(passgen-bench PUBLIC ${PASSGEN_LIB})

# the wordlist which the benchmarks use, unless another one is given with
# `-o wordlist=PATH`. it is downloaded by scripts/fetch-wordlists.sh, and the
# benchmarks which need it are skipped without it.
target_compile_definitions(passgen-bench PRIVATE
    PASSGEN_BENCH_WORDLIST="${PROJECT_SOURCE_DIR}/data/eff-large.txt")

add_custom_target(bench
    COMMAND
    passgen-bench
    DEPENDS passgen-bench)
