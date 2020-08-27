#/usr/bin
set -e
cd `dirname $0`

if [[ ! $PULSAR_DEP ]]; then
    echo "PULSAR_DEP must be defined!"
    exit 1
fi

PHP_VERSION=`php-config --version`
echo "PHP version: $PHP_VERSION"

case ${PHP_VERSION:0:1} in
    7)
        PHP_VERSION=php7
        ;;
    5)
        PHP_VERSION=php
        ;;
    *)
    echo "Unknown php version (not 5.x.y or 7.x.y): $PHP_VERSION"
    exit 1
esac

mkdir -p src
echo "swig -$PHP_VERSION -cppext cxx -c++ -o src/qbus_wrap.cxx qbus.i"
swig -$PHP_VERSION -cppext cxx -c++ -o src/qbus_wrap.cxx qbus.i

PHP_INCLUDES=$(php-config --includes | sed 's/ /;/g' | sed 's/-I//g')
echo "PHP_INCLUDES: $PHP_INCLUDES"

SOURCES=$PWD/src/qbus_wrap.cxx
mkdir -p _builds
cd _builds
cmake ../../cxx/src -DLIBNAME=QBus_php -DSOURCES=$SOURCES -DEXTRA_INCLUDE_DIRS="$PHP_INCLUDES" \
    -DCMAKE_CXX_FLAGS="-DNOT_USE_CONSUMER_CALLBACK" \
    -DCMAKE_PREFIX_PATH=$PULSAR_DEP
make
cd -
cp ./_builds/libQBus_php.so examples/qbus.so
cp ./src/qbus.php examples
