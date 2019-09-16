#ifndef QBUS_CONSUMER_CALLBACK_H_
#define QBUS_CONSUMER_CALLBACK_H_

#include <string>

struct rd_kafka_message_s;

namespace qbus {

struct QbusMsgContentInfo {
  std::string topic;
  std::string msg;
  size_t msg_len;
  rd_kafka_message_s* rd_message;
};

#ifndef NOT_USE_CONSUMER_CALLBACK
class QbusConsumerCallback {
 public:
  virtual ~QbusConsumerCallback() {}

 public:
  virtual void deliveryMsg(const std::string& topic, const char* msg,
                           size_t msg_len) const {}

  virtual void deliveryMsgForCommitOffset(
      const QbusMsgContentInfo& msg_info) const {}
};
#endif  //#ifdef NOT_USE_CONSUMER_CALLBACK

}  // namespace qbus
#endif  //#define QBUS_CONSUMER_CALLBACK_H_
