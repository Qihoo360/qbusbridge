#include "qbus_record_msg.h"

#include <log4cplus/fileappender.h>
#include <log4cplus/helpers/pointer.h>
#include <log4cplus/loggingmacros.h>

static const char* RECORD_DEFAULT_LOG_FILE_NAME = "record_msg.qbus";
static const char* RECORD_LOGGER_NAME = "__record_msg_for_send_fail__";
static const int RECORD_FILE_BACKUP_INDEX = 24;
//----------------------------------------
namespace qbus {
bool QbusRecordMsg::sInit = false;
log4cplus::Logger QbusRecordMsg::sLogger =
    log4cplus::Logger::getInstance(RECORD_LOGGER_NAME);

void QbusRecordMsg::recordMsg(const std::string& topic,
                              const std::string& msg) {
  if (!sInit) {
    init();
  }

  if (sInit) {
    LOG4CPLUS_INFO(sLogger, topic + "|" + msg);
  }
}

void QbusRecordMsg::init() {
  std::auto_ptr<log4cplus::Layout> layout(
      new log4cplus::PatternLayout("%m %n"));
  if (NULL != layout.get()) {
    log4cplus::SharedAppenderPtr fileAppender(
        new log4cplus::DailyRollingFileAppender(RECORD_DEFAULT_LOG_FILE_NAME,
                                                log4cplus::HOURLY, true,
                                                RECORD_FILE_BACKUP_INDEX));
    if (NULL != fileAppender.get()) {
      fileAppender->setName("record log");
      fileAppender->setLayout(layout);

      sLogger.addAppender(fileAppender);
    }

    sLogger.setLogLevel(log4cplus::INFO_LOG_LEVEL);

    sInit = true;
  }
}
}  // namespace qbus
