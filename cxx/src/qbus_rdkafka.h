#ifndef QBUS_RDKAFKA_H_
#define QBUS_RDKAFKA_H_

#include "librdkafka/rdkafka.h"

namespace qbus {

namespace rdkafka {

// Returns true if there is any broker which is not broken.
bool hasAnyBroker(rd_kafka_t* rk);

struct PartitionListGuard {
  rd_kafka_topic_partition_list_t* lst_;

  PartitionListGuard(rd_kafka_topic_partition_list_t* lst) : lst_(lst) {}
  ~PartitionListGuard() {
    if (lst_) rd_kafka_topic_partition_list_destroy(lst_);
  }
};

}  // namespace rdkafka

}  // namespace qbus

#endif  // QBUS_RDKAFKA_H_
