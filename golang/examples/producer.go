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

	qbus_producer := qbus.NewQbusProducer()
	if !qbus_producer.Init(os.Args[2],
		"./producer.log",
		"./producer.config",
		os.Args[1]) {
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
			if !qbus_producer.Produce(msg, (int64)(len(msg)), "") {
				fmt.Print("Failed to Produce")
				//Retry to Produce
			}
			time.Sleep(1 * time.Second)
		}
	}

	qbus_producer.Uninit()

	qbus.DeleteQbusProducer(qbus_producer)
}
