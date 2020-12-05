add_executable(passgen-tokens passgen-tokens.c)
target_link_libraries(passgen-tokens PUBLIC passgen-static ${libjansson})
passgen_target_defaults(passgen-tokens)

add_executable(utf8-recode utf8-recode.c)
target_link_libraries(utf8-recode PUBLIC passgen-static utf8proc)
passgen_target_defaults(utf8-recode)

add_executable(sizes sizes.c)
target_link_libraries(sizes PUBLIC passgen-static)
passgen_target_defaults(sizes)

add_executable(assertion assertion.c)
target_link_libraries(assertion PUBLIC passgen-static)
passgen_target_defaults(assertion)

add_executable(wordlist wordlist.c)
target_link_libraries(wordlist PUBLIC passgen-static)
passgen_target_defaults(wordlist)
