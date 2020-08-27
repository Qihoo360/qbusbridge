#include "qbus_consumer.h"
#include "qbus_consumer_config.h"

#include <algorithm>
#include <chrono>

#include "log4cplus_logger.h"
#include "log_utils.h"
#include "pulsar_config.h"
#include "to_string.h"

using std::string;
using std::vector;
using namespace pulsar;
DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

inline std::string removePartitionSuffix(const std::string& topic) {
    assert(!topic.empty());
    const auto pos = topic.rfind("-partition-");
    if (pos != std::string::npos && isdigit(topic.back())) {
        return topic.substr(0, pos);
    } else {
        return topic;
    }
}

QbusConsumer::QbusConsumer() = default;

QbusConsumer::~QbusConsumer() {
    if (client_) {
        LOG_INFO("acknowledgeAll");
        acknowledgeAll();
        Result result;
        if ((result = consumer_.close()) != ResultOk) {
            LOG_WARN("Failed to close consumer: " << result);
        }
        if ((result = client_->close()) != ResultOk) {
            LOG_WARN("Failed to close client: " << result);
        }
    }
}

bool QbusConsumer::init(const std::string& broker, const std::string& log_path,
                        const std::string& config_path, const QbusConsumerCallback& callback) {
    if (client_) {
        LOG_ERROR("QbusConsumer already initialized");
        return false;
    }

    PulsarConfig config;
    try {
        config.loadConfig(config_path);
    } catch (const PulsarConfig::Error& e) {
        LOG_ERROR(e.what());
        return false;
    }

    // User provided config has higher priority than config of file
    if (!broker.empty()) {
        config.put(config.KEY_BROKER, broker);
    }

    ClientConfiguration client_config;
    client_config.setStatsIntervalInSeconds(0);
    client_config.setLogger(new Log4cplusLoggerFactory(config.log_level(), log_path));

    consumer_conf_ = config.consumer_config();
    std::string jwt = consumer_conf_->auth_jwt;
    if (!jwt.empty()) {
        client_config.setAuth(::pulsar::AuthToken::createWithToken(jwt));
    }

    client_.reset(new Client(config.broker(), client_config));
    topic_prefix_ = config.topic_prefix();
    default_subscription_ = config.subscription();
    LOG_INFO("ConsumerConfig:\n" << consumer_conf_->to_string());

    pulsar_consumer_conf_.setConsumerType(ConsumerFailover);
    switch (consumer_conf_->initial_position) {
        case QbusConsumerConfig::InitialPosition::kEarliest:
            pulsar_consumer_conf_.setSubscriptionInitialPosition(InitialPositionEarliest);
            break;
        case QbusConsumerConfig::InitialPosition::kLatest:
            pulsar_consumer_conf_.setSubscriptionInitialPosition(InitialPositionLatest);
            break;
    }
    timer_.setIntervalMs(consumer_conf_->manual_commit_time_ms);

    consumer_cb_ = &callback;

    return true;
}

bool QbusConsumer::subscribe(const string& subscription, const vector<string>& topics) {
    if (!client_) {
        LOG_ERROR("QbusConsumer not initialized");
        return false;
    }

    vector<string> full_topics(topics.size());
    std::transform(topics.cbegin(), topics.cend(), full_topics.begin(),
                   [this](const std::string& topic) { return topic_prefix_ + topic; });

    const auto& actual_subscription = subscription.empty() ? default_subscription_ : subscription;
    auto result = client_->subscribe(full_topics, actual_subscription, pulsar_consumer_conf_, consumer_);
    if (result != ResultOk) {
        LOG_ERROR("Failed to subscribe topics " << to_string(topics) << ": " << result);
        return false;
    }

    has_subscribed_ = true;
    return true;
}

bool QbusConsumer::subscribeOne(const string& subscription, const string& topic) {
    if (!client_) {
        LOG_ERROR("QbusConsumer not initialized");
        return false;
    }

    const auto& actual_subscription = subscription.empty() ? default_subscription_ : subscription;
    auto result =
        client_->subscribe(topic_prefix_ + topic, actual_subscription, pulsar_consumer_conf_, consumer_);
    if (result != ResultOk) {
        LOG_ERROR("Failed to subscribe topic \"" << topic << "\": " << result);
        return false;
    }

    has_subscribed_ = true;
    return true;
}

bool QbusConsumer::start() {
    if (!client_) {
        LOG_ERROR("QbusConsumer not initialized");
        return false;
    }
    if (running_) {
        LOG_ERROR("QbusConsumer already started");
        return false;
    }
    if (!has_subscribed_) {
        LOG_ERROR("QbusConsumer not subscribed");
        return false;
    }

    LOG_INFO("ack interval ms: " << timer_.intervalMs());
    running_ = true;
    consume_loop_future_ = std::async(std::launch::async, &QbusConsumer::consumeLoop, this);
    return true;
}

void QbusConsumer::stop() {
    if (!client_) {
        LOG_ERROR("QbusConsumer not initialized");
        return;
    }
    if (!running_) {
        LOG_ERROR("QbusConsumer not started");
        return;
    }

    LOG_INFO("force stop: " << consumer_conf_->force_stop);
    running_ = false;
    if (consumer_conf_->force_stop) {
        consume_loop_future_.wait_for(std::chrono::milliseconds(1));
    } else {
        consume_loop_future_.get();
    }

    acknowledgeAll();
}

Result QbusConsumer::consume(Message& msg) {
    assert(consumer_conf_);
    const auto result = consumer_.receive(msg, consumer_conf_->poll_timeout_ms);
    if (result == ResultOk) {
        topic_latest_msg_id_map_[msg.getTopicName()] = msg.getMessageId();
        LOG_DEBUG("Consume msg from: " << msg.getTopicName() << " | " << msg.getMessageId());
    }
    return result;
}

void QbusConsumer::convertMessage(const Message& msg, qbus::QbusMsgContentInfo& info) {
    info.topic = removePartitionSuffix(msg.getTopicName());
    info.msg = msg.getDataAsString();
    info.msg_len = msg.getLength();
    msg.getMessageId().serialize(info.msg_id);
}

void QbusConsumer::consumeLoop() {
    timer_.start();
    while (running_) {
        DeferAcknowledge defer_acknowledge(*this);

        Message msg;
        const auto result = consume(msg);
        if (result == ResultTimeout) {
            continue;
        }
        if (result != ResultOk) {
            LOG_ERROR(" Receive failed: " << result);
            continue;
        }

        if (consumer_conf_->manual_ack) {
            QbusMsgContentInfo info;
            convertMessage(msg, info);
            consumer_cb_->deliveryMsgForCommitOffset(info);
        } else {
            std::string payload = msg.getDataAsString();
            consumer_cb_->deliveryMsg(removePartitionSuffix(msg.getTopicName()), payload.c_str(),
                                      payload.length());
        }
    }
}

bool QbusConsumer::acknowledge(const std::string& topic, const MessageId& id) {
    auto result = consumer_.acknowledgeCumulative(id);
    if (result != ResultOk) {
        LOG_ERROR("Failed with id: " << id << ", topic: " << topic << ", result: " << result);
        return false;
    } else {
        LOG_DEBUG("id: " << id << " topic: " << topic);
        return true;
    }
}

void QbusConsumer::acknowledgeAll() {
    for (auto& topic_and_id : topic_latest_msg_id_map_) {
        const std::string& topic = topic_and_id.first;
        MessageId& id = topic_and_id.second;
        if (id != MessageId() && acknowledge(topic, id)) {
            id = MessageId();
        }
    }
}

void QbusConsumer::checkTimeThenAcknowledgeAll() {
    if (consumer_conf_->manual_ack) {
        return;
    }
    int64_t interval_ms = timer_.stop();
    if (interval_ms >= timer_.intervalMs()) {
        acknowledgeAll();
    }
}

void QbusConsumer::commitOffset(const qbus::QbusMsgContentInfo& info) {
    if (!client_) {
        LOG_ERROR("QbusConsumer not initialized");
        return;
    }
    acknowledge(info.topic, MessageId::deserialize(info.msg_id));
}

bool QbusConsumer::pause(const vector<string>& topics) {
    LOG_ERROR("PulsarConsumer doesn't support pause");
    return false;
}

bool QbusConsumer::resume(const vector<string>& topics) {
    LOG_ERROR("PulsarConsumer doesn't support resume");
    return false;
}

bool QbusConsumer::consume(qbus::QbusMsgContentInfo& msg_content_info) {
    LOG_ERROR("PulsarConsumer doesn't support consume");
    return false;
}

}  // namespace pulsar
}  // namespace qbus
