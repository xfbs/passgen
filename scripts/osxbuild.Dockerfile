FROM liushuyu/osxcross

# install ruby -- needed for build process
RUN apt update && apt upgrade && apt install -y ruby && apt clean

# install up-to-date cmake
RUN mkdir /opt/cmake && wget -qO- https://github.com/Kitware/CMake/releases/download/v3.17.1/cmake-3.17.1-Linux-x86_64.tar.gz | tar --strip-components=1 -C /opt/cmake/ -xzvf -
