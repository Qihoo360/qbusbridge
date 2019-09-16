#include "qbus_rdkafka.h"

namespace qbus {

namespace rdkafka {

// NOTE: Following struct and function are implemented in rdkafka source but
// not exposed in `rdkafka.h`, so we explicitly copy their declarations here
// from `rdkafka_broker.h`.
#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_kafka_broker_s rd_kafka_broker_t;

rd_kafka_broker_t* rd_kafka_broker_any(rd_kafka_t* rk, int state,
                                       int (*filter)(rd_kafka_broker_t* rkb,
                                                     void* opaque),
                                       void* opaque, const char* reason);

#ifdef __cplusplus
}
#endif

bool hasAnyBroker(rd_kafka_t* rk) {
  // 4 is enum value RD_KAKFA_BROKER_STATE_AUTH in rd_kafka_broker_t
  return rd_kafka_broker_any(rk, 4, NULL, NULL, "Unknown");
}

}  // namespace rdkafka

}  // namespace qbus
