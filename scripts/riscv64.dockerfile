FROM debian:11

# create unprivileged user
RUN useradd -ms /bin/bash user

# install toolchain
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y \
        binfmt-support \
        cmake \
        gcc-riscv64-linux-gnu \
        git \
        libc6-riscv64-cross \
        ninja-build \
        python3 \
        qemu-user-static \
        ruby \
    && apt clean

# switch to unprivileged user
WORKDIR /home/user
USER user
