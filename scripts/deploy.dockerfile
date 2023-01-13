FROM alpine:latest

# install things needed for deployment
RUN apk add doxygen graphviz font-noto python3 py3-pip openssh-keygen bash curl
RUN pip3 install pybadges mkdocs mkdocs-material
