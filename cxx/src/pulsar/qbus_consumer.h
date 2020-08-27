#pragma once

#include "../qbus_consumer_imp.h"

#include <atomic>
#include <future>
#include <memory>
#include <unordered_map>

#include <pulsar/Client.h>

#include "timer.h"

namespace qbus {
namespace pulsar {

class QbusConsumerConfig;

class QbusConsumer final : public qbus::QbusConsumer::Imp {
   public:
    QbusConsumer();
    ~QbusConsumer();

    bool init(const std::string& cluster_name, const std::string& log_path, const std::string& config_path,
              const qbus::QbusConsumerCallback& callback) override;

    bool subscribe(const std::string& subscription, const std::vector<std::string>& topics) override;
    bool subscribeOne(const std::string& subscription, const std::string& topics) override;

    bool start() override;
    void stop() override;

    bool pause(const std::vector<std::string>& topics) override;
    bool resume(const std::vector<std::string>& topics) override;

    bool consume(QbusMsgContentInfo& msg_content_info) override;
    void commitOffset(const QbusMsgContentInfo& qbusMsgContentInfo) override;

   private:
    std::unique_ptr<::pulsar::Client> client_;
    ::pulsar::Consumer consumer_;
    bool has_subscribed_{false};
    const qbus::QbusConsumerCallback* consumer_cb_ = nullptr;

    ::pulsar::ConsumerConfiguration pulsar_consumer_conf_;

    std::string topic_prefix_;
    std::string default_subscription_;
    std::unique_ptr<QbusConsumerConfig> consumer_conf_;

    std::future<void> consume_loop_future_;
    std::atomic_bool running_{false};
    void consumeLoop();

    bool acknowledge(const std::string& topic, const ::pulsar::MessageId& id);

    // For non-partitioned topic "xxx", key is "xxx";
    // For partitioned topic "xxx", key is "xxx-partitioned-n", n is 0,1,...,N-1, N is the num of partitions
    // Value is the latest consumed message's id
    std::unordered_map<std::string, ::pulsar::MessageId> topic_latest_msg_id_map_;
    Timer timer_;

    void acknowledgeAll();
    void checkTimeThenAcknowledgeAll();

    struct DeferAcknowledge {
        DeferAcknowledge(QbusConsumer& consumer) : consumer_(consumer) {}
        ~DeferAcknowledge() { consumer_.checkTimeThenAcknowledgeAll(); }
        QbusConsumer& consumer_;
    };

    void convertMessage(const ::pulsar::Message& msg, qbus::QbusMsgContentInfo& info);
    ::pulsar::Result consume(::pulsar::Message& msg);

    friend class QbusPhpConsumer;
};

}  // namespace pulsar
}  // namespace qbus
