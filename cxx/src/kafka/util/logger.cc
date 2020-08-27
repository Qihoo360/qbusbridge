#include "logger.h"

#include <strings.h>

#include <log4cplus/helpers/pointer.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>

using namespace LUtil;

extern const char* version;

const char* DEFAULT_LOG_FILE_NAME = "__lutil_logger__.log";
const char* LOGGER_NAME = "__lutil_logger__";
//---------------------------------------------
log4cplus::Logger Logger::sRealLogger = log4cplus::Logger::getInstance(LOGGER_NAME);
log4cplus::Logger* Logger::sLogger = NULL;
bool Logger::sInit = false;

void Logger::init(LOG_LEVEL level, const char* fileName, bool outputConsole) {
    if (LL_NONE == level) {
        return;
    }

    if (!sInit) {
        if (NULL == sLogger) {
            sLogger = new log4cplus::Logger;
            if (NULL != sLogger) {
                sLogger->swap(sRealLogger);
            }
        }

        if (NULL == sLogger) {
            return;
        }

        if (NULL == fileName || '\0' == fileName[0]) {
            fileName = DEFAULT_LOG_FILE_NAME;
        }

        std::auto_ptr<log4cplus::Layout> layout(
            new log4cplus::PatternLayout("[%p] [%D{%m/%d/%y %H:%M:%S,%q}] [%t] %m %n"));
        if (NULL != layout.get()) {
            log4cplus::SharedAppenderPtr fileAppender(
                new log4cplus::RollingFileAppender(fileName, 100 * 1024 * 1024));
            if (NULL != fileAppender.get()) {
                fileAppender->setName("file log");
                fileAppender->setLayout(layout);

                sLogger->addAppender(fileAppender);
            }

            if (outputConsole) {
                log4cplus::SharedAppenderPtr consoleAppender(new log4cplus::ConsoleAppender());
                if (NULL != consoleAppender.get()) {
                    consoleAppender->setName("console log");
                    std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(
                        //"[%p] [%D{%m/%d/%y %H:%M:%S:%s}] [%t] [%l] - %m %n"));
                        "[%p] [%D{%m/%d/%y %H:%M:%S,%q}] [%t] [%l] - %m %n"));
                    if (NULL != layout.get()) {
                        consoleAppender->setLayout(layout);
                    }

                    sLogger->addAppender(consoleAppender);
                }
            }

            sLogger->setLogLevel(log4cplus::LogLevel(level));

            sInit = true;
            INFO("qbus version: " << version);
        }
    }
}

void Logger::setLogLevel(const std::string& log_level) {
    LOG_LEVEL level = LL_INFO;

    if (0 == strncasecmp(log_level.c_str(), "all", log_level.length())) {
        level = LL_ALL;
    } else if (0 == strncasecmp(log_level.c_str(), "debug", log_level.length())) {
        level = LL_DEBUG;
    } else if (0 == strncasecmp(log_level.c_str(), "info", log_level.length())) {
        level = LL_INFO;
    } else if (0 == strncasecmp(log_level.c_str(), "warning", log_level.length())) {
        level = LL_WARNING;
    } else if (0 == strncasecmp(log_level.c_str(), "error", log_level.length())) {
        level = LL_ERROR;
    } else if (0 == strncasecmp(log_level.c_str(), "none", log_level.length())) {
        level = LL_NONE;
    }

    if (LL_NONE != level) {
        sLogger->setLogLevel(log4cplus::LogLevel(level));
    }
}
//---------------------------------------------
