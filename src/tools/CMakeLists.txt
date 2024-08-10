add_executable(utf8-recode utf8-recode.c)
target_link_libraries(utf8-recode PUBLIC ${PASSGEN_LIB})
passgen_target_defaults(utf8-recode)

add_executable(sizes sizes.c)
target_link_libraries(sizes PUBLIC ${PASSGEN_LIB})
passgen_target_defaults(sizes)

add_executable(wordlist wordlist.c)
target_link_libraries(wordlist PUBLIC ${PASSGEN_LIB})
passgen_target_defaults(wordlist)

add_executable(tokenize tokenize.c)
target_link_libraries(tokenize PUBLIC ${PASSGEN_LIB})
passgen_target_defaults(tokenize)
