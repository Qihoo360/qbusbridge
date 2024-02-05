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
        wget https://boostorg.jfrog.io/artifactory/main/release/1.70.0/source/boost_1_70_0.tar.gz
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

build_pulsar() {
    PROTOC_PATH="$INSTALL_DIR/bin/protoc"
    PULSAR_CPP_DIR="$SOURCE_DIR/pulsar/pulsar-client-cpp"

    # Use our own CMakeLists.txt to compile libpulsar.a only
    cp ./pulsar-client-cpp/CMakeLists.txt "$PULSAR_CPP_DIR"
    cp ./pulsar-client-cpp/lib/CMakeLists.txt "$PULSAR_CPP_DIR/lib"

    pushd $PULSAR_CPP_DIR
    mkdir -p _builds
    pushd _builds
    CXX=g++ cmake .. \
        -DPROTOC_PATH="$PROTOC_PATH" \
        -DCMAKE_CXX_FLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" \
        -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
    # Here we use multiple threads to compile because it take long to compile with a single thread
    make -j4
    make install
    pushd $PULSAR_CPP_DIR
    git checkout -- .
    popd
    popd
    popd
}

if [[ ! -f $INSTALL_DIR/lib/librdkafka.a ]]; then
    build_rdkafka
fi
if [[ ! -f $INSTALL_DIR/lib/liblog4cplus.a ]]; then
    build_log4cplus
fi
if [[ ! -f $INSTALL_DIR/lib/libboost_regex.a ]] || [[ ! -f $INSTALL_DIR/lib/libboost_system.a ]]; then
    build_boost_1_70
fi
if [[ ! -f $INSTALL_DIR/lib/libprotobuf-lite.a ]]; then
    build_protobuf_2_6
fi
if [[ ! -f $INSTALL_DIR/lib/libpulsar.a ]]; then
    build_pulsar
fi
