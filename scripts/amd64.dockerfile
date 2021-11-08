FROM debian:11

RUN apt update \
    && apt install -y \
        clang \
        cmake \
        gcc \
        git \
        git \
        lcov \
        libjansson-dev \
        make \
        ninja-build \
        pkg-config \
        ruby \
        sudo \
    && apt clean

# create nonprivileged user to build stuff under with access to sudo
RUN adduser --disabled-password --shell /bin/bash --gecos '' user

USER user
