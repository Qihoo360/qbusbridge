package main

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"os/signal"
	"qbus"
)

func main() {
	if len(os.Args) < 4 {
		fmt.Printf("Usage: ./producer config_path topic_name cluster_name\n")
		return
	}

	configPath := os.Args[1]
	topicName := os.Args[2]
	clusterName := os.Args[3]

	producer := qbus.NewQbusProducer()
	defer qbus.DeleteQbusProducer(producer)

	if !producer.Init(clusterName, "producer.log", configPath, topicName) {
		fmt.Println("Init failed")
		os.Exit(1)
	}
	defer producer.Uninit()

	done := make(chan os.Signal, 1)
	signal.Notify(done, os.Interrupt)

	go func() {
		reader := bufio.NewReader(os.Stdin)
		running := true

		fmt.Println("%% Please input messages (Press Ctrl+C or Ctrl+D to exit):")
		for running {
			msg, err := reader.ReadString('\n')
			switch err {
			case nil:
				if !producer.Produce(msg, int64(len(msg))-1, "") {
					fmt.Println("Failed to Produce")
				}
			case io.EOF:
				done <- os.Interrupt
				running = false
			default:
				panic(err)
			}
		}
	}()

	<-done
	fmt.Println("%% Done.")
}
