file(GLOB SOURCE_LIST *.c)
file(GLOB HEADER_LIST *.h)

add_executable(passgen-test ${SOURCE_LIST} ${HEADER_LIST})

target_link_libraries(passgen-test passgen)
