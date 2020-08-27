#include "qbus_producer.h"

#include <signal.h>
#include <string.h>

#include <set>
#include <map>
#include <iostream>

#include "qbus_constant.h"
#include "qbus_helper.h"
#include "qbus_config.h"
#include "qbus_record_msg.h"
#include "qbus_producer_imp.h"
#include "qbus_producer_imp_map.h"
#include "qbus_rdkafka.h"
#include "qbus_thread.h"
#include "util/logger.h"
//----------------------------------------------------------------------
namespace qbus {
namespace kafka {

#ifdef NOT_USE_CONSUMER_CALLBACK
typedef std::map<std::string, QbusProducerImp*> BUFFER;

static Mutex kRmtx;
static BUFFER* kRmb = QbusProducerImpMap::instance();  // added by zk
#endif
//----------------------------------------------------------------------
QbusProducerImp::QbusProducerImp()
    : rd_kafka_conf_(NULL),
      rd_kafka_topic_conf_(NULL),
      rd_kafka_topic_(NULL),
      rd_kafka_handle_(NULL),
      sync_send_err_(RD_KAFKA_RESP_ERR_NO_ERROR),
      broker_list_(""),
      is_sync_send_(false),
      is_init_(false),
      is_record_msg_for_send_failed_(false),
      is_speedup_terminate_(false),
      fast_exit_(false),
      flush_timeout_ms_(RD_KAFKA_SDK_CONFIG_FLUSH_TIMEOUT_MS_DEFAULT) {}

QbusProducerImp::~QbusProducerImp() {}

bool QbusProducerImp::init(const std::string& cluster_name, const std::string& log_path,
                           const std::string& topic_name, const std::string& config_path) {
    std::string errstr;
    bool load_config_ok = config_loader_.loadConfig(config_path, errstr);

    QbusHelper::initLog(
        config_loader_.getSdkConfig(RD_KAFKA_SDK_CONFIG_LOG_LEVEL, RD_KAFKA_SDK_CONFIG_LOG_LEVEL_DEFAULT),
        log_path);

    if (!load_config_ok) {
        ERROR(__FUNCTION__ << " | loadConfig failed: " << errstr);
        return false;
    }

    INFO(__FUNCTION__ << " | Start init | qbus cluster: " << cluster_name << " | topic: " << topic_name
                      << " | config: " << config_path);
    if (cluster_name.find(":") == std::string::npos) {
        // fetch broker list from config
        is_init_ = QbusHelper::getQbusBrokerList(config_loader_, &broker_list_) && initRdKafkaConfig() &&
                   initRdKafkaHandle(topic_name);
    } else {
        broker_list_ = cluster_name;
        is_init_ = initRdKafkaConfig() && initRdKafkaHandle(topic_name);
    }

    INFO(__FUNCTION__ << " | broker list:" << broker_list_);

    if (is_init_) {
        std::string is_record_msg = config_loader_.getSdkConfig(RD_KAFKA_SDK_CONFIG_RECORD_MSG,
                                                                RD_KAFKA_SDK_CONFIG_RECORD_MSG_DEFAULT);
        if (0 == strncasecmp(is_record_msg.c_str(), "true", is_record_msg.length())) {
            is_record_msg_for_send_failed_ = true;
        }
    }

    return is_init_;
}

void QbusProducerImp::uninit() {
    is_init_ = false;

    INFO(__FUNCTION__ << " | Starting uninit...");

    if (rd_kafka_topic_) {
        rd_kafka_topic_destroy(rd_kafka_topic_);
        rd_kafka_topic_ = NULL;
    }

    if (rd_kafka_handle_) {
        rd_kafka_resp_err_t err = rd_kafka_flush(rd_kafka_handle_, flush_timeout_ms_);
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
            WARNING(__FUNCTION__ << " | rd_kafka_flush: " << rd_kafka_err2str(err));
        }
        rd_kafka_destroy(rd_kafka_handle_);
        rd_kafka_handle_ = NULL;
    }

    INFO(__FUNCTION__ << " | Finished uninit");
}

bool QbusProducerImp::internalProduce(const char* data, size_t data_len, const std::string& key,
                                      void* opaque) {
    bool rt = false;

    sync_send_err_ = (rd_kafka_resp_err_t)RD_KAFKA_PRODUCE_ERROR_INIT_VALUE;

    if (NULL == rd_kafka_handle_ ||
        -1 == rd_kafka_produce(rd_kafka_topic_, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
                               static_cast<void*>(const_cast<char*>(data)), data_len,
                               key.length() > 0 ? key.c_str() : NULL, key.length(), opaque)) {
        ERROR(__FUNCTION__ << " | Failed to produce"
                           << " | topic: " << rd_kafka_topic_name(rd_kafka_topic_) << " | partition: " << -1
                           << " | msg error: " << rd_kafka_err2str(rd_kafka_last_error()));
    } else if (is_sync_send_) {
        DEBUG(__FUNCTION__ << " | sync send msg");
        int current_retry_time = 0;
        while (RD_KAFKA_PRODUCE_ERROR_INIT_VALUE == sync_send_err_ &&
               current_retry_time++ < RD_KAFKA_SYNC_SEND_POLL_TIME) {
            rd_kafka_poll(rd_kafka_handle_, RD_KAFKA_PRODUCE_SYNC_SEND_POLL_TIMEOUT_MS);
        }

        if (RD_KAFKA_RESP_ERR_NO_ERROR != sync_send_err_) {
            ERROR(__FUNCTION__ << " | Failed to sync produce"
                               << " | topic: " << rd_kafka_topic_name(rd_kafka_topic_) << " | partition: "
                               << -1 << " | msg error: " << rd_kafka_err2str(sync_send_err_));
        } else {
            rt = true;
        }
    } else {
        rd_kafka_poll(rd_kafka_handle_, 0);
        rt = true;
    }

    return rt;
}

bool QbusProducerImp::produce(const char* data, size_t data_len, const std::string& key) {
    bool rt = false;

    if (is_init_) {
        rt = internalProduce(data, data_len, key, 0);
    }

    return rt;
}

bool QbusProducerImp::initRdKafkaHandle(const std::string& topic_name) {
    bool rt = false;

    char err_str[512] = {0};
    rd_kafka_handle_ = rd_kafka_new(RD_KAFKA_PRODUCER, rd_kafka_conf_, err_str, sizeof(err_str));
    if (NULL == rd_kafka_handle_) {
        ERROR(__FUNCTION__ << " | Failed to create new producer | error msg:" << err_str);
    } else if (0 == rd_kafka_brokers_add(rd_kafka_handle_, broker_list_.c_str())) {
        ERROR(__FUNCTION__ << " | Failed to rd_kafka_broker_add | broker list:" << broker_list_);
    } else {
        rd_kafka_topic_ = rd_kafka_topic_new(rd_kafka_handle_, topic_name.c_str(), rd_kafka_topic_conf_);
        if (NULL == rd_kafka_topic_) {
            ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_new");
        } else {
            rt = true;
        }
    }

    return rt;
}

void QbusProducerImp::msgDeliveredCallback(rd_kafka_t* rk, const rd_kafka_message_t* rkmessage,
                                           void* opaque) {
    QbusProducerImp* producer = static_cast<QbusProducerImp*>(opaque);
    if (NULL == producer) {
        ERROR(__FUNCTION__ << " | Failed to static_cast from opaque to QbusProducerImp");
        return;
    }

    rd_kafka_topic_t* rkt = rkmessage->rkt;
    const char* topic_name = rd_kafka_topic_name(rkt);

    if (producer->is_sync_send_) {
        producer->sync_send_err_ = rkmessage->err;
    } else if (rkmessage->err && producer->is_init_) {
        if (NULL == producer->rd_kafka_handle_ ||
            -1 == rd_kafka_produce(rkt, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY, rkmessage->payload,
                                   rkmessage->len, rkmessage->key, rkmessage->key_len, producer)) {
            ERROR(__FUNCTION__ << " | Failed to reproduce"
                               << " | topic: " << topic_name
                               << " | msg error: " << rd_kafka_err2str(rd_kafka_last_error()));
        }
    }

    char* payload = static_cast<char*>(rkmessage->payload);
    if (rkmessage->err) {
        ERROR(__FUNCTION__ << " | Failed to delivery message | err msg:" << rd_kafka_err2str(rkmessage->err)
                           << " | topic: " << topic_name << " | partition: " << rkmessage->partition
                           << " | msg: " << std::string(payload, rkmessage->len));
        if (producer->is_record_msg_for_send_failed_) {
            QbusRecordMsg::recordMsg(topic_name, std::string(payload, rkmessage->len));
        }
    } else {
        DEBUG(__FUNCTION__ << " Successed to delivery message | bytes: " << rkmessage->len
                           << " | offset: " << rkmessage->offset << " | partition: " << rkmessage->partition
                           << " | msg: " << std::string(payload, rkmessage->len)
                           << " | record failed msg: " << producer->is_record_msg_for_send_failed_);
    }
}

int32_t QbusProducerImp::partitionHashFunc(const rd_kafka_topic_t* rkt, const void* keydata, size_t keylen,
                                           int32_t partition_cnt, void* rkt_opaque, void* msg_opaque) {
    int32_t hit_partition = 0;

    if (keylen > 0 && NULL != keydata) {
        const char* key = static_cast<const char*>(keydata);
        DEBUG(__FUNCTION__ << " | KEY: " << std::string(key, keylen));

        // use djb hash
        unsigned int hash = 5381;
        for (size_t i = 0; i < keylen; i++) {
            hash = ((hash << 5) + hash) + key[i];
        }

        hit_partition = hash % partition_cnt;

        if (1 != rd_kafka_topic_partition_available(rkt, hit_partition)) {
            DEBUG(__FUNCTION__ << " | retry select partition | current invalid partition: " << hit_partition);
            //! LW!此时hash出来的partition是无效的,保险起见只能选择partition0
            hit_partition = 0;
        }
    } else {
        std::set<int32_t> partition_set;
        for (int32_t i = 0; i < partition_cnt; ++i) {
            partition_set.insert(i);
        }

        while (true) {
            hit_partition =
                rd_kafka_msg_partitioner_random(rkt, keydata, keylen, partition_cnt, rkt_opaque, msg_opaque);
            if (1 == rd_kafka_topic_partition_available(rkt, hit_partition)) {
                break;
            } else {
                DEBUG(__FUNCTION__ << " | retry select partition | current invalid partition: "
                                   << hit_partition);
                partition_set.erase(hit_partition);
                if (partition_set.empty()) {
                    DEBUG(__FUNCTION__ << " | failed to select partition | use RD_KAFKA_PARTITION_UA!");
                    hit_partition = rd_kafka_msg_partitioner_random(rkt, keydata, keylen, partition_cnt,
                                                                    rkt_opaque, msg_opaque);
                    ;
                    break;
                }
            }
        }
    }

    DEBUG(__FUNCTION__ << " | hit_partition:" << hit_partition);

    return hit_partition;
}

bool QbusProducerImp::initRdKafkaConfig() {
    INFO(__FUNCTION__ << " | Librdkafka version: " << rd_kafka_version_str() << " " << rd_kafka_version());

    bool rt = false;

    rd_kafka_conf_ = rd_kafka_conf_new();
    if (NULL != rd_kafka_conf_) {
        char tmp[16] = {0};
        snprintf(tmp, sizeof(tmp), "%i", SIGIO);
        QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_INTERNAL_TERMINATION_SIGNAL, tmp);

        rd_kafka_conf_set_opaque(rd_kafka_conf_, static_cast<void*>(this));

        rd_kafka_topic_conf_ = rd_kafka_topic_conf_new();
        if (NULL == rd_kafka_topic_conf_) {
            ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_conf_new");
        } else {
            config_loader_.loadRdkafkaConfig(rd_kafka_conf_, rd_kafka_topic_conf_);

            std::string enable_rdkafka_log = config_loader_.getSdkConfig(
                RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG, RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG_DEFAULT);
            if (0 == strncasecmp(enable_rdkafka_log.c_str(), RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG_DEFAULT,
                                 enable_rdkafka_log.length())) {
                rd_kafka_conf_set_log_cb(rd_kafka_conf_, NULL);
            } else {
                rd_kafka_conf_set_log_cb(rd_kafka_conf_, &QbusHelper::rdKafkaLogger);
            }

            rd_kafka_conf_set_dr_msg_cb(rd_kafka_conf_, msgDeliveredCallback);

            rd_kafka_topic_conf_set_partitioner_cb(rd_kafka_topic_conf_, &QbusProducerImp::partitionHashFunc);

            if (!config_loader_.isSetConfig(RD_KAFKA_TOPIC_MESSAGE_TIMEOUT, true)) {
                QbusHelper::setRdKafkaTopicConfig(rd_kafka_topic_conf_, RD_KAFKA_TOPIC_MESSAGE_TIMEOUT,
                                                  RD_KAFKA_TOPIC_MESSAGE_TIMEOUT_MS);
            }

            if (!config_loader_.isSetConfig(RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL, false)) {
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL,
                                             RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL_MS);
            }

            std::string speedup_terminate =
                config_loader_.getSdkConfig(RD_KAFKA_SDK_CONFIG_PRODUCER_SPEED_UP_TERMINATE,
                                            RD_KAFKA_SDK_CONFIG_PRODUCER_SPEED_UP_TERMINATE_DEFAULT);
            if (0 != strncasecmp(speedup_terminate.c_str(),
                                 RD_KAFKA_SDK_CONFIG_PRODUCER_SPEED_UP_TERMINATE_DEFAULT,
                                 speedup_terminate.length())) {
                is_speedup_terminate_ = true;
            }

            std::string sync_send = config_loader_.getSdkConfig(RD_KAFKA_SDK_CONFIG_SYNC_SEND,
                                                                RD_KAFKA_SDK_CONFIG_VALUE_SYNC_SEND_DEFAULT);
            if (0 ==
                strncasecmp(sync_send.c_str(), RD_KAFKA_SDK_CONFIG_VALUE_SYNC_SEND, sync_send.length())) {
                is_sync_send_ = true;
                if (!config_loader_.isSetConfig(RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL, false)) {
                    QbusHelper::setRdKafkaConfig(
                        rd_kafka_conf_, RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL,
                        RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL_WHEN_SYNC_SEND_MS);
                }
            }

            if (is_sync_send_) {
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_QUEUE_BUFFERING_MAX_MS,
                                             RD_KAFKA_CONFIG_QUEUE_BUFFERING_SYNC);
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_SOCKET_BLOKING_MAX_MX,
                                             RD_KAFKA_SDK_MINIMIZE_PRODUCER_LATENCY_VALUE);
            }

            std::string minimize_producer_latency =
                config_loader_.getSdkConfig(RD_KAFKA_SDK_CONFIG_PRODUCER_ENABLE_MINI_LATENCY,
                                            RD_KAFKA_SDK_CONFIG_PRODUCER_ENABLE_MINI_LATENCY_DEFAULT);
            if (0 != strncasecmp(minimize_producer_latency.c_str(),
                                 RD_KAFKA_SDK_CONFIG_PRODUCER_ENABLE_MINI_LATENCY_DEFAULT,
                                 minimize_producer_latency.length())) {
                DEBUG(__FUNCTION__ << " | enable minimize producer latency");
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_QUEUE_BUFFERING_MAX_MS,
                                             RD_KAFKA_SDK_MINIMIZE_PRODUCER_LATENCY_VALUE);
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_SOCKET_BLOKING_MAX_MX,
                                             RD_KAFKA_SDK_MINIMIZE_PRODUCER_LATENCY_VALUE);
            }

            std::string fast_exit = config_loader_.getSdkConfig(RD_KAFKA_SDK_PRODUCER_HA_FASET_EXIT,
                                                                RD_KAFKA_SDK_PRODUCER_HA_FASET_EXIT_DEFAULT);
            if (0 != strncasecmp(fast_exit.c_str(), RD_KAFKA_SDK_PRODUCER_HA_FASET_EXIT_DEFAULT,
                                 fast_exit.length())) {
                fast_exit_ = true;
            }

            if (!config_loader_.isSetConfig(RD_KAFKA_CONFIG_API_VERSION_REQUEST, false)) {
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_API_VERSION_REQUEST,
                                             RD_KAFKA_CONFIG_API_VERSION_REQUEST_FOR_OLDER);
                QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_BROKER_VERSION_FALLBACK,
                                             RD_KAFKA_CONFIG_BROKER_VERSION_FALLBACK_FOR_OLDER);
            }

            std::string flush_timeout_ms_str = config_loader_.getSdkConfig(
                RD_KAFKA_SDK_CONFIG_FLUSH_TIMEOUT_MS, RD_KAFKA_SDK_CONFIG_FLUSH_TIMEOUT_MS_DEFAULT_STR);
            flush_timeout_ms_ = static_cast<int>(strtol(flush_timeout_ms_str.c_str(), NULL, 10));
            if (flush_timeout_ms_ < 0) {
                WARNING(RD_KAFKA_SDK_CONFIG_FLUSH_TIMEOUT_MS << " is " << flush_timeout_ms_str
                                                             << " (< 0), set it to 0");
                flush_timeout_ms_ = 0;
            }

            rt = true;
        }
    } else {
        ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_new");
    }

    return rt;
}
//-----------------------------------------------------------------------    modified by zk
QbusProducer::QbusProducer() {
#ifndef NOT_USE_CONSUMER_CALLBACK
    qbus_producer_imp_ = new QbusProducerImp();
#endif
}

QbusProducer::~QbusProducer() {
#ifndef NOT_USE_CONSUMER_CALLBACK
    if (NULL != qbus_producer_imp_) {
        delete qbus_producer_imp_;
        qbus_producer_imp_ = NULL;
    }
#endif
}

bool QbusProducer::init(const std::string& cluster_name, const std::string& log_path,
                        const std::string& config_path, const std::string& topic_name) {
    bool rt = false;

#ifdef NOT_USE_CONSUMER_CALLBACK
    MutexGuard kRmtx_guard(kRmtx);

    std::string key = cluster_name + topic_name + config_path;
    BUFFER::iterator i = kRmb->find(key);
    if (i == kRmb->end()) {
        qbus_producer_imp_ = new QbusProducerImp();
        if (NULL != qbus_producer_imp_) {
            rt = qbus_producer_imp_->init(cluster_name, log_path, topic_name, config_path);
            if (rt) {
                kRmb->insert(BUFFER::value_type(key, qbus_producer_imp_));
                INFO(__FUNCTION__ << " | Producer init is OK!");
            } else {
                delete qbus_producer_imp_;
                ERROR(__FUNCTION__ << " | Failed to init");
            }
        }
    } else {
        qbus_producer_imp_ = i->second;

        rt = true;
    }
#else
    if (NULL != qbus_producer_imp_) {
        rt = qbus_producer_imp_->init(cluster_name, log_path, topic_name, config_path);
        if (rt) {
            INFO(__FUNCTION__ << " | Producer init is OK!");
        } else {
            ERROR(__FUNCTION__ << " | Failed to init");
        }
    }

#endif
    return rt;
}

void QbusProducer::uninit() {
#ifndef NOT_USE_CONSUMER_CALLBACK
    if (NULL != qbus_producer_imp_) {
        qbus_producer_imp_->uninit();
    }
#endif
}

bool QbusProducer::produce(const char* data, size_t data_len, const std::string& key) {
    bool rt = true;

    if (NULL != data && data_len > 0 && NULL != qbus_producer_imp_) {
        DEBUG(__FUNCTION__ << " | msg: " << std::string(data, data_len) << " | key: " << key);
        rt = qbus_producer_imp_->produce(data, data_len, key);
    } else {
        ERROR(__FUNCTION__ << " | Failed to produce | data is null: " << (NULL != data)
                           << " | data len: " << data_len);
    }

    return rt;
}

}  // namespace kafka
}  // namespace qbus
