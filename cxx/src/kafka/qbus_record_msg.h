#pragma once

#include <string>

#include <log4cplus/logger.h>
//--------------------------------------------------------------------
namespace qbus {
namespace kafka {

class QbusRecordMsg {
   public:
    static void recordMsg(const std::string& topic, const std::string& msg);

   private:
    static void init();

   private:
    static log4cplus::Logger sLogger;
    static bool sInit;

   private:
    QbusRecordMsg();
};

}  // namespace kafka
}  // namespace qbus
