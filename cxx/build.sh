#/usr/bin
set -o errexit
cd `dirname $0`

if [ $# != 1 ] ; then
  echo "USAGE: $0 [debug | release]"
  exit 1
fi

# TODO: set your own c/c++ compiler
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

mkdir -p build

cd build
rm -rf ./*

case "$1" in
  debug)
    cmake -DCMAKE_BUILD_TYPE=Debug ../src
    make
    cd ../
    ;;
  release)
    cmake -DCMAKE_BUILD_TYPE=Release ../src
    make
    cd ../
    ;;
  *)
    echo "USAGE: $0 [debug | release]"
    exit 2
esac
