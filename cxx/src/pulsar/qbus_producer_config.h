#pragma once

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace qbus {
namespace pulsar {

struct QbusProducerConfig {
    QbusProducerConfig() = default;

    QbusProducerConfig(const pt::ptree& tree);

    std::string to_string() const;

    static constexpr const char* KEY_AUTH_JWT = "auth.jwt";
    std::string auth_jwt;

    static constexpr const char* KEY_SEND_TIMEOUT_MS = "send.timeout.ms";
    int send_timeout_ms = 30000;

    static constexpr const char* KEY_TIMEOUT_RETRY_COUNT = "timeout.retry.count";
    int timeout_retry_count = 0;

    static constexpr const char* KEY_QUEUE_FULL_RETRY_COUNT = "queue.full.retry.count";
    int queue_full_retry_count = 5;

    static constexpr const char* KEY_QUEUE_FULL_RETRY_INTERVAL_MS = "queue.full.retry.interval.ms";
    int queue_full_retry_interval_ms = 10;

    static constexpr const char* KEY_MAX_PENDING_MESSAGES = "max.pending.messages";
    int max_pending_messages = 1000;

    static constexpr const char* KEY_BATCHING_ENABLED = "batching.enable";
    bool batching_enabled = true;

    static constexpr const char* KEY_BATCHING_MAX_MESSAGES = "batching.max.messages";
    int batching_max_messages = 1000;

    static constexpr const char* KEY_BATCHING_MAX_ALLOWED_SIZE_KBYTES = "batching.max.allowed.size.kbytes";
    int batching_max_allowed_size_kbytes = 128;

    static constexpr const char* KEY_BATCHING_MAX_PUBLISHED_DELAY_MS = "batching.max.publish.delay.ms";
    int batching_max_publish_delay_ms = 10;

    static constexpr const char* KEY_COMPRESSION_TYPE = "compression.type";
    std::string compression_type = "None";
};

}  // namespace pulsar
}  // namespace qbus
