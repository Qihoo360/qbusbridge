#pragma once

#include <pulsar/Producer.h>

namespace qbus {
namespace pulsar {

using SharedStringPtr = std::shared_ptr<std::string>;
using SharedResult = std::shared_ptr<::pulsar::Result>;

class RetryableSendCallback {
    SharedResult result_{std::make_shared<::pulsar::Result>()};

   public:
    RetryableSendCallback(::pulsar::Producer producer, int max_retry_count, int retry_count,
                          SharedStringPtr payload, const std::string& key, const std::string& topic)
        : producer_(producer),
          max_retry_count_(max_retry_count),
          retry_count_(retry_count),
          payload_(payload),
          key_(key),
          topic_(topic) {
        *result_ = ::pulsar::ResultOk;
    }

    void operator()(::pulsar::Result result, const ::pulsar::MessageId& id);

    decltype(*result_) result() const { return *result_; }

   private:
    ::pulsar::Producer producer_;

    const int max_retry_count_;
    int retry_count_;

    SharedStringPtr payload_;
    std::string key_;
    std::string topic_;
};

}  // namespace pulsar
}  // namespace qbus
