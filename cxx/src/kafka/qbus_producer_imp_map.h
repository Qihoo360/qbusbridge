#pragma once

#include "qbus_producer_imp.h"

namespace qbus {
namespace kafka {

class QbusProducerImpMap {
   public:
    typedef std::map<std::string, QbusProducerImp*> DataType;

    static DataType* instance() {
        static QbusProducerImpMap kProducerImpMap;
        return &kProducerImpMap.data_;
    }

   private:
    DataType data_;

    QbusProducerImpMap() {}
    ~QbusProducerImpMap();
};

}  // namespace kafka
}  // namespace qbus
