# Copyright Tomas Zeman 2019.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

function(clangformat_setup)
  set(CLANGFORMAT_EXECUTABLE "clang-format" CACHE STRING "Name of the clang-format executable")

  if(NOT EXISTS ${CLANGFORMAT_EXECUTABLE})
    find_program(clangformat_executable_tmp ${CLANGFORMAT_EXECUTABLE})
    if(clangformat_executable_tmp)
      set(CLANGFORMAT_EXECUTABLE ${clangformat_executable_tmp})
      unset(clangformat_executable_tmp)
      message(STATUS "ClangFormat: clang-format found at ${CLANGFORMAT_EXECUTABLE}")
    else()
      message(STATUS "ClangFormat: clang-format not found at ${CLANGFORMAT_EXECUTABLE}")
    endif()
  endif()

  add_custom_target(clangformat)
endfunction()

function(clangformat_add target)
  foreach(clangformat_source ${ARGN})
    get_filename_component(clangformat_source ${clangformat_source} ABSOLUTE)
    list(APPEND clangformat_sources ${clangformat_source})
  endforeach()

  add_custom_target(${target}_clangformat
    COMMAND
      ${CLANGFORMAT_EXECUTABLE}
      -style=file
      -i
      ${clangformat_sources}
    VERBATIM
    DEPENDS ${clangformat_sources}
    COMMENT
      "Formating with ${CLANGFORMAT_EXECUTABLE}"
  )

  add_dependencies(clangformat ${target}_clangformat)
endfunction()

function(target_clangformat_setup target)
  get_target_property(target_sources ${target} SOURCES)
  clangformat_add(${target} ${target_sources})
endfunction()
