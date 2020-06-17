#!/bin/bash
set -o errexit
cd `dirname $0`

mkdir -p src
swig -python -c++ -threads -o src/qbus_wrap.cxx qbus.i

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

PYTHON_LIB_DIR=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var('LIBDIR'))")
PYTHON_VERSION=$(python -c "import platform; print(platform.python_version()[:3])")
PYTHON_LIBRARY="$PYTHON_LIB_DIR/libpython$PYTHON_VERSION.so"
PYTHON_INCLUDE_DIR=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())") 

echo "PYTHON_INCLUDE_DIR: $PYTHON_INCLUDE_DIR"
echo "PYTHON_LIBRARY: $PYTHON_LIBRARY"

SOURCES=$PWD/src/qbus_wrap.cxx
mkdir -p build
cd build
cmake ../../cxx/src \
    -DSOURCES=$SOURCES \
    -DEXTRA_INCLUDE_DIRS=$PYTHON_INCLUDE_DIR \
    -DEXTRA_LIBS=$PYTHON_LIBRARY
make
cd -

mv ./lib/release/libQBus.so _qbus.so
rm -rf lib
cp -v ./_qbus.so ./src/qbus.py examples/
