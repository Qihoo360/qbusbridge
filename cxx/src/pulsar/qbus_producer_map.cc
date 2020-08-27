#include "qbus_producer_map.h"
#include "qbus_producer.h"
#include "qbus_producer_config.h"
#include "log4cplus_logger.h"

#include "log_utils.h"
DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

QbusProducerMap& QbusProducerMap::instance() {
    static QbusProducerMap producer_map;
    return producer_map;
}

QbusProducerMap::QbusProducerMap() {}

QbusProducerMap::~QbusProducerMap() {
    for (auto& kv : map_) {
        kv.second->uninit();
    }
}

std::shared_ptr<QbusProducer> QbusProducerMap::operator[](const std::string& key) {
    auto it = map_.find(key);
    if (it != map_.end()) {
        return it->second;
    } else {
        auto producer = std::make_shared<QbusProducer>();
        map_.emplace(key, producer);
        return producer;
    }
}

}  // namespace pulsar
}  // namespace qbus
