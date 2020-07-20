#!/bin/bash
cd `dirname $0`

rm -rf gopath
rm -rf build_go
rm *.so
rm -rf ./src
rm -rf ./lib

cd examples && ./clean.sh
