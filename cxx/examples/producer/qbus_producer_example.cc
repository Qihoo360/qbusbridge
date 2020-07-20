#include <iostream>
#include "qbus_producer.h"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " config_path topic_name cluster_name"
              << std::endl;
    return 1;
  }

  std::string config_path = argv[1];
  std::string topic_name = argv[2];
  std::string cluster_name = argv[3];

  qbus::QbusProducer producer;
  if (!producer.init(cluster_name, "producer.log", config_path, topic_name)) {
    std::cout << "Init failed" << std::endl;
    return 1;
  }

  std::cout << "%% Please input messages (Press Ctrl+D to exit):" << std::endl;

  while (!std::cin.eof()) {
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) continue;

    if (!producer.produce(line.c_str(), line.size(), "")) {
      std::cout << "Produce failed for " << line << std::endl;
    }
  }

  producer.uninit();
  return 0;
}
