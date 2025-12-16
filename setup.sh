#!/bin/bash

# setup docker container
docker build -t qutefuzz-env .
docker run -it --rm -v "$PWD":/qutefuzz --user $(id -u):$(id -g) qutefuzz-env

# compile project
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make