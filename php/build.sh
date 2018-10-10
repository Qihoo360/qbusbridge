#!/bin/bash

PHP_VER="php"
PHP_SRC="/usr/local/php/include/php"

print_usage()
{
    echo "Usage:
    -s </usr/local/php/include/php>            header file src
    -v <php>			   php version php/php7
    -h                             help"
 
    exit 1
}
 
while getopts "s:v:h" arg # 选项后面的冒号表示该选项需要参数
do
    case $arg in
        s)
	    if [ "$OPTARG" ];then
	    	PHP_SRC="$OPTARG"
	    fi
            ;;
        v)
	    if [ "$OPTARG" ];then
	    	PHP_VER="$OPTARG"
	    fi
            ;;
        h)
            print_usage
            ;;
        ?)
            print_usage
            ;;
    esac
done

echo $PHP_VER
echo $PHP_SRC

make clean

CENT_VER=""
ver=`rpm -q centos-release|cut -d- -f3`
if [ "$ver" == "7" ];then
	CENT_VER="-std=c++11"
fi

make -f Makefile SWIG_LIB_DIR=/usr/local/share/swig/3.0.12/ PHP_SRC=$PHP_SRC PHP_VER=$PHP_VER CENT_VER=$CENT_VER
