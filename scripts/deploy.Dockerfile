FROM alpine:latest

# create nonprivileged user
RUN adduser -s /bin/ash -D user

# install things needed for deployment
RUN apk add doxygen graphviz

# switch to non-privileged user
USER user