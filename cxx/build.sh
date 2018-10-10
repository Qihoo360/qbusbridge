#/usr/bin
if [ $# != 1 ] ; then
  echo "USAGE: $0 [debug | release]"
  exit 1
fi

./build_librdkafka.sh

rm -rf build
mkdir build

cd build

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
