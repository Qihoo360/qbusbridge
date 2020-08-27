#include "property_tree_proxy.h"
#include "log_utils.h"

DECLARE_LOG_OBJECT()

namespace qbus {
namespace pulsar {

void PropertyTreeProxy::handleBadDataError(const char* key, const std::string& value, const char* type,
                                           const pt::ptree_bad_data& e) {
    LOG_WARN("Failed to convert " << value << "(of key: " << key << ") to " << type);
}

}  // namespace pulsar
}  // namespace qbus
