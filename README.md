
## Introduction [中文](https://github.com/Qihoo360/kafkabridge/blob/master/README_ZH.md)
* Kafkabridge is based on the [librdkafka](https://github.com/edenhill/librdkafka) under the hook. A mass of details related to how to use has been hidden, that making QBus more simple and easy-to-use than [librdkafka](https://github.com/edenhill/librdkafka). For producing and consuming messages, the only thing need the users to do is to invoke a few APIs, for these  they don't need to understand too much about Kafka.
* The reliability of messages producing, that is may be the biggest concerns of the users, has been considerably improved.

## Features
* Multiple programming languages are supported, includes C/C++, PHP, Python, Golong, etc, with very consistent APIs.
* Few interfaces, so it is easy to use.
* For advanced users, adapting [librdkafka](https://github.com/edenhill/librdkafka)'s configration by profiles is also supported.
* In the case of writing data not by keys, the SDK will do the best to guarantee the messages being written successfully .
* Two writing modes, synchronous and asynchronous, are supported.
* As for messages consuming, the offset could be submited automatically, or by manual  configurating.
* For the case of using php-fpm, the connection is keeping-alived for reproduce messages uninterruptedly, saving the cost caused by recreating connections.

## Compiling

***Dependencies: liblog4cplus, boost, swig-3.0.12, cmake***

##### *cxx/c*
Navigate to the cxx/c installation directory，and run `build.sh -release`, you will get a new file named libqbus.so in the "./lib/release" directory.

#### *go*
Navigate to the Go installation directory，and run `build.sh`, you will get the new files qbus.go and libQBus_go.so in the directory "gopath/src/qbus".

#### *python*
Navigate to the Python installation directory, run `build.sh`, that will generate two files, qbus.py and _qbus.so in the current directory.

/usr/local/python2.7/include/python2.7

The complie script support some options, you can check them by append the `-h` option. Use the `-s` option passing the file path of Python's header files, '/usr/local/python2.7/include/python2.7' is the default value of `-s`.


#### *php*
Naviate to the PHP installation directory, run `build.sh`, that will generate the new files qbus.so and qbus.php in the current directory.

The complie script support some options, you can check them by append the `-h` option. Use the `-s` option passing the file path of PHP's head files, the default value of `-s` is '/usr/local/php -v php'.


## Usage

#### Data Producing
* In the case of writing data not by keys, the SDK will do it's best to submit every single message. As long as there is one broker in the Kafka cluster behaving normally, it will try to resend.
* Writing data only need to invoke the `produce` interface, and in the asynchronous mode, by checking the return value, you could know whether the sending queue is full or not.
* In the synchronous writing mode, `produce` interface will return value directively that indicate whether the current message has been written succuessfully. But that is at the expense of some extra performance loss and CPU usage. So asynchronous mode is recommended.
* The following is a C++ example demonstrating how to invoke the `produce` interface:
~~~
bool QbusProducer::init(const string& broker_list, const string& log_path, const string& config_path, const string& topic)
bool QbusProducer::produce(const char* data, size_t data_len, const std::string& key)
void QbusProducer::uninit()
~~~

* C++ SDK use example:

~~~
#include <string>
#include <iostream>
#include "qbus_producer.h"

int main(int argc, const char* argv[]) {
    qbus::QbusProducer qbus_producer;
    if (!qbus_producer.init("127.0.0.1:9092",
                    "./log",
                    "./config",
                    "topic_test")) {
        std::cout << "Failed to init" << std::endl;
        return 0;
    }

    std::string msg("test\n");
    if (!qbus_producer.produce(msg.c_str(), msg.length(), "key")) {
        std::cout << "Failed to produce" << std::endl;
    }

    qbus_producer.uninit();

    return 0;
}

~~~

#### Data Consuming
* Consuming data only need to invoke the `subscribeOne` to subscribe the 'topic' (also support subscribing multiple topics). The current process is not blocked, every message will send back to the user through the callback.
* The SDK also supports submit offset manually, users can submit the offset in the code of the message body that returned by through callbacks.
* The following is an example of C++, that demonstrate the usage of the consuming interface:
~~~
bool QbusConsumer::init(string broker_list, string log_path, string config_path, QbusConsumerCallback& callback)
bool QbusConsumer::subscribeOne(string group, string topic)
bool QbusConsumer::start()
void QbusConsumer::stop()
~~~

* C++ SDK use example: 

~~~
#include <iostream>
#include "qbus_consumer.h"

qbus::QbusConsumer qbus_consumer;
class MyCallback: public qbus::QbusConsumerCallback {
    public:
        virtual void deliveryMsg(const std::string& topic,
                    const char* msg,
                    const size_t msg_len) const {
            std::cout << "topic: " << topic << " | msg: " << std::string(msg, msg_len) << std::endl;
        }

};

int main(int argc, char* argv[]) {
    MyCallback my_callback;
    if (qbus_consumer.init("127.0.0.1:9092",
                    "log",
                    "config",
                    my_callback)) {
        if (qbus_consumer.subscribeOne("groupid_test", "topic_test")) {
            if (!qbus_consumer.start()) {
                std::cout << "Failed to start" << std::endl;
                return NULL;
            }

            while (1) sleep(1);  //other operations can appear here

            qbus_consumer.stop();
        } else {
            std::cout << "Failed subscribe" << std::endl;
        }
    } else {
        std::cout << "Failed init" << std::endl;
    }
    return 0;
}

~~~

## [CONFIGURATION](https://github.com/Qihoo360/kafkabridge/blob/master/CONFIGURATION)
