FROM alpine:3.8 as build
RUN apk add --no-cache musl-dev build-base linux-headers figlet strace

COPY . /build
RUN make -C /build sbin/detect

RUN apk add dos2unix
RUN dos2unix /build/result-good.sh
RUN dos2unix /build/result-bad.sh
RUN chmod a+x /build/result-good.sh /build/result-bad.sh
CMD [ "/build/sbin/detect" ]
