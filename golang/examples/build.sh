#/usr/bin
echo "1. 先将安装包解压缩后的gopath目录加入GOPATH; 2. 修改 QBUS_GO_SO_PATH为你解压缩后目录/gopath/src/qbus"
QBUS_GO_SO_PATH="`pwd`/../gopath/src/qbus"
export GOPATH="`pwd`/../gopath"
export CGO_LDFLAGS="-L$QBUS_GO_SO_PATH -lQBus_go" 

go build consumer.go
go build producer.go
go build producer_goroutine.go

export LD_LIBRARY_PATH=$QBUS_GO_SO_PATH
