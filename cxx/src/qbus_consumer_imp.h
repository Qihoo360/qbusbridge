#ifndef QBUS_QBUS_CONSUMER_IMP_H_
#define QBUS_QBUS_CONSUMER_IMP_H_

#include "qbus_consumer.h"

namespace qbus {

class QbusConsumer::Imp {
   public:
    virtual ~Imp() {}

    virtual bool init(const std::string& cluster, const std::string& log_path, const std::string& config_path,
                      const QbusConsumerCallback& callback) = 0;

    virtual bool subscribe(const std::string& group, const std::vector<std::string>& topics) = 0;
    virtual bool subscribeOne(const std::string& group, const std::string& topic) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual bool pause(const std::vector<std::string>& topics) = 0;
    virtual bool resume(const std::vector<std::string>& topics) = 0;

    virtual bool consume(QbusMsgContentInfo& info) = 0;

    virtual void commitOffset(const QbusMsgContentInfo& info) = 0;
};

}  // namespace qbus

#endif  // QBUS_QBUS_CONSUMER_IMP_H_
