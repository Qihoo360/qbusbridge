#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <pulsar/Logger.h>
#include <log4cplus/logger.h>

namespace qbus {
namespace pulsar {

class Log4cplusLogger final : public ::pulsar::Logger {
   public:
    Log4cplusLogger(const std::string& filename);

    bool isEnabled(Level level) override;
    void log(Level level, int line, const std::string& message) override;

    void setLevel(const std::string& level);
    void setLogPath(const std::string& path);

   private:
    log4cplus::Logger logger_;
    Level level_;
    const std::string filename_;

#ifdef NOT_USE_CONSUMER_CALLBACK
    static std::atomic_int kInitialized;

   public:
    static void uninit();
#endif
};

class Log4cplusLoggerFactory final : public ::pulsar::LoggerFactory {
   public:
    Log4cplusLoggerFactory(const std::string& level, const std::string& log_path);

    ::pulsar::Logger* getLogger(const std::string& filename) override;

    void setLevel(const std::string& level) { level_ = level; }
    void setLogPath(const std::string& log_path) { log_path_ = log_path; }

   private:
    std::string level_ = "debug";
    std::string log_path_;
    mutable std::mutex mutex_;
};

}  // namespace pulsar
}  // namespace qbus
