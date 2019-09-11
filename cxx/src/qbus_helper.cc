#include "qbus_helper.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define gettid() syscall(__NR_gettid)

#include <fstream>
#include <sstream>

#include "qbus_config.h"
#include "qbus_constant.h"
//-------------------------------------------------
namespace qbus {

static const char kRdkafkaLog[] = "./rdkafka.log";

LUtil::Logger::LOG_LEVEL QbusHelper::kLogLevel = LUtil::Logger::LL_INFO;
bool QbusHelper::kInitLog = false;
static pthread_mutex_t kInitLogMutex = PTHREAD_MUTEX_INITIALIZER;

void QbusHelper::InitLog(const std::string& log_level,
                         const std::string& log_path) {
  LUtil::Logger::LOG_LEVEL level = LUtil::Logger::LL_INFO;

  if (0 == strncasecmp(log_level.c_str(), "all", log_level.length())) {
    level = LUtil::Logger::LL_ALL;
  } else if (0 == strncasecmp(log_level.c_str(), "debug", log_level.length())) {
    level = LUtil::Logger::LL_DEBUG;
  } else if (0 == strncasecmp(log_level.c_str(), "info", log_level.length())) {
    level = LUtil::Logger::LL_INFO;
  } else if (0 ==
             strncasecmp(log_level.c_str(), "warning", log_level.length())) {
    level = LUtil::Logger::LL_WARNING;
  } else if (0 == strncasecmp(log_level.c_str(), "error", log_level.length())) {
    level = LUtil::Logger::LL_ERROR;
  } else if (0 == strncasecmp(log_level.c_str(), "none", log_level.length())) {
    level = LUtil::Logger::LL_NONE;
  }

  pthread_mutex_lock(&kInitLogMutex);

  if (!kInitLog && !log_path.empty()) {
    LUtil::Logger::init(level, log_path.c_str(), false);
    kInitLog = true;
  }

  pthread_mutex_unlock(&kInitLogMutex);
}

void QbusHelper::InitLog(LUtil::Logger::LOG_LEVEL log_level,
                         const std::string& log_path) {
  pthread_mutex_lock(&kInitLogMutex);

  if (!kInitLog && !log_path.empty()) {
    LUtil::Logger::init(log_level, log_path.c_str(), false);
    kInitLog = true;
  }

  pthread_mutex_unlock(&kInitLogMutex);
}

bool QbusHelper::GetQbusBrokerList(const QbusConfigLoader& config_loader,
                                   std::string* broker_list) {
  if (!broker_list) return false;

  // Priority:
  // 1. User provided non-empty *broker_list
  // 2. [sdk] configured broker.list
  // 3. [global] configured bootstrap.servers
  if (broker_list->empty()) {
    *broker_list =
        config_loader.GetSdkConfig(RD_KAFKA_SDK_CONFIG_BROKER_LIST, "");
  }
  if (broker_list->empty()) {
    *broker_list =
        config_loader.GetGlobalConfig(RD_KAFKA_CONFIG_BOOTSTRAP_SERVERS, "");
  }

  return !broker_list->empty();
}

bool QbusHelper::GetGroupId(const QbusConfigLoader& config_loader,
                            std::string* group) {
  if (!group) return false;

  // Priority:
  // 1. User provided *group
  // 2. [global] configured group.id
  if (group->empty()) {
    *group = config_loader.GetGlobalConfig(RD_KAFKA_CONFIG_GROUP_ID, "");
  }

  return !group->empty();
}

bool QbusHelper::SetRdKafkaConfig(rd_kafka_conf_t* rd_kafka_conf,
                                  const char* item, const char* value) {
  bool rt = false;

  if (NULL != rd_kafka_conf && NULL != item && 0 != item[0] && NULL != value &&
      0 != item[0]) {
    char err_str[512] = {0};

    if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(rd_kafka_conf, item, value,
                                              err_str, sizeof(err_str))) {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_set | item: " << item
                         << " | value: " << value
                         << " | error msg: " << err_str);
    } else {
      rt = true;
    }
  } else {
    ERROR(__FUNCTION__ << " | invailed parameter!"
                       << " | item: " << item << " | value: " << value);
  }

  return rt;
}

bool QbusHelper::SetRdKafkaTopicConfig(
    rd_kafka_topic_conf_t* rd_kafka_topic_conf, const char* item,
    const char* value) {
  bool rt = false;

  if (NULL != rd_kafka_topic_conf && NULL != item && 0 != item[0] &&
      NULL != value && 0 != item[0]) {
    char err_str[512] = {0};

    if (RD_KAFKA_CONF_OK != rd_kafka_topic_conf_set(rd_kafka_topic_conf, item,
                                                    value, err_str,
                                                    sizeof(err_str))) {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_conf_set | item: "
                         << item << " | value: " << value
                         << " | error msg: " << err_str);
    } else {
      rt = true;
    }
  } else {
    ERROR(__FUNCTION__ << " | invailed parameter!");
  }

  return rt;
}

std::string QbusHelper::FormatTopicPartitionList(
    const rd_kafka_topic_partition_list_t* partitions) {
  std::string rt("");

  if (NULL != partitions) {
    std::stringstream ss;
    for (int i = 0; i < partitions->cnt; ++i) {
      ss << partitions->elems[i].topic << "[" << partitions->elems[i].partition
         << "] |";
    }

    rt = ss.str();
    ss.clear();
  }

  return rt;
}

void QbusHelper::RdKafkaLogger(const rd_kafka_t* rk, int level, const char* fac,
                               const char* buf) {
  time_t timep;
  time(&timep);

  std::ofstream out;
  out.open(kRdkafkaLog, std::ios::out | std::ios::app);
  out << asctime(gmtime(&timep)) << " | RD_KAFKA_LOG"
      << " | level: " << level << " | fac: " << (NULL != fac ? fac : "")
      << " | msg: " << (NULL != buf ? buf : "") << std::endl;
  out.close();
}

void QbusHelper::SetClientId(rd_kafka_conf_t* rd_kafka_conf,
                             bool isAppendThreadId) {
  if (isAppendThreadId) {
    char clientId[100];

    time_t tt = time(NULL);
    tm* t = localtime(&tt);
    if (NULL != t) {
      snprintf(clientId, sizeof(clientId), "%s_%ld_%d-%02d-%02d-%02d-%02d-%02d",
               RD_KAFKA_CONFIG_CLIENT_ID_VALUE, gettid(), t->tm_year + 1900,
               t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    } else {
      snprintf(clientId, sizeof(clientId), "%s_%ld",
               RD_KAFKA_CONFIG_CLIENT_ID_VALUE, gettid());
    }
    SetRdKafkaConfig(rd_kafka_conf, RD_KAFKA_CONFIG_CLIENT_ID, clientId);
  } else {
    SetRdKafkaConfig(rd_kafka_conf, RD_KAFKA_CONFIG_CLIENT_ID,
                     RD_KAFKA_CONFIG_CLIENT_ID_VALUE);
  }
}

long QbusHelper::GetCurrentTimeMs() {
  struct timeval now_time;
  gettimeofday(&now_time, NULL);
  return ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;
}

std::string QbusHelper::FormatStringVector(
    const std::vector<std::string>& strings) {
  std::ostringstream oss;
  for (size_t i = 0; i < strings.size(); i++) {
    if (i > 0) oss << ",";
    oss << strings[i];
  }
  return oss.str();
}

}  // namespace qbus
