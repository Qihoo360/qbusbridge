#include "qbus_php_producer.h"
#include "qbus_producer.h"
#include "qbus_producer_map.h"

#include "log_utils.h"
DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

bool QbusPhpProducer::init(const std::string& broker, const std::string& log_path,
                           const std::string& config_path, const std::string& topic) {
    std::string key = newProducerKey(broker, topic, config_path);
    if (!producer_) {
        producer_ = QbusProducerMap::instance()[key];
        bool result = producer_->init(broker, log_path, config_path, topic);
        LOG_INFO("Producer of \"" << key << "\" init...");
        return result;
    } else {
        return true;
    }
}

void QbusPhpProducer::uninit() {}

bool QbusPhpProducer::produce(const char* data, size_t data_len, const std::string& key) {
    if (!producer_) {
        LOG_ERROR("Producer not initialized");
        return false;
    }
    return producer_->produce(data, data_len, key);
}

}  // namespace pulsar
}  // namespace qbus
