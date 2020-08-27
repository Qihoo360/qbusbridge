#include "qbus_producer.h"
#include "qbus_producer_imp.h"

#include <stdexcept>

#include "kafka/qbus_producer.h"

namespace qbus {

QbusProducer::QbusProducer() : imp_(nullptr) {}

QbusProducer::~QbusProducer() {
    if (imp_) delete imp_;
}

bool QbusProducer::init(const std::string& cluster, const std::string& log_path,
                        const std::string& config_path, const std::string& topic) {
    if (!imp_) {
        imp_ = new kafka::QbusProducer;
    }
    return imp_->init(cluster, log_path, config_path, topic);
}

void QbusProducer::uninit() {
    if (imp_) {
        imp_->uninit();
    }
}

bool QbusProducer::produce(const char* data, size_t data_len, const std::string& key) {
    if (!imp_) {
        throw std::runtime_error("QbusProducer not initialized");
    }
    return imp_->produce(data, data_len, key);
}

}  // namespace qbus
