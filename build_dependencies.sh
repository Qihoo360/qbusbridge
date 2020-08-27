#!/bin/bash
set -o errexit
cd `dirname $0`

VERSION=$(cat VERSION)
echo "const char* version = \"$VERSION\";" > ./cxx/src/kafka/util/version.cc

# init and download submodules
git submodule init
git submodule update

SOURCE_DIR="$PWD/cxx/thirdparts"
INSTALL_DIR="$SOURCE_DIR/local"

build_rdkafka() {
    cd "$SOURCE_DIR/librdkafka"
    ./configure --prefix="$INSTALL_DIR"
    make
    make install
    cd -
}

build_log4cplus() {
    cd "$SOURCE_DIR/log4cplus"
    ./scripts/fix-timestamps.sh
    CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" ./configure --prefix="$INSTALL_DIR" --enable-static --with-pic
    make
    make install
    cd -
}

build_boost_1_70() {
    pushd "$SOURCE_DIR"
    if [[ ! -f boost_1_70_0.tar.gz ]]; then
        wget https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz
    fi
    tar zxf boost_1_70_0.tar.gz
    pushd boost_1_70_0
    ./bootstrap.sh --prefix="$INSTALL_DIR" --with-libraries=regex,system
    ./b2 cxxflags="-D_GLIBCXX_USE_CXX11_ABI=0 -fPIC" install
    popd
    popd
}

build_protobuf_2_6() {
    pushd "$SOURCE_DIR"
    if [[ ! -f protobuf-2.6.1.tar.gz ]]; then
        wget https://github.com/protocolbuffers/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.gz
    fi
    tar zxf protobuf-2.6.1.tar.gz
    pushd protobuf-2.6.1
    CXX=g++ CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 -fPIC" ./configure --prefix="$INSTALL_DIR"
    make
    make install
    popd
    popd
}

build_rdkafka
build_log4cplus
build_boost_1_70
build_protobuf_2_6
