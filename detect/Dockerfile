FROM alpine:3.8 as build
RUN apk add --no-cache musl-dev build-base linux-headers figlet

COPY . /build
RUN make -C /build sbin/detect

CMD [ "/build/go.sh" ]
