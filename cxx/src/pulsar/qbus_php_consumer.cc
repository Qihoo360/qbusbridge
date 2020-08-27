#include "qbus_php_consumer.h"
#include "log_utils.h"

using namespace ::pulsar;

DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

QbusPhpConsumer::QbusPhpConsumer() {}
QbusPhpConsumer::~QbusPhpConsumer() {}

bool QbusPhpConsumer::init(const std::string& cluster_name, const std::string& log_path,
                           const std::string& config_path, const qbus::QbusConsumerCallback& callback) {
    return consumer_.init(cluster_name, log_path, config_path, callback);
}

bool QbusPhpConsumer::subscribe(const std::string& subscription, const std::vector<std::string>& topics) {
    return consumer_.subscribe(subscription, topics);
}

bool QbusPhpConsumer::subscribeOne(const std::string& subscription, const std::string& topic) {
    return consumer_.subscribeOne(subscription, topic);
}

bool QbusPhpConsumer::start() {
    if (!consumer_.client_) {
        LOG_ERROR("QbusPhpConsumer not initialized");
        return false;
    }
    if (consumer_.running_) {
        LOG_ERROR("QbusPhpConsumer already started");
        return false;
    }
    if (!consumer_.has_subscribed_) {
        LOG_ERROR("QbusPhpConsumer not subscribed");
        return false;
    }

    consumer_.timer_.start();
    consumer_.running_ = true;
    return true;
}

void QbusPhpConsumer::stop() {
    if (!consumer_.client_) {
        LOG_ERROR("QbusPhpConsumer not initialized");
        return;
    }
    if (!consumer_.running_) {
        LOG_ERROR("QbusPhpConsumer not started");
        return;
    }

    consumer_.running_ = false;
}

bool QbusPhpConsumer::pause(const std::vector<std::string>& topics) { throw "PHP not supported"; }

bool QbusPhpConsumer::resume(const std::vector<std::string>& topics) { throw "PHP not supported"; }

bool QbusPhpConsumer::consume(qbus::QbusMsgContentInfo& msg_content_info) {
    if (!consumer_.running_) {
        LOG_ERROR("QbusPhpConsumer not started");
        return false;
    }

    QbusConsumer::DeferAcknowledge defer_acknowledge(consumer_);

    Message msg;
    const auto result = consumer_.consume(msg);
    if (result != ResultOk) {
        if (result != ResultTimeout) {
            LOG_ERROR("failed: " << result);
        }
        return false;
    }

    consumer_.convertMessage(msg, msg_content_info);
    return true;
}

void QbusPhpConsumer::commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo) {
    consumer_.commitOffset(qbusMsgContentInfo);
}

}  // namespace pulsar
}  // namespace qbus
