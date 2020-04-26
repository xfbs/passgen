
# dependency: utf8proc
if(BUILD_UTF8PROC)
  add_subdirectory(utf8proc)
  # target_code_coverage(utf8proc)
else()
  find_package(utf8proc)
endif()

if(BUILD_JANSSON)
  set(JANSSON_BUILD_DOCS OFF CACHE BOOL "")
  set(JANSSON_EXAMPLES OFF CACHE BOOL "")
  set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "")
  add_subdirectory(jansson)
  set(libjansson jansson CACHE STRING "Jansson library")
else()
  pkg_check_modules(JANSSON REQUIRED jansson>=2)
  find_library(libjansson ${JANSSON_LIBRARIES})
  add_library(jansson::jansson UNKNOWN IMPORTED GLOBAL)
  set_target_properties(jansson::jansson PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${JANSSON_INCLUDE_DIRS}"
    IMPORTED_LOCATION "${libjansson}")
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
  find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)
