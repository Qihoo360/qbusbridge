#ifndef QBUS_PRODUCER_C_H_
#define QBUS_PRODUCER_C_H_

#include <stdio.h>
#include <stdint.h>

#include "qbus_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* QbusProducerHandle;

QbusProducerHandle NewQbusProducer();

void DeleteQbusProducer(QbusProducerHandle producer_handle);

QbusResult InitQbusProducer(QbusProducerHandle handle,
            const char* cluster_name,
            const char* log_path,
            const char* config_path,
            const char* topic_name);

void UninitQbusProducer(QbusProducerHandle handle);

QbusResult QbusProducerProduce(QbusProducerHandle handle,
            const char* data,
            int64_t data_len,
            const char* key);

#ifdef __cplusplus
}
#endif
#endif//#ifndef QBUS_PRODUCER_H_
