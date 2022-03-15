FROM ubuntu:20.04

SHELL ["/bin/bash", "-c"]
RUN apt update -y

RUN apt install g++ git -y

WORKDIR /judgedir
COPY ./src/judge.cpp .
RUN g++ -O2 -o judge judge.cpp
RUN git clone https://github.com/atcoder/ac-library

COPY ./compile.sh .
RUN chmod 777 ./compile.sh

ENTRYPOINT ["/bin/sh","-c","while :; do sleep 10;done"]