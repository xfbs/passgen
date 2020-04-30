FROM alpine:latest

# create nonprivileged user
RUN adduser -s /bin/ash -D user

# install things needed for deployment
RUN apk add doxygen graphviz font-noto

# switch to non-privileged user
USER user
