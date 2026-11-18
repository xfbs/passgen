FROM alpine:3.24

# doxygen and graphviz for the API documentation, python for measuring how
# much of it there is, and ssh-keygen for signing the nightly builds.
RUN apk add doxygen graphviz font-noto python3 py3-pip openssh-keygen bash curl xz

# pybadges is not packaged by alpine. pip refuses to install into the python
# of the system (PEP 668) unless told to, which is fine in an image which only
# runs the scripts for deploying.
RUN pip3 install --break-system-packages pybadges
