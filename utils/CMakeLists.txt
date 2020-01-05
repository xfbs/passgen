# disabled for now, the sources don't work.

#add_executable(pattern-choices pattern-choices.c)
#add_executable(pattern-dump pattern-dump.c)

#target_link_libraries(pattern-choices passgen)
#target_link_libraries(pattern-dump passgen)

if(USE_LIBEXECINFO)
#target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)
