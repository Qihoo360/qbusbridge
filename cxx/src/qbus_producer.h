#ifndef QBUS_QBUS_PRODUCER_H_
#define QBUS_QBUS_PRODUCER_H_

#include <string>

namespace qbus {

class QbusProducer {
   public:
    QbusProducer();
    ~QbusProducer();

    bool init(const std::string& cluster, const std::string& log_path, const std::string& config_path,
              const std::string& topic);
    void uninit();
    bool produce(const char* data, size_t data_len, const std::string& key);

    class Imp;

   private:
    Imp* imp_;

    QbusProducer(const QbusProducer&);
    QbusProducer& operator=(const QbusProducer&);
};

}  // namespace qbus

#endif  // QBUS_QBUS_PRODUCER_H_
