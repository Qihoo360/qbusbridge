#pragma once

#include <string>
#include "../qbus_producer_imp.h"
//-----------------------------------------------------
namespace qbus {
namespace kafka {

class QbusProducerImp;
class QbusProducer : public qbus::QbusProducer::Imp {
   public:
    QbusProducer();
    ~QbusProducer();

   public:
    bool init(const std::string& cluster_name, const std::string& log_path, const std::string& config_path,
              const std::string& topic_name);
    void uninit();
    bool produce(const char* data, size_t data_len, const std::string& key);

   private:
    QbusProducerImp* qbus_producer_imp_;
};

}  // namespace kafka
}  // namespace qbus
