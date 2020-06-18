#include "qbus_producer_imp_map.h"
#include <iostream>
#include "util/logger.h"

namespace qbus {

QbusProducerImpMap::QbusProducerImpMap() {}

QbusProducerImpMap::~QbusProducerImpMap() {
  if (data_.size() > 0) {
    LUtil::Logger::uninit();
  }
  for (DataType::iterator it = data_.begin(); it != data_.end(); ++it) {
    if (it->second) {
      it->second->Uninit();
      delete it->second;
    }
  }
}

}  // namespace qbus
