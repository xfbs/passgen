FROM debian:11

# create unprivileged user
RUN useradd -ms /bin/bash user

# install toolchain
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y \
        cmake \
        gcc-mingw-w64 \
        git \
        ninja-build \
        python3 \
        ruby \
    && apt clean

# switch to unprivileged user
WORKDIR /home/user
USER user
