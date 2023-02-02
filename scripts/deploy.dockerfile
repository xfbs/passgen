FROM alpine:latest as builder

RUN apk add curl gcc libc-dev
RUN curl "https://pikchr.org/home/raw/02f23c6c39e1af5d95734124b8c3c59fa3bf62bea1dab93a191ba54ab4386a45?at=pikchr.c" -o /tmp/pikchr.c
RUN gcc -DPIKCHR_SHELL -o /usr/local/bin/pikchr /tmp/pikchr.c -lm

FROM alpine:latest

# install things needed for deployment
COPY --from=builder /usr/local/bin/pikchr /usr/local/bin/pikchr
RUN apk add doxygen graphviz font-noto python3 py3-pip openssh-keygen bash curl xz
RUN pip3 install pybadges mkdocs mkdocs-material
