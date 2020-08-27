#include "qbus_helper.h"

#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#define gettid() syscall(__NR_gettid)

#include <fstream>
#include <sstream>

#include "qbus_config.h"
#include "qbus_constant.h"
#include "qbus_thread.h"
//-------------------------------------------------
namespace qbus {
namespace kafka {
LUtil::Logger::LOG_LEVEL QbusHelper::kLog_level = LUtil::Logger::LL_INFO;
pthread_once_t QbusHelper::kPthread_once = PTHREAD_ONCE_INIT;
bool QbusHelper::kInit_log = false;
static Mutex kInit_log_mutex;

void QbusHelper::initLog(const std::string& log_level, const std::string& log_path) {
    LUtil::Logger::LOG_LEVEL level = LUtil::Logger::LL_INFO;

    if (0 == strncasecmp(log_level.c_str(), "all", log_level.length())) {
        level = LUtil::Logger::LL_ALL;
    } else if (0 == strncasecmp(log_level.c_str(), "debug", log_level.length())) {
        level = LUtil::Logger::LL_DEBUG;
    } else if (0 == strncasecmp(log_level.c_str(), "info", log_level.length())) {
        level = LUtil::Logger::LL_INFO;
    } else if (0 == strncasecmp(log_level.c_str(), "warning", log_level.length())) {
        level = LUtil::Logger::LL_WARNING;
    } else if (0 == strncasecmp(log_level.c_str(), "error", log_level.length())) {
        level = LUtil::Logger::LL_ERROR;
    } else if (0 == strncasecmp(log_level.c_str(), "none", log_level.length())) {
        level = LUtil::Logger::LL_NONE;
    }

    MutexGuard kInit_log_mutex_guard(kInit_log_mutex);

    if (!kInit_log && !log_path.empty()) {
        LUtil::Logger::init(level, log_path.c_str(), false);
        kInit_log = true;
    }
}

void QbusHelper::initLog(LUtil::Logger::LOG_LEVEL log_level, const std::string& log_path) {
    MutexGuard kInit_log_mutex_guard(kInit_log_mutex);

    if (!kInit_log && !log_path.empty()) {
        LUtil::Logger::init(log_level, log_path.c_str(), false);
        kInit_log = true;
    }
}

bool QbusHelper::getQbusBrokerList(const QbusConfigLoader& config_loader, std::string* broker_list) {
    bool rt = false;

    if (NULL != broker_list) {
        // SDK configuration has higher priority than global configuration.
        *broker_list = config_loader.getSdkConfig(RD_KAFKA_SDK_CONFIG_BROKER_LIST, "");
        if ("" == *broker_list) {
            *broker_list = config_loader.getGlobalConfig(RD_KAFKA_CONFIG_BOOTSTRAP_SERVERS, "");
        }
        rt = ("" != *broker_list);
    }

    return rt;
}

bool QbusHelper::setRdKafkaConfig(rd_kafka_conf_t* rd_kafka_conf, const char* item, const char* value) {
    bool rt = false;

    if (NULL != rd_kafka_conf && NULL != item && 0 != item[0] && NULL != value && 0 != item[0]) {
        char err_str[512] = {0};

        if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(rd_kafka_conf, item, value, err_str, sizeof(err_str))) {
            ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_set | item: " << item << " | value: " << value
                               << " | error msg: " << err_str);
        } else {
            rt = true;
        }
    } else {
        ERROR(__FUNCTION__ << " | invalid parameter!"
                           << " | item: " << item << " | value: " << value);
    }

    return rt;
}

bool QbusHelper::setRdKafkaTopicConfig(rd_kafka_topic_conf_t* rd_kafka_topic_conf, const char* item,
                                       const char* value) {
    bool rt = false;

    if (NULL != rd_kafka_topic_conf && NULL != item && 0 != item[0] && NULL != value && 0 != item[0]) {
        char err_str[512] = {0};

        if (RD_KAFKA_CONF_OK !=
            rd_kafka_topic_conf_set(rd_kafka_topic_conf, item, value, err_str, sizeof(err_str))) {
            ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_conf_set | item: " << item
                               << " | value: " << value << " | error msg: " << err_str);
        } else {
            rt = true;
        }
    } else {
        ERROR(__FUNCTION__ << " | invalid parameter!");
    }

    return rt;
}

std::string QbusHelper::formatTopicPartitionList(const rd_kafka_topic_partition_list_t* partitions) {
    std::string rt("");

    if (NULL != partitions) {
        std::stringstream ss;
        for (int i = 0; i < partitions->cnt; ++i) {
            ss << partitions->elems[i].topic << "[" << partitions->elems[i].partition << "] |";
        }

        rt = ss.str();
        ss.clear();
    }

    return rt;
}

void QbusHelper::rdKafkaLogger(const rd_kafka_t* rk, int level, const char* fac, const char* buf) {
    time_t timep;
    time(&timep);

    std::ofstream out;
    out.open("./rdkafka.log", std::ios::out | std::ios::app);
    out << asctime(gmtime(&timep)) << " | RD_KAFKA_LOG"
        << " | level: " << level << " | fac: " << (NULL != fac ? fac : "")
        << " | msg: " << (NULL != buf ? buf : "") << std::endl;
    out.close();
}

void QbusHelper::setClientId(rd_kafka_conf_t* rd_kafka_conf, bool isAppendThreadId) {
    if (isAppendThreadId) {
        char clientId[100];

        time_t tt = time(NULL);
        tm* t = localtime(&tt);
        if (NULL != t) {
            snprintf(clientId, sizeof(clientId), "%s_%ld_%d-%02d-%02d-%02d-%02d-%02d",
                     RD_KAFKA_CONFIG_CLIENT_ID_VALUE, gettid(), t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                     t->tm_hour, t->tm_min, t->tm_sec);
        } else {
            snprintf(clientId, sizeof(clientId), "%s_%ld", RD_KAFKA_CONFIG_CLIENT_ID_VALUE, gettid());
        }
        setRdKafkaConfig(rd_kafka_conf, RD_KAFKA_CONFIG_CLIENT_ID, clientId);
    } else {
        setRdKafkaConfig(rd_kafka_conf, RD_KAFKA_CONFIG_CLIENT_ID, RD_KAFKA_CONFIG_CLIENT_ID_VALUE);
    }
}

long QbusHelper::getCurrentTimeMs() {
    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    return ((long)now_time.tv_sec) * 1000 + (long)now_time.tv_usec / 1000;
}

}  // namespace kafka
}  // namespace qbus
