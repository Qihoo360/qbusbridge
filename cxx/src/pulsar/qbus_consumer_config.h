#pragma once

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace qbus {
namespace pulsar {

struct QbusConsumerConfig {
    QbusConsumerConfig() = default;

    QbusConsumerConfig(const pt::ptree& tree);

    std::string to_string() const;

    static constexpr const char* KEY_AUTH_JWT = "auth.jwt";
    std::string auth_jwt;

    enum class InitialPosition
    {
        kEarliest,
        kLatest
    };
    static constexpr const char* KEY_INITIAL_POSITION = "initial.position";
    InitialPosition initial_position = InitialPosition::kLatest;

    static constexpr const char* KEY_POLL_TIMEOUT_MS = "poll.timeout.ms";
    int poll_timeout_ms = 100;

    static constexpr const char* KEY_MANUAL_ACK = "manual.ack";
    bool manual_ack = false;

    static constexpr const char* KEY_FORCE_STOP = "force.stop";
    bool force_stop = false;

    static constexpr const char* KEY_MANUAL_COMMIT_TIME_MS = "manual.commit.time.ms";
    int manual_commit_time_ms = 200;
};

}  // namespace pulsar
}  // namespace qbus
