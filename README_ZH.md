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

***依赖liblog4cplus, boost, swig-3.0.12, cmake***

##### *cxx/c*
 进入cxx/c目录，执行build.sh -release，在./lib/release下会产生libqbus.so。

#### *go*
进入go目录，执行build.sh，在gopath/src/qbus 目录下生成qbus.go和libQBus_go.so。

#### *python*
进入python目录，执行build.sh，在当前目录生成qbus.py和_qbus.so。
编译脚本提供了选项，可以通过-h查看。可以通过-s选项传递python相关头文件路径。默认-s /usr/local/python2.7/include/python2.7

#### *php*
进入python目录, 执行build.sh，当前目录生成扩展qbus.so和qbus.php。
编译脚本提供了选项，可以通过-h查看。可以通过s选项传递php相关头文件路径，可以通过-v传递php的版本。默认选项-s /usr/local/php -v php。


## 使用
#### 数据生产
* 在非按key写入的情况下，sdk尽最大努力提交每一条消息，只要Kafka集群存有一台broker正常，就会重试发送;
* 每次写入数据只需要调用*produce*接口，在异步发送的场景下，通过返回值可以判断发送队列是否填满，发送队列可通过配置文件调整;
* 在同步发送的场景中，*produce*接口返回当前消息是否写入成功，但是写入性能会有所下降，CPU使用率会有所上升,推荐还是使用异步写入方式;。
* 下面是生产接口，以c++为例：
~~~
bool QbusProducer::init(const string& broker_list, const string& log_path, const string& config_path, const string& topic)
bool QbusProducer::produce(const char* data, size_t data_len, const std::string& key)
void QbusProducer::uninit()
~~~
* c++ sdk的使用范例：

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


#### 数据消费
* 消费只需调用subscribeOne订阅topic（也支持同时订阅多个topic），然后执行start就开始消费，当前进程非阻塞，每条消息通过callback接口回调给使用者;
* sdk还支持用户手动提交offset方式，用户可以通过callback中返回的消息体，在代码其他逻辑中进行提交。
* 下面是消费接口，以c++为例：
~~~
bool QbusConsumer::init(string broker_list, string log_path, string config_path, QbusConsumerCallback& callback)
bool QbusConsumer::subscribeOne(string group, string topic)
bool QbusConsumer::start()
void QbusConsumer::stop()
~~~

* c++ sdk的使用范例：

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


## 配置
[kafkabridge具体配置](https://github.com/Qihoo360/kafkabridge/blob/master/CONFIGURATION_ZH)

## QQ 群
![](https://github.com/Qihoo360/kafkabridge/blob/master/kafkabridge.png)
