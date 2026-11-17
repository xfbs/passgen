# Fuzzing harnesses. Each one is a function which takes arbitrary bytes and
# feeds them to some part of the library.
#
# By default, they are built into binaries which run them over the files given
# as arguments, and are run over their corpus as part of the tests. That way,
# inputs which once caused a problem are checked on every build.
#
# With PASSGEN_FUZZ enabled, they are built with libFuzzer instead (which
# needs Clang), to actually go looking for new inputs:
#
#   CC=clang cmake -S . -B build-fuzz -DPASSGEN_FUZZ=ON -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-fuzz
#   ./build-fuzz/src/fuzz/fuzz-pattern src/fuzz/corpus/pattern

set(FUZZ_TARGETS pattern random utf8 wordlist)

foreach(name ${FUZZ_TARGETS})
    set(target fuzz-${name})
    set(corpus ${CMAKE_CURRENT_SOURCE_DIR}/corpus/${name})

    if(PASSGEN_FUZZ)
        add_executable(${target} ${name}.c)
        target_compile_options(${target} PRIVATE -fsanitize=fuzzer)
        target_link_options(${target} PRIVATE -fsanitize=fuzzer)

        # runs the corpus, without looking for new inputs
        add_test(NAME ${target} COMMAND ${target} -runs=0 ${corpus})
    else()
        add_executable(${target} ${name}.c standalone.c)
        add_test(NAME ${target} COMMAND ${target} ${corpus})
    endif()

    # always link against the static library, so that all of it is
    # instrumented.
    target_link_libraries(${target} PRIVATE passgen-static)
    passgen_target_defaults(${target})
endforeach()
