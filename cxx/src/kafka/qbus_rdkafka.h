#pragma once

#include <librdkafka/rdkafka.h>
#include "util/logger.h"

#include <sstream>
#include <string>

namespace qbus {
namespace kafka {
namespace rdkafka {

struct TopicGuard {
    rd_kafka_topic_t* rkt_;

    TopicGuard(rd_kafka_topic_t* rkt) : rkt_(rkt) {}
    ~TopicGuard() {
        if (rkt_) rd_kafka_topic_destroy(rkt_);
    }
};

struct PartitionListGuard {
    rd_kafka_topic_partition_list_t* lst_;

    PartitionListGuard(rd_kafka_topic_partition_list_t* lst) : lst_(lst) {}
    ~PartitionListGuard() {
        if (lst_) {
            rd_kafka_topic_partition_list_destroy(lst_);
        }
    }
};

// If partition_cnt > 0, returns all partitions of each topic in \p topics;
// else returns RD_KAFKA_PARTITION_UA of each topic in \p topics.
rd_kafka_topic_partition_list_t* createTopicLists(const std::vector<std::string>& topics, int partition_cnt);

// Returns comma-seperated "<topic>[<partition>]" formatted string of \p lst.
// If \p show_offsets is true, add offsets info ": <offset>" for each partition.
// If lst is null, returns "(null)".
std::string formatPartitionList(rd_kafka_topic_partition_list_t* lst, bool show_offsets = false);

}  // namespace rdkafka
}  // namespace kafka
}  // namespace qbus
