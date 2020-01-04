file(GLOB SOURCE_LIST *.c)
file(GLOB HEADER_LIST *.h)

add_executable(passgen-test ${SOURCE_LIST} ${HEADER_LIST})

if(BUILD_STATIC)
target_link_libraries(passgen-test "-static")
endif()

target_link_libraries(passgen-test passgen)
