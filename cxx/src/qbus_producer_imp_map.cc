#include "qbus_producer_imp_map.h"
#include <iostream>
#include "util/logger.h"

namespace qbus {

QbusProducerImpMap::QbusProducerImpMap() {}

QbusProducerImpMap::~QbusProducerImpMap() {
  for (DataType::iterator it = data_.begin(); it != data_.end(); ++it) {
    if (it->second) {
      it->second->Uninit();
      delete it->second;
    }
  }
  LUtil::Logger::uninit();
}

}  // namespace qbus
