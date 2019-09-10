#include "qbus_consumer.h"

#include "qbus_consumer_imp.h"
#include "util/logger.h"
//------------------------------------------------------------
namespace qbus {

QbusConsumer::QbusConsumer() : qbus_consumer_imp_(NULL) {}

QbusConsumer::~QbusConsumer() {
  if (NULL != qbus_consumer_imp_) {
    delete qbus_consumer_imp_;
    qbus_consumer_imp_ = NULL;
  }
}

bool QbusConsumer::init(const std::string& broker_list,
                        const std::string& log_path,
                        const std::string& config_path
#ifndef NOT_USE_CONSUMER_CALLBACK
                        ,
                        const QbusConsumerCallback& callback
#endif
) {
  bool rt = false;

  qbus_consumer_imp_ = new QbusConsumerImp(broker_list
#ifndef NOT_USE_CONSUMER_CALLBACK
                                           ,
                                           callback
#endif
  );
  if (NULL != qbus_consumer_imp_) {
    rt = qbus_consumer_imp_->Init(log_path, config_path);
  }

  if (rt) {
    INFO(__FUNCTION__ << " | QbusConsumer::init is OK");
  } else {
    ERROR(__FUNCTION__ << " | Failed to QbusConsumer::init");
  }

  return rt;
}

bool QbusConsumer::subscribe(const std::string& group,
                             const std::vector<std::string>& topics) {
  bool rt = false;

  for (std::vector<std::string>::const_iterator i = topics.begin(),
                                                e = topics.end();
       i != e; ++i) {
    INFO(__FUNCTION__ << " | group: " << group << " | topic: " << *i);
  }

  if (NULL != qbus_consumer_imp_) {
    rt = qbus_consumer_imp_->Subscribe(group, topics);
  }

  if (rt) {
    INFO(__FUNCTION__ << " | QbusConsumer::subscribe is OK");
  } else {
    ERROR(__FUNCTION__ << " | Failed to QbusConsumer::subscribe");
  }

  return rt;
}

bool QbusConsumer::subscribeOne(const std::string& group,
                                const std::string& topic) {
  std::vector<std::string> topics;
  topics.push_back(topic);
  return subscribe(group, topics);
}

bool QbusConsumer::start() {
  bool rt = false;
  if (NULL != qbus_consumer_imp_) {
    rt = qbus_consumer_imp_->Start();
  }

  if (rt) {
    INFO(__FUNCTION__ << " | QbusConsumer::start OK");
  } else {
    ERROR(__FUNCTION__ << " | Failed to QbusConsumer::start");
  }

  return rt;
}

void QbusConsumer::stop() {
  if (NULL != qbus_consumer_imp_) {
    qbus_consumer_imp_->Stop();
  }
}

void QbusConsumer::commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo) {
  if (NULL != qbus_consumer_imp_) {
    qbus_consumer_imp_->CommitOffset(qbusMsgContentInfo);
  }
}

#ifdef NOT_USE_CONSUMER_CALLBACK
bool QbusConsumer::consume(QbusMsgContentInfo& msg_content_info) {
  bool rt = false;

  if (NULL != qbus_consumer_imp_) {
    rt = qbus_consumer_imp_->Consume(msg_content_info);
  }

  return rt;
}
#endif

}  // namespace qbus
