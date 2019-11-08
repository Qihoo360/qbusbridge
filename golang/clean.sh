#!/bin/bash
cd `dirname $0`

rm qbus.go *_wrap.*
rm -rf gopath
rm -rf build_go
rm *.so

cd examples && ./clean.sh
