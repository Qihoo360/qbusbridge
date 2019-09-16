#ifndef QBUS_PRODUCER_IMP_H_
#define QBUS_PRODUCER_IMP_H_

#include <string>

#include "qbus_config.h"
//-----------------------------------------------------
namespace qbus {

class QbusProducerImp {
 public:
  QbusProducerImp();
  ~QbusProducerImp();

  bool Init(const std::string& cluster_name, const std::string& log_path,
            const std::string& topic_name, const std::string& config_path);
  void Uninit();

  bool Produce(const char* data, size_t data_len, const std::string& key);

 private:
  static void MsgDeliveredCallback(rd_kafka_t* rk,
                                   const rd_kafka_message_t* rkmessage,
                                   void* opaque);
  static int32_t PartitionHashFunc(const rd_kafka_topic_t* rkt,
                                   const void* keydata, size_t keylen,
                                   int32_t partition_cnt, void* rkt_opaque,
                                   void* msg_opaque);

  bool InitRdKafkaConfig();
  bool InitRdKafkaHandle(const std::string& topic_name);

  bool InternalProduce(const char* data, size_t data_len,
                       const std::string& key, void* opaque);

 private:
  rd_kafka_conf_t* rd_kafka_conf_;
  rd_kafka_topic_conf_t* rd_kafka_topic_conf_;
  rd_kafka_topic_t* rd_kafka_topic_;
  rd_kafka_t* rd_kafka_handle_;
  rd_kafka_resp_err_t sync_send_err_;

  std::string broker_list_;

  QbusConfigLoader config_loader_;

  bool is_sync_send_;
  bool is_init_;
  bool is_record_msg_for_send_failed_;
  bool is_speedup_terminate_;
  bool fast_exit_;
};
}  // namespace qbus
#endif  //#define QBUS_PRODUCER_IMP_H_
