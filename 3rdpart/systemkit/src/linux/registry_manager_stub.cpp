#include "systemkit/registry/registry_manager.hpp"

namespace leigod {
namespace manager {

/**
 * @class RegistryManagerStub
 * @brief Stub implementation of the IRegistryManager interface for non-Windows platforms.
 */
class RegistryManagerStub : public IRegistryManager {
public:
    void addStringValue(const std::string& path, const std::string& name,
                        const std::string& value) override {
        // Stub implementation
    }

    void addDwordValue(const std::string& path, const std::string& name, int64_t value) override {
        // Stub implementation
    }

    void addSubKey(const std::string& path, const std::string& name) override {
        // Stub implementation
    }

    void removeKey(const std::string& path, const std::string& name) override {
        // Stub implementation
    }

    void removeKey(const std::string& path) {
        // Stub implementation
    }

    std::map<std::string, std::string> getStringValues(const std::string& path) const override {
        // Stub implementation
        return {};
    }

    std::map<std::string, int64_t> getDwordValues(const std::string& path) const override {
        // Stub implementation
        return {};
    }

    std::map<std::string, std::string> getSubKeys(const std::string& path) const override {
        // Stub implementation
        return {};
    }
};

}  // namespace manager
}  // namespace leigod