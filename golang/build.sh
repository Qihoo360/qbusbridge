#/usr/bin
make clean
make

rm -rf build_go
mkdir build_go

cd build_go
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
cd ../

rm -rf ./examples/src
mkdir -p ./gopath/src/qbus
cp libQBus_go.so ./gopath/src/qbus
cp qbus.go ./gopath/src/qbus
