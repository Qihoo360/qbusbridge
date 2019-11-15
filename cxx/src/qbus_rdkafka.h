#ifndef QBUS_RDKAFKA_H_
#define QBUS_RDKAFKA_H_

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

}  // namespace rdkafka

}  // namespace qbus

#endif  // QBUS_RDKAFKA_H_
