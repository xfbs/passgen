# Copyright Tomas Zeman 2019.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

option(ENABLE_IWYU "Enable IncludeWhatYouUse" OFF)

function(iwyu_setup)
  if(ENABLE_IWYU)
    if(NOT EXISTS ${IWYU_EXECUTABLE})
      find_program(iwyu_executable_tmp NAMES include-what-you-use iwyu)
      if(iwyu_executable_tmp)
        set(IWYU_EXECUTABLE ${iwyu_executable_tmp} CACHE STRING "IncludeWhatYouUse executable path")
        unset(iwyu_executable_tmp)
        message(STATUS "IncludeWhatYouUse: found at ${IWYU_EXECUTABLE}")
      else()
          message(FATAL_ERROR "IncludeWhatYouUse: include-what-you-use binary not found, set manually with -DIWYU_EXECUTABLE=/path/to/include-what-you-use")
      endif()
    else()
      message(STATUS "IncludeWhatYouUse: using manually set ${IWYU_EXECUTABLE}")
    endif()
  endif()
endfunction()

function(target_iwyu_setup target)
  if(ENABLE_IWYU)
    set_property(TARGET ${target}
        PROPERTY
        C_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
  endif()
endfunction()
