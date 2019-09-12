// qbus暂停/恢复消费功能示例：对每个topic，每次消费一个批次(固定数量)的消息，就会暂停消费该topic一段时间，之后恢复消费。
package main

import (
	"fmt"
	"os"
	"os/signal"
	"qbus"
	"strings"
	"time"
)

const (
	// 日志路径
	logPath = "./consumer.log"
	// 配置文件路径
	configPath = "./consumer.config"
	// 一个批次的消息数量
	messageBatchSize = 2
	// 暂停消费的时间(单位：秒)
	pausedTimeS = 3
)

var qbusConsumer qbus.QbusConsumer

// GoCallback 实现qbus消费回调接口
type GoCallback struct {
	qbus.QbusConsumerCallback

	// topic名 => 该topic上已消费的消息数量
	messageCountMap map[string]int64
}

// NewGoCallback 创建GoCallback实例
func NewGoCallback() *GoCallback {
	cb := new(GoCallback)
	cb.messageCountMap = make(map[string]int64)
	return cb
}

// DeliveryMsg 自动提交offset模式中消费消息的回调函数
func (p *GoCallback) DeliveryMsg(topic string, msg string, msgLen int64) {
	id, ok := p.messageCountMap[topic]
	if ok {
		id++
	} else {
		id = 1
	}

	p.messageCountMap[topic] = id
	// NOTE: 如果消息中含有字节0（对应C字符串终止符），则转换成string时会截断，导致len(msg)<msgLen
	fmt.Printf("Topic:%s id:%d | msg[%d]:%s\n", topic, id, msgLen, msg)

	if id%messageBatchSize == 0 {
		// 若消息消息数量构成了一个批次，暂停消费
		fmt.Printf("%v | Pause consuming %s\n", time.Now().Format("2006-01-02 15:04:05.000"), topic)
		if !qbusConsumer.Pause(toStringVector([]string{topic})) {
			fmt.Printf("Failed to pause %s\n", topic)
			os.Exit(1)
		}
		go func(topic string) {
			// 暂停若干秒后，恢复该topic的消费
			time.Sleep(time.Second * pausedTimeS)
			fmt.Printf("%v | Resume consuming %s\n", time.Now().Format("2006-01-02 15:04:05.000"), topic)
			if !qbusConsumer.Resume(toStringVector([]string{topic})) {
				fmt.Printf("Failed to resume %s\n", topic)
				os.Exit(1)
			}
		}(topic)
	}
}

func main() {
	if len(os.Args) < 4 {
		fmt.Printf("Usage: %s topic_list group_name cluster_name\n"+
			"  topic_list is comma-splitted topics like \"topic1,topic2,topic3\"\n",
			os.Args[0])
		return
	}

	// 1. 解析命令行参数
	topicList := strings.Split(os.Args[1], ",")
	groupName := os.Args[2]
	clusterName := os.Args[3]
	fmt.Printf("topics: %v | group: %s\n", topicList, groupName)

	// 2. 启动消费者订阅消息
	qbusConsumer = qbus.NewQbusConsumer()
	defer qbus.DeleteQbusConsumer(qbusConsumer)

	callback := qbus.NewDirectorQbusConsumerCallback(NewGoCallback())
	defer qbus.DeleteDirectorQbusConsumerCallback(callback)

	if !qbusConsumer.Init(clusterName, logPath, configPath, callback) {
		fmt.Println("Failed to Init")
		os.Exit(1)
	}

	if !qbusConsumer.Subscribe(groupName, toStringVector(topicList)) {
		fmt.Println("Failed to Subscribe")
		os.Exit(1)
	}

	// 3. Ctrl+C退出主循环
	c := make(chan os.Signal, 1)
	done := make(chan bool, 1)

	go func() {
		for sig := range c {
			fmt.Printf("received ctrl+c(%v)\n", sig)
			done <- true
		}
	}()
	signal.Notify(c, os.Interrupt)

	// 4. 启动消费线程进行消费
	if !qbusConsumer.Start() {
		fmt.Println("Failed to Start")
		os.Exit(1)
	}
	defer qbusConsumer.Stop()
	fmt.Println("%% Start consuming... (Press Ctrl+C to exit)")

	<-done
	fmt.Println("Done.")
}

func toStringVector(s []string) qbus.StringVector {
	result := qbus.NewStringVector()
	for i := 0; i < len(s); i++ {
		result.Add(s[i])
	}
	return result
}
