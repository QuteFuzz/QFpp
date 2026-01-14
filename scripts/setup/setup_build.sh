#!/bin/bash

BUILD_TYPE="Release"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -bt)
            BUILD_TYPE="$2"
            shift 2
            ;;
    esac
done

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make