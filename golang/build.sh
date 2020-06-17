#/usr/bin
set -o errexit
cd `dirname $0`

mkdir -p src
COMMAND="swig -go -c++ -cgo -intgosize 64 -o src/qbus_wrap.cxx qbus.i"
echo "$ $COMMAND"
eval $COMMAND

mkdir -p build_go

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

SOURCES=$PWD/src/qbus_wrap.cxx
cd build_go
cmake ../../cxx/src -DLIBNAME=QBus_go -DSOURCES=$SOURCES
make
cd ../

mv ./lib/release/libQBus_go.so .

rm -rf ./examples/src
mkdir -p ./gopath/src/qbus
cp libQBus_go.so ./gopath/src/qbus
cp src/qbus.go ./gopath/src/qbus

while true; do
    echo -n "Use go module for examples? [Y/n]: "
    read ANSWER

    if [[ $ANSWER =~ ^[yYnN] ]]; then
        if [[ ${ANSWER:0:1} = [yY] ]]; then
            export GO111MODULE=on
            mkdir -p examples/qbus
            cp libQBus_go.so ./examples/qbus
            cp src/qbus.go ./examples/qbus

            pushd examples && go mod init examples
            echo "
require qbus v1.0.0

replace qbus => ./qbus" >> go.mod
            pushd qbus && go mod init qbus
            popd
            popd
        fi
        break
    else
        echo "[ERROR] invalid input: $ANSWER!"
    fi
done

rm libQBus_go.so
