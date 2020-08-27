#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace qbus {
namespace pulsar {

class QbusProducer;

class QbusProducerMap {
   public:
    static QbusProducerMap& instance();

    // Return a non-null shared pointer of QbusProducer, if key doesn't exist, insert a new QbusProducer to
    // the map and return it
    std::shared_ptr<QbusProducer> operator[](const std::string& key);

   private:
    std::unordered_map<std::string, std::shared_ptr<QbusProducer>> map_;

    QbusProducerMap();
    ~QbusProducerMap();

    QbusProducerMap(const QbusProducerMap&) = delete;
    QbusProducerMap& operator=(const QbusProducerMap&) = delete;
};

}  // namespace pulsar
}  // namespace qbus
