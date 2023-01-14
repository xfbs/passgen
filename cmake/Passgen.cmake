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
      -p utf8
      -e _init
      -e _fini
      $<TARGET_FILE:${target}>
    DEPENDS ${target}
    VERBATIM)

  add_dependencies(symbols-check ${target}-symbols-check)
endfunction()

function(passgen_target_defaults target)
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

  target_code_coverage(${target})

  target_iwyu_setup(${target})
endfunction()
