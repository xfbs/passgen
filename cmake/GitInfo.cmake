find_package(Git QUIET REQUIRED)

macro(GIT_INFO)
execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE ret
    OUTPUT_VARIABLE PROJECT_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "Fatal error getting the git commit hash. Set BUILD_GIT_INFO false to disable.")
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --long
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE ret
    OUTPUT_VARIABLE PROJECT_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "Fatal error getting the git describe. Set BUILD_GIT_INFO false to disable.")
endif()

set_property(GLOBAL APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/index")
endmacro(GIT_INFO)
