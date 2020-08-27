#!/bin/bash
set -o errexit
cd `dirname $0`

VERSION=$(cat VERSION)
echo "const char* version = \"$VERSION\";" > ./cxx/src/kafka/util/version.cc

# init and download submodules
git submodule init
git submodule update

INSTALL_DIR=$PWD/cxx/thirdparts/local

# build librdkafka (support old gcc of CentOS 6)
cd ./cxx/thirdparts/librdkafka
./configure --prefix=$INSTALL_DIR --disable-ssl --disable-sasl
make
make install

cd -

# build log4cplus static library and support linked in a shared library
cd ./cxx/thirdparts/log4cplus
./scripts/fix-timestamps.sh
CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" ./configure --prefix=$INSTALL_DIR --enable-static --with-pic
make
make install

cd -
