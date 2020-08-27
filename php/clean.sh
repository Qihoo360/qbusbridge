#!/bin/bash
set -e
cd `dirname "$0"`

rm -rf ./_builds
rm -rf ./src
rm -f ./examples/qbus.php
rm -f ./examples/*.so
