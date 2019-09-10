#include "qbus_consumer.h"

#include "src/qbus_consumer.h"
#include "src/qbus_consumer_callback.h"
//-------------------------------------------------------
class QbusConsumerCallbackImp : public qbus::QbusConsumerCallback {
 public:
  QbusConsumerCallbackImp() : qbus_consumer_delivery_func(NULL) {}

 public:
  virtual void deliveryMsg(const std::string& topic, const char* msg,
                           size_t msg_len) const {
    if (NULL != msg && msg_len > 0 && NULL != qbus_consumer_delivery_func) {
      qbus_consumer_delivery_func(topic.c_str(), msg, msg_len);
    }
  }

  virtual void deliveryMsgForCommitOffset(
      const qbus::QbusMsgContentInfo& msg_info) const {
    if (NULL != qbus_consumer_delivery_for_commit_offset_func) {
      qbus_consumer_delivery_for_commit_offset_func(
          msg_info.topic.c_str(), msg_info.msg.c_str(), msg_info.msg_len,
          (QbusCommitOffsetInfoType)(&msg_info));
    }
  }

  void setCallbackFunc(QbusConsumerDeliveryFuncType func) {
    qbus_consumer_delivery_func = func;
  }

  void setCallbackFuncForCommitOffset(
      QbusConsumerDeliveryForCommitOffsetFuncType func) {
    qbus_consumer_delivery_for_commit_offset_func = func;
  }

 private:
  QbusConsumerDeliveryFuncType qbus_consumer_delivery_func;
  QbusConsumerDeliveryForCommitOffsetFuncType
      qbus_consumer_delivery_for_commit_offset_func;
};

struct QbusConsumerRealHandle {
  qbus::QbusConsumer* qbus_consumer;
  QbusConsumerCallbackImp* qbus_consumer_callback;
};

QbusConsumerHandle NewQbusConsumerRealHandle() {
  QbusConsumerRealHandle* qbus_real_handle = new QbusConsumerRealHandle();
  if (NULL != qbus_real_handle) {
    qbus_real_handle->qbus_consumer = new qbus::QbusConsumer();
    if (NULL == qbus_real_handle->qbus_consumer) {
      delete qbus_real_handle;
      qbus_real_handle = NULL;
    } else {
      qbus_real_handle->qbus_consumer_callback = new QbusConsumerCallbackImp();
      if (NULL == qbus_real_handle) {
        delete qbus_real_handle->qbus_consumer;
        delete qbus_real_handle;
        qbus_real_handle = NULL;
      }
    }
  }

  return (QbusConsumerHandle)qbus_real_handle;
}

void DeleteQbusConsumerRealHandle(QbusConsumerHandle qbus_consumer_handle) {
  if (NULL != qbus_consumer_handle) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)qbus_consumer_handle;
    if (NULL != qbus_consumer_real_handle) {
      delete qbus_consumer_real_handle->qbus_consumer_callback;
      delete qbus_consumer_real_handle->qbus_consumer;
      delete qbus_consumer_real_handle;
    }

    qbus_consumer_handle = NULL;
  }
}

QbusConsumerHandle NewQbusConsumer() { return NewQbusConsumerRealHandle(); }

void DeleteQbusConsumer(QbusConsumerHandle handle) {
  DeleteQbusConsumerRealHandle(handle);
}

QbusResult InitQbusConsumer(QbusConsumerHandle handle, const char* cluster_name,
                            const char* log_path, const char* config_path,
                            QbusConsumerDeliveryFuncType callback) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle && NULL != cluster_name && '\0' != cluster_name[0]) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer &&
        NULL != qbus_consumer_real_handle->qbus_consumer_callback) {
      qbus_consumer_real_handle->qbus_consumer_callback->setCallbackFunc(
          callback);
      rt = qbus_consumer_real_handle->qbus_consumer->init(
               cluster_name, NULL != log_path ? log_path : "",
               NULL != config_path ? config_path : "",
               *(qbus_consumer_real_handle->qbus_consumer_callback))
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}

QbusResult InitQbusConsumerEx(QbusConsumerHandle handle,
                              const char* cluster_name, const char* log_path,
                              const char* config_path,
                              QbusConsumerCallbackInfo callback_info) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle && NULL != cluster_name && '\0' != cluster_name[0]) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer &&
        NULL != qbus_consumer_real_handle->qbus_consumer_callback) {
      qbus_consumer_real_handle->qbus_consumer_callback->setCallbackFunc(
          callback_info.callback);
      qbus_consumer_real_handle->qbus_consumer_callback
          ->setCallbackFuncForCommitOffset(
              callback_info.callback_for_commit_offset);
      rt = qbus_consumer_real_handle->qbus_consumer->init(
               cluster_name, NULL != log_path ? log_path : "",
               NULL != config_path ? config_path : "",
               *(qbus_consumer_real_handle->qbus_consumer_callback))
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}

QbusResult QbusConsumerSubscribeOne(QbusConsumerHandle handle,
                                    const char* group, const char* topic) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer &&
        NULL != qbus_consumer_real_handle->qbus_consumer_callback) {
      rt = qbus_consumer_real_handle->qbus_consumer->subscribeOne(
               NULL != group ? group : "", NULL != topic ? topic : "")
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}

QbusResult QbusConsumerSubscribe(QbusConsumerHandle handle, const char* group,
                                 const char* topics[], int32_t topics_count) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle && NULL != topics && topics_count > 0) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer &&
        NULL != qbus_consumer_real_handle->qbus_consumer_callback) {
      std::vector<std::string> topic_list;
      for (int32_t i = 0; i < topics_count; ++i) {
        if (NULL != topics[i]) {
          topic_list.push_back(topics[i]);
        }
      }

      rt = qbus_consumer_real_handle->qbus_consumer->subscribe(
               NULL != group ? group : "", topic_list)
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}

QbusResult QbusConsumerStart(QbusConsumerHandle handle) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer &&
        NULL != qbus_consumer_real_handle->qbus_consumer_callback) {
      rt = qbus_consumer_real_handle->qbus_consumer->start()
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}

void QbusConsumerStop(QbusConsumerHandle handle) {
  if (NULL != handle) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer &&
        NULL != qbus_consumer_real_handle->qbus_consumer_callback) {
      qbus_consumer_real_handle->qbus_consumer->stop();
    }
  }
}

void QbusConsumerCommitOffset(QbusConsumerHandle handle,
                              QbusCommitOffsetInfoType offset_info) {
  if (NULL != handle) {
    QbusConsumerRealHandle* qbus_consumer_real_handle =
        (QbusConsumerRealHandle*)handle;
    if (NULL != qbus_consumer_real_handle &&
        NULL != qbus_consumer_real_handle->qbus_consumer) {
      qbus::QbusMsgContentInfo* msg_info =
          (qbus::QbusMsgContentInfo*)offset_info;
      if (NULL != msg_info) {
        qbus_consumer_real_handle->qbus_consumer->commitOffset(*msg_info);
      }
    }
  }
}
