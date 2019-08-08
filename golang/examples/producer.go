package main

import (
	"bufio"
	"fmt"
	"os"
	"qbus"
	"time"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Printf("Usage: ./produer topic_name cluster_name\n")
		return
	}

	topic := os.Args[1]
	cluster := os.Args[2]

	qbusProducer := qbus.NewQbusProducer()
	if !qbusProducer.Init(cluster, "./producer.log", "./producer.config", topic) {
		fmt.Printf("Failed to Init")
		return
	}

	running := true
	reader := bufio.NewReader(os.Stdin)
	data, _, _ := reader.ReadLine()
	for running {
		//data, _, _ := reader.ReadLine()
		msg := string(data)
		if msg == "stop" {
			running = false
		} else {
			if !qbusProducer.Produce(msg, (int64)(len(msg)), "") {
				fmt.Print("Failed to Produce")
				//Retry to Produce
			}
			time.Sleep(1 * time.Second)
		}
	}

	qbusProducer.Uninit()

	qbus.DeleteQbusProducer(qbusProducer)
}
