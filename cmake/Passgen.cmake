function(passgen_target_defaults target)
  # enable clang-format
  target_clangformat_setup(${target})

  # enable compiler warnings
  target_compile_options(${target}
    PUBLIC -Wall -Wextra)

  # use c99 standard
  target_compile_features(${target}
    PUBLIC c_std_99)
endfunction()
