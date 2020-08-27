#include "qbus_producer_imp_map.h"
#include "util/logger.h"

namespace qbus {
namespace kafka {

QbusProducerImpMap::~QbusProducerImpMap() {
    if (data_.size() > 0) {
        LUtil::Logger::uninit();
    }
    for (DataType::iterator it = data_.begin(); it != data_.end(); ++it) {
        it->second->uninit();
        delete it->second;
    }
}

}  // namespace kafka
}  // namespace qbus
