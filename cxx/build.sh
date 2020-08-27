#!/bin/bash
set -o errexit
cd `dirname $0`

THIRD_PARTY_DIR=./thirdparts/local
THIRD_LIB_DIR=$THIRD_PARTY_DIR/lib

BUILD_DIR=_builds

build_lib() {
    CMAKE_BUILD_TYPE=$1
    mkdir -p $BUILD_DIR/$1
    cd $BUILD_DIR/$1
    cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ../../src
    make
    make install
    cd -
}

build_lib Debug
build_lib Release
