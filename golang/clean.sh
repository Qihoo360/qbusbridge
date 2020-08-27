#!/bin/bash
set -e
cd `dirname "$0"`

rm -rf ./_builds
rm -rf ./gopath
rm -rf ./src
rm -f ./examples/go.mod
rm -rf ./examples/qbus
cd examples
rm -f consumer producer consume_pause_resume
cd -
