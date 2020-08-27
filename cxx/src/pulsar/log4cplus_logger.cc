#include "log4cplus_logger.h"
#include <assert.h>
#include <mutex>
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>

using namespace ::pulsar;

namespace qbus {
namespace pulsar {

static const log4cplus::LogLevel kLog4cplusLevel[4] = {log4cplus::DEBUG_LOG_LEVEL, log4cplus::INFO_LOG_LEVEL,
                                                       log4cplus::WARN_LOG_LEVEL, log4cplus::ERROR_LOG_LEVEL};

Log4cplusLogger::Log4cplusLogger(const std::string& filename)
    : logger_(log4cplus::Logger::getRoot()), level_(LEVEL_DEBUG), filename_(filename) {}

bool Log4cplusLogger::isEnabled(Level level) { return level >= level_; }

void Log4cplusLogger::log(Level level, int line, const std::string& message) {
    assert(level >= 0 && level < 4);
    logger_.log(kLog4cplusLevel[level], message, filename_.c_str(), line);
}

void Log4cplusLogger::setLevel(const std::string& level) {
    logger_.setLogLevel(log4cplus::LogLevelManager().fromString(level));
}

void Log4cplusLogger::setLogPath(const std::string& path) {
    using namespace log4cplus;

    const char* pattern = "[%p] [%D{%m/%d/%y %H:%M:%S,%q}] [%t] [%l] - %m %n";
    std::auto_ptr<Layout> layout(new PatternLayout(pattern));
    SharedAppenderPtr appender;
    if (!path.empty()) {
        appender = new RollingFileAppender(path, 100 * 1024 * 1024 /* 100 MB */);
    } else {
        appender = new ConsoleAppender();
    }

    appender->setLayout(layout);
    logger_.removeAllAppenders();
    logger_.addAppender(appender);
}

Log4cplusLoggerFactory::Log4cplusLoggerFactory(const std::string& level, const std::string& log_path)
    : level_(level), log_path_(log_path) {
    static std::once_flag flag;
    std::call_once(flag, [] { log4cplus::initialize(); });
}

::pulsar::Logger* Log4cplusLoggerFactory::getLogger(const std::string& filename) {
    auto log4cplus_logger = new Log4cplusLogger(filename);
    log4cplus_logger->setLevel(level_);
    log4cplus_logger->setLogPath(log_path_);
    return static_cast<::pulsar::Logger*>(log4cplus_logger);
}

}  // namespace pulsar
}  // namespace qbus
