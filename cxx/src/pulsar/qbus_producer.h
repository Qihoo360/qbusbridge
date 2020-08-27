#pragma once

#include "../qbus_producer_imp.h"
#include <memory>
#include <pulsar/Client.h>

namespace qbus {
namespace pulsar {

using SharedStringPtr = std::shared_ptr<std::string>;

class QbusProducerConfig;

class QbusProducer final : public qbus::QbusProducer::Imp {
   public:
    bool init(const std::string& broker, const std::string& log_path, const std::string& config_path,
              const std::string& topic) override;

    void uninit() override;

    bool produce(const char* data, size_t data_len, const std::string& key) override;

    static ::pulsar::Message createMessage(const SharedStringPtr& payload, const std::string& key);

   private:
    std::string topic_;
    std::unique_ptr<::pulsar::Client> client_;
    ::pulsar::Producer producer_;
    std::unique_ptr<QbusProducerConfig> producer_config_;

    // Returns ResultOk only means the message has been pushed to the internal queue
    ::pulsar::Result sendAsync(const SharedStringPtr& payload, const std::string& key);
};

}  // namespace pulsar
}  // namespace qbus
