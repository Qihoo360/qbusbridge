#!/bin/bash

PYTHON_LINK="-lpython2.7"
PYTHON_INCLUDE="-I/usr/local/python2.7/include/python2.7"

print_usage()
{
    echo "Usage:
    -s </usr/local/python2.7/include/python2.7>   header file src
    -v <python2.7>			   	  python version 
    -h                             		  help"
 
    exit 1
}
 
while getopts "s:v:h" arg # 选项后面的冒号表示该选项需要参数
do
    case $arg in
        s)
	    if [ "$OPTARG" ];then
	    	PYTHON_INCLUDE="-I$OPTARG"
	    fi
            ;;
        v)
	    if [ "$OPTARG" ];then
	    	PYTHON_LINK="-l$OPTARG"
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

echo $PYTHON_INCLUDE
echo $PYTHON_LINK

make clean

CENT_VER=""
ver=`rpm -q centos-release|cut -d- -f3`
if [ "$ver" == "7" ];then
	CENT_VER="-std=c++11"
fi


make -f Makefile SWIG_LIB_DIR=/usr/local/share/swig/3.0.12/ PYTHON_INCLUDE=$PYTHON_INCLUDE CENT_VER=$CENT_VER #PYTHON_LINK=$PYTHON_LINK 
