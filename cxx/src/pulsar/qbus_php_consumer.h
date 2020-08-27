#pragma once

#include "../qbus_consumer_imp.h"
#include "qbus_consumer.h"

namespace qbus {
namespace pulsar {

class QbusPhpConsumer final : public qbus::QbusConsumer::Imp {
   public:
    QbusPhpConsumer();
    ~QbusPhpConsumer();

    bool init(const std::string& cluster_name, const std::string& log_path, const std::string& config_path,
              const qbus::QbusConsumerCallback& callback) override;

    bool subscribe(const std::string& subscription, const std::vector<std::string>& topics) override;
    bool subscribeOne(const std::string& subscription, const std::string& topic) override;

    bool start() override;
    void stop() override;

    bool pause(const std::vector<std::string>& topics) override;
    bool resume(const std::vector<std::string>& topics) override;

    bool consume(qbus::QbusMsgContentInfo& msg_content_info) override;
    void commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo) override;

   private:
    QbusConsumer consumer_;
};

}  // namespace pulsar
}  // namespace qbus
