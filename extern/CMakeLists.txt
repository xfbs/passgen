# dependency: utf8proc
if(BUILD_UTF8PROC)
    add_subdirectory(utf8proc EXCLUDE_FROM_ALL)
    target_compile_options(utf8proc PRIVATE -fvisibility=hidden)
else()
    find_package(utf8proc)
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
    find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)

# dependency: secconf, if requested.
if(PASSGEN_SECCOMP)
    if(PASSGEN_STATIC)
        find_library(libseccomp NAMES libseccomp.a seccomp)
    else()
        find_library(libseccomp seccomp)
    endif()
endif()
