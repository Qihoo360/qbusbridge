package main

import (
	"fmt"
	"os"
	"os/signal"
	"qbus"
)

// GoCallback 消费者的回调，可携带用户自定义数据，比如这里的 Consumer
type GoCallback struct {
	// 仅用于 DeliveryMsgForCommitOffset 才需要这个字段
	Consumer qbus.QbusConsumer
}

// DeliveryMsg 默认的回调函数
func (cb *GoCallback) DeliveryMsg(topic string, msg string, msgLen int64) {
	fmt.Printf("Topic: %s | msg: %s\n", topic, msg)
}

// DeliveryMsgForCommitOffset 用户自行管理并提交 offset 或确认消息的回调函数，需要修改默认配置
func (cb *GoCallback) DeliveryMsgForCommitOffset(info qbus.QbusMsgContentInfo) {
	fmt.Printf("User commit offset | Topic: %s | msg: %s\n", info.GetTopic(), info.GetMsg())
	cb.Consumer.CommitOffset(info)
}

func main() {
	if len(os.Args) < 5 {
		fmt.Printf("Usage: ./consumer config_path topic_name group_name cluster_name\n")
		return
	}

	configPath := os.Args[1]
	topicName := os.Args[2]
	group := os.Args[3]
	clusterName := os.Args[4]

	fmt.Printf("topic: %s | group: %s | cluster: %s\n", topicName, group, clusterName)

	sigs := make(chan os.Signal, 1)
	done := make(chan bool, 1)

	signal.Notify(sigs, os.Interrupt)

	go func() {
		sig := <-sigs
		fmt.Println()
		fmt.Println(sig)
		done <- true
	}()

	var goCallback GoCallback
	callback := qbus.NewDirectorQbusConsumerCallback(&goCallback)
	defer qbus.DeleteQbusConsumerCallback(callback)

	consumer := qbus.NewQbusConsumer()
	defer qbus.DeleteQbusConsumer(consumer)

	goCallback.Consumer = consumer

	if !consumer.Init(clusterName, "consumer.log", configPath, callback) {
		fmt.Println("Init failed")
		os.Exit(1)
	}

	if !consumer.SubscribeOne(group, topicName) {
		fmt.Println("SubscribeOne failed")
		os.Exit(2)
	}

	if !consumer.Start() {
		fmt.Println("Start failed")
		os.Exit(3)
	}

	<-done
	consumer.Stop()
	fmt.Println("done")
}
