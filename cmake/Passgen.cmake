function(passgen_check_symbols_setup)
  add_custom_target(symbols-check)
  find_program(NM_PATH nm)
endfunction()

# TODO: limit exported symbols to passgen_*
function(passgen_check_symbols target)
  add_custom_target(${target}-symbols-check
    COMMAND
    ruby ${PROJECT_SOURCE_DIR}/scripts/check_symbols.rb
      -n ${NM_PATH}
      -p passgen
      -p pattern
      -p reader
      -p unicode
      -p utf8
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
    PUBLIC -Wall -Wextra)

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
