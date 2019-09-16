#!/bin/bash
echo "1. 先将安装包解压缩后的gopath目录加入GOPATH;"
echo "2. 要运行编译后的文件，请手动将libQBus_go.so路径加入LD_LIBRARY_PATH，比如"
echo '  LD_LIBRARY_PATH=$PWD/../gopath/src/qbus ./consumer'
export GOPATH="$PWD/../gopath"

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
