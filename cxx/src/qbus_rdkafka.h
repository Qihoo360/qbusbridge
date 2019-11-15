#ifndef QBUS_RDKAFKA_H_
#define QBUS_RDKAFKA_H_

#include <string>
#include "librdkafka/rdkafka.h"

namespace qbus {

namespace rdkafka {

struct PartitionListGuard {
  rd_kafka_topic_partition_list_t* lst_;

  PartitionListGuard(rd_kafka_topic_partition_list_t* lst) : lst_(lst) {}
  ~PartitionListGuard() {
    if (lst_) rd_kafka_topic_partition_list_destroy(lst_);
  }
};

inline std::string BytesToString(const void* data, size_t size,
                                 const char* default_value = "") {
  std::string value = default_value;
  if (data) value = std::string(static_cast<const char*>(data), size);
  return value;
}

class MessageRef {
 public:
  MessageRef(const rd_kafka_message_t& msg) : msg_(msg) {}

  // --START------------------------------------------------------------------
  // getters of fields of rd_kafka_message_t
  rd_kafka_resp_err_t err() const { return msg_.err; }
  rd_kafka_topic_t* rkt() const { return msg_.rkt; }
  int32_t partition() const { return msg_.partition; }
  void* payload() const { return msg_.payload; }
  size_t len() const { return msg_.len; }
  void* key() const { return msg_.key; }
  size_t key_len() const { return msg_.key_len; }
  int64_t offset() const { return msg_.offset; }
  // --END--------------------------------------------------------------------

  bool hasError() const { return err() != RD_KAFKA_RESP_ERR_NO_ERROR; }
  const char* errorString() const { return rd_kafka_err2str(err()); }

  const char* topicName() const { return rd_kafka_topic_name(rkt()); }

  std::string keyString() const { return BytesToString(key(), key_len(), "<null>"); }

  std::string payloadString() const { return BytesToString(payload(), len()); }

 private:
  const rd_kafka_message_t& msg_;
};

}  // namespace rdkafka

}  // namespace qbus

#endif  // QBUS_RDKAFKA_H_
