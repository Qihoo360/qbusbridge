#include "qbus_topic_partition_set.h"
#include <sstream>
#include "qbus_rdkafka.h"
#include "util/logger.h"

namespace qbus {
namespace kafka {

void TopicPartitionSet::init(const rd_kafka_topic_partition_list_t* lst) {
    if (!lst) {
        data_.clear();
        return;
    }

    for (int i = 0; i < lst->cnt; i++) {
        const rd_kafka_topic_partition_t& p = lst->elems[i];

        // NOTE: If key (p.topic) exists, map::insert() returns an iterator to the
        // element that prevented the insertion and a false value.
        std::pair<MapStrToIntSet::iterator, bool> res = data_.insert(std::make_pair(p.topic, IntSet()));
        res.first->second.insert(p.partition);
    }
}

rd_kafka_topic_partition_list_t* TopicPartitionSet::findTopics(const std::vector<std::string>& topics) const {
    rd_kafka_topic_partition_list_t* lst = rd_kafka_topic_partition_list_new(1);

    for (size_t i = 0; i < topics.size(); i++) {
        MapStrToIntSet::const_iterator iter = data_.find(topics[i]);
        if (iter == data_.end()) {
            WARNING(__FUNCTION__ << " | topic: " << topics[i] << " not found");
            continue;
        }

        const IntSet& s = iter->second;
        for (IntSet::const_iterator id_iter = s.begin(); id_iter != s.end(); ++id_iter) {
            rd_kafka_topic_partition_list_add(lst, iter->first.c_str(), static_cast<int32_t>(*id_iter));
        }
    }

    return lst;
}

}  // namespace kafka
}  // namespace qbus
