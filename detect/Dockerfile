FROM alpine:3.8 as build
RUN apk add --no-cache musl-dev build-base linux-headers figlet

COPY . /build
RUN make -C /build sbin/detect

RUN apk add dos2unix
RUN dos2unix /build/go.sh
CMD [ "/bin/sh", "-e", "/build/go.sh" ]
