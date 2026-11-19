function(passgen_check_symbols_setup)
  add_custom_target(symbols-check)
  find_program(NM_PATH nm)
endfunction()

function(passgen_metrics_setup)
    add_custom_target(metrics)
endfunction()

function(passgen_metrics target)
    add_custom_command(
        OUTPUT ${target}.symbols.txt
        COMMAND ${NM_PATH} -D --defined-only $<TARGET_FILE:${target}> > ${target}.symbols.txt
        DEPENDS ${target})
    add_custom_target(${target}-metrics
        DEPENDS ${target}.symbols.txt)
    add_dependencies(metrics ${target}-metrics)
endfunction()

# TODO: limit exported symbols to passgen_*
function(passgen_check_symbols target)
  add_custom_target(${target}-symbols-check
    COMMAND
    ruby ${PROJECT_SOURCE_DIR}/scripts/symbols-check.rb
      -n ${NM_PATH}
      -p passgen
      -e _init
      -e _fini
      $<TARGET_FILE:${target}>
    DEPENDS ${target}
    VERBATIM)

  add_dependencies(symbols-check ${target}-symbols-check)
endfunction()

function(passgen_target_defaults target)
  # with PUBLIC_ONLY, the target only gets the public headers, like anything
  # outside of this project would. the command-line interface does this.
  cmake_parse_arguments(PARSE_ARGV 1 defaults "PUBLIC_ONLY" "" "")

  # the headers in include/ are the public interface, and are what is
  # installed. the ones for the internals are in src/include/, and can only be
  # used by what is built as part of this project: the library itself, the
  # tests, the benchmarks and the tools.
  if(NOT defaults_PUBLIC_ONLY)
    target_include_directories(${target}
        PRIVATE ${PROJECT_SOURCE_DIR}/src/include)
  endif()

  # enable clang-format
  target_clangformat_setup(${target})

  # enable compiler warnings
  target_compile_options(${target}
      PUBLIC -Wall -Wextra -Werror=implicit-function-declaration -g)

  # use c99 standard
  target_compile_features(${target}
    PUBLIC c_std_99)

  if(USE_BACKTRACE)
    target_compile_definitions(${target}
      PRIVATE PASSGEN_BACKTRACE)
  endif()

  # monocypher (src/monocypher.c and its header) and tomlc17 (in src/cli) are
  # vendored, and tested upstream, so they are left out of the coverage
  # reports, where they would only make the rest harder to see. clang uses
  # llvm-cov, which takes a regular expression, and gcc uses lcov, which takes
  # a wildcard pattern for the whole path.
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(coverage_exclude "/(monocypher|tomlc17)\\.[ch]")
  else()
    set(coverage_exclude
        "*/monocypher.c" "*/monocypher.h" "*/tomlc17.c" "*/tomlc17.h")
  endif()
  target_code_coverage(${target} EXCLUDE ${coverage_exclude})

  target_iwyu_setup(${target})
endfunction()
