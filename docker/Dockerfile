FROM ubuntu:focal

RUN addgroup --gid 3001 lime
RUN adduser --home /home/lime --disabled-login --uid 2001 --ingroup lime lime

RUN apt -qq update
RUN DEBIAN_FRONTEND="noninteractive" apt -qq install -y gcc-9 g++-9 git cmake libgsl-dev libboost-log-dev libboost-test-dev libboost-math-dev libtbb-dev

RUN git clone https://github.com/dmry/lime.git /home/lime/lime
RUN install -d -o lime -g lime /home/lime/lime/build

RUN cd /home/lime/lime/build &&\
    CXX=/usr/bin/g++-9 cmake /home/lime/lime/ &&\
    make lime

RUN install -d -o lime -g lime /home/lime/output/

ENV PATH="/home/lime/lime/build/:${PATH}"