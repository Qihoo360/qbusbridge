#pragma once

#include <vector>
#include <string>

namespace qbus {
namespace pulsar {

inline std::string to_string(const std::vector<std::string>& v) {
    std::string result = "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i > 0) result += ", ";
        result += v[i];
    }
    result += "]";
    return result;
}

}  // namespace pulsar
}  // namespace qbus
