#ifndef QBUS_TOPIC_PARTITION_LIST_H_
#define QBUS_TOPIC_PARTITION_LIST_H_

#include <map>
#include <set>
#include <string>
#include <vector>

struct rd_kafka_topic_partition_list_s;

namespace qbus {

class TopicPartitionSet {
 public:
  typedef std::set<int> IntSet;
  typedef std::map<std::string, IntSet> MapStrToIntSet;

  bool empty() const { return data_.empty(); }

  // Init \c data_ from \p lst.
  // If \p lst is null, treat it as an empty lst.
  void init(const rd_kafka_topic_partition_list_s* lst);

  // Returns a list that contains all partitions of each topic of \p topics.
  // Partitions were found from \c data_.
  rd_kafka_topic_partition_list_s* findTopics(
      const std::vector<std::string>& topics) const;

 private:
  // Map: topic name => partition id set
  MapStrToIntSet data_;
};

}  // namespace qbus

#endif  // QBUS_TOPIC_PARTITION_LIST_H_
