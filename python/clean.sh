#!/bin/bash
set -e
cd `dirname "$0"`

rm -rf ./_builds
rm -rf ./src
cd examples
rm -rf ./_qbus.so
rm -f ./qbus.py*
cd -
