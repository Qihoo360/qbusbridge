#pragma once

#include "../qbus_producer_imp.h"

#include <memory>
#include <unordered_map>

namespace qbus {
namespace pulsar {

class QbusProducer;

class QbusPhpProducer final : public qbus::QbusProducer::Imp {
   public:
    bool init(const std::string& broker, const std::string& log_path, const std::string& config_path,
              const std::string& topic) override;

    void uninit() override;

    bool produce(const char* data, size_t data_len, const std::string& key) override;

   private:
    std::shared_ptr<QbusProducer> producer_;

    static std::string newProducerKey(const std::string& broker, const std::string& topic,
                                      const std::string& config_path) {
        return broker + topic + config_path;
    }
};

}  // namespace pulsar
}  // namespace qbus
