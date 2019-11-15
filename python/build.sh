#!/bin/bash
set -o errexit
cd `dirname $0`

swig -python -c++ -threads -o qbus_wrap.cxx qbus.i

mkdir -p build
rm -rf build/*
cd build

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

PYTHON_LIB_DIR=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var('LIBDIR'))")
PYTHON_VERSION=$(python -c "import platform; print(platform.python_version()[:3])")
PYTHON_LIBRARY="$PYTHON_LIB_DIR/libpython$PYTHON_VERSION.so"
PYTHON_INCLUDE_DIR=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())") 

cmake .. \
    -DPYTHON_INCLUDE_DIR=$PYTHON_INCLUDE_DIR \
    -DPYTHON_LIBRARY=$PYTHON_LIBRARY
make

cd -
mv libQBus_py.so _qbus.so
cp -v ./_qbus.so ./qbus.py examples
