add_executable(passgen-cli passgen.c main.c)
target_link_libraries(passgen-cli PUBLIC ${PASSGEN_LIB} m)
set_target_properties(passgen-cli PROPERTIES OUTPUT_NAME passgen)
passgen_target_defaults(passgen-cli)

# link with seccomp, if turned on
if(PASSGEN_SECCOMP)
    target_link_libraries(passgen-cli PUBLIC ${libseccomp})
    target_compile_definitions(passgen-cli PUBLIC PASSGEN_SECCOMP)
endif()

# enable static linking when requested
if(PASSGEN_STATIC)
    target_link_options(passgen-cli PRIVATE -static)
endif()

install(TARGETS passgen-cli
    DESTINATION bin
    COMPONENT bin
    OPTIONAL)

add_custom_target(test-cli ${CMAKE_SOURCE_DIR}/scripts/test-passgen-cli.sh $<TARGET_FILE:passgen-cli>
    DEPENDS passgen-cli)

if(USE_LIBEXECINFO)
  # target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
