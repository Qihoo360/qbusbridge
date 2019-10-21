#!/bin/bash
echo '1. 如果不使用go.mod，请先将安装包解压缩后的gopath目录加入GOPATH，比如：'
echo '  export GOPATH=$PWD/../gopath'
echo '2. 要运行编译后的文件，请手动将libQbus.so的路径加入LD_LIBRARY_PATH，比如：'
echo '  export LD_LIBRARY_PATH=$PWD/../gopath/src/qbus:$LD_LIBRARY_PATH'

TARGETS=( \
	consumer \
	producer \
	consumer_commit_in_goroutine \
    consume_pause_resume
)
for TARGET in "${TARGETS[@]}"; do
	COMMAND="go build $TARGET.go"
	echo $COMMAND
	eval $COMMAND
done
