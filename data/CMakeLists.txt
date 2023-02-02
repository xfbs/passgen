# FIXME: install to /etc/passgen.conf
#install(
#    FILES passgen.conf
#    DESTINATION etc/
#    COMPONENT data)

file(GLOB wordlists "*.txt")

install(
    FILES ${wordlists}
    DESTINATION share/passgen
    COMPONENT data)
