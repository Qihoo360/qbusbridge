package main

import (
	"fmt"
	"os"
	"os/signal"
	"qbus"
)

//var qbusConsumer qbus.QbusConsumer = nil
var qbusConsumer = qbus.NewQbusConsumer()
var msgChan = make(chan *qbus.QbusMsgContentInfo, 1000)

type GoCallback struct {
}

func (p *GoCallback) DeliveryMsg(topic string, msg string, msg_len int64) {
	//fmt.Printf("Topic:%s | msg:%s\n", topic, string(msg[0:msg_len]))
	//fmt.Printf("Topic:%s | msg:%d\n", topic, msg_len)
	fmt.Printf("Topic:%s | msg:%s\n", topic, msg)
}

//纯手工提交offset, 需要在consumer.config中添加user.manual.commit.offset=true
func (p *GoCallback) DeliveryMsgForCommitOffset(msgInfo qbus.QbusMsgContentInfo) {
	fmt.Printf("User commit offset | Topic:%s | msg: %v\n",
		msgInfo.GetTopic(),
		msgInfo.GetRd_message())

	msgInfoNew := qbus.NewQbusMsgContentInfo()
	msgInfoNew.SetTopic(msgInfo.GetTopic())
	msgInfoNew.SetMsg(msgInfo.GetMsg())
	msgInfoNew.SetMsg_len(msgInfo.GetMsg_len())
	msgInfoNew.SetRd_message(msgInfo.GetRd_message())
	msgChan <- &msgInfoNew
}

func main() {
	if len(os.Args) < 4 {
		fmt.Printf("Usage: ./consumer topic_name group_name cluster_name\n")
		return
	}

	fmt.Printf("topic: %s | group: %s\n", os.Args[1], os.Args[2])

	topic := os.Args[1]
	group := os.Args[2]
	cluster := os.Args[3]

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)

	done := make(chan bool, 1)
	go func() {
		for sig := range c {
			fmt.Printf("received ctrl+c(%v)\n", sig)
			done <- true
		}
	}()

	callback := qbus.NewDirectorQbusConsumerCallback(&GoCallback{})
	//qbusConsumer := qbus.NewQbusConsumer()
	if !qbusConsumer.Init(cluster, "consumer.log", "./consumer.config", callback) {
		fmt.Println("Failed to Init")
		os.Exit(0)
	}

	if !qbusConsumer.SubscribeOne(group, topic) {
		fmt.Println("Failed to SubscribeOne")
		os.Exit(0)
	}

	go func() {
		for msgInfo := range msgChan {
			qbusConsumer.CommitOffset(*msgInfo)
			qbus.DeleteQbusMsgContentInfo(*msgInfo)
		}
	}()

	qbusConsumer.Start()

	if <- done {
		qbusConsumer.Stop()
		qbus.DeleteQbusConsumer(qbusConsumer)
		qbus.DeleteDirectorQbusConsumerCallback(callback)

		close(msgChan)
	}

}
