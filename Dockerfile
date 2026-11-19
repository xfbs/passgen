FROM alpine:3.24 AS builder

RUN apk add gcc musl-dev cmake ninja git ruby bash grep
ADD . /code
RUN mkdir /build
RUN cmake -S /code -B /build -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
RUN ninja -C /build
RUN ctest --test-dir /build --output-on-failure
RUN ninja -C /build install

FROM alpine:3.24

COPY --from=builder /usr/bin/passgen /usr/bin/passgen
COPY --from=builder /usr/lib/libpassgen.so* /usr/lib/
COPY --from=builder /usr/include/passgen /usr/include/passgen
COPY --from=builder /usr/share/passgen /usr/share/passgen

ENTRYPOINT ["/usr/bin/passgen"]
