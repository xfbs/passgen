FROM alpine:3.18

# install things needed for deployment
RUN apk add doxygen graphviz font-noto python3 py3-pip openssh-keygen bash curl xz
RUN pip3 install pybadges mkdocs mkdocs-material mkdocs-kroki-plugin
