#include "qbus_consumer_config.h"
#include "log_utils.h"
#include "property_tree_proxy.h"
#include <sstream>

DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

QbusConsumerConfig::QbusConsumerConfig(const pt::ptree& tree) {
    PropertyTreeProxy proxy(tree);

    proxy.config(KEY_AUTH_JWT, auth_jwt);

    std::string initial_position_desc;
    proxy.config(KEY_INITIAL_POSITION, initial_position_desc);
    if (!initial_position_desc.empty()) {
        if (initial_position_desc == "earliest") {
            initial_position = InitialPosition::kEarliest;
        } else if (initial_position_desc == "latest") {
            initial_position = InitialPosition::kLatest;
        } else {
            LOG_WARN(KEY_INITIAL_POSITION << ": " << initial_position_desc
                                          << " is invalid, set it to latest");
        }
    }

    proxy.configInt(KEY_POLL_TIMEOUT_MS, poll_timeout_ms);
    proxy.configBool(KEY_MANUAL_ACK, manual_ack);
    proxy.configBool(KEY_FORCE_STOP, force_stop);
    proxy.configInt(KEY_MANUAL_COMMIT_TIME_MS, manual_commit_time_ms);
}

inline std::string toString(QbusConsumerConfig::InitialPosition initial_position) {
    switch (initial_position) {
        case QbusConsumerConfig::InitialPosition::kEarliest:
            return "earliest";
        case QbusConsumerConfig::InitialPosition::kLatest:
            return "latest";
    }
    return "???";
}

std::string QbusConsumerConfig::to_string() const {
    std::ostringstream oss;
    oss << KEY_INITIAL_POSITION << "=" << toString(initial_position) << "\n"
        << KEY_MANUAL_ACK << "=" << std::boolalpha << manual_ack << "\n"
        << KEY_FORCE_STOP << "=" << std::boolalpha << force_stop << "\n"
        << KEY_MANUAL_COMMIT_TIME_MS << "=" << manual_commit_time_ms;
    return oss.str();
}

}  // namespace pulsar
}  // namespace qbus
