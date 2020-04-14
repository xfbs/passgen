# disabled for now, the sources don't work.

if(BUILD_UTILS)
  # add_executable(pattern-choices pattern-choices.c) add_executable(pattern-
  # dump pattern-dump.c)

  # target_link_libraries(pattern-choices passgen) target_link_libraries
  # (pattern-dump passgen)
endif()

add_executable(passgen-tokens passgen-tokens.c)
target_link_libraries(passgen-tokens passgen-static ${libjansson})

add_executable(utf8-recode utf8-recode.c)
target_link_libraries(utf8-recode passgen-static utf8proc)

if(USE_LIBEXECINFO)
  # target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
