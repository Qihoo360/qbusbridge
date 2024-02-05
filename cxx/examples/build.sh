#/usr/bin
set -o errexit
cd `dirname $0`

if [ $# != 1 ] ; then
  echo "USAGE: $0 [debug | release]"
  exit 1
fi

case "$1" in
  debug)
    LIB_DIR=`pwd`/../lib/debug
    ;;
  release)
    LIB_DIR=`pwd`/../lib/release
    ;;
  *)
    echo "USAGE: $0 [debug | release]"
    exit 2
esac

make clean
CXX=/usr/bin/g++
make CXX=$CXX LIB_DIR=$LIB_DIR
