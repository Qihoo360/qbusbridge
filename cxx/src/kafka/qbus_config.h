#pragma once

#include <string>
#include <map>
#include "qbus_rdkafka.h"

//------------------------------------------------------
namespace qbus {
namespace kafka {
class QbusConfigLoader {
   public:
    enum ConfigType
    {
        CT_CONSUMER = 0,
        CT_PRODUCER,
    };

    QbusConfigLoader() {}

    bool loadConfig(const std::string& path, std::string& errstr);
    void loadRdkafkaConfig(rd_kafka_conf_t* rd_kafka_conf, rd_kafka_topic_conf_t* rd_kafka_topic_conf);
    bool isSetConfig(const std::string& config_name, bool is_topic_config) const;
    std::string getGlobalConfig(const std::string& config_name, const std::string& default_value = "") const;
    std::string getTopicConfig(const std::string& config_name, const std::string& default_value = "") const;
    std::string getSdkConfig(const std::string& config_name, const std::string& default_value = "") const;

   private:
    typedef std::map<std::string, std::string> MapStringToString;
    MapStringToString set_global_config_items_;
    MapStringToString set_topic_config_items_;
    MapStringToString set_sdk_configs_;

    static std::string getConfig(const MapStringToString& config_map, const std::string& config_name,
                                 const std::string& default_value);

   private:
    QbusConfigLoader(const QbusConfigLoader&);
    QbusConfigLoader& operator=(const QbusConfigLoader&);
};
}  // namespace kafka
}  // namespace qbus
