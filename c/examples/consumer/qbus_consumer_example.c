#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "qbus_consumer.h"

//如果想使用纯手动提交offset,请打开下面的注释,并且需要在consumer.config中设置user.manual.commit.offset=true
//采用纯手动提交offset模式,需要调用InitQbusConsumerEx作初始化,而不是InitQbusConsumer
//对于绝大多数应用,这个不是必需的
//#define USE_COMMIT_OFFSET_CALLBACK

int kStop = 0;

QbusConsumerHandle consumer_handle = NULL;

void QbusConsumerDeliveryMsg(const char* topic, const char* msg,
                             int64_t msg_len) {
  printf("Topic: %s | msg: %s\n", topic, msg);
}

void QbusConsumerDeliveryForCommitOffsetFunc(
    const char* topic, const char* msg, int64_t msg_len,
    const QbusCommitOffsetInfoType offset_info) {
  printf("User commit | Topic: %s | msg: %s\n", topic, msg);
  QbusConsumerCommitOffset(consumer_handle, offset_info);
}

void Stop(int s) { kStop = 1; }

int main(int argc, char* argv[]) {
  if (argc < 5) {
    printf("Usage: %s config_path topic_name group_name cluster_name\n",
           argv[0]);
    return 1;
  }

  const char* config_path = argv[1];
  const char* topic_name = argv[2];
  const char* group = argv[3];
  const char* cluster_name = argv[4];

  printf("topic: %s | group: %s | cluster: %s\n", topic_name, group,
         cluster_name);

  signal(SIGINT, Stop);

#ifdef USE_COMMIT_OFFSET_CALLBACK
  QbusConsumerCallbackInfo callback_info;
  callback_info.callback = QbusConsumerDeliveryMsg;
  callback_info.callback_for_commit_offset =
      QbusConsumerDeliveryForCommitOffsetFunc;
#endif

  consumer_handle = NewQbusConsumer();

  if (QBUS_RESULT_OK !=
#ifdef USE_COMMIT_OFFSET_CALLBACK
      InitQbusConsumerEx(consumer_handle, cluster_name, "./consumer.log",
                         config_path, callback_info)
#else
      InitQbusConsumer(consumer_handle, cluster_name, "./consumer.log",
                       config_path, QbusConsumerDeliveryMsg)
#endif
  ) {
    printf("Init failed\n");
    goto failed_exit;
  }

  if (QbusConsumerSubscribeOne(consumer_handle, group, topic_name) !=
      QBUS_RESULT_OK) {
    printf("SubscribeOne failed\n");
    goto failed_exit;
  }

  if (QbusConsumerStart(consumer_handle) != QBUS_RESULT_OK) {
    printf("Start failed\n");
    goto failed_exit;
  }

  signal(SIGINT, Stop);
  while (!kStop) {
    sleep(1);
  }

  QbusConsumerStop(consumer_handle);
  DeleteQbusConsumer(consumer_handle);
  printf("done\n");
  return 0;

failed_exit:
  DeleteQbusConsumer(consumer_handle);
  return 2;
}
