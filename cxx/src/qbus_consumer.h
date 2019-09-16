#ifndef QBUS_QBUS_CONSUMER_H_
#define QBUS_QBUS_CONSUMER_H_

#include <string>
#include <vector>

#include "qbus_consumer_callback.h"
//-------------------------------------------------------------
struct rd_kafka_message_s;

namespace qbus {

class QbusConsumerImp;
class QbusConsumerCallback;
class QbusMsgContentInfo;

class QbusConsumer {
 public:
  QbusConsumer();
  ~QbusConsumer();

 public:
  bool init(const std::string& broker_list, const std::string& log_path,
            const std::string& config_path
#ifndef NOT_USE_CONSUMER_CALLBACK
            ,
            const QbusConsumerCallback& callbck
#endif
  );
  bool subscribe(const std::string& group,
                 const std::vector<std::string>& topics);
  bool subscribeOne(const std::string& group, const std::string& topics);
  bool start();
  void stop();

#ifdef NOT_USE_CONSUMER_CALLBACK
  bool consume(QbusMsgContentInfo& msg_content_info);
#endif
  void commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo);

  bool pause(const std::vector<std::string>& topics);
  bool resume(const std::vector<std::string>& topics);

 private:
  QbusConsumerImp* qbus_consumer_imp_;

 private:
  QbusConsumer(const QbusConsumer&);
  QbusConsumer& operator=(const QbusConsumer&);
};
}  // namespace qbus
#endif  //#ifndef QBUS_QBUS_CONSUMER_H_
