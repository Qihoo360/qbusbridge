#include "retryable_send_callback.h"
#include "qbus_producer.h"
#include "log_utils.h"

using namespace pulsar;
DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

void RetryableSendCallback::operator()(Result result, const MessageId& id) {
    *result_ = result;
    if (result == ResultOk) {
        LOG_DEBUG("Sent msg to " << id << " | topic: " << topic_);
    } else if (result == ResultTimeout) {
        if (retry_count_ < max_retry_count_) {
            retry_count_++;
            LOG_INFO("Send error: " << result << ", retry " << retry_count_ << " of " << max_retry_count_
                                    << " | topic: " << topic_);
            // TODO: expose pulsar API to get partition count. If result is ResultProducerQueueIsFull, choose
            // another partition to send
            producer_.sendAsync(QbusProducer::createMessage(payload_, key_), *this);
        } else {
            LOG_ERROR("Send error: " << result << " retry count exceeded limit: " << max_retry_count_
                                     << " | topic: " << topic_ << " | key: " << key_
                                     << " | msg size: " << payload_->size()
                                     << " | last sequence id: " << producer_.getLastSequenceId());
        }
    } else if (result != ResultProducerQueueIsFull) {
        // We'll print the QueueIsFull error in QbusProducer::produce()
        LOG_ERROR("Send unretryable error: " << result << " | topic: " << topic_
                                             << " | msg size: " << payload_->size()
                                             << " | last sequence id: " << producer_.getLastSequenceId());
    }  // else: result == ResultProducerQueueIsFull
}

}  // namespace pulsar
}  // namespace qbus
