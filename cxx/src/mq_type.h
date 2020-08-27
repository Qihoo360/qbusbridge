#pragma once

#include <cctype>
#include <algorithm>
#include <boost/property_tree/ini_parser.hpp>
namespace pt = boost::property_tree;

namespace qbus {

enum class MqType
{
    KAFKA,
    PULSAR
};

inline MqType mqType(const std::string& config_path) {
    pt::ptree root;
    try {
        pt::ini_parser::read_ini(config_path, root);
    } catch (const pt::ini_parser_error& e) {
        throw std::runtime_error("Failed to parse " + config_path + ": " + e.what());
    }

    auto iter = root.find("mq.type");
    if (iter == root.not_found()) {
        return MqType::KAFKA;  // default type is kafka
    }

    auto mq_type = iter->second.get_value<std::string>();
    std::transform(mq_type.cbegin(), mq_type.cend(), mq_type.begin(), [](char ch) { return tolower(ch); });

    if (mq_type == "kafka") {
        return MqType::KAFKA;
    } else if (mq_type == "pulsar") {
        return MqType::PULSAR;
    } else {
        throw std::runtime_error("Unsupported mq.type: " + mq_type);
    }
}

}  // namespace qbus
