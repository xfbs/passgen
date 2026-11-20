add_executable(passgen-cli
    main.c
    color.c
    config.c
    login.c
    master.c
    options.c
    output.c
    passgen.c
    prompt.c
    tomlc17.c)
set_target_properties(passgen-cli PROPERTIES OUTPUT_NAME passgen)
# tomlc17 is vendored, so it is not part of the unity build, like monocypher.
set_source_files_properties(tomlc17.c PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION YES)

# the command-line interface is linked against the static library by default,
# so that it is a single binary. it can also be linked against the shared
# library, like any other program which uses it.
if(PASSGEN_CLI_SHARED)
    if(NOT LIBPASSGEN_SHARED OR PASSGEN_STATIC)
        message(FATAL_ERROR
            "PASSGEN_CLI_SHARED needs LIBPASSGEN_SHARED, and cannot be "
            "used with PASSGEN_STATIC")
    endif()
    target_link_libraries(passgen-cli PRIVATE passgen-shared m)

    # the binary looks for the library relative to where it is (such as
    # ../lib), so that it can be installed or unpacked anywhere.
    if(APPLE)
        set_target_properties(passgen-cli PROPERTIES
            INSTALL_RPATH "@loader_path/${PASSGEN_BIN_TO_LIBDIR}")
    else()
        set_target_properties(passgen-cli PROPERTIES
            INSTALL_RPATH "$ORIGIN/${PASSGEN_BIN_TO_LIBDIR}")
    endif()
else()
    target_link_libraries(passgen-cli PRIVATE ${PASSGEN_LIB} m)
endif()
# the command-line interface only uses the public interface of the library.
passgen_target_defaults(passgen-cli PUBLIC_ONLY)

# where the configuration is, which is installed along with passgen, and the
# one for the whole system (which is /etc for the prefix /usr).
target_compile_definitions(passgen-cli PRIVATE
    PASSGEN_CLI_CONFIG_DATA="${CMAKE_INSTALL_PREFIX}/share/passgen/config.toml"
    PASSGEN_CLI_CONFIG_SYSTEM="${CMAKE_INSTALL_FULL_SYSCONFDIR}/passgen/config.toml")

# lock memory, if turned on
if(PASSGEN_MLOCK)
    target_compile_definitions(passgen-cli PRIVATE PASSGEN_MLOCK)
endif()

# link with seccomp, if turned on
if(PASSGEN_SECCOMP)
    target_link_libraries(passgen-cli PUBLIC ${libseccomp})
    target_compile_definitions(passgen-cli PUBLIC PASSGEN_SECCOMP)
    target_sources(passgen-cli PRIVATE seccomp.c)
    # it includes <asm/termbits.h>, which conflicts with <termios.h>.
    set_source_files_properties(seccomp.c PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION YES)
endif()

# enable static linking when requested
if(PASSGEN_STATIC)
    target_link_options(passgen-cli PRIVATE -static)
endif()

install(TARGETS passgen-cli
    DESTINATION bin
    COMPONENT runtime
    OPTIONAL)

add_custom_target(test-cli ${CMAKE_SOURCE_DIR}/scripts/test-passgen-cli.sh $<TARGET_FILE:passgen-cli>
    DEPENDS passgen-cli)

if(USE_LIBEXECINFO)
  # target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
