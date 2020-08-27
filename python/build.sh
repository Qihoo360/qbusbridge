#/usr/bin
set -e
cd `dirname $0`

if [[ ! $PULSAR_DEP ]]; then
    echo "PULSAR_DEP must be defined!"
    exit 1
fi

mkdir -p src
echo 'swig -python -c++ -threads -o src/qbus_wrap.cxx qbus.i'
swig -python -c++ -threads -o src/qbus_wrap.cxx qbus.i

PYTHON_LIB_DIR=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var('LIBDIR'))")
PYTHON_VERSION=$(python -c "import platform; print(platform.python_version()[:3])")
PYTHON_LIBRARY="$PYTHON_LIB_DIR/libpython$PYTHON_VERSION.so"
PYTHON_INCLUDE_DIR=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())")

echo "PYTHON_INCLUDE_DIR: $PYTHON_INCLUDE_DIR"
echo "PYTHON_LIBRARY: $PYTHON_LIBRARY"

SOURCES=$PWD/src/qbus_wrap.cxx
mkdir -p _builds
cd _builds
cmake ../../cxx/src -DLIBNAME=QBus_py -DSOURCES=$SOURCES \
    -DEXTRA_INCLUDE_DIRS=$PYTHON_INCLUDE_DIR -DEXTRA_LIBS=$PYTHON_LIBRARY \
    -DCMAKE_PREFIX_PATH=$PULSAR_DEP
make
cd -
cp ./src/qbus.py examples
cp ./_builds/libQBus_py.so examples/_qbus.so
