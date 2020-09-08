
## Introduction [中文](https://github.com/Qihoo360/kafkabridge/blob/master/README_ZH.md)
* Qbusbridge is a client SDK for pub-sub messaging systems. Currently it supports:

  * [Apache Kafka](http://kafka.apache.org/)
  * [Apache Pulsar](https://pulsar.apache.org/)

  User could switch to any pub-sub messaging system by changing the configuration file. The default config is accessing Kafka, if you want to change it to Pulsar, change the config to:

  ```ini
  mq.type=pulsar
  # Other configs for pulsar...
  ```

  See [config](config/) for more details.

  >  TODO: English config docs is missed currently.

* Qbusbridge-Kafka is based on the [librdkafka](https://github.com/edenhill/librdkafka) under the hook. A mass of details related to how to use has been hidden, that making QBus more simple and easy-to-use than [librdkafka](https://github.com/edenhill/librdkafka). For producing and consuming messages, the only thing need the users to do is to invoke a few APIs, for these  they don't need to understand too much about Kafka.

* The reliability of messages producing, that is may be the biggest concerns of the users, has been considerably improved.

## Features
* Multiple programming languages are supported, includes C++, PHP, Python, Golong, with very consistent APIs.
* Few interfaces, so it is easy to use.
* For advanced users, adapting [librdkafka](https://github.com/edenhill/librdkafka)'s configration by profiles is also supported.
* In the case of writing data not by keys, the SDK will do the best to guarantee the messages being written successfully .
* Two writing modes, synchronous and asynchronous, are supported.
* As for messages consuming, the offset could be submited automatically, or by manual  configurating.
* For the case of using php-fpm, the connection is keeping-alived for reproduce messages uninterruptedly, saving the cost caused by recreating connections.

## Compiling

Ensure your system has g++ (>= 4.8.5), boost (>= 1.41), cmake (>= 3.1) and swig (>= 3.0.12) installed.

In addition, qbus SDK is linking libstdc++ statically, so you must ensure that `libstdc++.a` exists. For CentOS users, run:

```bash
sudo yum install -y glibc-static libstdc++-static
```

#### git clone:
git clone --recursive https://github.com/Qihoo360/qbusbridge.git

### 1. Install submodules

Run `./build_dependencies.sh`.

It will automatically download submodules and install them to `cxx/thirdparts/local` where `CMakeLists.txt` finds headers and libraries.

See `./cxx/thirdparts/local`:

```
include/
  librdkafka/
    rdkafka.h
  log4cplus/
    logger.h
lib/
  librdkafka.a
  liblog4cplus.a
```

### 2. Build SDK

#### C++

Navigate to the `cxx` directory and run `./build.sh`, following files will be generated:

```
include/
  qbus_consumer.h
  qbus_producer.h
lib/
  debug/libQBus.so
  release/libQBus.so
```

> Though building C++ SDK requires C++11 support, the SDK could be used with older g++. eg. build qbus SDK with g++ 4.8.5 and use qbus SDK with g++ 4.4.7.

#### Go
Navigate to the `golang` directory and run `./build.sh`, following files will be generated:

```
gopath/
  src/
    qbus/
      qbus.go
      libQBus_go.so
```

You can enable go module for examples by running `USE_GO_MOD=1 ./build.sh`. Then following files will be generated:

```
examples/
  go.mod
  qbus/
    qbus.go
    go.mod
    libQBus_go.so
```

#### Python
Navigate to the `python` directory and run `./build.sh`, following files will be generated:

```
examples/
  qbus.py
  _qbus.so
```

#### PHP
Navigate to the `php` directory and run `build.sh`, following files will be generated:

```
examples/
  qbus.php
  qbus.so
```

### 3. Build examples

#### C++

Navigate to `examples` subdirectory and run `./build.sh [debug|release]` to generate executable files. `debug` is using `libQBus.so` in `lib/debug` subdirectory, `release` is using `libQBus.so` in `lib/release` subdirectory. Run `make clean` to delete them.

If you want to build your own programs, see how `Makefile` does.

#### Go

Navigate to `examples` subdirectory and run `./build.sh` to generate executable files, run `./clean.sh` to delete them.

Add path of `libQBus_go.so` to env `LD_LIBRARY_PATH`, eg.

```bash
export LD_LIBRARY_PATH=$PWD/gopath/src/qbus:$LD_LIBRARY_PATH
```

If you want to build your own programs, add generated `gopath` directory to env `GOPATH`, or move `gopath/src/qbus` directory to `$GOPATH/src`.

#### Python

Copy generated `qbus.py` and `_qbus.so` to the path of the Python scripts to run.

#### PHP

Edit `php.ini` and add `extension=<module-path>`, `<module-path>` is the path of `qbus.so`.


## Usage

#### Data Producing
* In the case of writing data not by keys, the SDK will do it's best to submit every single message. As long as there is one broker in the Kafka cluster behaving normally, it will try to resend.
* Writing data only need to invoke the `produce` interface, and in the asynchronous mode, by checking the return value, you could know whether the sending queue is full or not.
* In the synchronous writing mode, `produce` interface will return value directively that indicate whether the current message has been written succuessfully. But that is at the expense of some extra performance loss and CPU usage. So asynchronous mode is recommended.
* The following is a C++ example demonstrating how to invoke the `produce` interface:
~~~c++
bool QbusProducer::init(const string& broker_list,
                        const string& log_path,
                        const string& config_path,
                        const string& topic_name);
bool QbusProducer::produce(const char* data,
                           size_t data_len,
                           const std::string& key);
void QbusProducer::uninit();
~~~

* C++ SDK use example:

~~~c++
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
~~~c++
bool QbusConsumer::init(const std::string& broker_list,
                        const std::string& log_path,
                        const std::string& config_path,
                        const QbusConsumerCallback& callback);
bool QbusConsumer::subscribeOne(const std::string& group, const std::string& topic);
bool QbusConsumer::subscribe(const std::string& group,
                             const std::vector<std::string>& topics);
bool QbusConsumer::start();
void QbusConsumer::stop();
bool QbusConsumer::pause(const std::vector<std::string>& topics);
bool QbusConsumer::resume(const std::vector<std::string>& topics);
~~~

* C++ SDK use example: 

~~~c++
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

You can use `pause()` and `resume()` methods to pause or resume consuming some topics, see [qbus_pause_resume_example.cc](./cxx/examples/consumer/qbus_pause_resume_example.cc)

See examples in [C examples](c/examples/)，[C++ examples](cxx/examples/)，[Go examples](golang/examples/)，[Python examples](python/examples/)，[PHP examples](php/examples/) for more usage.

## CONFIGURATION

The configuration file is in [INI](https://en.wikipedia.org/wiki/INI_file) format:

```ini
[global]

[topic]

[sdk]
```

See [rdkafka 1.0.x configuration](https://github.com/edenhill/librdkafka/blob/1.0.x/CONFIGURATION.md) for *global* and *topic* configurations, and [sdk configuration](https://github.com/Qihoo360/kafkabridge/blob/master/CONFIGURATION.md) for *sdk* configuration.

Normally kafkabridge works with an empty configuration file, but if your broker version < 0.10.0.0, you must specify api.version-related configuration parameters, see [broker version compatibility](https://github.com/edenhill/librdkafka/blob/master/INTRODUCTION.md#broker-version-compatibility).

eg. for broker 0.9.0.1, following configurations are necessary:

```ini
[global]
api.version.request=false
broker.version.fallback=0.9.0.1
```

**The default config is now compatible with broker 0.9.0.1. Therefore, if higher version broker is used, `api.version.request` should be set true. Otherwise, the message protocol would be older version, e.g. no timestamp field.**

## Contact

 QQ group: 876834263

![](https://github.com/Qihoo360/qbusbridge/blob/master/kafkabridge.png)
