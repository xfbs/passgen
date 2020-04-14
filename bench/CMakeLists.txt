add_library(passgen-bench-lib STATIC "bench.c" "bench.h")
target_include_directories(passgen-bench-lib PUBLIC "." "/usr/local/include")
target_link_libraries(passgen-bench-lib PUBLIC ${libjansson})
passgen_target_defaults(passgen-bench-lib)

set(SOURCE_FILES
  parse.c
  all.c)

find_package(Threads REQUIRED)
add_executable(passgen-bench ${SOURCE_FILES})
passgen_target_defaults(passgen-bench)
target_link_libraries(passgen-bench PUBLIC passgen-static passgen-bench-lib Threads::Threads)

add_executable(passgen-random-bench "random.c")
passgen_target_defaults(passgen-random-bench)
target_link_libraries(passgen-random-bench PUBLIC passgen-static)
