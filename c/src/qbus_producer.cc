#include "qbus_producer.h"

#include "src/qbus_producer.h"
//-----------------------------------------
struct QbusProducerRealHandle {
  qbus::QbusProducer* qbus_producer;
};
//-----------------------------------------
QbusProducerHandle NewQbusProducer() {
  QbusProducerRealHandle* qbus_real_handle = new QbusProducerRealHandle();
  if (NULL != qbus_real_handle) {
    qbus_real_handle->qbus_producer = new qbus::QbusProducer();
    if (NULL == qbus_real_handle->qbus_producer) {
      delete qbus_real_handle;
      qbus_real_handle = NULL;
    }
  }

  return (QbusProducerHandle)qbus_real_handle;
}

void DeleteQbusProducer(QbusProducerHandle producer_handle) {
  if (NULL != producer_handle) {
    QbusProducerRealHandle* producer_real_handle =
        (QbusProducerRealHandle*)producer_handle;
    if (NULL != producer_real_handle) {
      delete producer_real_handle->qbus_producer;
      delete producer_real_handle;
      producer_handle = NULL;
    }
  }
}

QbusResult InitQbusProducer(QbusProducerHandle handle, const char* cluster_name,
                            const char* log_path, const char* config_path,
                            const char* topic_name) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle && NULL != cluster_name && NULL != topic_name) {
    QbusProducerRealHandle* producer_real_handle =
        (QbusProducerRealHandle*)handle;
    if (NULL != producer_real_handle &&
        NULL != producer_real_handle->qbus_producer) {
      rt = producer_real_handle->qbus_producer->init(
               cluster_name, NULL != log_path ? log_path : "",
               NULL != config_path ? config_path : "", topic_name)
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}

void UninitQbusProducer(QbusProducerHandle handle) {
  if (NULL != handle) {
    QbusProducerRealHandle* producer_real_handle =
        (QbusProducerRealHandle*)handle;
    if (NULL != producer_real_handle &&
        NULL != producer_real_handle->qbus_producer) {
      producer_real_handle->qbus_producer->uninit();
    }
  }
}

QbusResult QbusProducerProduce(QbusProducerHandle handle, const char* data,
                               int64_t data_len, const char* key) {
  QbusResult rt = QBUS_RESULT_FAILED;

  if (NULL != handle && NULL != data && data_len > 0) {
    QbusProducerRealHandle* producer_real_handle =
        (QbusProducerRealHandle*)handle;
    if (NULL != producer_real_handle &&
        NULL != producer_real_handle->qbus_producer) {
      rt = producer_real_handle->qbus_producer->produce(data, data_len,
                                                        NULL != key ? key : "")
               ? QBUS_RESULT_OK
               : QBUS_RESULT_FAILED;
    }
  }

  return rt;
}
