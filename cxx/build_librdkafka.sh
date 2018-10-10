#!/bin/sh
current_path=$(cd "$(dirname "$0")"; pwd)

cd ./thirdparts/librdkafka

make clean 

./configure  --disable-ssl --disable-sasl

sed -i "s/^#define HAVE_ATOMICS_32.*/ /g" $current_path/thirdparts/librdkafka/config.h
sed -i "s/^#define HAVE_ATOMICS_32_ATOMIC.*/ /g" $current_path/thirdparts/librdkafka/config.h
sed -i "s/^#define HAVE_ATOMICS_64.*/ /g" $current_path/thirdparts/librdkafka/config.h
sed -i "s/^#define HAVE_ATOMICS_64_ATOMIC.*/ /g" $current_path/thirdparts/librdkafka/config.h

make
