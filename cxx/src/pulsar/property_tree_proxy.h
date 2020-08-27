#pragma once

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace qbus {
namespace pulsar {

class PropertyTreeProxy {
   public:
    PropertyTreeProxy(const pt::ptree& tree) : tree_(tree) {}

    void config(const char* key, std::string& value) {
        auto iter = tree_.find(key);
        if (iter != tree_.not_found()) {
            value = iter->second.get_value<std::string>();
        }
    }

    template <typename T>
    void config(const char* key, T& value, const char* type) {
        auto iter = tree_.find(key);
        if (iter != tree_.not_found()) {
            try {
                value = iter->second.get_value<T>();
            } catch (const pt::ptree_bad_data& e) {
                handleBadDataError(key, iter->second.get_value<std::string>(), type, e);
            }
        }
    }

    void configInt(const char* key, int& value) { config(key, value, "int"); }

    void configBool(const char* key, bool& value) { config(key, value, "bool"); }

   private:
    const pt::ptree& tree_;

    void handleBadDataError(const char* key, const std::string& value, const char* type,
                            const pt::ptree_bad_data& e);
};

}  // namespace pulsar
}  // namespace qbus
