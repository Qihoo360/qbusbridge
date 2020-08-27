#include "pulsar_config.h"

#include <unordered_set>
#include "make_unique.h"
#include "qbus_producer_config.h"
#include "qbus_consumer_config.h"

namespace qbus {
namespace pulsar {

void PulsarConfig::put(const std::string& key, const std::string& value) {
    // Here we don't catch ptree_bad_data, because conversion from string to string won't fail
    config_.put(key, value);
}

void PulsarConfig::loadConfig(const std::string& filename) {
    try {
        pt::ini_parser::read_ini(filename, config_);
    } catch (const pt::ini_parser_error& e) {
        throw Error(std::string("Failed to load config: ") + e.what());
    }
}

std::string PulsarConfig::broker() const {
    auto iter = config_.find(KEY_BROKER);
    if (iter == config_.not_found()) {
        throw NotFound(KEY_BROKER);
    }

    return "pulsar://" + iter->second.get_value<std::string>();
}

std::string PulsarConfig::subscription() const {
    auto iter = config_.find(KEY_SUBSCRIPTION);
    if (iter == config_.not_found()) {
        throw NotFound(KEY_SUBSCRIPTION);
    }

    return iter->second.get_value<std::string>();
}

std::string PulsarConfig::topic_prefix() const {
    std::string persistent = find<bool>(KEY_PERSISTENT, true) ? "persistent" : "non-persistent";
    auto tenant = find<std::string>(KEY_TENANT, "public");
    auto namespace_ = find<std::string>(KEY_NAMESPACE, "default");

    return persistent + "://" + tenant + "/" + namespace_ + "/";
}

std::string PulsarConfig::log_level() const {
    static const std::unordered_set<std::string> kValidLogLevels{"debug", "info", "warn", "error"};

    auto iter = config_.find(KEY_LOG_LEVEL);
    if (iter == config_.not_found()) {
        return "info";
    }

    auto level = iter->second.get_value<std::string>();
    if (kValidLogLevels.find(level) == kValidLogLevels.cend()) {
        return "info";
    }
    return level;
}

std::unique_ptr<QbusProducerConfig> PulsarConfig::producer_config() const {
    auto child = config_.get_child_optional("producer");
    if (child) {
        return std::make_unique<QbusProducerConfig>(*child);
    } else {
        return std::make_unique<QbusProducerConfig>();
    }
}

std::unique_ptr<QbusConsumerConfig> PulsarConfig::consumer_config() const {
    auto child = config_.get_child_optional("consumer");
    if (child) {
        return std::make_unique<QbusConsumerConfig>(*child);
    } else {
        return std::make_unique<QbusConsumerConfig>();
    }
}

}  // namespace pulsar
}  // namespace qbus
