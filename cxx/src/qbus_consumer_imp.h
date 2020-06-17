#ifndef QBUS_CONSUMER_IMP_H_
#define QBUS_CONSUMER_IMP_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "qbus_config.h"
#include "qbus_rdkafka.h"
#include "qbus_topic_partition_set.h"
//------------------------------------------------------------
namespace qbus {

class QbusConsumerCallback;
class QbusMsgContentInfo;

class QbusConsumerImp {
 public:
  QbusConsumerImp(const std::string& broker_list
#ifndef NOT_USE_CONSUMER_CALLBACK
                  ,
                  const QbusConsumerCallback& callback
#endif
  );
  ~QbusConsumerImp();

 public:
  bool Init(const std::string& log_path, const std::string& config_path);
  bool Subscribe(const std::string& group,
                 const std::vector<std::string>& topics);
  bool Start();
  void Stop();

#ifdef NOT_USE_CONSUMER_CALLBACK
  bool Consume(QbusMsgContentInfo& msg_content_info);
#endif

  void CommitOffset(const QbusMsgContentInfo& qbusMsgContentInfo);

  bool Pause(const std::vector<std::string>& topics);
  bool Resume(const std::vector<std::string>& topics);

 private:
  static void rdkafka_rebalance_cb(rd_kafka_t* rk, rd_kafka_resp_err_t err,
                                   rd_kafka_topic_partition_list_t* partitions,
                                   void* opaque);
#ifndef NOT_USE_CONSUMER_CALLBACK
  static void* ConsumePollThread(void* arg);
#endif

  bool InitRdKafka();
  bool InitRdKafkaHandle();
  bool InitRdKafkaConfig();
#ifndef NOT_USE_CONSUMER_CALLBACK
  void ReceivedConsumeMsg(rd_kafka_message_t* rkmessage, void* opaque);
#endif

  void ManualCommitOffset(const rd_kafka_message_t* rkmessage);
  bool CheckMsg(rd_kafka_message_t* rdkafka_massage);
  void ManualCommitWaitOffset(bool face);
  void AddWaitCommitOffset(rd_kafka_message_t* rd_kafka_message);
  std::string GetWaitOffsetKey(rd_kafka_message_t* msg);
  void ClearWaitDestroyMsgs();
  void AddWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message);
  void RemoveWaitDestroyMsgs(rd_kafka_message_t* rd_kafka_message);

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
  bool force_terminate_;

  long last_commit_ms_;
  long long consumer_poll_time_;
  long long manual_commit_time_;

  pthread_t poll_thread_id_;

  QbusConfigLoader config_loader_;

#ifndef NOT_USE_CONSUMER_CALLBACK
  const QbusConsumerCallback& qbus_consumer_callback_;
#endif

  std::string group_;
  std::vector<std::string> topics_;

  pthread_mutex_t wait_commit_msgs_mutex_;
  std::map<std::string, rd_kafka_message_t*> wait_commit_msgs_;

  typedef std::vector<rd_kafka_message_t*> RdkafkaMsgVectorType;
  std::map<std::string, RdkafkaMsgVectorType> wait_destroy_msgs_for_uncommit_;

  // Map: topic name => partition id set
  TopicPartitionSet topic_partition_set_;

  // Since partitions to `pause()` must be previously assigned, we have to
  // ensure that `pause()` is called after `rd_kafka_assign()`.
  bool has_assigned_;
  mutable pthread_mutex_t has_assigned_mutex_;

  // Synchronized getter and setter
  bool has_assigned() const;
  void SetHasAssigned(bool new_value);

  bool ReadyToPauseResume() const;
};
}  // namespace qbus

#endif  // QBUS_PRODUCER_IMP_H_
