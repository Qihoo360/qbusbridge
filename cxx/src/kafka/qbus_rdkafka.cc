#include "qbus_rdkafka.h"

namespace qbus {
namespace kafka {
namespace rdkafka {

rd_kafka_topic_partition_list_t* createTopicLists(const std::vector<std::string>& topics, int partition_cnt) {
    rd_kafka_topic_partition_list_t* lst = rd_kafka_topic_partition_list_new(partition_cnt);

    for (size_t i = 0; i < topics.size(); i++) {
        if (partition_cnt >= 0) {
            rd_kafka_topic_partition_list_add_range(lst, topics[i].c_str(), 0, partition_cnt - 1);
        } else {
            rd_kafka_topic_partition_list_add(lst, topics[i].c_str(), RD_KAFKA_PARTITION_UA);
        }
    }

    return lst;
}

std::string formatPartitionList(rd_kafka_topic_partition_list_t* lst, bool show_offsets) {
    if (!lst) return "(null)";

    std::ostringstream oss;
    oss << "{";

    for (int i = 0; i < lst->cnt; i++) {
        if (i > 0) oss << ", ";

        const rd_kafka_topic_partition_t& p = lst->elems[i];
        oss << p.topic << "[" << p.partition << "]";

        if (show_offsets) oss << ":" << p.offset;
    }

    oss << "}";
    return oss.str();
}

}  // namespace rdkafka
}  // namespace kafka
}  // namespace qbus
