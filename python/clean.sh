#!/bin/bash
cd `dirname $0`

rm -r ./build
rm -f qbus.py *_wrap.* *.pyc
rm -f *.so

cd examples && rm -f *.so qbus.py *.pyc
