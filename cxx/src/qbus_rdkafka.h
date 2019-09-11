#ifndef QBUS_RDKAFKA_H_
#define QBUS_RDKAFKA_H_

#include "thirdparts/librdkafka/src/rdkafka.h"

namespace qbus {

namespace rdkafka {

// Returns true if there is any broker which is not broken.
bool hasAnyBroker(rd_kafka_t* rk);

}  // namespace rdkafka

}  // namespace qbus

#endif  // QBUS_RDKAFKA_H_
