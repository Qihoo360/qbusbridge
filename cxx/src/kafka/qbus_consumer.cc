#include "qbus_consumer.h"

#include <signal.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include <set>
#include <iostream>

// from rdkafka_int.h, avoid to redefine
#define RD_KAFKA_OFFSET_ERROR -1001

#include "qbus_config.h"
#include "qbus_constant.h"
#include "qbus_helper.h"
#include "qbus_rdkafka.h"
#include "qbus_thread.h"
#include "qbus_topic_partition_set.h"
#include "util/logger.h"
//------------------------------------------------------------
namespace qbus {
namespace kafka {

class QbusConsumer::QbusConsumerImp {
   public:
    QbusConsumerImp(const std::string& cluster_name
#ifndef NOT_USE_CONSUMER_CALLBACK
                    ,
                    const QbusConsumerCallback& callback
#endif
    );
    ~QbusConsumerImp();

   public:
    bool init(const std::string& log_path, const std::string& config_path);
    bool subscribe(const std::string& group, const std::vector<std::string>& topics);
    bool start();
    void stop();

    bool pause(const std::vector<std::string>& topics);
    bool resume(const std::vector<std::string>& topics);

#ifdef NOT_USE_CONSUMER_CALLBACK
    bool consume(QbusMsgContentInfo& msg_content_info);
#endif

    void commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo);

   private:
    static void rdkafka_rebalance_cb(rd_kafka_t* rk, rd_kafka_resp_err_t err,
                                     rd_kafka_topic_partition_list_t* partitions, void* opaque);
#ifndef NOT_USE_CONSUMER_CALLBACK
    static void* ConsumePollThread(void* arg);
#endif

    bool initRdKafka();
    bool initRdKafkaHandle();
    bool initRdKafkaConfig();
#ifndef NOT_USE_CONSUMER_CALLBACK
    void receivedConsumeMsg(rd_kafka_message_t* rkmessage, void* opaque);
#endif
    void resetAllOffsetOfAllPartitionOfTopic(rd_kafka_topic_partition_list_t* partitions);

    void manualCommitOffset(const rd_kafka_message_t* rkmessage);
    bool checkMsg(rd_kafka_message_t* rdkafka_massage);
    void manualCommitWaitOffset(bool face);
    void addWaitCommitOffset(rd_kafka_message_t* rd_kafka_message);
    std::string getWaitOffsetKey(rd_kafka_message_t* msg);
    void clearWaitDestroyMsgs();
    void addWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message);
    void removeWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message);

   private:
    rd_kafka_conf_t* rd_kafka_conf_;
    rd_kafka_topic_conf_t* rd_kafka_topic_conf_;
    rd_kafka_t* rd_kafka_handle_;

    std::string cluster_name_;
    std::string broker_list_;
    std::set<std::string> efficacious_topics_;

    bool start_flag_;
    bool enable_rdkafka_logger_;
    bool is_auto_commit_offset_;
    bool is_user_manual_commit_offset_;
    bool is_force_destroy_;

    long last_commit_ms_;
    long long consumer_poll_time_;
    long long manual_commit_time_;

    Thread poll_thread_;

    QbusConfigLoader config_loader_;

#ifndef NOT_USE_CONSUMER_CALLBACK
    const QbusConsumerCallback& qbus_consumer_callback_;
#endif

    std::string group_;
    std::vector<std::string> topics_;

    mutable Mutex wait_commit_msgs_mutex_;
    std::map<std::string, rd_kafka_message_t*> wait_commit_msgs_;

    typedef std::vector<rd_kafka_message_t*> RdkafkaMsgVectorType;
    std::map<std::string, RdkafkaMsgVectorType> wait_destroy_msgs_for_uncommit_;

    // Map: topic name => partition id set
    TopicPartitionSet topic_partition_set_;

    // Since partitions to `pause()` must be previously assigned, we have to ensure
    // that `pause()` is called after `rd_kafka_assign()`.
    bool has_assigned_;  // whether partitions to consume have been assigned.
    mutable Mutex has_assigned_mutex_;

    // Synchronized getter and setter
    bool has_assigned() const;
    void setHasAssigned(bool has_assigned);
};

QbusConsumer::QbusConsumerImp::QbusConsumerImp(const std::string& cluster_name
#ifndef NOT_USE_CONSUMER_CALLBACK
                                               ,
                                               const QbusConsumerCallback& callback
#endif
                                               )
    : rd_kafka_conf_(NULL),
      rd_kafka_topic_conf_(NULL),
      rd_kafka_handle_(NULL),
      cluster_name_(cluster_name),
      broker_list_(""),
      start_flag_(false),
      enable_rdkafka_logger_(false),
      is_auto_commit_offset_(true),
      is_user_manual_commit_offset_(false),
      is_force_destroy_(false),
      last_commit_ms_(0),
      consumer_poll_time_(RD_KAFKA_CONSUMER_POLL_TIMEOUT_MS),
      manual_commit_time_(RD_KAFKA_SDK_MANUAL_COMMIT_TIME_DEFAULT_MS),
#ifndef NOT_USE_CONSUMER_CALLBACK
      qbus_consumer_callback_(callback),
#endif
      has_assigned_(false) {
}

QbusConsumer::QbusConsumerImp::~QbusConsumerImp() {}

bool QbusConsumer::QbusConsumerImp::init(const std::string& log_path, const std::string& config_path) {
    std::string errstr;
    bool load_config_ok = config_loader_.loadConfig(config_path, errstr);

    QbusHelper::initLog(
        config_loader_.getSdkConfig(RD_KAFKA_SDK_CONFIG_LOG_LEVEL, RD_KAFKA_SDK_CONFIG_LOG_LEVEL_DEFAULT),
        log_path);

    if (!load_config_ok) {
        ERROR(__FUNCTION__ << " | loadConfig failed: " << errstr);
        return false;
    }

    bool rt = QbusHelper::getQbusBrokerList(config_loader_, &broker_list_);
    INFO(__FUNCTION__ << " | Start init | qbus cluster: " << cluster_name_ << " | config: " << config_path
                      << " | broker_lsit:" << broker_list_);

    return (rt && initRdKafka());
}

bool QbusConsumer::QbusConsumerImp::subscribe(const std::string& group,
                                              const std::vector<std::string>& topics) {
    if (!group.empty()) group_ = group;  // may overwrite configured group.id
    topics_ = topics;

    bool rt = false;

    if (!group_.empty()) {
        rt = QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_GROUP_ID, group_.c_str());
        if (!rt) {
            ERROR(__FUNCTION__ << " | Failed to set group: " << group);
        }
    }

    if (rt) {
        rt = initRdKafkaHandle();
    }

    if (rt && !topics.empty()) {
        rd_kafka_topic_partition_list_t* rd_kafka_topic_list =
            rd_kafka_topic_partition_list_new(topics.size());
        if (NULL != rd_kafka_topic_list) {
            std::set<std::string> efficacious_topics;

            for (size_t i = 0; i < topics.size(); ++i) {
                rd_kafka_topic_partition_t* res =
                    rd_kafka_topic_partition_list_add(rd_kafka_topic_list, topics[i].c_str(),
                                                      -1);  //-1 mean consumed all partitions

                if (NULL == res) {
                    ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_partition_list_add | group:" << group
                                       << " | topic: " << topics[i]);
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
                    efficacious_topics_.insert(efficacious_topics.begin(), efficacious_topics.end());
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

bool QbusConsumer::QbusConsumerImp::start() {
    INFO(__FUNCTION__ << " | Starting consume...")

    bool rt = true;

    if (!start_flag_ && !efficacious_topics_.empty()) {
        start_flag_ = true;

#ifndef NOT_USE_CONSUMER_CALLBACK
        if (!poll_thread_.start(&ConsumePollThread, this)) {
            start_flag_ = false;
            rt = false;
            ERROR(__FUNCTION__ << " | ConsumePollThread | Failed to start!");
        }
#endif
    }

    return rt;
}

void QbusConsumer::QbusConsumerImp::stop() {
    INFO(__FUNCTION__ << " | Starting stop consumer..."
                      << " | is_auto_commit_offset:" << is_auto_commit_offset_
                      << " | is_user_manual_commit_offset:" << is_user_manual_commit_offset_
                      << " | is_force_destroy:" << is_force_destroy_
                      << " | wait destroy msgs:" << wait_destroy_msgs_for_uncommit_.size());

    if (start_flag_) {
        start_flag_ = false;
        topic_partition_set_.init(NULL);

#ifndef NOT_USE_CONSUMER_CALLBACK
        poll_thread_.stop();
#endif
        if (!is_auto_commit_offset_) {
            manualCommitWaitOffset(true);
        }

        if (is_user_manual_commit_offset_) {
            clearWaitDestroyMsgs();
        }
    }

    INFO(__FUNCTION__ << " | Starting clean rdkafka...");

    if (NULL != rd_kafka_handle_) {
        INFO(__FUNCTION__ << " | Starting consumer close...");
        rd_kafka_resp_err_t err = rd_kafka_consumer_close(rd_kafka_handle_);
        if (RD_KAFKA_RESP_ERR_NO_ERROR != err) {
            ERROR(__FUNCTION__ << " | Failed to close consumer | err msg: " << rd_kafka_err2str(err));
        }
    }

    INFO(__FUNCTION__ << " | Starting destory rdkafka...");

    if (NULL != rd_kafka_handle_) {
        if (is_user_manual_commit_offset_ && is_force_destroy_) {
            // rd_kafka_destroy_noblock(rd_kafka_handle_);
        } else {
            rd_kafka_destroy(rd_kafka_handle_);
        }
        rd_kafka_handle_ = NULL;
    }

    INFO(__FUNCTION__ << " | Starting wait destoryed rdkafka...");
    /* Let background threads clean up and terminate cleanly. */
    int run = 5;
    while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1) {
        DEBUG(__FUNCTION__ << " | Waiting for librdkafka to decommission");
    }

    INFO(__FUNCTION__ << " | Consumer clean up done!");
}

bool QbusConsumer::QbusConsumerImp::pause(const std::vector<std::string>& topics) {
    if (!start_flag_) {
        ERROR(__FUNCTION__ << " | consumer not started");
        return false;
    }

    if (!has_assigned()) {
        ERROR(__FUNCTION__ << " | pause before partitions assigned");
        return false;
    }

    rd_kafka_topic_partition_list_t* lst = topic_partition_set_.findTopics(topics);
    rdkafka::PartitionListGuard lst_guard(lst);

    INFO(__FUNCTION__ << " | Pause " << rdkafka::formatPartitionList(lst));
    rd_kafka_resp_err_t err = rd_kafka_pause_partitions(rd_kafka_handle_, lst);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        ERROR(__FUNCTION__ << " | Failed: " << rd_kafka_err2str(err));
        return false;
    }

    return true;
}

bool QbusConsumer::QbusConsumerImp::resume(const std::vector<std::string>& topics) {
    if (!start_flag_) {
        ERROR(__FUNCTION__ << " | consumer not started");
        return false;
    }

    if (!has_assigned()) {
        ERROR(__FUNCTION__ << " | resume before partitions assigned");
        return false;
    }

    rd_kafka_topic_partition_list_t* lst = topic_partition_set_.findTopics(topics);
    rdkafka::PartitionListGuard lst_guard(lst);

    INFO(__FUNCTION__ << " | Resume " << rdkafka::formatPartitionList(lst));
    rd_kafka_resp_err_t err = rd_kafka_resume_partitions(rd_kafka_handle_, lst);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        ERROR(__FUNCTION__ << " | Failed: " << rd_kafka_err2str(err));
        return false;
    }

    return true;
}

bool QbusConsumer::QbusConsumerImp::checkMsg(rd_kafka_message_t* rkmessage) {
    bool rt = false;

    if (RD_KAFKA_RESP_ERR_NO_ERROR != rkmessage->err) {
        if (RD_KAFKA_RESP_ERR__PARTITION_EOF == rkmessage->err) {
            DEBUG(__FUNCTION__ << " | Consumer reached end of " << rd_kafka_topic_name(rkmessage->rkt) << "["
                               << rkmessage->partition << "]"
                               << " | offset: " << rkmessage->offset);
        } else if (NULL != rkmessage->rkt) {
            ERROR(__FUNCTION__ << " | Consumer error for" << rd_kafka_topic_name(rkmessage->rkt) << "["
                               << rkmessage->partition << "]"
                               << " | offset: " << rkmessage->offset
                               << " | err msg: " << rd_kafka_message_errstr(rkmessage));
        } else {
            ERROR(__FUNCTION__ << " | Consumer error | err msg: " << rkmessage->err
                               << " | rd_kafka_message's err msg: " << rd_kafka_message_errstr(rkmessage));
        }

        if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
            rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC) {
            // TODO:
            // start_flag_ = false;
        }
    } else {
        rt = true;
    }

    return rt;
}

#ifdef NOT_USE_CONSUMER_CALLBACK
bool QbusConsumer::QbusConsumerImp::consume(QbusMsgContentInfo& msg_content_info) {
    bool rt = false;

    if (start_flag_) {
        rd_kafka_message_t* rkmessage = NULL;
        rkmessage = rd_kafka_consumer_poll(rd_kafka_handle_, consumer_poll_time_);
        if (NULL != rkmessage && checkMsg(rkmessage) && NULL != rkmessage->payload) {
            std::string topic_name(
                NULL != rd_kafka_topic_name(rkmessage->rkt) ? rd_kafka_topic_name(rkmessage->rkt) : "");
            DEBUG(__FUNCTION__ << " | Successed consumed msg of " << rd_kafka_topic_name(rkmessage->rkt)
                               << "[" << rkmessage->partition << "]"
                               << " | offset: " << rkmessage->offset
                               << " | msg len: " << (int)rkmessage->len);

            msg_content_info.topic = topic_name;
            msg_content_info.msg = std::string(static_cast<const char*>(rkmessage->payload), rkmessage->len);
            msg_content_info.msg_len = rkmessage->len;
            msg_content_info.rd_message = rkmessage;

            if (!is_auto_commit_offset_) {
                if (!is_user_manual_commit_offset_) {
                    addWaitCommitOffset(rkmessage);
                } else {
                    addWaitDestroyMsgs(rkmessage);
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
        manualCommitWaitOffset(false);
    }

    return rt;
}
#endif

void QbusConsumer::QbusConsumerImp::commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo) {
    if (is_user_manual_commit_offset_) {
        if (NULL != qbusMsgContentInfo.rd_message) {
            addWaitCommitOffset(qbusMsgContentInfo.rd_message);
            removeWaitDestroyMsgs(qbusMsgContentInfo.rd_message);
        }
    }
}

std::string QbusConsumer::QbusConsumerImp::getWaitOffsetKey(rd_kafka_message_t* msg) {
    std::stringstream ss;
    const char* name = rd_kafka_topic_name(msg->rkt);
    ss << (NULL != name ? name : "") << msg->partition;
    std::string key = ss.str();
    ss.str("");
    return key;
}

void QbusConsumer::QbusConsumerImp::clearWaitDestroyMsgs() {
    unsigned long long msg_count = 0;
    MutexGuard wait_commit_msgs_mutex_guard(wait_commit_msgs_mutex_);
    for (std::map<std::string, RdkafkaMsgVectorType>::iterator i = wait_destroy_msgs_for_uncommit_.begin(),
                                                               e = wait_destroy_msgs_for_uncommit_.end();
         i != e; ++i) {
        RdkafkaMsgVectorType& rdmsg = i->second;
        for (RdkafkaMsgVectorType::iterator k = rdmsg.begin(), ke = rdmsg.end(); k != ke; ++k) {
            rd_kafka_message_t* msg = *k;
            if (NULL != msg) {
                rd_kafka_message_destroy(msg);
                ++msg_count;
                msg = NULL;
            }
        }
    }
}

void QbusConsumer::QbusConsumerImp::addWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message) {
    std::string key = getWaitOffsetKey(rd_kafka_message);

    MutexGuard wait_commit_msgs_mutex_guard(wait_commit_msgs_mutex_);
    std::map<std::string, RdkafkaMsgVectorType>::iterator found = wait_destroy_msgs_for_uncommit_.find(key);
    if (wait_destroy_msgs_for_uncommit_.end() != found) {
        RdkafkaMsgVectorType& msgs = found->second;
        msgs.push_back(rd_kafka_message);
    } else {
        RdkafkaMsgVectorType msgs;
        msgs.push_back(rd_kafka_message);
        wait_destroy_msgs_for_uncommit_.insert(
            std::map<std::string, RdkafkaMsgVectorType>::value_type(key, msgs));
    }
}

void QbusConsumer::QbusConsumerImp::removeWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message) {
    std::string key = getWaitOffsetKey(rd_kafka_message);

    MutexGuard wait_commit_msgs_mutex_guard(wait_commit_msgs_mutex_);
    std::map<std::string, RdkafkaMsgVectorType>::iterator found = wait_destroy_msgs_for_uncommit_.find(key);
    if (wait_destroy_msgs_for_uncommit_.end() != found) {
        RdkafkaMsgVectorType& msgs = found->second;
        RdkafkaMsgVectorType::iterator f = std::find(msgs.begin(), msgs.end(), rd_kafka_message);
        if (f != msgs.end()) {
            for (RdkafkaMsgVectorType::iterator k = msgs.begin();  // this loop added by zk
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
}

void QbusConsumer::QbusConsumerImp::addWaitCommitOffset(rd_kafka_message_t* rd_kafka_message) {
    std::string key = getWaitOffsetKey(rd_kafka_message);

    MutexGuard wait_commit_msgs_mutex_guard(wait_commit_msgs_mutex_);

    std::map<std::string, rd_kafka_message_t*>::iterator found = wait_commit_msgs_.find(key);
    if (wait_commit_msgs_.end() != found) {
        rd_kafka_message_t* msg = found->second;
        if (NULL != msg) {
            rd_kafka_message_destroy(msg);
            msg = NULL;
        }
    }
    wait_commit_msgs_[key] = rd_kafka_message;
}

#ifndef NOT_USE_CONSUMER_CALLBACK
void* QbusConsumer::QbusConsumerImp::ConsumePollThread(void* arg) {
    QbusConsumerImp* consumer = static_cast<QbusConsumerImp*>(arg);

    if (NULL != consumer) {
        rd_kafka_message_t* rdkafka_message = NULL;
        while (consumer->start_flag_) {
            rdkafka_message =
                rd_kafka_consumer_poll(consumer->rd_kafka_handle_, consumer->consumer_poll_time_);
            if (NULL != rdkafka_message && consumer->checkMsg(rdkafka_message)) {
                if (!consumer->is_auto_commit_offset_) {
                    if (!consumer->is_user_manual_commit_offset_) {
                        consumer->addWaitCommitOffset(rdkafka_message);
                    } else {
                        consumer->addWaitDestroyMsgs(rdkafka_message);
                    }
                    consumer->receivedConsumeMsg(rdkafka_message, NULL);
                } else {
                    consumer->receivedConsumeMsg(rdkafka_message, NULL);
                    rd_kafka_message_destroy(rdkafka_message);
                    rdkafka_message = NULL;
                }
            } else if (NULL != rdkafka_message) {
                //! LW!checkMsg失败,也要destroy, 不然会有内存泄漏
                rd_kafka_message_destroy(rdkafka_message);
                rdkafka_message = NULL;
            }

            if (!consumer->is_auto_commit_offset_) {
                consumer->manualCommitWaitOffset(false);
            }
        }

        if (!consumer->is_auto_commit_offset_) {
            consumer->manualCommitWaitOffset(false);
        }
    }

    return (void*)(NULL);
}
#endif

void QbusConsumer::QbusConsumerImp::manualCommitWaitOffset(bool face) {
    long now = QbusHelper::getCurrentTimeMs();

    if (face || now - last_commit_ms_ >= manual_commit_time_) {
        wait_commit_msgs_mutex_.lock();
        for (std::map<std::string, rd_kafka_message_t*>::iterator i = wait_commit_msgs_.begin(),
                                                                  e = wait_commit_msgs_.end();
             i != e; ++i) {
            rd_kafka_message_t* rdmsg = i->second;
            if (NULL != rdmsg) {
                manualCommitOffset(i->second);
                rd_kafka_message_destroy(rdmsg);
                rdmsg = NULL;
            }
        }

        wait_commit_msgs_.clear();

        wait_commit_msgs_mutex_.unlock();
        last_commit_ms_ = now;
    }
}

void QbusConsumer::QbusConsumerImp::manualCommitOffset(const rd_kafka_message_t* rkmessage) {
    rd_kafka_resp_err_t rt = rd_kafka_commit_message(rd_kafka_handle_, rkmessage, 0);
    if (RD_KAFKA_RESP_ERR_NO_ERROR != rt) {
        ERROR(__FUNCTION__ << " | Failed to rd_kafka_commit_message | error msg: " << rd_kafka_err2str(rt));
    }
}

#ifndef NOT_USE_CONSUMER_CALLBACK
void QbusConsumer::QbusConsumerImp::receivedConsumeMsg(rd_kafka_message_t* rkmessage, void* opaque) {
    if (NULL == rkmessage) {
        return;
    }

    std::string topic_name(NULL != rd_kafka_topic_name(rkmessage->rkt) ? rd_kafka_topic_name(rkmessage->rkt)
                                                                       : "");
    DEBUG(__FUNCTION__ << " | Successed consumed msg of " << rd_kafka_topic_name(rkmessage->rkt) << "["
                       << rkmessage->partition << "]"
                       << " | offset: " << rkmessage->offset << " | msg len: " << (int)rkmessage->len);
    if (!is_user_manual_commit_offset_) {
        std::string copy_msg(static_cast<const char*>(rkmessage->payload), rkmessage->len);
        qbus_consumer_callback_.deliveryMsg(topic_name, copy_msg.c_str(), rkmessage->len);
    } else {
        //! LW! set user.manual.commit.offset=true into consumer config file and user need to call to
        //! commitOffset
        QbusMsgContentInfo msg_content_info;
        msg_content_info.topic = topic_name;
        msg_content_info.msg = std::string(
            static_cast<const char*>(rkmessage->payload != NULL ? rkmessage->payload : ""), rkmessage->len);
        msg_content_info.msg_len = rkmessage->len;
        msg_content_info.rd_message = rkmessage;
        qbus_consumer_callback_.deliveryMsgForCommitOffset(msg_content_info);
    }
}
#endif

bool QbusConsumer::QbusConsumerImp::initRdKafkaConfig() {
    bool rt = false;

    rd_kafka_conf_ = rd_kafka_conf_new();
    if (NULL != rd_kafka_conf_) {
        char tmp[16] = {0};
        snprintf(tmp, sizeof(tmp), "%i", SIGIO);
        QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_INTERNAL_TERMINATION_SIGNAL, tmp);

        rd_kafka_conf_set_opaque(rd_kafka_conf_, static_cast<void*>(this));
        rd_kafka_conf_set_rebalance_cb(rd_kafka_conf_, &QbusConsumer::QbusConsumerImp::rdkafka_rebalance_cb);

        rd_kafka_topic_conf_ = rd_kafka_topic_conf_new();
        if (NULL != rd_kafka_topic_conf_) {
            config_loader_.loadRdkafkaConfig(rd_kafka_conf_, rd_kafka_topic_conf_);

            // set group.id if configured
            group_ = config_loader_.getGlobalConfig(RD_KAFKA_CONFIG_GROUP_ID, "");

            // set callback for this log of rdkafka
            std::string enable_rdkafka_log = config_loader_.getSdkConfig(
                RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG, RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG_DEFAULT);
            if (0 == strncasecmp(enable_rdkafka_log.c_str(), RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG_DEFAULT,
                                 enable_rdkafka_log.length())) {
                rd_kafka_conf_set_log_cb(rd_kafka_conf_, NULL);
            } else {
                rd_kafka_conf_set_log_cb(rd_kafka_conf_, &QbusHelper::rdKafkaLogger);
            }

            // set client.id
            QbusHelper::setClientId(rd_kafka_conf_);

            // set stored offset into zk or broker
            if (config_loader_.isSetConfig(RD_KAFKA_TOPIC_CONFIG_OFFSET_STORED_METHOD, true)) {
                rt = true;
            } else {
                rt = QbusHelper::setRdKafkaTopicConfig(rd_kafka_topic_conf_,
                                                       RD_KAFKA_TOPIC_CONFIG_OFFSET_STORED_METHOD,
                                                       RD_KAFKA_TOPIC_CONFIG_OFFSET_STORED_METHOD_BROKER);
            }

            if (!rt) {
                ERROR(__FUNCTION__ << " | set topic config[" << RD_KAFKA_TOPIC_CONFIG_OFFSET_STORED_METHOD
                                   << "] failed");
            } else {
                rd_kafka_conf_set_default_topic_conf(rd_kafka_conf_, rd_kafka_topic_conf_);
                rt = true;
            }

            // set whether auto reset offset when current offset is invalid, default earliest
            if (config_loader_.isSetConfig(RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET, true)) {
                rt = true;
            } else {
                rt = QbusHelper::setRdKafkaTopicConfig(rd_kafka_topic_conf_,
                                                       RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET,
                                                       RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET_EARLIEST);
            }

            if (!rt) {
                ERROR(__FUNCTION__ << " | set topic config[" << RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET
                                   << "] failed");
            } else {
                char auto_reset_offset[20] = {0};
                size_t auto_reset_offset_size = 0;
                rd_kafka_conf_res_t get_auto_reset_offset_rt =
                    rd_kafka_topic_conf_get(rd_kafka_topic_conf_, RD_KAFKA_TOPIC_CONFIG_AUTO_OFFSET_RESET,
                                            auto_reset_offset, &auto_reset_offset_size);
                if (RD_KAFKA_CONF_OK == get_auto_reset_offset_rt) {
                    INFO(__FUNCTION__ << " | Reset offset to " << auto_reset_offset
                                      << " if the current offset is invalid");
                } else {
                    INFO(__FUNCTION__ << " | Reset offset if the current offset is invalid");
                }
            }

            consumer_poll_time_ = atoll(
                config_loader_
                    .getSdkConfig(RD_KAFKA_SDK_CONSUMER_POLL_TIME, RD_KAFKA_SDK_CONSUMER_POLL_TIME_DEFAULT_MS)
                    .c_str());
            manual_commit_time_ = atoll(config_loader_
                                            .getSdkConfig(RD_KAFKA_SDK_CONSUMER_MANUAL_COMMIT_TIME,
                                                          RD_KAFKA_SDK_MANUAL_COMMIT_TIME_DEFAULT_MS_STR)
                                            .c_str());

            // get whether auto commit offset
            char is_auto_commit_offset[10];
            size_t is_auto_commit_offset_size = 0;
            rd_kafka_conf_res_t get_auto_commit_rt =
                rd_kafka_conf_get(rd_kafka_conf_, RD_KAFKA_CONFIG_ENABLE_AUTO_COMMIT, is_auto_commit_offset,
                                  &is_auto_commit_offset_size);
            if (RD_KAFKA_CONF_OK == get_auto_commit_rt) {
                is_auto_commit_offset[is_auto_commit_offset_size] = '\0';
                is_auto_commit_offset_ =
                    (0 == strncasecmp(is_auto_commit_offset, "true", is_auto_commit_offset_size));
            } else {
                ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_get for "
                                   << RD_KAFKA_CONFIG_ENABLE_AUTO_COMMIT);
            }

            // get whether user manual commit offset and if use use manual commit offset, then set
            // is_auto_commit_offset to false
            std::string user_manual_commit_offset = config_loader_.getSdkConfig(
                RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET, RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET_DEFAULT);
            if (0 == strncasecmp(user_manual_commit_offset.c_str(),
                                 RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET_DEFAULT,
                                 user_manual_commit_offset.length())) {
                is_user_manual_commit_offset_ = false;
            } else {
                DEBUG(__FUNCTION__ << " | use user_manual_commit_offset option");
                is_user_manual_commit_offset_ = true;
                if (is_auto_commit_offset_) {
                    is_auto_commit_offset_ = false;
                    QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_ENABLE_AUTO_COMMIT, "false");
                }
            }

            std::string force_destroy =
                config_loader_.getSdkConfig(RD_KAFKA_SDK_FORCE_DESTROY, RD_KAFKA_SDK_FORCE_DESTROY_DEFAULT);
            if (0 == strncasecmp(force_destroy.c_str(), RD_KAFKA_SDK_USER_MANUAL_COMMIT_OFFSET_DEFAULT,
                                 force_destroy.length())) {
                is_force_destroy_ = false;
            } else {
                is_force_destroy_ = true;
            }
        } else {
            ERROR(__FUNCTION__ << " | Failed to rd_kafka_topic_conf_new");
        }

        //! lw! Default compatibility kafka broker 0.9.0.1
        if (!config_loader_.isSetConfig(RD_KAFKA_CONFIG_API_VERSION_REQUEST, false)) {
            QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_API_VERSION_REQUEST,
                                         RD_KAFKA_CONFIG_API_VERSION_REQUEST_FOR_OLDER);
            QbusHelper::setRdKafkaConfig(rd_kafka_conf_, RD_KAFKA_CONFIG_BROKER_VERSION_FALLBACK,
                                         RD_KAFKA_CONFIG_BROKER_VERSION_FALLBACK_FOR_OLDER);
        }
    } else {
        ERROR(__FUNCTION__ << " | Failed to rd_kafka_conf_new");
    }

    return rt;
}

bool QbusConsumer::QbusConsumerImp::initRdKafkaHandle() {
    bool rt = false;

    if (NULL != rd_kafka_handle_) {
        rt = true;
    } else {
        char err_str[512] = {0};
        rd_kafka_handle_ = rd_kafka_new(RD_KAFKA_CONSUMER, rd_kafka_conf_, err_str, sizeof(err_str));
        if (NULL == rd_kafka_handle_) {
            ERROR(__FUNCTION__ << "Failed to create new consumer | error msg:" << err_str);
        } else if (0 == rd_kafka_brokers_add(rd_kafka_handle_, broker_list_.c_str())) {
            ERROR(__FUNCTION__ << " | Failed to rd_kafka_broker_add | broker list:" << broker_list_);
        } else {
            rd_kafka_resp_err_t resp_rs = rd_kafka_poll_set_consumer(rd_kafka_handle_);
            if (RD_KAFKA_RESP_ERR_NO_ERROR != resp_rs) {
                ERROR(__FUNCTION__ << " | Failed to rd_kafka_poll_set_consumer | err msg:"
                                   << rd_kafka_err2str(resp_rs));
            } else {
                rt = true;
            }
            rt = true;
        }
    }

    return rt;
}

bool QbusConsumer::QbusConsumerImp::initRdKafka() {
    INFO(__FUNCTION__ << " | Librdkafka version: " << rd_kafka_version_str() << " " << rd_kafka_version());

    return initRdKafkaConfig();
}

void QbusConsumer::QbusConsumerImp::resetAllOffsetOfAllPartitionOfTopic(
    rd_kafka_topic_partition_list_t* partitions) {
    if (NULL != partitions) {
        rd_kafka_resp_err_t err = rd_kafka_position(rd_kafka_handle_, partitions);
        if (RD_KAFKA_RESP_ERR_NO_ERROR == err) {
            for (int i = 0; i < partitions->cnt; ++i) {
                rd_kafka_topic_partition_t* p = &partitions->elems[i];
                if (NULL != p) {
                    if (p->err) {
                        ERROR(__FUNCTION__ << " | Failed to rd_kafka_position"
                                           << " | topic: " << (NULL != p->topic ? p->topic : "")
                                           << " | partition: " << p->partition
                                           << " | err msg: " << rd_kafka_err2str(p->err));
                    } else {
                        DEBUG(__FUNCTION__ << " | Sucesses to rd_kafka_position"
                                           << " | topic: " << (NULL != p->topic ? p->topic : "")
                                           << " | partition: " << p->partition << " | offset: " << p->offset);
                        if (RD_KAFKA_OFFSET_ERROR == p->offset) {
                            // TODO:set offset
                            // p->offset = RD_KAFKA_OFFSET_BEGINNING;
                            // p->offset = RD_KAFKA_OFFSET_END;
                            // p->offset = RD_KAFKA_OFFSET_TAIL(1);
                        }
                    }
                }
            }
        }
    }
}

void QbusConsumer::QbusConsumerImp::rdkafka_rebalance_cb(rd_kafka_t* rk, rd_kafka_resp_err_t err,
                                                         rd_kafka_topic_partition_list_t* partitions,
                                                         void* opaque) {
    assert(opaque);
    QbusConsumerImp* consumer_imp = static_cast<QbusConsumerImp*>(opaque);

    switch (err) {
        case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
            INFO(__FUNCTION__ << " | rebalance result OK: "
                              << QbusHelper::formatTopicPartitionList(partitions));
            assert(consumer_imp);
            // consumer_imp->resetAllOffsetOfAllPartitionOfTopic(partitions);
            rd_kafka_assign(rk, partitions);
            consumer_imp->topic_partition_set_.init(partitions);
            consumer_imp->setHasAssigned(true);
            break;
        case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
            INFO(__FUNCTION__ << " | rebalance result revoke | msg: " << rd_kafka_err2str(err) << " | "
                              << QbusHelper::formatTopicPartitionList(partitions));
            assert(consumer_imp);
            if (!consumer_imp->is_auto_commit_offset_) {
                consumer_imp->manualCommitWaitOffset(true);
            }

            rd_kafka_assign(rk, NULL);
            consumer_imp->setHasAssigned(false);
            break;
        default:
            ERROR(__FUNCTION__ << " | Failed to rebalance | err msg: " << rd_kafka_err2str(err));
            rd_kafka_assign(rk, NULL);
            consumer_imp->setHasAssigned(false);
            break;
    }
}

bool QbusConsumer::QbusConsumerImp::has_assigned() const {
    MutexGuard has_assigned_mutex_guard(has_assigned_mutex_);
    return has_assigned_;
}

void QbusConsumer::QbusConsumerImp::setHasAssigned(bool has_assigned) {
    MutexGuard has_assigned_mutex_guard(has_assigned_mutex_);
    has_assigned_ = has_assigned;
}

//------------------------------------------------------------
QbusConsumer::QbusConsumer() : qbus_consumer_imp_(NULL) {}

QbusConsumer::~QbusConsumer() {
    if (NULL != qbus_consumer_imp_) {
        delete qbus_consumer_imp_;
        qbus_consumer_imp_ = NULL;
    }
}

bool QbusConsumer::init(const std::string& cluster_name, const std::string& log_path,
                        const std::string& config_path, const QbusConsumerCallback& callback) {
    bool rt = false;

    qbus_consumer_imp_ = new QbusConsumerImp(cluster_name
#ifndef NOT_USE_CONSUMER_CALLBACK
                                             ,
                                             callback
#endif
    );
    if (NULL != qbus_consumer_imp_) {
        rt = qbus_consumer_imp_->init(log_path, config_path);
    }

    if (rt) {
        INFO(__FUNCTION__ << " | QbusConsumer::init is OK");
    } else {
        ERROR(__FUNCTION__ << " | Failed to QbusConsumer::init");
    }

    return rt;
}

bool QbusConsumer::subscribe(const std::string& group, const std::vector<std::string>& topics) {
    bool rt = false;

    for (std::vector<std::string>::const_iterator i = topics.begin(), e = topics.end(); i != e; ++i) {
        INFO(__FUNCTION__ << " | group: " << group << " | topic: " << *i);
    }

    if (NULL != qbus_consumer_imp_) {
        rt = qbus_consumer_imp_->subscribe(group, topics);
    }

    if (rt) {
        INFO(__FUNCTION__ << " | QbusConsumer::subscribe is OK");
    } else {
        ERROR(__FUNCTION__ << " | Failed to QbusConsumer::subscribe");
    }

    return rt;
}

bool QbusConsumer::subscribeOne(const std::string& group, const std::string& topic) {
    std::vector<std::string> topics;
    topics.push_back(topic);
    return subscribe(group, topics);
}

bool QbusConsumer::start() {
    bool rt = false;
    if (NULL != qbus_consumer_imp_) {
        rt = qbus_consumer_imp_->start();
    }

    if (rt) {
        INFO(__FUNCTION__ << " | QbusConsumer::start OK");
    } else {
        ERROR(__FUNCTION__ << " | Failed to QbusConsumer::start");
    }

    return rt;
}

void QbusConsumer::stop() {
    if (NULL != qbus_consumer_imp_) {
        qbus_consumer_imp_->stop();
    }
}

bool QbusConsumer::pause(const std::vector<std::string>& topics) {
    if (NULL != qbus_consumer_imp_) {
        return qbus_consumer_imp_->pause(topics);
    }
    return false;
}

bool QbusConsumer::resume(const std::vector<std::string>& topics) {
    if (NULL != qbus_consumer_imp_) {
        return qbus_consumer_imp_->resume(topics);
    }
    return false;
}

void QbusConsumer::commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo) {
    if (NULL != qbus_consumer_imp_) {
        qbus_consumer_imp_->commitOffset(qbusMsgContentInfo);
    }
}

bool QbusConsumer::consume(QbusMsgContentInfo& msg_content_info) {
    bool rt = false;

#ifdef NOT_USE_CONSUMER_CALLBACK
    if (NULL != qbus_consumer_imp_) {
        rt = qbus_consumer_imp_->consume(msg_content_info);
    }
#endif

    return rt;
}

}  // namespace kafka
}  // namespace qbus
