#include "qbus_consumer.h"
#include "qbus_consumer_imp.h"

#include <stdexcept>
#include "mq_type.h"

#include "kafka/qbus_consumer.h"
#include "pulsar/qbus_consumer.h"
#include "pulsar/qbus_php_consumer.h"

namespace qbus {

QbusConsumer::QbusConsumer() : imp_(nullptr) {}

QbusConsumer::~QbusConsumer() {
    if (imp_) delete imp_;
}

bool QbusConsumer::init(const std::string& cluster, const std::string& log_path,
                        const std::string& config_path, const QbusConsumerCallback& callback) {
    if (!imp_) {
        switch (mqType(config_path)) {
            case MqType::KAFKA:
                imp_ = new kafka::QbusConsumer;
                break;
            case MqType::PULSAR:
#ifdef NOT_USE_CONSUMER_CALLBACK
                imp_ = new pulsar::QbusPhpConsumer;
#else
                imp_ = new pulsar::QbusConsumer;
#endif
                break;
        }
    }
    return imp_->init(cluster, log_path, config_path, callback);
}

bool QbusConsumer::subscribe(const std::string& group, const std::vector<std::string>& topics) {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->subscribe(group, topics);
}

bool QbusConsumer::subscribeOne(const std::string& group, const std::string& topic) {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->subscribeOne(group, topic);
}

bool QbusConsumer::start() {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->start();
}

void QbusConsumer::stop() {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->stop();
}

bool QbusConsumer::pause(const std::vector<std::string>& topics) {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->pause(topics);
}

bool QbusConsumer::resume(const std::vector<std::string>& topics) {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->resume(topics);
}

bool QbusConsumer::consume(QbusMsgContentInfo& info) {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    return imp_->consume(info);
}

void QbusConsumer::commitOffset(const QbusMsgContentInfo& info) {
    if (!imp_) {
        throw std::runtime_error("QbusConsumer not initialized");
    }
    imp_->commitOffset(info);
}

}  // namespace qbus
