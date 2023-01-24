FROM alpine:latest

# install dependencies
RUN apk add build-base cmake ninja make sudo git ruby llvm clang valgrind bash
