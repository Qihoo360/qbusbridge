#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qbus_producer.h"
#include "qbus_result.h"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("Usage: %s config_path topic_name cluster_name\n", argv[0]);
    return 1;
  }

  const char* config_path = argv[1];
  const char* topic_name = argv[2];
  const char* cluster_name = argv[3];

  QbusProducerHandle producer = NewQbusProducer();
  if (InitQbusProducer(producer, cluster_name, "producer.log", config_path,
                       topic_name) != QBUS_RESULT_OK) {
    printf("Init failed\n");
    DeleteQbusProducer(producer);
    return 2;
  }

  printf("%% Please input messages (Press Ctrl+D to exit):\n");

  char line[1024];
  while (fgets(line, sizeof(line), stdin)) {
    size_t len = strlen(line);
    assert(len > 0);
    --len;
    line[len] == '\0';  // replace '\n' to '\0'
    if (len == 0) continue;

    if (QbusProducerProduce(producer, line, len, "") != QBUS_RESULT_OK) {
      printf("Produce failed for %s\n", line);
    }
  }

  UninitQbusProducer(producer);
  DeleteQbusProducer(producer);
  return 0;
}
