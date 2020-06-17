#include "qbus_consumer_imp.h"

#include <errno.h>
#include <string.h>
#include <strings.h>

#include <iostream>

#include <pthread.h>

// from rdkafka_int.h, avoid to redefine
#define RD_KAFKA_OFFSET_ERROR -1001

#include "qbus_constant.h"
#include "qbus_consumer_callback.h"
#include "qbus_helper.h"
#include "util/logger.h"
//------------------------------------------------------------
namespace qbus {

QbusConsumerImp::QbusConsumerImp(const std::string& broker_list
#ifndef NOT_USE_CONSUMER_CALLBACK
                                 ,
                                 const QbusConsumerCallback& callback
#endif
                                 )
    : rd_kafka_conf_(NULL),
      rd_kafka_topic_conf_(NULL),
      rd_kafka_handle_(NULL),
      broker_list_(broker_list),
      start_flag_(false),
      enable_rdkafka_logger_(false),
      is_auto_commit_offset_(true),
      is_user_manual_commit_offset_(false),
      force_terminate_(false),
      last_commit_ms_(0),
      consumer_poll_time_(RD_KAFKA_CONSUMER_POLL_TIMEOUT_MS),
      manual_commit_time_(RD_KAFKA_SDK_MANUAL_COMMIT_TIME_DEFAULT_MS),
      poll_thread_id_(0),
#ifndef NOT_USE_CONSUMER_CALLBACK
      qbus_consumer_callback_(callback),
#endif
      has_assigned_(false) {
}

QbusConsumerImp::~QbusConsumerImp() {
  pthread_mutex_destroy(&wait_commit_msgs_mutex_);
}

bool QbusConsumerImp::Init(const std::string& log_path,
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

  int status = pthread_mutex_init(&wait_commit_msgs_mutex_, NULL);
  if (0 != status) {
    ERROR(__FUNCTION__
          << " | Failed to pthread_mutex_init for wait_commit_msgs_mutex_"
          << " | error code:" << status);
    return false;
  }
  status = pthread_mutex_init(&has_assigned_mutex_, NULL);
  if (0 != status) {
    ERROR(__FUNCTION__
          << " | Failed to pthread_mutex_init for has_assigned_mutex_"
          << " | error code:" << status);
    return false;
  }

  bool rt = QbusHelper::GetQbusBrokerList(config_loader_, &broker_list_);
  INFO(__FUNCTION__ << " | Start init | qbus cluster: " << broker_list_
                    << " | config: " << config_path);

  return (rt && InitRdKafka());
}

bool QbusConsumerImp::InitRdKafka() {
  INFO(__FUNCTION__ << " | Librdkafka version: " << rd_kafka_version_str()
                    << " " << rd_kafka_version());
  return InitRdKafkaConfig();
}

bool QbusConsumerImp::InitRdKafkaConfig() {
  bool rt = false;

  rd_kafka_conf_ = rd_kafka_conf_new();
  if (NULL != rd_kafka_conf_) {
    rd_kafka_conf_set_opaque(rd_kafka_conf_, static_cast<void*>(this));
    rd_kafka_conf_set_rebalance_cb(rd_kafka_conf_,
                                   &QbusConsumerImp::rdkafka_rebalance_cb);

    rd_kafka_topic_conf_ = rd_kafka_topic_conf_new();
    if (NULL != rd_kafka_topic_conf_) {
      config_loader_.LoadRdkafkaConfig(rd_kafka_conf_, rd_kafka_topic_conf_);

      // set client.id
      QbusHelper::SetClientId(rd_kafka_conf_);

      rd_kafka_conf_set_default_topic_conf(rd_kafka_conf_,
                                           rd_kafka_topic_conf_);

      // set whether auto reset offset when current offset is invalid, default
      // earliest
      if (config_loader_.IsSetConfig(RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET,
                                     true)) {
        rt = true;
      } else {
        rt = QbusHelper::SetRdKafkaTopicConfig(
            rd_kafka_topic_conf_, RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET,
            RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET_EARLIEST);
        if (!rt) {
          ERROR(__FUNCTION__ << " | set topic config["
                             << RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET
                             << "] failed");
        }
      }

      char auto_reset_offset[20] = {0};
      size_t auto_reset_offset_size = 0;
      rd_kafka_conf_res_t get_auto_reset_offset_rt = rd_kafka_topic_conf_get(
          rd_kafka_topic_conf_, RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET,
          auto_reset_offset, &auto_reset_offset_size);
      if (RD_KAFKA_CONF_OK == get_auto_reset_offset_rt) {
        INFO(__FUNCTION__ << " | Reset offset to " << auto_reset_offset
                          << " if the current offset is invalid");
      } else {
        INFO(
            __FUNCTION__ << " | Reset offset if the current offset is invalid");
      }

      consumer_poll_time_ =
          atoll(config_loader_
                    .GetSdkConfig(RD_KAFKA_SDK_CONSUMER_POLL_TIME,
                                  RD_KAFKA_SDK_CONSUMER_POLL_TIME_DEFAULT_MS)
                    .c_str());
      manual_commit_time_ = atoll(
          config_loader_
              .GetSdkConfig(RD_KAFKA_SDK_CONSUMER_MANUAL_COMMIT_TIME,
                            RD_KAFKA_SDK_MANUAL_COMMIT_TIME_DEFAULT_MS_STR)
              .c_str());

      // get whether auto commit offset
      char is_auto_commit_offset[10];
      size_t is_auto_commit_offset_size = 0;
      rd_kafka_conf_res_t get_auto_commit_rt =
          rd_kafka_conf_get(rd_kafka_conf_, RD_KAFKA_CONFIG_ENABLE_AUTO_COMMIT,
                            is_auto_commit_offset, &is_auto_commit_offset_size);
      if (RD_KAFKA_CONF_OK == get_auto_commit_rt) {
        is_auto_commit_offset[is_auto_commit_offset_size] = '\0';
        is_auto_commit_offset_ =
            (0 == strncasecmp(is_auto_commit_offset, "true",
                              is_auto_commit_offset_size));
      } else {
        ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_get for "
                           << RD_KAFKA_CONFIG_ENABLE_AUTO_COMMIT);
      }

      // set whether to force destroy in Stop() if user manual commit offset
      std::string force_destroy = config_loader_.GetSdkConfig(
          RD_KAFKA_SDK_CONSUMER_FORCE_TERMINATE, "false");
      if (0 ==
          strncasecmp(force_destroy.c_str(), "false", force_destroy.length())) {
        force_terminate_ = false;
      } else {
        force_terminate_ = true;
      }

      // get whether user manual commit offset and if use use manual commit
      // offset, then set is_auto_commit_offset to false
      std::string user_manual_commit_offset = config_loader_.GetSdkConfig(
          RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET,
          RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET_DEFAULT);
      if (0 == strncasecmp(user_manual_commit_offset.c_str(),
                           RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET_DEFAULT,
                           user_manual_commit_offset.length())) {
        is_user_manual_commit_offset_ = false;
      } else {
        DEBUG(__FUNCTION__ << " | use user_manual_commit_offset option");
        is_user_manual_commit_offset_ = true;
        if (is_auto_commit_offset_) {
          is_auto_commit_offset_ = false;
          QbusHelper::SetRdKafkaConfig(
              rd_kafka_conf_, RD_KAFKA_CONFIG_ENABLE_AUTO_COMMIT, "false");
        }
      }
    } else {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_conf_new");
    }
  } else {
    ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_new");
  }

  return rt;
}

bool QbusConsumerImp::InitRdKafkaHandle() {
  bool rt = false;

  if (NULL != rd_kafka_handle_) {
    rt = true;
  } else {
    char err_str[512] = {0};
    rd_kafka_handle_ = rd_kafka_new(RD_KAFKA_CONSUMER, rd_kafka_conf_, err_str,
                                    sizeof(err_str));
    if (NULL == rd_kafka_handle_) {
      ERROR(__FUNCTION__ << "Failed to create new consumer | error msg:"
                         << err_str);
    } else if (0 ==
               rd_kafka_brokers_add(rd_kafka_handle_, broker_list_.c_str())) {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_broker_add | broker list:"
                         << broker_list_);
    } else {
      rd_kafka_resp_err_t resp_rs =
          rd_kafka_poll_set_consumer(rd_kafka_handle_);
      if (RD_KAFKA_RESP_ERR_NO_ERROR != resp_rs) {
        ERROR(
            __FUNCTION__ << " | Failed to rd_kafka_poll_set_consumer | err msg:"
                         << rd_kafka_err2str(resp_rs));
      } else {
        rt = true;
      }
      rt = true;
    }
  }

  return rt;
}

bool QbusConsumerImp::Subscribe(const std::string& group,
                                const std::vector<std::string>& topics) {
  std::string old_group = QbusHelper::GetGroupId(config_loader_);
  if (!group.empty()) {
    // User provided group has higher priority than configured group.id
    INFO(__FUNCTION__ << " | Update group.id from \"" << old_group << "\" to \""
                      << group << "\"");
    if (!QbusHelper::SetRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_GROUP_ID,
                                      group.c_str())) {
      ERROR(__FUNCTION__ << " | SetRdKafkaConfig failed");
      return false;
    }
    group_ = group;
  } else {
    group_ = old_group;
  }

  topics_ = topics;
  INFO(__FUNCTION__ << " | group: " << group_
                    << " | topic: " << QbusHelper::FormatStringVector(topics));

  bool rt = InitRdKafkaHandle();

  if (rt && !topics.empty()) {
    rd_kafka_topic_partition_list_t* rd_kafka_topic_list =
        rd_kafka_topic_partition_list_new(topics.size());
    if (NULL != rd_kafka_topic_list) {
      std::set<std::string> efficacious_topics;

      for (size_t i = 0; i < topics.size(); ++i) {
        rd_kafka_topic_partition_t* res = rd_kafka_topic_partition_list_add(
            rd_kafka_topic_list, topics[i].c_str(),
            -1);  //-1 mean consumed all partitions

        if (NULL == res) {
          ERROR(__FUNCTION__
                << " | Failed to rd_kafka_topic_partition_list_add | group:"
                << group << " | topic: " << topics[i]);
          rt = false;
          break;
        } else {
          efficacious_topics.insert(topics[i]);
        }

        rt = true;
      }

      rt = !efficacious_topics.empty();

      if (rt) {
        rd_kafka_resp_err_t err;
        if (RD_KAFKA_RESP_ERR_NO_ERROR !=
            (err = rd_kafka_subscribe(rd_kafka_handle_, rd_kafka_topic_list))) {
          rt = false;
          ERROR(__FUNCTION__ << " | Failed to rd_kafka_subscribe | err msg:"
                             << rd_kafka_err2str(err));
        } else {
          efficacious_topics_.insert(efficacious_topics.begin(),
                                     efficacious_topics.end());
        }
      }
      rd_kafka_topic_partition_list_destroy(rd_kafka_topic_list);
      rd_kafka_topic_list = NULL;
    } else {
      ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_partition_list_new")
    }
  }

  return rt;
}

bool QbusConsumerImp::Start() {
  INFO(__FUNCTION__ << " | Starting to consume...")

  bool rt = true;

  if (!start_flag_ && !efficacious_topics_.empty()) {
    start_flag_ = true;

#ifndef NOT_USE_CONSUMER_CALLBACK
    int res = pthread_create(&poll_thread_id_, NULL, &ConsumePollThread,
                             static_cast<void*>(this));
    if (0 != res) {
      start_flag_ = false;
      rt = false;
      ERROR(__FUNCTION__ << " | ConsumePollThread | Failed to pthread_create!");
    }
#endif
  }

  return rt;
}

void QbusConsumerImp::Stop() {
  INFO(__FUNCTION__ << " | Starting to stop consumer..."
                    << " | is_auto_commit_offset:" << is_auto_commit_offset_
                    << " | is_user_manual_commit_offset:"
                    << is_user_manual_commit_offset_ << " | is_force_destroy:"
                    << force_terminate_ << " | wait destroy msgs:"
                    << wait_destroy_msgs_for_uncommit_.size());

  if (start_flag_) {
    start_flag_ = false;
    topic_partition_set_.init(NULL);

#ifndef NOT_USE_CONSUMER_CALLBACK
    pthread_join(poll_thread_id_, NULL);
#endif
    if (!is_auto_commit_offset_) {
      ManualCommitWaitOffset(true);
    }

    if (is_user_manual_commit_offset_) {
      ClearWaitDestroyMsgs();
    }
  }

  INFO(__FUNCTION__ << " | Starting clean rdkafka...");

  if (NULL != rd_kafka_handle_) {
    INFO(__FUNCTION__ << " | Starting consumer close...");
    rd_kafka_resp_err_t err = rd_kafka_consumer_close(rd_kafka_handle_);
    if (RD_KAFKA_RESP_ERR_NO_ERROR != err) {
      ERROR(__FUNCTION__ << " | Failed to close consumer | err msg: "
                         << rd_kafka_err2str(err));
    }
  }

  INFO(__FUNCTION__ << " | Starting destroy rdkafka...");

  if (NULL != rd_kafka_handle_) {
    if (force_terminate_) {
      WARNING(__FUNCTION__ << " | Consumer force terminate")
    } else {
      rd_kafka_destroy(rd_kafka_handle_);
    }
    rd_kafka_handle_ = NULL;
  }

  INFO(__FUNCTION__ << " | Starting wait destroyed rdkafka...");
  /* Let background threads clean up and terminate cleanly. */
  int run = 3;
  while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1) {
    DEBUG(__FUNCTION__ << " | Waiting for librdkafka to decommission");
  }

  INFO(__FUNCTION__ << " | Consumer clean up done!");
}

bool QbusConsumerImp::CheckMsg(rd_kafka_message_t* rkmessage) {
  bool rt = false;

  if (RD_KAFKA_RESP_ERR_NO_ERROR != rkmessage->err) {
    if (RD_KAFKA_RESP_ERR__PARTITION_EOF == rkmessage->err) {
      DEBUG(__FUNCTION__ << " | Consumer reached end of "
                         << rd_kafka_topic_name(rkmessage->rkt) << "["
                         << rkmessage->partition << "]"
                         << " | offset: " << rkmessage->offset);
    } else if (NULL != rkmessage->rkt) {
      ERROR(__FUNCTION__ << " | Consumer error for"
                         << rd_kafka_topic_name(rkmessage->rkt) << "["
                         << rkmessage->partition << "]"
                         << " | offset: " << rkmessage->offset << " | err msg: "
                         << rd_kafka_message_errstr(rkmessage));
    } else {
      ERROR(__FUNCTION__ << " | Consumer error | err msg: " << rkmessage->err
                         << " | rd_kafka_message's err msg: "
                         << rd_kafka_message_errstr(rkmessage));
    }
  } else {
    rt = true;
  }

  return rt;
}

#ifdef NOT_USE_CONSUMER_CALLBACK
bool QbusConsumerImp::Consume(QbusMsgContentInfo& msg_content_info) {
  bool rt = false;

  if (start_flag_) {
    rd_kafka_message_t* rkmessage = NULL;
    rkmessage = rd_kafka_consumer_poll(rd_kafka_handle_, consumer_poll_time_);
    if (NULL != rkmessage && CheckMsg(rkmessage) &&
        NULL != rkmessage->payload) {
      std::string topic_name(NULL != rd_kafka_topic_name(rkmessage->rkt)
                                 ? rd_kafka_topic_name(rkmessage->rkt)
                                 : "");
      DEBUG(__FUNCTION__ << " | Successed consumed msg of "
                         << rd_kafka_topic_name(rkmessage->rkt) << "["
                         << rkmessage->partition << "]"
                         << " | offset: " << rkmessage->offset
                         << " | msg len: " << (int)rkmessage->len);

      msg_content_info.topic = topic_name;
      msg_content_info.msg = std::string(
          static_cast<const char*>(rkmessage->payload), rkmessage->len);
      msg_content_info.msg_len = rkmessage->len;
      msg_content_info.rd_message = rkmessage;

      if (!is_auto_commit_offset_) {
        if (!is_user_manual_commit_offset_) {
          AddWaitCommitOffset(rkmessage);
        } else {
          AddWaitDestroyMsgs(rkmessage);
        }
      } else {
        rd_kafka_message_destroy(rkmessage);
        rkmessage = NULL;
      }

      rt = true;
    } else if (NULL != rkmessage) {
      rd_kafka_message_destroy(rkmessage);
      rkmessage = NULL;
    }
  }

  if (!is_auto_commit_offset_) {
    ManualCommitWaitOffset(false);
  }

  return rt;
}
#endif

void QbusConsumerImp::CommitOffset(
    const QbusMsgContentInfo& qbusMsgContentInfo) {
  if (is_user_manual_commit_offset_) {
    if (NULL != qbusMsgContentInfo.rd_message) {
      AddWaitCommitOffset(qbusMsgContentInfo.rd_message);
      RemoveWaitDestroyMsgs(qbusMsgContentInfo.rd_message);
    }
  }
}

bool QbusConsumerImp::ReadyToPauseResume() const {
  if (!start_flag_) {
    ERROR(__FUNCTION__ << " | consumer not started");
    return false;
  }

  if (!has_assigned()) {
    ERROR(__FUNCTION__ << " | pause/resume before partitions assigned");
    return false;
  }

  return true;
}

bool QbusConsumerImp::Pause(const std::vector<std::string>& topics) {
  INFO(__FUNCTION__ << " | topics: " << QbusHelper::FormatStringVector(topics));
  if (!ReadyToPauseResume()) return false;

  rd_kafka_topic_partition_list_t* lst =
      topic_partition_set_.findTopics(topics);
  rdkafka::PartitionListGuard lst_guard(lst);

  rd_kafka_resp_err_t err = rd_kafka_pause_partitions(rd_kafka_handle_, lst);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    ERROR(__FUNCTION__ << " | Failed: " << rd_kafka_err2str(err));
    return false;
  }

  return true;
}

bool QbusConsumerImp::Resume(const std::vector<std::string>& topics) {
  INFO(__FUNCTION__ << " | topics: " << QbusHelper::FormatStringVector(topics));
  if (!ReadyToPauseResume()) return false;

  rd_kafka_topic_partition_list_t* lst =
      topic_partition_set_.findTopics(topics);
  rdkafka::PartitionListGuard lst_guard(lst);

  rd_kafka_resp_err_t err = rd_kafka_resume_partitions(rd_kafka_handle_, lst);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    ERROR(__FUNCTION__ << " | Failed: " << rd_kafka_err2str(err));
    return false;
  }

  return true;
}

std::string QbusConsumerImp::GetWaitOffsetKey(rd_kafka_message_t* msg) {
  std::stringstream ss;
  const char* name = rd_kafka_topic_name(msg->rkt);
  ss << (NULL != name ? name : "") << ":" << msg->partition;
  std::string key = ss.str();
  ss.str("");
  return key;
}

void QbusConsumerImp::ClearWaitDestroyMsgs() {
  unsigned long long msg_count = 0;
  pthread_mutex_lock(&wait_commit_msgs_mutex_);
  for (std::map<std::string, RdkafkaMsgVectorType>::iterator
           i = wait_destroy_msgs_for_uncommit_.begin(),
           e = wait_destroy_msgs_for_uncommit_.end();
       i != e; ++i) {
    RdkafkaMsgVectorType& rdmsg = i->second;
    for (RdkafkaMsgVectorType::iterator k = rdmsg.begin(), ke = rdmsg.end();
         k != ke; ++k) {
      rd_kafka_message_t* msg = *k;
      if (NULL != msg) {
        rd_kafka_message_destroy(msg);
        ++msg_count;
        msg = NULL;
      }
    }
  }
  pthread_mutex_unlock(&wait_commit_msgs_mutex_);
}

void QbusConsumerImp::AddWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message) {
  std::string key = GetWaitOffsetKey(rd_kafka_message);

  pthread_mutex_lock(&wait_commit_msgs_mutex_);
  std::map<std::string, RdkafkaMsgVectorType>::iterator found =
      wait_destroy_msgs_for_uncommit_.find(key);
  if (wait_destroy_msgs_for_uncommit_.end() != found) {
    RdkafkaMsgVectorType& msgs = found->second;
    msgs.push_back(rd_kafka_message);
  } else {
    RdkafkaMsgVectorType msgs;
    msgs.push_back(rd_kafka_message);
    wait_destroy_msgs_for_uncommit_.insert(
        std::map<std::string, RdkafkaMsgVectorType>::value_type(key, msgs));
  }
  pthread_mutex_unlock(&wait_commit_msgs_mutex_);
}

void QbusConsumerImp::RemoveWaitDestroyMsgs(
    rd_kafka_message_t* rd_kafka_message) {
  std::string key = GetWaitOffsetKey(rd_kafka_message);

  pthread_mutex_lock(&wait_commit_msgs_mutex_);
  std::map<std::string, RdkafkaMsgVectorType>::iterator found =
      wait_destroy_msgs_for_uncommit_.find(key);
  if (wait_destroy_msgs_for_uncommit_.end() != found) {
    RdkafkaMsgVectorType& msgs = found->second;
    RdkafkaMsgVectorType::iterator f =
        std::find(msgs.begin(), msgs.end(), rd_kafka_message);
    if (f != msgs.end()) {
      for (RdkafkaMsgVectorType::iterator k =
               msgs.begin();  // this loop added by zk
           k < f; ++k) {
        rd_kafka_message_t* msg = *k;
        if (NULL != msg) {
          rd_kafka_message_destroy(msg);
          *k = NULL;
        }
      }
      msgs.erase(msgs.begin(), f + 1);
    }
  }
  pthread_mutex_unlock(&wait_commit_msgs_mutex_);
}

void QbusConsumerImp::AddWaitCommitOffset(
    rd_kafka_message_t* rd_kafka_message) {
  std::string key = GetWaitOffsetKey(rd_kafka_message);

  pthread_mutex_lock(&wait_commit_msgs_mutex_);

  std::map<std::string, rd_kafka_message_t*>::iterator found =
      wait_commit_msgs_.find(key);
  if (wait_commit_msgs_.end() != found) {
    rd_kafka_message_t* msg = found->second;
    if (NULL != msg) {
      rd_kafka_message_destroy(msg);
      msg = NULL;
    }
  }
  wait_commit_msgs_[key] = rd_kafka_message;

  pthread_mutex_unlock(&wait_commit_msgs_mutex_);
}

#ifndef NOT_USE_CONSUMER_CALLBACK
void* QbusConsumerImp::ConsumePollThread(void* arg) {
  QbusConsumerImp* consumer = static_cast<QbusConsumerImp*>(arg);

  if (NULL != consumer) {
    rd_kafka_message_t* rdkafka_message = NULL;
    while (consumer->start_flag_) {
      rdkafka_message = rd_kafka_consumer_poll(consumer->rd_kafka_handle_,
                                               consumer->consumer_poll_time_);
      if (NULL != rdkafka_message && consumer->CheckMsg(rdkafka_message)) {
        if (!consumer->is_auto_commit_offset_) {
          if (!consumer->is_user_manual_commit_offset_) {
            consumer->AddWaitCommitOffset(rdkafka_message);
          } else {
            consumer->AddWaitDestroyMsgs(rdkafka_message);
          }
          consumer->ReceivedConsumeMsg(rdkafka_message, NULL);
        } else {
          consumer->ReceivedConsumeMsg(rdkafka_message, NULL);
          rd_kafka_message_destroy(rdkafka_message);
          rdkafka_message = NULL;
        }
      } else if (NULL != rdkafka_message) {
        rd_kafka_message_destroy(rdkafka_message);
        rdkafka_message = NULL;
      }

      if (!consumer->is_auto_commit_offset_) {
        consumer->ManualCommitWaitOffset(false);
      }
    }

    if (!consumer->is_auto_commit_offset_) {
      consumer->ManualCommitWaitOffset(false);
    }
  }

  return (void*)(NULL);
}
#endif

void QbusConsumerImp::ManualCommitWaitOffset(bool face) {
  long now = QbusHelper::GetCurrentTimeMs();

  if (face || now - last_commit_ms_ >= manual_commit_time_) {
    pthread_mutex_lock(&wait_commit_msgs_mutex_);
    for (std::map<std::string, rd_kafka_message_t*>::iterator
             i = wait_commit_msgs_.begin(),
             e = wait_commit_msgs_.end();
         i != e; ++i) {
      rd_kafka_message_t* rdmsg = i->second;
      if (NULL != rdmsg) {
        ManualCommitOffset(i->second);
        rd_kafka_message_destroy(rdmsg);
        rdmsg = NULL;
      }
    }

    wait_commit_msgs_.clear();

    pthread_mutex_unlock(&wait_commit_msgs_mutex_);
    last_commit_ms_ = now;
  }
}

void QbusConsumerImp::ManualCommitOffset(const rd_kafka_message_t* rkmessage) {
  rd_kafka_resp_err_t rt =
      rd_kafka_commit_message(rd_kafka_handle_, rkmessage, 0);
  if (RD_KAFKA_RESP_ERR_NO_ERROR != rt) {
    ERROR(__FUNCTION__ << " | Failed to rd_kafka_commit_message | error msg: "
                       << rd_kafka_err2str(rt));
  }
}

#ifndef NOT_USE_CONSUMER_CALLBACK
void QbusConsumerImp::ReceivedConsumeMsg(rd_kafka_message_t* rkmessage,
                                         void* opaque) {
  if (NULL == rkmessage) {
    return;
  }

  std::string topic_name(NULL != rd_kafka_topic_name(rkmessage->rkt)
                             ? rd_kafka_topic_name(rkmessage->rkt)
                             : "");
  DEBUG(__FUNCTION__ << " | Successed consumed msg of "
                     << rd_kafka_topic_name(rkmessage->rkt) << "["
                     << rkmessage->partition << "]"
                     << " | offset: " << rkmessage->offset
                     << " | msg len: " << (int)rkmessage->len);
  if (!is_user_manual_commit_offset_) {
    std::string copy_msg(static_cast<const char*>(rkmessage->payload),
                         rkmessage->len);
    qbus_consumer_callback_.deliveryMsg(topic_name, copy_msg.c_str(),
                                        rkmessage->len);
  } else {
    //! LW! set user.manual.commit.offset=true into consumer config file and
    //! user need to call to commitOffset
    QbusMsgContentInfo msg_content_info;
    msg_content_info.topic = topic_name;
    msg_content_info.msg =
        std::string(static_cast<const char*>(
                        rkmessage->payload != NULL ? rkmessage->payload : ""),
                    rkmessage->len);
    msg_content_info.msg_len = rkmessage->len;
    msg_content_info.rd_message = rkmessage;
    qbus_consumer_callback_.deliveryMsgForCommitOffset(msg_content_info);
  }
}
#endif

void QbusConsumerImp::rdkafka_rebalance_cb(
    rd_kafka_t* rk, rd_kafka_resp_err_t err,
    rd_kafka_topic_partition_list_t* partitions, void* opaque) {
  assert(opaque);
  QbusConsumerImp* consumer_imp = static_cast<QbusConsumerImp*>(opaque);

  switch (err) {
    case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS: {
      DEBUG(__FUNCTION__ << " | rebalance result OK: "
                         << QbusHelper::FormatTopicPartitionList(partitions));
      rd_kafka_assign(rk, partitions);
      consumer_imp->topic_partition_set_.init(partitions);
      consumer_imp->SetHasAssigned(true);
    } break;
    case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS: {
      DEBUG(__FUNCTION__ << " | rebalance result revoke | msg: "
                         << rd_kafka_err2str(err) << " | "
                         << QbusHelper::FormatTopicPartitionList(partitions));
      if (!consumer_imp->is_auto_commit_offset_) {
        consumer_imp->ManualCommitWaitOffset(true);
      }

      rd_kafka_assign(rk, NULL);
      consumer_imp->SetHasAssigned(false);
    } break;
    default:
      ERROR(__FUNCTION__ << " | Failed to rebalance | err msg: "
                         << rd_kafka_err2str(err));
      rd_kafka_assign(rk, NULL);
      consumer_imp->SetHasAssigned(false);
      break;
  }
}

bool QbusConsumerImp::has_assigned() const {
  bool result;
  pthread_mutex_lock(&has_assigned_mutex_);
  result = has_assigned_;
  pthread_mutex_unlock(&has_assigned_mutex_);
  return result;
}

void QbusConsumerImp::SetHasAssigned(bool new_value) {
  pthread_mutex_lock(&has_assigned_mutex_);
  has_assigned_ = new_value;
  pthread_mutex_unlock(&has_assigned_mutex_);
}

}  // namespace qbus
