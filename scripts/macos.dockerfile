FROM liushuyu/osxcross

# install ruby -- needed for build process
RUN apt update && apt upgrade -y && apt install -y ruby && apt clean
