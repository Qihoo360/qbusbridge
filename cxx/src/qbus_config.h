#ifndef QBUS_CONFIG_H
#define QBUS_CONFIG_H

#include <string>

#include <boost/property_tree/ptree.hpp>

#include "thirdparts/librdkafka/src/rdkafka.h"

namespace pt = boost::property_tree;

namespace qbus {
class QbusConfigLoader {
 public:
  enum ConfigType {
    CT_CONSUMER = 0,
    CT_PRODUCER,
  };

  QbusConfigLoader() {}

  void LoadConfig(const std::string& path);
  void LoadRdkafkaConfig(rd_kafka_conf_t* rd_kafka_conf,
                         rd_kafka_topic_conf_t* rd_kafka_topic_conf);
  bool IsSetConfig(const std::string& config_name, bool is_topic_config) const;
  std::string GetSdkConfig(const std::string& config_name,
                           const std::string& default_value) const;

 private:
  pt::ptree root_tree_;
  pt::ptree set_global_config_items_;
  pt::ptree set_topic_config_items_;
  pt::ptree set_sdk_configs_;

 private:
  QbusConfigLoader(const QbusConfigLoader&);
  QbusConfigLoader& operator=(const QbusConfigLoader&);
};  // QbusConfigLoader

}  // namespace qbus

#endif  //#define QBUS_CONFIG_H
