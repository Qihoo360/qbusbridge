## 简介 [English](https://github.com/Qihoo360/kafkabridge/blob/master/README.md)
* kafkabridge 底层基于 [librdkafka](https://github.com/edenhill/librdkafka), 与之相比封装了大量的使用细节，简单易用，使用者无需了解过多的Kafka系统细节，只需调用极少量的接口，就可完成消息的生产和消费;
* 针对使用者比较关心的消息生产的可靠性，作了近一步的提升

## 特点
* 支持多种语言：c++/c、php、python、golang, 且各语言接口完全统一;
* 接口少，简单易用;
* 针对高级用户，支持通过配置文件调整所有的librdkafka的配置;
* 在非按key写入数据的情况下，尽最大努力将消息成功写入;
* 支持同步和异步两种数据写入方式;
* 在消费时，除默认自动提交offset外，允许用户通过配置手动提交offset;
* 在php-fpm场景中，复用长连接生产消息，避免频繁创建断开连接的开销;

## 编译

确保你的系统上安装了 boost (>= 1.41)，cmake (>= 2.8)，swig (>= 3.0.12)。

#### git clone

git clone --recursive https://github.com/Qihoo360/kafkabridge.git

### 1. 安装子模块

运行`./build_dependencies.sh`。

它会自动下载子模块，并将其安装到`cxx/thirdparts/local`，即`CMakeLists.txt`查找头文件和库文件的目录。

### 2. 编译SDK

#### C/C++

C的SDK依赖于C++的SDK，因此得先编译C++ SDK。

进入`cxx`目录，执行`./build.sh <BUILD_TYPE>`（其中`<BUILD_TYPE>`是`debug`或者`release`），在`cxx/lib/<BUILD_TYPE>`目录下会生成`libQBus.so`。

然后进入`c`目录，执行`./build.sh <BUILD_TYPE>`，在`c/lib/<BUILD_TYPE>`目录下会生成`libQBus_C.so`。

#### Go
进入`golang`目录，执行`./build.sh`，在`gopath/src/qbus`子目录下会生成`qbus.go`和`libQBus_go.so`。

#### Python
进入`python`目录，执行`./build.sh`，在当前目录会生成`qbus.py`和`_qbus.so`。

#### PHP
进入`php`目录，执行`./build.sh`，在当前目录会生成`qbus.so`和`qbus.php`。

### 3. 编译示例程序

#### C/C++

进入`examples`子目录，运行`make`生成可执行文件，运行`make clean`删除它们。

如果要运行自己的程序，可以参考`Makefile`文件。

#### Go

进入`examples`子目录，运行`./build.sh`生成可执行文件，运行`./clean.sh`删除它们。

运行可执行文件时把`libQBus_go.so`路径加入`LD_LIBRARY_PATH`环境变量。

如果要运行自己的程序，将生成的`gopath`目录加入`GOPATH`环境变量，或者将`gopath/src/qbus`目录移动到`$GOPATH/src`下。

#### Python

将生成的`qbus.py`和`_qbus.so`拷贝至要运行的Python脚本同一路径即可。

#### PHP

编辑`php.ini`文件，添加`extension=<module-path>`，`<module-path>`为`qbus.so`路径。


## 使用
#### 数据生产
* 在非按key写入的情况下，sdk尽最大努力提交每一条消息，只要Kafka集群存有一台broker正常，就会重试发送;
* 每次写入数据只需要调用*produce*接口，在异步发送的场景下，通过返回值可以判断发送队列是否填满，发送队列可通过配置文件调整;
* 在同步发送的场景中，*produce*接口返回当前消息是否写入成功，但是写入性能会有所下降，CPU使用率会有所上升,推荐还是使用异步写入方式;。
* 下面是生产接口，以c++为例：
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
* c++ sdk的使用范例：

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


#### 数据消费
* 消费只需调用subscribeOne订阅topic（也支持同时订阅多个topic），然后执行start就开始消费，当前进程非阻塞，每条消息通过callback接口回调给使用者;
* sdk还支持用户手动提交offset方式，用户可以通过callback中返回的消息体，在代码其他逻辑中进行提交。
* 下面是消费接口，以c++为例：
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

* c++ sdk的使用范例：

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

            while (1) sleep(1);  //可以执行其他业务逻辑

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

可以用`pause()`和`resume()`方法来暂停或恢复某些主题的消费，具体示例见[qbus_pause_resume_example.cc](cxx/examples/consumer/qbus_pause_resume_example.cc)。

更多API使用方法参考[C examples](c/examples/)，[C++ examples](cxx/examples/)，[Go examples](golang/examples/)，[Python examples](python/examples/)，[PHP examples](php/examples/)目录下的示例代码。

## 配置

配置文件是[INI](https://en.wikipedia.org/wiki/INI_file)格式:

```ini
[global]

[topic]

[sdk]
```

*global*和*topic*配置见[rdkafka 1.0.x configuration](https://github.com/edenhill/librdkafka/blob/1.0.x/CONFIGURATION.md)，*sdk*配置见[sdk configuration](https://github.com/Qihoo360/kafkabridge/blob/master/CONFIGURATION_ZH.md)。

通常情况下kafkabridge使用空配置文件即可工作，但是如果broker版本低于0.10.0.0，必须添加api.version相关的配置，见[broker version compatibility](https://github.com/edenhill/librdkafka/blob/master/INTRODUCTION.md#broker-version-compatibility).

例如，对0.9.0.1版本的broker，必须添加以下配置：

```ini
[global]
api.version.request=false
broker.version.fallback=0.9.0.1
```

## QQ 群 : 876834263
![](https://github.com/Qihoo360/kafkabridge/blob/master/kafkabridge.png)
