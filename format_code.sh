#!/bin/bash
# Using clang-format and gofmt to format c/c++/golang source code.

formatByClangFormat() {
	for file in $(find $1 -regextype "posix-egrep" -regex ".*[A-Za-z_]\.(h|c|cc)")
	do
		COMMAND="clang-format -i $file"
		echo $COMMAND
		eval $COMMAND
	done
}

if command -v clang-format >/dev/null; then
	DIRS=( ./c/src ./c/examples ./cxx/src ./cxx/examples ./cxx/util )
	for DIR in "${DIRS[@]}"
	do
		formatByClangFormat $DIR
	done
else
	echo "[ERROR] clang-format is required to format c/c++ code"
fi


formatByGoFmt() {
	for file in $(find $1 -name "*.go")
	do
		COMMAND="gofmt -w $file"
		echo $COMMAND
		eval $COMMAND
	done
}

if command -v gofmt >/dev/null; then
	formatByGoFmt ./golang/examples
else
	echo "[ERROR] gofmt is required to format golang code"
fi
