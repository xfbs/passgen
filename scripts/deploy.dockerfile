FROM alpine:3.24

# doxygen and graphviz for the API documentation, python for measuring how
# much of it there is, and ssh-keygen for signing the nightly builds.
RUN apk add doxygen graphviz font-noto python3 openssh-keygen bash curl xz
