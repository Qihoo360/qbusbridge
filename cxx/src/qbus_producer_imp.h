#ifndef QBUS_QBUS_PRODUCER_IMP_H_
#define QBUS_QBUS_PRODUCER_IMP_H_

#include "qbus_producer.h"

namespace qbus {

class QbusProducer::Imp {
   public:
    virtual ~Imp() {}

    virtual bool init(const std::string& cluster, const std::string& log_path, const std::string& config_path,
                      const std::string& topic) = 0;

    virtual void uninit() = 0;

    virtual bool produce(const char* data, size_t data_len, const std::string& key) = 0;
};

}  // namespace qbus

#endif  // QBUS_QBUS_PRODUCER_IMP_H_
