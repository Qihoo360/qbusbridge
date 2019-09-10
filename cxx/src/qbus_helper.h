#ifndef QBUS_QBUS_HELPER_H_
#define QBUS_QBUS_HELPER_H_

#include <string>

#include <pthread.h>

#include "thirdparts/librdkafka/src/rdkafka.h"

#include "qbus_config.h"

#include "util/logger.h"
//----------------------------------------------------------------------
namespace qbus {
class QbusConfigLoader;

class QbusHelper {
 public:
  static void InitLog(LUtil::Logger::LOG_LEVEL log_level,
                      const std::string& log_path);
  static void InitLog(const std::string& log_level,
                      const std::string& log_path);
  static bool GetQbusBrokerList(const QbusConfigLoader& config_loader,
                                std::string* broker_list);
  static bool SetRdKafkaConfig(rd_kafka_conf_t* rd_kafka_conf, const char* item,
                               const char* value);
  static bool SetRdKafkaTopicConfig(rd_kafka_topic_conf_t* rd_kafka_topic_conf,
                                    const char* item, const char* value);
  static std::string FormatTopicPartitionList(
      const rd_kafka_topic_partition_list_t* partitions);
  static void RdKafkaLogger(const rd_kafka_t* rk, int level, const char* fac,
                            const char* buf);
  static void SetClientId(rd_kafka_conf_t* rd_kafka_conf,
                          bool isAppendThreadId = true);

  static long GetCurrentTimeMs();

 private:
  static LUtil::Logger::LOG_LEVEL kLogLevel;
  static std::string kLogPath;
  static bool kInitLog;

 private:
  QbusHelper();
};
}  // namespace qbus
#endif  //#ifndef QBUS_QBUS_HELPER_H_
