
# dependency: utf8proc
if(BUILD_UTF8PROC)
    add_subdirectory(utf8proc EXCLUDE_FROM_ALL)
  # target_code_coverage(utf8proc)
else()
  find_package(utf8proc)
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
  find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)
