# disabled for now, the sources don't work.

if(BUILD_UTILS)
  # add_executable(pattern-choices pattern-choices.c) add_executable(pattern-
  # dump pattern-dump.c)

  # target_link_libraries(pattern-choices passgen) target_link_libraries
  # (pattern-dump passgen)
endif()

add_executable(passgen-bin passgen.c main.c pattern_presets.c)
target_link_libraries(passgen-bin PUBLIC ${PASSGEN_LIB} m)
set_target_properties(passgen-bin PROPERTIES OUTPUT_NAME passgen)
passgen_target_defaults(passgen-bin)

install(TARGETS passgen-bin
    DESTINATION bin
    COMPONENT bin
    OPTIONAL)

if(USE_LIBEXECINFO)
  # target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
