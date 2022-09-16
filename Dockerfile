FROM alpine:3.16.2

RUN apk add g++ make binutils cmake openssl-dev boost-dev=1.78.0-r1 curl-dev zlib-dev yt-dlp clang sqlite-dev ffmpeg 

ENV CXXFLAGS="-I/usr/include -I/usr/include/boost"
WORKDIR /app
COPY src src
COPY third_party third_party
COPY Makefile Makefile

RUN make clean
RUN make -j16
CMD ["./bin/main.out"]
