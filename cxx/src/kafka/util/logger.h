#ifndef RRRTC_LOGGER_HXX
#define RRRTC_LOGGER_HXX

#include <string.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

//---------------------------------------------
namespace LUtil {

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define FILE_AND_LINE LOG4CPLUS_TEXT("[") << __FILENAME__ << ":" << __LINE__ << LOG4CPLUS_TEXT("] ")

#define INFO(msg)                                                                             \
    do {                                                                                      \
        if (NULL != LUtil::Logger::instance()) {                                              \
            LOG4CPLUS_INFO(*LUtil::Logger::instance(), FILE_AND_LINE << LOG4CPLUS_TEXT(msg)); \
        }                                                                                     \
    } while (0);
#define DEBUG(msg)                                                                             \
    do {                                                                                       \
        if (NULL != LUtil::Logger::instance()) {                                               \
            LOG4CPLUS_DEBUG(*LUtil::Logger::instance(), FILE_AND_LINE << LOG4CPLUS_TEXT(msg)); \
        }                                                                                      \
    } while (0);
#define WARNING(msg)                                                                          \
    do {                                                                                      \
        if (NULL != LUtil::Logger::instance()) {                                              \
            LOG4CPLUS_WARN(*LUtil::Logger::instance(), FILE_AND_LINE << LOG4CPLUS_TEXT(msg)); \
        }                                                                                     \
    } while (0);
#define ERROR(msg)                                                                             \
    do {                                                                                       \
        if (NULL != LUtil::Logger::instance()) {                                               \
            LOG4CPLUS_ERROR(*LUtil::Logger::instance(), FILE_AND_LINE << LOG4CPLUS_TEXT(msg)); \
        }                                                                                      \
    } while (0);

class Logger {
   public:
    enum LOG_LEVEL
    {
        LL_ALL = 0,
        LL_DEBUG = 10000,
        LL_INFO = 20000,
        LL_WARNING = 30000,
        LL_ERROR = 40000,
        LL_NONE = 50000,
    };

   public:
    static void init(LOG_LEVEL level, const char* fileName, bool outputConsole);
    static void setLogLevel(const std::string& log_level);
    static void uninit() {
        if (NULL != sLogger) {
            delete sLogger;
            sLogger = NULL;
        }
        sInit = false;
    }
    static log4cplus::Logger* instance() {
        if (sInit) {
            if (NULL == sLogger) {
                sLogger = new log4cplus::Logger;
                if (NULL != sLogger) {
                    sLogger->swap(sRealLogger);
                }
            }

            return sLogger;
        } else {
            return NULL;
        }
    }

   private:
    static log4cplus::Logger* sLogger;
    static log4cplus::Logger sRealLogger;
    static bool sInit;

   private:
    Logger(const Logger&);
    Logger& operator=(const Logger&);
};
}  // namespace LUtil
#endif  //#ifndef RRRTC_LOGGER_HXX
