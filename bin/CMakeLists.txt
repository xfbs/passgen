# disabled for now, the sources don't work.

if(BUILD_UTILS)
  # add_executable(pattern-choices pattern-choices.c) add_executable(pattern-
  # dump pattern-dump.c)

  # target_link_libraries(pattern-choices passgen) target_link_libraries
  # (pattern-dump passgen)
endif()

if(BUILD_BINARY)
  add_executable(passgen-bin passgen.c)
  target_link_libraries(passgen-bin passgen-static)
  set_target_properties(passgen-bin PROPERTIES OUTPUT_NAME passgen)

  add_executable(passgen2-bin passgen2.c)
  target_link_libraries(passgen2-bin passgen-static)
  set_target_properties(passgen2-bin PROPERTIES OUTPUT_NAME passgen2)
endif()

if(USE_LIBEXECINFO)
  # target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
