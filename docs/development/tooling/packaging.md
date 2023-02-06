# Packaging

CPack is supported for generating packages from the built code.

    cpack -G TXZ
    cpack -G DEB

These are used in the CI pipeline to create nightly releases.
