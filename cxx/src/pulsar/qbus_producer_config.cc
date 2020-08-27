#include "qbus_producer_config.h"
#include "property_tree_proxy.h"
#include <sstream>

namespace qbus {
namespace pulsar {

QbusProducerConfig::QbusProducerConfig(const pt::ptree& tree) {
    PropertyTreeProxy proxy(tree);

    proxy.config(KEY_AUTH_JWT, auth_jwt);
    proxy.configInt(KEY_SEND_TIMEOUT_MS, send_timeout_ms);
    proxy.configInt(KEY_TIMEOUT_RETRY_COUNT, timeout_retry_count);
    proxy.configInt(KEY_QUEUE_FULL_RETRY_COUNT, queue_full_retry_count);
    proxy.configInt(KEY_QUEUE_FULL_RETRY_INTERVAL_MS, queue_full_retry_interval_ms);
    if (queue_full_retry_interval_ms < 0) {
        queue_full_retry_interval_ms = 0;
    }

    proxy.configInt(KEY_MAX_PENDING_MESSAGES, max_pending_messages);
    proxy.configBool(KEY_BATCHING_ENABLED, batching_enabled);
    proxy.configInt(KEY_BATCHING_MAX_MESSAGES, batching_max_messages);
    proxy.configInt(KEY_BATCHING_MAX_ALLOWED_SIZE_KBYTES, batching_max_allowed_size_kbytes);
    proxy.configInt(KEY_BATCHING_MAX_PUBLISHED_DELAY_MS, batching_max_publish_delay_ms);
    proxy.config(KEY_COMPRESSION_TYPE, compression_type);
}

std::string QbusProducerConfig::to_string() const {
    std::ostringstream oss;
    oss << KEY_SEND_TIMEOUT_MS << "=" << send_timeout_ms << "\n"
        << KEY_TIMEOUT_RETRY_COUNT << "=" << timeout_retry_count << "\n"
        << KEY_QUEUE_FULL_RETRY_COUNT << "=" << queue_full_retry_count << "\n"
        << KEY_QUEUE_FULL_RETRY_INTERVAL_MS << "=" << queue_full_retry_interval_ms << "\n"
        << KEY_MAX_PENDING_MESSAGES << "=" << max_pending_messages << "\n"
        << KEY_BATCHING_ENABLED << "=" << std::boolalpha << batching_enabled << "\n"
        << KEY_BATCHING_MAX_MESSAGES << "=" << batching_max_messages << "\n"
        << KEY_BATCHING_MAX_ALLOWED_SIZE_KBYTES << "=" << batching_max_allowed_size_kbytes << "\n"
        << KEY_BATCHING_MAX_PUBLISHED_DELAY_MS << "=" << batching_max_publish_delay_ms << "\n"
        << KEY_COMPRESSION_TYPE << "=" << compression_type;
    return oss.str();
}

}  // namespace pulsar
}  // namespace qbus
