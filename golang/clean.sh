#!/bin/bash
cd `dirname $0`

rm qbus.go *_wrap.*
rm -rf gopath
rm -rf build_go
rm *.so
rm -rf ./src
rm -rf ./lib

cd examples && ./clean.sh
