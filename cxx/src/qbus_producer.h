#ifndef QBUS_PRODUCER_H_
#define QBUS_PRODUCER_H_

#include <string>
//-----------------------------------------------------
namespace qbus {

class QbusProducerImp;

class QbusProducer {
  public:
    QbusProducer();
    ~QbusProducer();

  public:
    bool init(const std::string& broker_list,
        const std::string& log_path,
        const std::string& config_path,
        const std::string& topic_name);
    void uninit();
    bool produce(const char* data, size_t data_len, const std::string& key);

  private:
    QbusProducerImp* qbus_producer_imp_;
};
} //namespace qbus
#endif //#define QBUS_PRODUCER_H_

