# disabled for now, the sources don't work.

if(BUILD_UTILS)
  # add_executable(pattern-choices pattern-choices.c) add_executable(pattern-
  # dump pattern-dump.c)

  # target_link_libraries(pattern-choices passgen) target_link_libraries
  # (pattern-dump passgen)
endif()

add_executable(passgen-bin passgen.c main.c)
target_link_libraries(passgen-bin PUBLIC ${PASSGEN_LIB} m)
set_target_properties(passgen-bin PROPERTIES OUTPUT_NAME passgen)
passgen_target_defaults(passgen-bin)

# link with seccomp, if turned on
if(PASSGEN_SECCOMP)
    target_link_libraries(passgen-bin PUBLIC ${libseccomp})
    target_compile_definitions(passgen-bin PUBLIC PASSGEN_SECCOMP)
endif()

# enable static linking when requested
if(PASSGEN_STATIC)
    target_link_options(passgen-bin PRIVATE -static)
endif()

install(TARGETS passgen-bin
    DESTINATION bin
    COMPONENT bin
    OPTIONAL)

if(USE_LIBEXECINFO)
  # target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
