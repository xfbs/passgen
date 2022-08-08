FROM alpine:latest

# install dependencies
RUN apk add build-base cmake ninja make sudo libexecinfo-dev jansson-dev git ruby llvm clang valgrind bash
