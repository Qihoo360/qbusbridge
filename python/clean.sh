#!/bin/bash
cd `dirname $0`

rm -rf ./build
rm -f qbus.py *_wrap.* *.pyc
rm -f *.so
rm -rf ./src
rm -rf ./lib

cd examples && rm -f *.so qbus.py *.pyc
