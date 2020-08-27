#!/bin/bash
cd `dirname $0`
if [[ `which clang-format` ]]; then
    cd ..
    ./format_code.sh
    cd -
else
    echo "[WARN] Your system doesn't have clang-format, please ensure your code style is right"
fi

set -o errexit

mkdir -p src
echo 'swig -go -c++ -cgo -intgosize 64 -o src/qbus_wrap.cxx qbus.i'
swig -go -c++ -cgo -intgosize 64 -o src/qbus_wrap.cxx qbus.i

SOURCES=$PWD/src/qbus_wrap.cxx
mkdir -p _builds
cd _builds
cmake ../../cxx/src -DLIBNAME=QBus_go -DSOURCES=$SOURCES -DCMAKE_PREFIX_PATH=$PULSAR_DEP
make
cd -

if [[ $USE_GO_MOD ]]; then
    mkdir -p examples/qbus
    cp ./_builds/libQBus_go.so examples/qbus
    cp ./src/qbus.go examples/qbus
    pushd examples/qbus
    go mod init qbus
    pushd ..
    go mod init examples
    echo "
require qbus v1.1.0
replace qbus => ./qbus" >> go.mod
    popd
    popd
else
    mkdir -p gopath/src/qbus
    cp ./_builds/libQBus_go.so gopath/src/qbus
    cp ./src/qbus.go gopath/src/qbus
fi
