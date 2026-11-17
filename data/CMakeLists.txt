# FIXME: install to /etc/passgen.conf
#install(
#    FILES passgen.conf
#    DESTINATION etc/
#    COMPONENT data)

# wordlists are not part of the repository, they are downloaded by
# scripts/fetch-wordlists.sh.
file(GLOB wordlists "*.txt")
list(FILTER wordlists EXCLUDE REGEX "/(CMakeLists|checksums)\\.txt$")

install(
    FILES ${wordlists}
    DESTINATION share/passgen
    COMPONENT data)
