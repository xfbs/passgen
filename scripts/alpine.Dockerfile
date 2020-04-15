FROM alpine:latest

# setup normal user and sudo
RUN addgroup sudo
RUN adduser -s /bin/ash -G sudo -D user

# install dependencies
RUN apk add build-base cmake ninja make sudo libexecinfo-dev jansson-dev git ruby llvm9 clang

# add user to sudoers file
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER user
