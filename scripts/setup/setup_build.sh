#!/bin/bash

BUILD_TYPE="Release"
BUILD_DIR="build/"

while [[ $# -gt 0 ]]; do
    case $1 in
        -bt)
            BUILD_TYPE="$2"
            shift 2
            ;;
    esac
done

if [ -d "${BUILD_DIR}" ]; then
    echo "${BUILD_DIR} already exists, cleaning before rebuild"
    rm -rf ${BUILD_DIR}
fi

mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make
