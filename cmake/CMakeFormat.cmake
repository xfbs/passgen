function(cmakeformat_setup)
  add_custom_target(cmakeformat
    COMMAND
      cmake-format
        -c ${PROJECT_SOURCE_DIR}/.cmake-format
        -i
        ${ARGN}
    COMMENT "Formatting with cmake-format")
endfunction()
