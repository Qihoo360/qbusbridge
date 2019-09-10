## 编译
golang SDK依赖`librdkafka`，因此需要先进入[cxx](../cxx)目录执行`./build_librdkafka.sh`编译`librdkakfa`库。

之后在当前目录执行`./build.sh`即可，会生成中间文件和创建`gopath/src/qbus`目录，里面有`qbus.go`和`libQBus_go.so`文件。

执行`./clean.sh`清理生成的文件。

## 示例程序
在[examples](./examples)目录中，进入该目录，使用`build.sh`编译，使用`clean.sh`清理。

编译时需要将创建的`gopath`目录加入环境变量`GOPATH`中，运行时需要将`libQBus_go.so`加入环境变量`LD_LIBRARY_PATH`，确保可执行文件能找到该动态库。
