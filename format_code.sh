#!/bin/bash
# Using clang-format and gofmt to format c/c++/golang source code.

formatByClangFormat() {
    for file in $(find $1 -regextype "posix-egrep" -regex ".*[A-Za-z_]\.(h|c|cc)")
    do
        COMMAND="clang-format -i $file"
        eval $COMMAND
    done
    echo "clang-format for directory: $1"
}

if command -v clang-format >/dev/null; then
    DIRS=( ./cxx/src ./cxx/examples )
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
    echo "[WARN] gofmt is required to format golang code"
fi
