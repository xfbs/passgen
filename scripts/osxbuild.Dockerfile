FROM liushuyu/osxcross

RUN apt update && apt install ruby libjansson-dev && apt clean
