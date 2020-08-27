#pragma once

#include "../qbus_consumer_imp.h"

//-------------------------------------------------------------

namespace qbus {
namespace kafka {

class QbusConsumer : public qbus::QbusConsumer::Imp {
   public:
    QbusConsumer();
    ~QbusConsumer();

   public:
    bool init(const std::string& cluster_name, const std::string& log_path, const std::string& config_path,
              const QbusConsumerCallback& callback);
    // If group is an empty string, group.id must be configured in global configuration.
    // Otherwise, group will overwrite configured group.id.
    bool subscribe(const std::string& group, const std::vector<std::string>& topics);
    bool subscribeOne(const std::string& group, const std::string& topics);
    bool start();
    void stop();

    bool pause(const std::vector<std::string>& topics);
    bool resume(const std::vector<std::string>& topics);

    bool consume(qbus::QbusMsgContentInfo& msg_content_info);
    void commitOffset(const qbus::QbusMsgContentInfo& qbusMsgContentInfo);

   private:
    class QbusConsumerImp;

    QbusConsumerImp* qbus_consumer_imp_;

   private:
    QbusConsumer(const QbusConsumer&);
    QbusConsumer& operator=(const QbusConsumer&);
};

}  // namespace kafka
}  // namespace qbus
