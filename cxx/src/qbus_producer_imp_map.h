#ifndef QBUS_PRODUCER_IMP_MAP_H_
#define QBUS_PRODUCER_IMP_MAP_H_

#include <map>
#include "qbus_producer_imp.h"

namespace qbus {

class QbusProducerImp;

class QbusProducerImpMap {
 public:
  typedef std::map<std::string, QbusProducerImp*> DataType;

  static DataType* instance() {
    static QbusProducerImpMap kProducerImpMap;
    return &kProducerImpMap.data_;
  }

 private:
  DataType data_;

  QbusProducerImpMap();
  ~QbusProducerImpMap();
};

}  // namespace qbus

#endif  // ifndef QBUS_PRODUCER_IMP_MAP_H_
