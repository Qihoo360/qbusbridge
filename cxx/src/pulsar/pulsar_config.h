#pragma once

#include <memory>
#include <stdexcept>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace qbus {
namespace pulsar {

class QbusProducerConfig;
class QbusConsumerConfig;

class PulsarConfig {
   public:
    class Error : public std::runtime_error {
       public:
        Error(const std::string& msg) : std::runtime_error(msg) {}
    };

    class NotFound : public Error {
       public:
        NotFound(const std::string& msg) : Error(msg + " not found") {}
    };

    static constexpr const char* KEY_BROKER = "broker";
    static constexpr const char* KEY_SUBSCRIPTION = "subscription";
    static constexpr const char* KEY_PERSISTENT = "persistent";
    static constexpr const char* KEY_TENANT = "tenant";
    static constexpr const char* KEY_NAMESPACE = "namespace";
    static constexpr const char* KEY_LOG_LEVEL = "log.level";

    PulsarConfig() = default;
    PulsarConfig(const PulsarConfig&) = delete;
    PulsarConfig& operator=(const PulsarConfig&) = delete;

    /**
     * Load config from local file
     *
     * @param filename local file's path
     * @throws PulsarConfig::Error if failed
     */
    void loadConfig(const std::string& filename);

    /**
     * Manual put a key-value pair to `config_`
     */
    void put(const std::string& key, const std::string& value);

    /**
     * Get broker's service url, eg. "pulsar://localhost:6650"
     *
     * @throws PulsarConfig::NotFound if not found
     */
    std::string broker() const;

    /**
     * Get consumer's subscription
     *
     * @throws PulsarConfig::NotFound if not found
     */
    std::string subscription() const;

    /**
     * Get prefix of topic url, eg. "persistent://my-tenant/my-namespace/"
     * If persistent, tenant or namespace is not configured, use following default configs:
     *   persistent: true
     *   tenant:     public
     *   namespace:  default
     * i.e. "persistent://public/default/"
     *
     * @return prefix of topic url
     */
    std::string topic_prefix() const;

    /**
     * Get log level, must be one of "debug", "info", "warn", "error" (ignore case)
     *
     * @return log level string. default: "info"
     */
    std::string log_level() const;

    std::unique_ptr<QbusProducerConfig> producer_config() const;

    std::unique_ptr<QbusConsumerConfig> consumer_config() const;

   private:
    pt::ptree config_;

    template <typename T>
    T find(const std::string& key, const T& default_value) const {
        auto kv = config_.find(key);
        if (kv != config_.not_found()) {
            return kv->second.get_value<T>();
        } else {
            return default_value;
        }
    }
};

}  // namespace pulsar
}  // namespace qbus
