#ifndef QBUS_QBUS_CONSUMER_H_
#define QBUS_QBUS_CONSUMER_H_

#include <string>
#include <vector>

struct rd_kafka_message_s;

namespace qbus {

struct QbusMsgContentInfo {
    std::string topic;
    std::string msg;
    size_t msg_len;

    // kafka only
    rd_kafka_message_s* rd_message;

    // pulsar only
    std::string msg_id;
};

class QbusConsumerCallback {
   public:
    virtual ~QbusConsumerCallback() {}

    virtual void deliveryMsg(const std::string& topic, const char* msg, size_t msg_len) const {}

    virtual void deliveryMsgForCommitOffset(const QbusMsgContentInfo& info) const {}
};

class QbusConsumer {
   public:
    QbusConsumer();
    ~QbusConsumer();

    bool init(const std::string& cluster, const std::string& log_path, const std::string& config_path,
              const QbusConsumerCallback& callback = QbusConsumerCallback());

    bool subscribe(const std::string& group, const std::vector<std::string>& topics);
    bool subscribeOne(const std::string& group, const std::string& topic);
    bool start();
    void stop();

    bool pause(const std::vector<std::string>& topics);
    bool resume(const std::vector<std::string>& topics);

    bool consume(QbusMsgContentInfo& info);

    void commitOffset(const QbusMsgContentInfo& info);

    class Imp;

   private:
    Imp* imp_;

    QbusConsumer(const QbusConsumer&);
    QbusConsumer& operator=(const QbusConsumer&);
};

}  // namespace qbus

#endif  // QBUS_QBUS_CONSUMER_H_
