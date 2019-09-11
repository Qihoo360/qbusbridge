#include "qbus_config.h"

#include <signal.h>

#include <fstream>
#include <iostream>

#include <boost/optional.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "util/logger.h"

#include "qbus_constant.h"
#include "qbus_helper.h"

namespace qbus {

static const char* GLOBAL_CONFIG = "global";
static const char* TOPIC_CONFIG = "topic";
static const char* SDK_CONFIG = "sdk";
static const char INI_CONFIG_KEY_VALUE_SPLIT = '|';

void QbusConfigLoader::LoadConfig(const std::string& path) {
  std::ifstream stream(path.c_str());
  if (!stream) {
    WARNING(__FUNCTION__ << " | Can't open config file : " << path
                         << " | use default config");
    return;
  }
  stream.close();

  pt::ini_parser::read_ini(path, root_tree_);
  boost::optional<pt::ptree&> set_global_config_items =
      root_tree_.get_child_optional(GLOBAL_CONFIG);
  if (set_global_config_items) {
    set_global_config_items_ = *set_global_config_items;
  }
  boost::optional<pt::ptree&> set_topic_config_items =
      root_tree_.get_child_optional(TOPIC_CONFIG);
  if (set_topic_config_items) {
    set_topic_config_items_ = *set_topic_config_items;
  }
  boost::optional<pt::ptree&> set_sdk_configs =
      root_tree_.get_child_optional(SDK_CONFIG);
  if (set_sdk_configs) {
    set_sdk_configs_ = *set_sdk_configs;
  }
}

void QbusConfigLoader::LoadRdkafkaConfig(
    rd_kafka_conf_t* rd_kafka_conf,
    rd_kafka_topic_conf_t* rd_kafka_topic_conf) {
  char tmp[16] = {0};
  snprintf(tmp, sizeof(tmp), "%i", SIGIO);
  QbusHelper::SetRdKafkaConfig(
      rd_kafka_conf, RD_KAFKA_CONFIG_INTERNAL_TERMINATION_SIGNAL, tmp);

  std::string enable_rdkafka_log =
      GetSdkConfig(RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG,
                   RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG_DEFAULT);
  if (0 == strncasecmp(enable_rdkafka_log.c_str(),
                       RD_KAFKA_SDK_CONFIG_ENABLE_RD_KAFKA_LOG_DEFAULT,
                       enable_rdkafka_log.length())) {
    rd_kafka_conf_set_log_cb(rd_kafka_conf, NULL);
  } else {
    rd_kafka_conf_set_log_cb(rd_kafka_conf, &QbusHelper::RdKafkaLogger);
  }

  for (pt::ptree::iterator i = set_global_config_items_.begin(),
                           e = set_global_config_items_.end();
       i != e; ++i) {
    QbusHelper::SetRdKafkaConfig(rd_kafka_conf, i->first.c_str(),
                                 i->second.data().c_str());
  }

  for (pt::ptree::iterator i = set_topic_config_items_.begin(),
                           e = set_topic_config_items_.end();
       i != e; ++i) {
    QbusHelper::SetRdKafkaTopicConfig(rd_kafka_topic_conf, i->first.c_str(),
                                      i->second.data().c_str());
  }
}

std::string QbusConfigLoader::GetSdkConfig(
    const std::string& config_name, const std::string& default_value) const {
  std::string value(default_value);

  boost::optional<std::string> found =
      set_sdk_configs_.get_optional<std::string>(
          pt::ptree::path_type(config_name, INI_CONFIG_KEY_VALUE_SPLIT));
  if (boost::none != found) {
    value = *found;
  }

  return value;
}

bool QbusConfigLoader::IsSetConfig(const std::string& config_name,
                                   bool is_topic_config) const {
  DEBUG(__FUNCTION__ << " | Check topic config | key: " << config_name);
  return is_topic_config
             ? set_topic_config_items_.get_child_optional(pt::ptree::path_type(
                   config_name, INI_CONFIG_KEY_VALUE_SPLIT)) != boost::none
             : set_global_config_items_.get_child_optional(pt::ptree::path_type(
                   config_name, INI_CONFIG_KEY_VALUE_SPLIT)) != boost::none;
}

}  // namespace qbus
