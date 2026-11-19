# the configuration which is installed along with passgen. it defines the
# wordlists next to it, by relative paths.
install(
    FILES config.toml
    DESTINATION share/passgen
    COMPONENT data)

# wordlists are not part of the repository, they are downloaded by
# scripts/fetch-wordlists.sh.
file(GLOB wordlists "*.txt")
list(FILTER wordlists EXCLUDE REGEX "/(CMakeLists|checksums)\\.txt$")

install(
    FILES ${wordlists}
    DESTINATION share/passgen
    COMPONENT data)
