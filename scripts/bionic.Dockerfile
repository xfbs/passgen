FROM ubuntu:bionic

# install essential stuff
RUN apt update && \
  apt install -y apt-transport-https ca-certificates gnupg software-properties-common wget && \
  wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add - && \
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | apt-key add - && \
  apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' && \
  apt-add-repository 'deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-10 main' && \
  apt clean

RUN apt update && \
  apt install -y sudo gcc ninja-build make cmake git libjansson-dev ruby git clang-10 lcov pkg-config && \
  apt clean

# create nonprivileged user to build stuff under with access to sudo
RUN adduser --disabled-password --ingroup sudo --shell /bin/bash --gecos '' user
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER user
