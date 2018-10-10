#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qbus_producer.h"
#include "qbus_result.h"

int main(int argc, char* argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    QbusProducerHandle producer_handle = NULL;

    if (argc < 3) {
        printf("Usage: qbus_producer_example cluster_name topic_name\n");
        return 0;
    }

    producer_handle = NewQbusProducer();
    if (NULL != producer_handle) {
        if (QBUS_RESULT_OK == InitQbusProducer(producer_handle,
                        argv[1],
                        "./producer.log",
                        "./producer.config",
                        argv[2])) {
            printf("Producer init OK!\n");

            while ((read = getline(&line, &len, stdin)) != -1) {
                if (0 == strncmp(line, "stop", read - 1)) {
                    break;
                }
                line[read -1] = '\0';
                if (QBUS_RESULT_FAILED == QbusProducerProduce(producer_handle,
                            line,
                            read,
                            "")) {
                    printf("Failed produce\n");
                    //retry to QbusProducerProduce
                }
            }

            free(line);

            UninitQbusProducer(producer_handle);
        } else {
            printf("Failed to InitQbusProducer\n");
        }

        DeleteQbusProducer(producer_handle);
    }

    return 0;
}
