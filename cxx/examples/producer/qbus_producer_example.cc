#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "qbus_producer.h"
//---------------------------------------
bool kStop = false;
const char* kConfigPath = "";

void Stop(int) { kStop = true; }

int main(int argc, const char* argv[]) {
  const char* topic_name = "";

  if (argc > 5) {
    topic_name = argv[1];
    std::cout << "Topic: " << topic_name << std::endl;
  } else {
    std::cout << "Usage: ./qbus_producer_example topic key[\"\" or ] "
                 "send_count config_full_path cluster_list"
              << std::endl;
    return 0;
  }

  signal(SIGINT, Stop);
  signal(SIGALRM, Stop);
  int64_t loop = atoll(argv[3]);
  std::string key = argv[2];

  struct timeval now_time;
  gettimeofday(&now_time, NULL);
  int64_t start_time_ms =
      ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;

  qbus::QbusProducer qbus_producer;
  if (!qbus_producer.init(argv[5], "./qbus_producer_example.log", argv[4],
                          topic_name)) {
    std::cout << "Failed to init" << std::endl;
    return 0;
  }

  std::cout << "Producer init is ok!" << std::endl;

  std::string msg("aaaaaaaaaa");
  while (!kStop) {
    if (kStop) {
      break;
    }

    if (-1 == loop) {
      while (!kStop) {
        if (!qbus_producer.produce(msg.c_str(), msg.length(), key)) {
          std::cout << "Failed to produce" << std::endl;
          // Retry to produce
        }
        usleep(5000);
      }
    } else {
      for (int i = 0; i < loop; ++i) {
        if (kStop) {
          break;
        }
        if (!qbus_producer.produce(msg.c_str(), msg.length(), key)) {
          std::cout << "Failed to produce" << std::endl;
          // Retry to produce
        }
      }
      break;
    }
  }

  qbus_producer.uninit();

  gettimeofday(&now_time, NULL);
  int64_t end_time_ms =
      ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;

  std::cout << "benchmark: " << end_time_ms - start_time_ms << std::endl;

  return 0;
}
