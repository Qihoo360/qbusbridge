// qbus暂停/恢复消费功能示例：对给定topic，若消费消息数量达到阈值，则暂停消费一段时间后恢复消费。
#include "qbus_consumer.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int current_message_count = 0;
// kMessageBatchSize为一个批次的消息数量，每次消费该数量的消息，就会暂停消费该topic。
// 等到kPauseSeconds秒后恢复消费，即模拟对该批次的消息进行消费的时间。
static int kMessageBatchSize = 2;
static int kPauseSeconds = 3;
static const char* kConfigPath = "./consumer.config";
static const char* kLogPath = "./qbus_pause_resume_example.log";
static const char* kTopics[1];  // 支持1个topic的消费/暂停/恢复，以简化代码逻辑

#define CHECK_RETVAL(expr)                         \
  if (expr != QBUS_RESULT_OK) {                    \
    fprintf(stderr, "[ERROR] " #expr " failed\n"); \
    exit(1);                                       \
  }

static volatile int kRun = 1;
static void stop(int signum) { kRun = 0; }

static QbusConsumerHandle kConsumerHandle = NULL;

static const char* now();

static void alarmHandler(int signum) {
  CHECK_RETVAL(QbusConsumerResume(kConsumerHandle, kTopics, 1));
  // 实际场景中不应该在信号处理器中调用耗时的printf函数，可以将Resume操作及之后
  // 的打印函数放到子线程中进行，回调函数中通过条件变量通知然后调用打印函数。
  printf("%s | Resume consuming %s\n", now(), kTopics[0]);
}

void QbusConsumerDeliveryMsg(const char* topic, const char* msg,
                             int64_t msg_len) {
  // 实际场景下会保存该消息到用户自定义的数据结构中，以实现分批消费
  printf("topic:%s | %.*s\n", topic, (int)msg_len, msg);
  current_message_count++;

  if (current_message_count % kMessageBatchSize == 0) {
    CHECK_RETVAL(QbusConsumerPause(kConsumerHandle, &topic, 1));
    printf("%s | Pause consuming %s for %d seconds\n", now(), topic,
           kPauseSeconds);
    alarm((unsigned)kPauseSeconds);
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2 || strcmp(argv[1], "-h") == 0 ||
      strcmp(argv[1], "--help") == 0) {
    fprintf(stderr,
            "Usage: %s topic [conf-path] [log-path]\n"
            "  default conf-path is %s\n"
            "  default log-path is %s\n"
            "Note: bootstrap.servers and group.id must be configured!\n",
            argv[0], kConfigPath, kLogPath);
    exit(1);
  }

  kTopics[0] = argv[1];
  if (argc > 2) kConfigPath = argv[2];
  if (argc > 3) kLogPath = argv[3];

  kConsumerHandle = NewQbusConsumer();
  assert(kConsumerHandle);

  CHECK_RETVAL(InitQbusConsumer(kConsumerHandle, "cluster", kLogPath,
                                kConfigPath, QbusConsumerDeliveryMsg));
  CHECK_RETVAL(QbusConsumerSubscribe(kConsumerHandle, "", kTopics, 1));

  current_message_count = 0;
  signal(SIGALRM, alarmHandler);
  CHECK_RETVAL(QbusConsumerStart(kConsumerHandle));
  printf("%% Consumer started... (Press Ctrl+C to stop)\n");
  signal(SIGINT, stop);
  while (kRun) {
    sleep(1);
  }

  QbusConsumerStop(kConsumerHandle);
  printf("\n%% Consumer stopped.\n");
  return 0;
}

static const char* now() {
  static char buf[128];
  struct timeval tv;
  gettimeofday(&tv, NULL);

  struct tm* tmp = localtime(&tv.tv_sec);
  assert(tmp);
  assert(strftime(buf, sizeof(buf), "%F %T", tmp));

  size_t len = strlen(buf);
  snprintf(buf + len, sizeof(buf) - len, ".%03d", tv.tv_usec / 1000);

  return buf;
}
