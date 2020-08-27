#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "qbus_consumer.h"

#if __cplusplus >= 201103L
#define OVERRIDE override
#else
#define OVERRIDE
#endif

static void signalHandler(int) {}

class Callback : public qbus::QbusConsumerCallback {
   public:
    Callback(qbus::QbusConsumer& consumer) : consumer_(consumer) {}

    void deliveryMsg(const std::string& topic, const char* msg, size_t msg_len) const OVERRIDE {
        std::cout << "Topic: " << topic << " | msg: " << std::string(msg, msg_len) << std::endl;
    }

    // 用户自行管理并提交 offset 或确认消息的回调函数，需要修改默认配置
    void deliveryMsgForCommitOffset(const qbus::QbusMsgContentInfo& info) const OVERRIDE {
        std::cout << "Topic: " << info.topic << " | msg: " << info.msg << std::endl;
        consumer_.commitOffset(info);
    }

   private:
    qbus::QbusConsumer& consumer_;  // 仅用于用户手动提交
};

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cout << "Usage: " << argv[0] << " config_path topic_name group_name cluster_name" << std::endl;
        return 1;
    }

    std::string config_path = argv[1];
    std::string topic_name = argv[2];
    std::string group = argv[3];
    std::string cluster_name = argv[4];

    std::cout << "topic: " << topic_name << " | group: " << group << " | cluster: " << cluster_name
              << std::endl;

    qbus::QbusConsumer consumer;
    Callback callback(consumer);

    if (!consumer.init(cluster_name, "consumer.log", config_path, callback)) {
        std::cout << "Init failed" << std::endl;
        return 2;
    }

    if (!consumer.subscribeOne(group, topic_name)) {
        std::cout << "SubscribeOne failed" << std::endl;
        return 3;
    }

    if (!consumer.start()) {
        std::cout << "Start failed" << std::endl;
        return 4;
    }

    signal(SIGINT, signalHandler);
    pause();
    consumer.stop();
    std::cout << "done" << std::endl;
    return 0;
}
