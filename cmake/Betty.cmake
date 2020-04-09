# convenience functions

function(betty_library target)
    set(flags INCLUDE_BINARY_DIR)
    set(multi_args SOURCES HEADERS LIBRARIES FEATURES)
    set(single_args STANDARD)
    cmake_parse_arguments(PARSE_ARGV 1 BETTY "${flags}" "${single_args}" "${multi_args}")

    add_library(${target} ${BETTY_SOURCES} ${BETTY_HEADERS})
    target_link_libraries(${target} ${BETTY_LIBRARIES})
    target_compile_features(${target}
        PUBLIC ${BETTY_STANDARD})
    target_include_directories(${target}
        PUBLIC .. ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(${target}
        PRIVATE -Wall -Wextra -pedantic)

    if(BETTY_INCLUDE_BINARY_DIR)
        target_include_directories(${target}
            PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    target_clangformat_setup(${target})
    target_iwyu_setup(${target})
endfunction(betty_library)

function(betty_executable target)
    set(single_args SOURCE STANDARD)
    set(multi_args LIBRARIES)
    cmake_parse_arguments(PARSE_ARGV 1 BETTY "" "${single_args}" "${multi_args}")

    add_executable(${target} ${BETTY_SOURCE})
    target_link_libraries(${target} ${BETTY_LIBRARIES})
    target_compile_features(${target}
        PUBLIC ${BETTY_STANDARD})
    target_compile_options(${target}
        PRIVATE -Wall -Wextra -pedantic)

    target_clangformat_setup(${target})
    target_iwyu_setup(${target})
endfunction()

function(betty_test target)
    set(single_args STANDARD)
    set(multi_args LIBRARIES SOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 BETTY "" "${single_args}" "${multi_args}")

    add_executable(${target} ${BETTY_SOURCES})
    target_compile_features(${target}
        PUBLIC ${BETTY_STANDARD})
    target_link_libraries(${target}
        Catch2::Catch2Main
        ${BETTY_LIBRARIES})
    target_compile_options(${target}
        PRIVATE -Wall -Wextra -pedantic)

    target_clangformat_setup(${target})

    add_test(
        NAME ${target}
        COMMAND ${target})
endfunction()
