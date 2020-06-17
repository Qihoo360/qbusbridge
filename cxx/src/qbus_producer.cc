#include "qbus_producer.h"

#include <string.h>

#include <iostream>
#include <map>
#include <set>

#include "util/logger.h"

#include "qbus_config.h"
#include "qbus_constant.h"
#include "qbus_helper.h"
#include "qbus_producer_imp.h"
#include "qbus_producer_imp_map.h"
#include "qbus_rdkafka.h"
#include "qbus_record_msg.h"
//----------------------------------------------------------------------
namespace qbus {

#ifdef NOT_USE_CONSUMER_CALLBACK
typedef std::map<std::string, QbusProducerImp*> BUFFER;

static pthread_mutex_t kRmtx = PTHREAD_MUTEX_INITIALIZER;
static QbusProducerImpMap::DataType* kRmb = QbusProducerImpMap::instance();
#endif
//----------------------------------------------------------------------
QbusProducerImp::QbusProducerImp()
    : rd_kafka_conf_(NULL),
      rd_kafka_topic_conf_(NULL),
      rd_kafka_topic_(NULL),
      rd_kafka_handle_(NULL),
      sync_send_err_(RD_KAFKA_RESP_ERR_NO_ERROR),
      is_brokers_all_down_(false),
      broker_list_(""),
      is_sync_send_(false),
      is_init_(false),
      is_record_msg_for_send_failed_(false) {}

QbusProducerImp::~QbusProducerImp() {}

bool QbusProducerImp::Init(const std::string& broker_list,
                           const std::string& log_path,
                           const std::string& topic_name,
                           const std::string& config_path) {
  std::string errstr;
  bool load_config_ok = config_loader_.LoadConfig(config_path, errstr);

  QbusHelper::InitLog(
      config_loader_.GetSdkConfig(RD_KAFKA_SDK_CONFIG_LOG_LEVEL,
                                  RD_KAFKA_SDK_CONFIG_LOG_LEVEL_DEFAULT),
      log_path);

  if (!load_config_ok) {
    ERROR(__FUNCTION__ << " | LoadConfig failed: " << errstr);
    return false;
  }

  broker_list_ = broker_list;
  is_init_ = QbusHelper::GetQbusBrokerList(config_loader_, &broker_list_) &&
             InitRdKafkaConfig() && InitRdKafkaHandle(topic_name);
  INFO(__FUNCTION__ << " | Start init | qbus cluster: " << broker_list_
                    << " | topic: " << topic_name
                    << " | config: " << config_path);

  if (is_init_) {
    std::string is_record_msg = config_loader_.GetSdkConfig(
        RD_KAFKA_SDK_CONFIG_RECORD_MSG, RD_KAFKA_SDK_CONFIG_RECORD_MSG_DEFAULT);
    if (0 ==
        strncasecmp(is_record_msg.c_str(), "true", is_record_msg.length())) {
      is_record_msg_for_send_failed_ = true;
    }
  }

  return is_init_;
}

void QbusProducerImp::Uninit() {
  is_init_ = false;

  INFO(__FUNCTION__ << " | Starting uninit...");

  if (NULL != rd_kafka_handle_ && NULL != rd_kafka_topic_) {
    int current_poll_time = 0;
    while (rd_kafka_outq_len(rd_kafka_handle_) > 0) {
      rd_kafka_poll(rd_kafka_handle_, RD_KAFKA_POLL_TIMIE_OUT_MS);
      if (is_sync_send_ &&
          current_poll_time++ >= RD_KAFKA_SYNC_SEND_UINIT_POLL_TIME) {
        break;
      }
    }

    rd_kafka_topic_destroy(rd_kafka_topic_);
    rd_kafka_topic_ = NULL;

    rd_kafka_destroy(rd_kafka_handle_);
    rd_kafka_handle_ = NULL;
  }

  INFO(__FUNCTION__ << " | Finished uninit");
}

bool QbusProducerImp::AsyncProduce(const void* payload, size_t len,
                                   const void* key, size_t keylen) const {
  assert(rd_kafka_topic_);

  // We use RD_KAFKA_MSG_F_COPY, so it's safe to remove const qualifier
  return rd_kafka_produce(rd_kafka_topic_, RD_KAFKA_PARTITION_UA,
                          RD_KAFKA_MSG_F_COPY, const_cast<void*>(payload), len,
                          key, keylen, NULL) == 0;
}

bool QbusProducerImp::InternalProduce(const char* data, size_t data_len,
                                      const std::string& key) {
  assert(rd_kafka_handle_ && rd_kafka_topic_);

  // Poll at frequent intervals to serve following callbacks
  // - MsgDeliveredCallback
  // - StatsCallback
  // - ErrorCallback
  // `is_brokers_all_down_` may be set to false in StatsCallback and may be set
  // to true in ErrorCallback.
  // Also, if produce queue is full, rd_kafka_produce() will failed with
  // ERR__QUEUE_FULL, rd_kafka_poll() may retrieve events from it.
  rd_kafka_poll(rd_kafka_handle_, 0);

  if (is_brokers_all_down_) {
    ERROR(__FUNCTION__ << " | Failed: all broker connections are down");
    return false;
  }

  bool rt = false;

  sync_send_err_ = (rd_kafka_resp_err_t)RD_KAFKA_PRODUCE_ERROR_INIT_VALUE;

  const char* key_ptr = (key.empty() ? NULL : key.c_str());
  if (!AsyncProduce(data, data_len, key_ptr, key.length())) {
    ERROR(__FUNCTION__ << " | Failed to produce"
                       << " | topic: " << rd_kafka_topic_name(rd_kafka_topic_)
                       << " | error: "
                       << rd_kafka_err2str(rd_kafka_last_error()));
  } else if (is_sync_send_) {
    DEBUG(__FUNCTION__ << " | sync send msg");
    int current_retry_time = 0;
    while (RD_KAFKA_PRODUCE_ERROR_INIT_VALUE == sync_send_err_ &&
           current_retry_time++ < RD_KAFKA_SYNC_SEND_POLL_TIME) {
      rd_kafka_poll(rd_kafka_handle_,
                    RD_KAFKA_PRODUCE_SYNC_SEND_POLL_TIMEOUT_MS);
    }

    // We have logged error info in MsgDeliveredCallback() before.
    rt = (sync_send_err_ == RD_KAFKA_RESP_ERR_NO_ERROR);
  } else {
    rd_kafka_poll(rd_kafka_handle_, 0);
    rt = true;
  }

  return rt;
}

bool QbusProducerImp::Produce(const char* data, size_t data_len,
                              const std::string& key) {
  if (!is_init_ || !rd_kafka_handle_ || !rd_kafka_topic_) {
    ERROR(__FUNCTION__ << " | Producer not initialized");
    return false;
  }

  assert(data);
  bool rt = InternalProduce(data, data_len, key);

  if (!rt && is_record_msg_for_send_failed_) {
    QbusRecordMsg::recordMsg(rd_kafka_topic_name(rd_kafka_topic_),
                             std::string(data, data_len));
  }

  return rt;
}

bool QbusProducerImp::InitRdKafkaHandle(const std::string& topic_name) {
  bool rt = false;

  char err_str[512] = {0};
  rd_kafka_handle_ =
      rd_kafka_new(RD_KAFKA_PRODUCER, rd_kafka_conf_, err_str, sizeof(err_str));
  if (NULL == rd_kafka_handle_) {
    ERROR(__FUNCTION__ << " | Failed to create new producer | error msg:"
                       << err_str);
  } else if (0 ==
             rd_kafka_brokers_add(rd_kafka_handle_, broker_list_.c_str())) {
    ERROR(__FUNCTION__ << " | Failed to rd_kafka_broker_add | broker list:"
                       << broker_list_);
  } else {
    rd_kafka_topic_ = rd_kafka_topic_new(rd_kafka_handle_, topic_name.c_str(),
                                         rd_kafka_topic_conf_);
    if (NULL == rd_kafka_topic_) {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_new");
    } else {
      rt = true;
    }
  }
  return rt;
}

void QbusProducerImp::MsgDeliveredCallback(rd_kafka_t* rk,
                                           const rd_kafka_message_t* rkmessage,
                                           void* opaque) {
  assert(opaque);
  QbusProducerImp& producer = *static_cast<QbusProducerImp*>(opaque);
  rdkafka::MessageRef msg_ref(*rkmessage);

  if (msg_ref.hasError()) {
    ERROR(__FUNCTION__ << " | Failed to delivery message"
                       << " | error: " << msg_ref.errorString()
                       << " | topic: " << msg_ref.topicName()
                       << " | partition: " << msg_ref.partition());
    DEBUG(__FUNCTION__ << " | Failed to delivery message"
                       << " | key: " << msg_ref.keyString()
                       << " | payload: " << msg_ref.payloadString());

    if (producer.is_record_msg_for_send_failed_) {
      QbusRecordMsg::recordMsg(msg_ref.topicName(), msg_ref.payloadString());
    }
  } else {
    DEBUG(__FUNCTION__ << " | Message delivered successfully"
                       << " | topic: " << msg_ref.topicName()
                       << " | partition: " << msg_ref.partition()
                       << " | offset: " << msg_ref.offset()
                       << " | key: " << msg_ref.keyString()
                       << " | payload: " << msg_ref.payloadString());
  }

  if (producer.is_sync_send_) {
    producer.sync_send_err_ = msg_ref.err();
  } else if (msg_ref.hasError() && producer.is_init_) {
    if (!producer.AsyncProduce(msg_ref.payload(), msg_ref.len(), msg_ref.key(),
                               msg_ref.key_len())) {
      ERROR(__FUNCTION__ << " | Failed to reproduce"
                         << " | topic: " << msg_ref.topicName()
                         << " | partition: " << msg_ref.partition()
                         << " | error: "
                         << rd_kafka_err2str(rd_kafka_last_error()));
      if (producer.is_record_msg_for_send_failed_) {
        QbusRecordMsg::recordMsg(msg_ref.topicName(), msg_ref.payloadString());
      }
    }
  }
}

void QbusProducerImp::ErrorCallback(rd_kafka_t* rk, int err, const char* reason,
                                    void* opaque) {
  assert(opaque);
  QbusProducerImp& producer = *static_cast<QbusProducerImp*>(opaque);

  ERROR(__FUNCTION__ << " | error: "
                     << rd_kafka_err2name((rd_kafka_resp_err_t)err)
                     << " | reason: " << reason);

  if (err == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN)
    producer.is_brokers_all_down_ = true;

  if (err != RD_KAFKA_RESP_ERR__FATAL) return;

  // Extract the actual underlying error code and description
  char errstr[512];
  rd_kafka_resp_err_t orig_err =
      rd_kafka_fatal_error(rk, errstr, sizeof(errstr));
  ERROR(__FUNCTION__ << " | fatal error: " << rd_kafka_err2name(orig_err)
                     << " | reason: " << errstr);

  // FIXME: maybe we should recreate it?
}

int QbusProducerImp::StatsCallback(rd_kafka_t* rk, char* json, size_t json_len,
                                   void* opaque) {
  assert(opaque);
  QbusProducerImp& producer = *static_cast<QbusProducerImp*>(opaque);

  // check if brokers are up again
  if (producer.is_brokers_all_down_) {
    std::string broker = rdkafka::findAnyUpBroker(json);
    if (!broker.empty()) {
      producer.is_brokers_all_down_ = false;
      INFO(__FUNCTION__ << " | broker " << broker << " is up again.");
    }
  }

  return 0;
}

int32_t QbusProducerImp::PartitionHashFunc(const rd_kafka_topic_t* rkt,
                                           const void* keydata, size_t keylen,
                                           int32_t partition_cnt,
                                           void* rkt_opaque, void* msg_opaque) {
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
      DEBUG(__FUNCTION__
            << " | retry select partition | current invalid partiton: "
            << hit_partition);
      hit_partition = 0;
    }
  } else {
    std::set<int32_t> partition_set;
    for (int32_t i = 0; i < partition_cnt; ++i) {
      partition_set.insert(i);
    }

    while (true) {
      hit_partition = rd_kafka_msg_partitioner_random(
          rkt, keydata, keylen, partition_cnt, rkt_opaque, msg_opaque);
      if (1 == rd_kafka_topic_partition_available(rkt, hit_partition)) {
        break;
      } else {
        DEBUG(__FUNCTION__
              << " | retry select partition | current invalid partiton: "
              << hit_partition);
        partition_set.erase(hit_partition);
        if (partition_set.empty()) {
          DEBUG(
              __FUNCTION__
              << " | failed to select partition | use RD_KAFKA_PARTITION_UA!");
          hit_partition = rd_kafka_msg_partitioner_random(
              rkt, keydata, keylen, partition_cnt, rkt_opaque, msg_opaque);
          ;
          break;
        }
      }
    }
  }

  DEBUG(__FUNCTION__ << " | hit_partition:" << hit_partition);

  return hit_partition;
}

bool QbusProducerImp::InitRdKafkaConfig() {
  INFO(__FUNCTION__ << " | Librdkafka version: " << rd_kafka_version_str()
                    << " " << rd_kafka_version());

  bool rt = false;

  rd_kafka_conf_ = rd_kafka_conf_new();
  if (NULL != rd_kafka_conf_) {
    rd_kafka_conf_set_opaque(rd_kafka_conf_, static_cast<void*>(this));

    rd_kafka_topic_conf_ = rd_kafka_topic_conf_new();
    if (NULL == rd_kafka_topic_conf_) {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_conf_new");
    } else {
      config_loader_.LoadRdkafkaConfig(rd_kafka_conf_, rd_kafka_topic_conf_);
      rd_kafka_conf_set_dr_msg_cb(rd_kafka_conf_,
                                  &QbusProducerImp::MsgDeliveredCallback);
      rd_kafka_conf_set_error_cb(rd_kafka_conf_,
                                 &QbusProducerImp::ErrorCallback);
      rd_kafka_conf_set_stats_cb(rd_kafka_conf_,
                                 &QbusProducerImp::StatsCallback);

      rd_kafka_topic_conf_set_partitioner_cb(
          rd_kafka_topic_conf_, &QbusProducerImp::PartitionHashFunc);

      if (!config_loader_.IsSetConfig(RD_KAFKA_TOPIC_MESSAGE_RETRIES, true)) {
        QbusHelper::SetRdKafkaConfig(rd_kafka_conf_,
                                     RD_KAFKA_TOPIC_MESSAGE_RETRIES,
                                     RD_KAFKA_TOPIC_MESSAGE_RETRIES_VALUE);
      }

      if (!config_loader_.IsSetConfig(
              RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL, false)) {
        QbusHelper::SetRdKafkaConfig(
            rd_kafka_conf_, RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL,
            RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL_MS);
      }

      // enable statistics
      if (!config_loader_.IsSetConfig(RD_KAFKA_CONFIG_STATISTICS_INTERVAL_MS,
                                      false)) {
        QbusHelper::SetRdKafkaConfig(
            rd_kafka_conf_, RD_KAFKA_CONFIG_STATISTICS_INTERVAL_MS,
            RD_KAFKA_CONFIG_STATISTICS_INTERVAL_MS_DEFAULT);
      }

      std::string sync_send = config_loader_.GetSdkConfig(
          RD_KAFKA_SDK_CONFIG_SYNC_SEND,
          RD_KAFKA_SDK_CONFIG_VALUE_SYNC_SEND_DEFAULT);
      if (0 == strncasecmp(sync_send.c_str(),
                           RD_KAFKA_SDK_CONFIG_VALUE_SYNC_SEND,
                           sync_send.length())) {
        is_sync_send_ = true;
        if (!config_loader_.IsSetConfig(
                RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL, false)) {
          QbusHelper::SetRdKafkaConfig(
              rd_kafka_conf_, RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL,
              RD_KAFKA_CONFIG_TOPIC_METADATA_REFRESH_INTERVAL_WHEN_SYNC_SEND_MS);
        }
        QbusHelper::SetRdKafkaConfig(rd_kafka_conf_,
                                     RD_KAFKA_CONFIG_QUEUE_BUFFERING_MAX_MS,
                                     RD_KAFKA_CONFIG_QUEUE_BUFFERING_SYNC);
        QbusHelper::SetRdKafkaConfig(
            rd_kafka_conf_, RD_KAFKA_CONFIG_SOCKET_BLOCKING_MAX_MS,
            RD_KAFKA_SDK_MINIMIZE_PRODUCER_LATENCY_VALUE);
      }

      std::string minimize_producer_latency = config_loader_.GetSdkConfig(
          RD_KAFKA_SDK_CONFIG_PRODUCER_ENABLE_MINI_LATENCY,
          RD_KAFKA_SDK_CONFIG_PRODUCER_ENABLE_MINI_LATENCY_DEFAULT);
      if (0 !=
          strncasecmp(minimize_producer_latency.c_str(),
                      RD_KAFKA_SDK_CONFIG_PRODUCER_ENABLE_MINI_LATENCY_DEFAULT,
                      minimize_producer_latency.length())) {
        DEBUG(__FUNCTION__ << " | enable minimize producer latency");
        QbusHelper::SetRdKafkaConfig(
            rd_kafka_conf_, RD_KAFKA_CONFIG_QUEUE_BUFFERING_MAX_MS,
            RD_KAFKA_SDK_MINIMIZE_PRODUCER_LATENCY_VALUE);
        QbusHelper::SetRdKafkaConfig(
            rd_kafka_conf_, RD_KAFKA_CONFIG_SOCKET_BLOCKING_MAX_MS,
            RD_KAFKA_SDK_MINIMIZE_PRODUCER_LATENCY_VALUE);
      }

      rt = true;
    }
  } else {
    ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_new");
  }

  return rt;
}
//-----------------------------------------------------------------------
// modified by zk
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

bool QbusProducer::init(const std::string& broker_list,
                        const std::string& log_path,
                        const std::string& config_path,
                        const std::string& topic_name) {
  bool rt = false;

#ifdef NOT_USE_CONSUMER_CALLBACK
  pthread_mutex_lock(&kRmtx);
  if (NULL == kRmb) {
    kRmb = new BUFFER;
  }

  std::string key = broker_list + topic_name + config_path;
  BUFFER::iterator i = kRmb->find(key);
  if (i == kRmb->end()) {
    qbus_producer_imp_ = new QbusProducerImp();
    if (NULL != qbus_producer_imp_) {
      rt = qbus_producer_imp_->Init(broker_list, log_path, topic_name,
                                    config_path);
      if (rt) {
        kRmb->insert(BUFFER::value_type(key, qbus_producer_imp_));
        INFO(__FUNCTION__ << " | Procuder init is OK!");
      } else {
        delete qbus_producer_imp_;
        ERROR(__FUNCTION__ << " | Failed to init");
      }
    }
    pthread_mutex_unlock(&kRmtx);
  } else {
    qbus_producer_imp_ = i->second;
    pthread_mutex_unlock(&kRmtx);

    rt = true;
  }
#else
  if (NULL != qbus_producer_imp_) {
    rt = qbus_producer_imp_->Init(broker_list, log_path, topic_name,
                                  config_path);
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
    qbus_producer_imp_->Uninit();
  }
#endif
}

bool QbusProducer::produce(const char* data, size_t data_len,
                           const std::string& key) {
  bool rt = true;

  if (NULL != data && data_len > 0 && NULL != qbus_producer_imp_) {
    DEBUG(__FUNCTION__ << " | msg: " << std::string(data, data_len)
                       << " | key: " << key);
    rt = qbus_producer_imp_->Produce(data, data_len, key);
  } else {
    ERROR(__FUNCTION__ << " | Failed to produce | data is null: "
                       << (NULL != data) << " | data len: " << data_len);
  }

  return rt;
}

}  // namespace qbus
