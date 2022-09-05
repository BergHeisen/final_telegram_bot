FROM alpine:3.16.2

RUN apk add g++ make binutils cmake openssl-dev boost-dev curl-dev zlib-dev yt-dlp

WORKDIR /app
COPY src src
COPY third_party third_party
COPY Makefile Makefile

RUN make
CMD ["./bin/main.out"]
