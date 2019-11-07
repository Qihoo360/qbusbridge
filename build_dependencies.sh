#!/bin/bash
set -o errexit
cd `dirname $0`

# download submodules
git submodule update

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

INSTALL_DIR=$PWD/cxx/thirdparts/local

# build librdkafka (support old gcc of CentOS 6)
cd ./cxx/thirdparts/librdkafka
./configure --prefix=$INSTALL_DIR --disable-ssl --disable-sasl
make
make install

#sed -i "s/^#define HAVE_ATOMICS_32.*/ /g" config.h
#sed -i "s/^#define HAVE_ATOMICS_32_ATOMIC.*/ /g" config.h
#sed -i "s/^#define HAVE_ATOMICS_64.*/ /g" config.h
#sed -i "s/^#define HAVE_ATOMICS_64_ATOMIC.*/ /g" config.h

cd -

# build log4cplus static library and support linked in a shared library
cd ./cxx/thirdparts/log4cplus
./scripts/fix-timestamps.sh
./configure --prefix=$INSTALL_DIR --enable-static --with-pic
make
make install
