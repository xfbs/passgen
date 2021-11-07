FROM debian:11

# create unprivileged user
RUN useradd -ms /bin/bash user

# install toolchain
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y git cmake gcc-riscv64-linux-gnu ruby python3 qemu-user-static binfmt-support ninja-build libc6-riscv64-cross && apt clean

# switch to unprivileged user
WORKDIR /home/user
USER user
