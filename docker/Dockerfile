# Alpine would be preferred, but it has poor support for intel-tbb and gsl
FROM ubuntu:focal

# Prepare unpriviledged user
RUN addgroup --gid 3001 lime
RUN adduser --home /home/lime --disabled-login --uid 2001 --ingroup lime lime

# Install dependencies
RUN apt -qq update
RUN DEBIAN_FRONTEND="noninteractive" apt -qq install -y gcc-9 g++-9 git cmake libgsl-dev libboost-log-dev libboost-test-dev libboost-math-dev libtbb-dev

# Get lime from GitHub repository
RUN git clone https://github.com/dmry/lime.git /home/lime/lime
RUN install -d -o lime -g lime /home/lime/lime/build

# Compile Lime
RUN cd /home/lime/lime/build &&\
    CXX=/usr/bin/g++-9 cmake /home/lime/lime/ &&\
    make lime

# Prepare default output directory from the README.md
RUN install -d -o lime -g lime /home/lime/output/

# Set correct permissions for unpriviledged user
RUN chmod +x /home/lime/lime/build/lime &&\
    chown lime:lime /home/lime/lime/build/lime

# Drop root priviledges
USER lime

# Allow users to simply run "lime command --flags" (e.g. not using an absolute path)
ENV PATH="/home/lime/lime/build/:${PATH}"