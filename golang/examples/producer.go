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
	if len(os.Args) < 3 {
		fmt.Printf("Usage: ./producer topic_name cluster_name\n")
		return
	}

	qbusProducer := qbus.NewQbusProducer()
	defer qbus.DeleteQbusProducer(qbusProducer)

	if !qbusProducer.Init(os.Args[2],
		"./producer.log",
		"./producer.config",
		os.Args[1]) {
		fmt.Printf("Failed to Init")
		return
	}
	defer qbusProducer.Uninit()

	quitChan := make(chan os.Signal, 1)
	signal.Notify(quitChan, os.Interrupt)

	go func() {
		reader := bufio.NewReader(os.Stdin)
		running := true

		fmt.Println("%% Please input messages (Press Ctrl+C or Ctrl+D to exit):")
		for running {
			msg, err := reader.ReadString('\n')
			switch err {
			case nil:
				if !qbusProducer.Produce(msg, int64(len(msg))-1, "") {
					fmt.Println("Failed to Produce")
				}
			case io.EOF:
				quitChan <- os.Interrupt
				running = false
			default:
				panic(err)
			}
		}
	}()

	<-quitChan
	fmt.Println("%% Done.")
}
