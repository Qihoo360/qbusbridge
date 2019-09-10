#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "qbus_consumer.h"
//----------------------------------------------------
static bool kStop = false;
const char* kGroupName = "qbus_consumer";
const char* kTopicName = "";
const char* kConfigPath = "";
const char* kClusterName = "";

qbus::QbusConsumer qbus_consumer;
class MyCallback : public qbus::QbusConsumerCallback {
 public:
  virtual void deliveryMsg(const std::string& topic, const char* msg,
                           const size_t msg_len) const {
    std::cout << "topic: " << topic << " | msg: " << std::string(msg, msg_len)
              << std::endl;
  }

  //如果想使用纯手动提交offset,需要业务自行实现下面的函数，用户可以自行保存msg_info，然后在合适的位置提交offset，在此回调中作业务处理,并且需要在consumer.config中设置user.manual.commit.offset=true,
  //对于绝大多数应用,这个不是必需的
  virtual void deliveryMsgForCommitOffset(
      const qbus::QbusMsgContentInfo& msg_info) const {
    static int count = 0;
    std::cout << "user manual committ offset | topic: " << msg_info.topic
              << " | msg: " << msg_info.msg << std::endl;
    if (count == 100) {
      qbus_consumer.commitOffset(msg_info);
      std::cout << "tijiao\n";
    }
    count++;
  }
};

void Stop(int) { kStop = true; }

void* simpleConsumer() {
  MyCallback my_callback;
  if (qbus_consumer.init(kClusterName, "./qbus_consumer_example.log",
                         kConfigPath, my_callback)) {
    std::vector<std::string> topics;
    topics.push_back(kTopicName);
    std::cout << " topic: " << kTopicName << " | group: " << kGroupName
              << std::endl;
    if (qbus_consumer.subscribeOne(kGroupName, kTopicName)) {
      if (!qbus_consumer.start()) {
        std::cout << "Failed to start" << std::endl;
        return NULL;
      }

      while (!kStop) {
        sleep(1);
      }

      qbus_consumer.stop();
    } else {
      std::cout << "Failed subscribe" << std::endl;
    }
  } else {
    std::cout << "Failed init" << std::endl;
  }

  return NULL;
}

int main(int argc, char* argv[]) {
  if (argc > 4) {
    kTopicName = argv[1];
    kGroupName = argv[2];
    kConfigPath = argv[3];
    kClusterName = argv[4];
  } else {
    std::cout << "Usage: qbsu_consumer_example topic_name group_name "
                 "config_full_path cluster_list"
              << std::endl;
    return 0;
  }

  signal(SIGINT, Stop);

  simpleConsumer();

  return 0;
}
