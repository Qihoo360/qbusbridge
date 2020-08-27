#include "qbus_config.h"

#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>

#include "util/logger.h"
#include "qbus_helper.h"

namespace pt = boost::property_tree;
//-----------------------------------------------------
namespace {
void internalLoadConfig(const char* config_item, pt::ptree& root_tree,
                        std::map<std::string, std::string>& configs) {
    boost::optional<pt::ptree&> tree_optional = root_tree.get_child_optional(config_item);
    if (tree_optional) {
        pt::ptree& tree = *tree_optional;
        for (pt::ptree::iterator i = tree.begin(); i != tree.end(); ++i) {
            const pt::ptree::data_type& k_data = i->first.data();
            const pt::ptree::data_type& v_data = i->second.data();

            configs.insert(std::make_pair(k_data, v_data));
        }
    }
}
}  // namespace

namespace qbus {
namespace kafka {

static const char* GLOBAL_CONFIG = "global";
static const char* TOPIC_CONFIG = "topic";
static const char* SDK_CONFIG = "sdk";

bool QbusConfigLoader::loadConfig(const std::string& path, std::string& errstr) {
    pt::ptree root_tree;
    try {
        pt::ini_parser::read_ini(path, root_tree);
    } catch (const pt::ini_parser_error& e) {
        errstr = e.what();
        return false;
    }

    internalLoadConfig(GLOBAL_CONFIG, root_tree, set_global_config_items_);

    internalLoadConfig(TOPIC_CONFIG, root_tree, set_topic_config_items_);

    internalLoadConfig(SDK_CONFIG, root_tree, set_sdk_configs_);

    return true;
}

void QbusConfigLoader::loadRdkafkaConfig(rd_kafka_conf_t* rd_kafka_conf,
                                         rd_kafka_topic_conf_t* rd_kafka_topic_conf) {
    for (std::map<std::string, std::string>::iterator i = set_global_config_items_.begin();
         i != set_global_config_items_.end(); ++i) {
        QbusHelper::setRdKafkaConfig(rd_kafka_conf, i->first.c_str(), i->second.c_str());
    }

    for (std::map<std::string, std::string>::iterator i = set_topic_config_items_.begin();
         i != set_topic_config_items_.end(); ++i) {
        QbusHelper::setRdKafkaTopicConfig(rd_kafka_topic_conf, i->first.c_str(), i->second.c_str());
    }
}

std::string QbusConfigLoader::getConfig(const MapStringToString& config_map, const std::string& config_name,
                                        const std::string& default_value) {
    std::string value(default_value);

    MapStringToString::const_iterator found = config_map.find(config_name);
    if (config_map.end() != found) {
        value = found->second;
    }

    return value;
}

std::string QbusConfigLoader::getGlobalConfig(const std::string& config_name,
                                              const std::string& default_value) const {
    return getConfig(set_global_config_items_, config_name, default_value);
}

std::string QbusConfigLoader::getTopicConfig(const std::string& config_name,
                                             const std::string& default_value) const {
    return getConfig(set_topic_config_items_, config_name, default_value);
}

std::string QbusConfigLoader::getSdkConfig(const std::string& config_name,
                                           const std::string& default_value) const {
    return getConfig(set_sdk_configs_, config_name, default_value);
}

bool QbusConfigLoader::isSetConfig(const std::string& config_name, bool is_topic_config) const {
    DEBUG(__FUNCTION__ << " | Check topic config | key: " << config_name);
    return is_topic_config ? set_topic_config_items_.find(config_name) != set_topic_config_items_.end()
                           : set_global_config_items_.find(config_name) != set_global_config_items_.end();
}

}  // namespace kafka
}  // namespace qbus
