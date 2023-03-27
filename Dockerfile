FROM ubuntu:20.04

SHELL ["/bin/bash", "-c"]
RUN apt update -y

RUN apt install g++ git -y

WORKDIR /judgedir
RUN git clone https://github.com/atcoder/ac-library

COPY ./src/judge.cpp ./ac-library/atcoder/
COPY ./src/game.hpp ./ac-library/atcoder/
COPY ./src/compile.sh .
RUN chmod 777 compile.sh

ENTRYPOINT ["/bin/sh","-c","while :; do sleep 10;done"]