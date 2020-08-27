#include "qbus_producer.h"
#include "qbus_producer_config.h"
#include "retryable_send_callback.h"

#include "log4cplus_logger.h"
#include "log_utils.h"
#include "pulsar_config.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <chrono>
#include <thread>

using std::string;
using namespace pulsar;
DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

inline ::pulsar::CompressionType toCompressionType(std::string compression) {
    std::transform(compression.cbegin(), compression.cend(), compression.begin(),
                   [](char c) -> char { return tolower(c); });
    if (compression == "none") {
        return ::pulsar::CompressionNone;
    } else if (compression == "lz4") {
        return ::pulsar::CompressionLZ4;
    } else if (compression == "zlib") {
        return ::pulsar::CompressionZLib;
    } else if (compression == "zstd") {
        return ::pulsar::CompressionZSTD;
    } else {
        LOG_WARN("Unsupported compression: " << compression << ", set it to none");
        return ::pulsar::CompressionNone;
    }
}

bool QbusProducer::init(const string& broker, const string& log_path, const string& config_path,
                        const string& topic) {
    if (client_) {
        LOG_ERROR("QbusProducer already initialized");
        return false;
    }

    PulsarConfig config;
    try {
        config.loadConfig(config_path);
    } catch (const PulsarConfig::Error& e) {
        LOG_ERROR(e.what());
        return false;
    }

    // User provided config has higher priority than config of file
    if (!broker.empty()) {
        config.put(config.KEY_BROKER, broker);
    }

    ClientConfiguration client_config;
    client_config.setStatsIntervalInSeconds(0);
    client_config.setLogger(new Log4cplusLoggerFactory(config.log_level(), log_path));

    producer_config_ = config.producer_config();
    std::string jwt = producer_config_->auth_jwt;
    if (!jwt.empty()) {
        client_config.setAuth(::pulsar::AuthToken::createWithToken(jwt));
    }

    client_.reset(new Client(config.broker(), client_config));

    LOG_INFO("ProducerConfig: \n" << producer_config_->to_string());
    auto conf =
        ProducerConfiguration()
            .setPartitionsRoutingMode(ProducerConfiguration::RoundRobinDistribution)
            .setBlockIfQueueFull(false)
            .setSendTimeout(producer_config_->send_timeout_ms)
            .setBatchingEnabled(producer_config_->batching_enabled)
            .setBatchingMaxMessages(producer_config_->batching_max_messages)
            .setBatchingMaxAllowedSizeInBytes(producer_config_->batching_max_allowed_size_kbytes * 1024)
            .setBatchingMaxPublishDelayMs(producer_config_->batching_max_publish_delay_ms)
            .setCompressionType(toCompressionType(producer_config_->compression_type))
            .setMaxPendingMessages(producer_config_->max_pending_messages)
            .setMaxPendingMessagesAcrossPartitions(INT_MAX);

    topic_ = config.topic_prefix() + topic;
    auto result = client_->createProducer(topic_, conf, producer_);
    if (result != ResultOk) {
        producer_config_.reset(nullptr);
        // Synchronous closes of producer and client are necessary because some internal producers may still
        // have some messages to log. If we don't close here and caller exits the program immediatly, the
        // logger may be used after main function returns, while the logger may be invalid.
        producer_.close();
        client_->close();
        client_.reset(nullptr);
        LOG_ERROR("Failed to create producer: " << result);
        return false;
    }

    return true;
}

void QbusProducer::uninit() {
    if (!client_) {
        LOG_WARN("QbusProducer not initialized");
        return;
    }

    Result result;
    if ((result = producer_.flush()) != ResultOk) {
        LOG_ERROR("Failed to flush: " << result);
    }
    if ((result = producer_.close()) != ResultOk) {
        LOG_WARN("Failed to close producer: " << result);
    }
    if ((result = client_->close()) != ResultOk) {
        LOG_WARN("Failed to close client: " << result);
    }
    client_.reset(nullptr);
}

bool QbusProducer::produce(const char* data, size_t data_len, const string& key) {
    if (!client_) {
        LOG_ERROR("QbusProducer not initialized");
        return false;
    }

    auto payload = std::make_shared<std::string>(data, data_len);
    Result result = sendAsync(payload, key);
    if (result != ResultOk) {
        LOG_ERROR("Failed to produce: " << result << " msg size: " << payload->size() << " | key: " << key);
        return false;
    }
    return true;
}

Message QbusProducer::createMessage(const SharedStringPtr& payload, const std::string& key) {
    MessageBuilder builder;
    if (!key.empty()) {
        builder.setPartitionKey(key);
    }
    builder.setAllocatedContent(const_cast<char*>(payload->c_str()), payload->length());
    return builder.build();
}

Result QbusProducer::sendAsync(const SharedStringPtr& payload, const std::string& key) {
    assert(producer_config_);
    for (int i = 0; i < producer_config_->queue_full_retry_count; i++) {
        RetryableSendCallback callback(producer_, producer_config_->timeout_retry_count, 0, payload, key,
                                       topic_);
        producer_.sendAsync(createMessage(payload, key), callback);

        Result result = callback.result();
        if (result == ResultOk) {
            return ResultOk;
        } else if (result == ResultProducerQueueIsFull) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(producer_config_->queue_full_retry_interval_ms));
        } else {
            return result;
        }
    }
    return ResultProducerQueueIsFull;
}

}  // namespace pulsar

}  // namespace qbus
