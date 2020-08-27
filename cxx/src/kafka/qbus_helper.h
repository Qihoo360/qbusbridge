#pragma once

#include <string>

#include <pthread.h>

#include "qbus_config.h"

#include "util/logger.h"
//----------------------------------------------------------------------
namespace qbus {
namespace kafka {
class QbusConfigLoader;

class QbusHelper {
   public:
    static void initLog(LUtil::Logger::LOG_LEVEL log_level, const std::string& log_path);
    static void initLog(const std::string& log_level, const std::string& log_path);
    static bool getQbusBrokerList(const QbusConfigLoader& config_loader, std::string* broker_list);
    static bool setRdKafkaConfig(rd_kafka_conf_t* rd_kafka_conf, const char* item, const char* value);
    static bool setRdKafkaTopicConfig(rd_kafka_topic_conf_t* rd_kafka_topic_conf, const char* item,
                                      const char* value);
    static std::string formatTopicPartitionList(const rd_kafka_topic_partition_list_t* partitions);
    static void rdKafkaLogger(const rd_kafka_t* rk, int level, const char* fac, const char* buf);
    static void setClientId(rd_kafka_conf_t* rd_kafka_conf, bool isAppendThreadId = true);

    static long getCurrentTimeMs();

   private:
    static LUtil::Logger::LOG_LEVEL kLog_level;
    static std::string kLog_path;
    static pthread_once_t kPthread_once;
    static bool kInit_log;

   private:
    QbusHelper();
};
}  // namespace kafka
}  // namespace qbus
