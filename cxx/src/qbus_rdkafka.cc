#include "qbus_rdkafka.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include "util/logger.h"

namespace qbus {

namespace rdkafka {

static bool findStringInJsonBlock(const char* json, const std::string& key,
                                  const char*& value, size_t& value_len) {
  value = json;
  value_len = 0;

  if (!json || key.empty()) return false;

  value = strstr(json, key.c_str());
  if (!value) return false;

  value += key.length();
  while (isspace(*value)) value++;

  if (*value != '"') return false;
  ++value;  // not include '"' in the head

  size_t index = 0;
  while (value[index] != '\0' && value[index] != '"') {
    if (value[index] == '\\' && value[index + 1] == '"') {
      index += 2;
    } else {
      index++;
    }
  }
  if (value[index] != '"') return false;

  value_len = index;  // not include '"' in the tail
  return true;
}

std::string findAnyUpBroker(const char* json) {
  if (!json) return "";

  static const std::string kBrokersField = "\"brokers\":";
  static const std::string kNameField = "\"name\":";
  static const std::string kStateField = "\"state\":";

  json = strstr(json, kBrokersField.c_str());

  const char* state;
  const char* name;
  size_t state_len;
  size_t name_len;

  while (findStringInJsonBlock(json, kStateField.c_str(), state, state_len) &&
         findStringInJsonBlock(json, kNameField.c_str(), name, name_len)) {
    DEBUG(__FUNCTION__ << " | broker: " << std::string(name, name_len) << " => "
                       << std::string(state, state_len));
    if (strncmp("UP", state, state_len) == 0) {
      return std::string(name, name_len);
    } else {
      json = state + state_len + 1;
    }
  }

  return "";
}

}  // namespace rdkafka

}  // namespace qbus
