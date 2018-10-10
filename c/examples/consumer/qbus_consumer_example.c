#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "qbus_consumer.h"

//如果想使用纯手动提交offset,请打开下面的注释,并且需要在consumer.config中设置user.manual.commit.offset=true,详情见wiki:http://add.corp.qihoo.net:8360/pages/viewpage.action?pageId=13787969
//采用纯手动提交offset模式,需要调用InitQbusConsumerEx作初始化,而不是InitQbusConsumer
//对于绝大多数应用,这个不是必需的
//#define USE_COMMIT_OFFSET_CALLBACK 

int kStop = 0;

QbusConsumerHandle consumer_handle = NULL;

void QbusConsumerDeliveryMsg(const char* topic,
            const char* msg,
            int64_t msg_len) {
    printf("Topic: %s| msg: %s\n", topic, msg);
}

void QbusConsumerDeliveryForCommitOffsetFunc(const char* topic,
            const char* msg,
            int64_t msg_len,
            const QbusCommitOffsetInfoType offset_info) {
    printf("User commit | Topic: %s| msg: %s\n", topic, msg);
    QbusConsumerCommitOffset(consumer_handle, offset_info);
}

void Stop(int s) {
    kStop = 1;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: qbuns_consumer topic group cluster_name\n");
        return 0;
    }
    
    signal(SIGINT, Stop);

#ifdef USE_COMMIT_OFFSET_CALLBACK
    QbusConsumerCallbackInfo callback_info;
    callback_info.callback = QbusConsumerDeliveryMsg;
    callback_info.callback_for_commit_offset = QbusConsumerDeliveryForCommitOffsetFunc;
#endif

    consumer_handle = NewQbusConsumer();    
    if (NULL != consumer_handle) {
        if (QBUS_RESULT_OK == 
#ifndef USE_COMMIT_OFFSET_CALLBACK
                    InitQbusConsumer(consumer_handle,
                        argv[3],
                        "./consumer.log",
                        "./consumer.config",
                        QbusConsumerDeliveryMsg)
#else

                    InitQbusConsumerEx(consumer_handle,
                        argv[3],
                        "./consumer.log",
                        "./consumer.config",
                        callback_info)
#endif
                    ) {

            if (QBUS_RESULT_OK == QbusConsumerSubscribeOne(consumer_handle, argv[2], argv[1])) {
                if (QBUS_RESULT_OK == QbusConsumerStart(consumer_handle)) {
                    while (!kStop) {
                        sleep(1);
                    }

                    QbusConsumerStop(consumer_handle);
                } else {
                    printf("Failed to QbusConsumerStart\n");
                }
            } else {
                printf("Failed to QbusConsumerSubscribeOne\n");
            }
        } else {
            printf("Failed to InitQbusConsumer\n");
        }

        DeleteQbusConsumer(consumer_handle);
    }

    return 0;
}
