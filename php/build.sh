#!/bin/bash
set -o errexit
cd `dirname $0`

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

swig -$PHP_VERSION -cppext cxx -c++ -o qbus_wrap.cxx qbus.i

PHP_INCLUDES=`php-config --includes`
echo "PHP includes: $PHP_INCLUDES"

mkdir -p build
rm -rf build/*
cd build

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

cmake .. -DPHP_INCLUDE_DIRS="$PHP_INCLUDES"
make

cd -
mv libQBus_php.so qbus.so
