# dependency: utf8proc
if(BUILD_UTF8PROC)
    add_subdirectory(utf8proc EXCLUDE_FROM_ALL)
else()
    find_package(utf8proc)
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
    find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)

# dependency: secconf, if requested.
if(PASSGEN_SECCOMP)
    find_library(libseccomp seccomp)
endif()

add_library(criterion INTERFACE)
target_link_libraries(criterion INTERFACE ${CRITERION_LIBRARIES})
target_include_directories(criterion INTERFACE ${CRITERION_INCLUDE_DIRS})
