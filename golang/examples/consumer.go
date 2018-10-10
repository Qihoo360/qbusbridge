package main

import (
	"fmt"
	"os"
	"os/signal"
	"qbus"
	"time"
)

//var qbus_consumer qbus.QbusConsumer = nil
var qbus_consumer = qbus.NewQbusConsumer()

type GoCallback struct {
}

func (p *GoCallback) DeliveryMsg(topic string, msg string, msg_len int64) {
	//fmt.Printf("Topic:%s | msg:%s\n", topic, string(msg[0:msg_len]))
	//fmt.Printf("Topic:%s | msg:%d\n", topic, msg_len)
	fmt.Printf("Topic:%s | msg:%s\n", topic, msg)
}

//纯手工提交offset, 需要在consumer.config中添加user.manual.commit.offset=true
func (p *GoCallback) DeliveryMsgForCommitOffset(msg_info qbus.QbusMsgContentInfo) {
	fmt.Printf("User commit offset | Topic:%s | msg:%s\n",
		msg_info.GetTopic(),
		msg_info.GetMsg())

	qbus_consumer.CommitOffset(msg_info)
}

func main() {
	if len(os.Args) < 4 {
		fmt.Printf("Usage: ./consumer topic_name group_name cluster_name\n")
		return
	}

	fmt.Printf("topic: %s | group: %s\n", os.Args[1], os.Args[2])

	running := true

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	go func() {
		for sig := range c {
			fmt.Printf("received ctrl+c(%v)\n", sig)
			//os.Exit(0)
			running = false
		}
	}()

	callback := qbus.NewDirectorQbusConsumerCallback(&GoCallback{})
	//qbus_consumer := qbus.NewQbusConsumer()
	if !qbus_consumer.Init(os.Args[3],
		"consumer.log",
		"./consumer.config",
		callback) {
		fmt.Println("Failed to Init")
		os.Exit(0)
	}

	if !qbus_consumer.SubscribeOne(os.Args[2], os.Args[1]) {
		fmt.Println("Failed to SubscribeOne")
		os.Exit(0)
	}

	qbus_consumer.Start()

	for running {
		time.Sleep(1 * time.Second)
	}

	qbus_consumer.Stop()

	qbus.DeleteQbusConsumer(qbus_consumer)
	qbus.DeleteDirectorQbusConsumerCallback(callback)
}
