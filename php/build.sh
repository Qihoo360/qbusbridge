#!/bin/bash
set -o errexit
cd `dirname $0`

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

PHP_VERSION=`php-config --version`
echo "PHP version: $PHP_VERSION"

case ${PHP_VERSION:0:1} in
    7)
        PHP_VERSION=php7
        ;;
    5)
        PHP_VERSION=php
        ;;
    *)
    echo "Unknown php version (not 5.x.y or 7.x.y): $PHP_VERSION"
    exit 1
esac

mkdir -p src
swig -$PHP_VERSION -cppext cxx -c++ -o src/qbus_wrap.cxx qbus.i

PHP_INCLUDES=$(php-config --includes | sed 's/ /;/g' | sed 's/-I//g')
echo "PHP_INCLUDES: $PHP_INCLUDES"

SOURCES=$PWD/src/qbus_wrap.cxx
mkdir -p build
rm -rf build/*
cd build
cmake ../../cxx/src \
    -DSOURCES=$SOURCES \
    -DEXTRA_INCLUDE_DIRS="$PHP_INCLUDES" \
    -DCMAKE_CXX_FLAGS="-DNOT_USE_CONSUMER_CALLBACK"
make
cd -
mv lib/release/libQBus.so qbus.so
mv qbus.so examples/
cp ./src/qbus.php .
