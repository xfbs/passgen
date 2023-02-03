FROM alpine AS builder

RUN apk add gcc musl-dev cmake ninja git ruby
ADD . /code
RUN mkdir /build
RUN cmake -S /code -B /build -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
RUN ninja -C /build
RUN ninja -C /build test
RUN ninja -C /build install

FROM alpine

COPY --from=builder /usr/bin/passgen /usr/bin/passgen
COPY --from=builder /usr/lib/libpassgen.so /usr/lib/
COPY --from=builder /usr/include/passgen /usr/include/passgen
COPY --from=builder /code/data/passgen.conf /etc/passgen.conf
COPY --from=builder /usr/share/passgen /usr/share/passgen

ENTRYPOINT ["/usr/bin/passgen"]
