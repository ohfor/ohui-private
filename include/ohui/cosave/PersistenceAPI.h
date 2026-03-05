#pragma once

#include "ohui/cosave/CosaveManager.h"
#include "ohui/core/Result.h"

#include <cstdint>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ohui::cosave {

class PersistenceAPI {
public:
    static constexpr size_t kMaxEntrySize = 64 * 1024;       // 64 KB
    static constexpr size_t kMaxModTotalSize = 1024 * 1024;   // 1 MB

    explicit PersistenceAPI(CosaveManager& mgr);

    // Registration
    Result<void> Register(std::string_view modNamespace);
    bool IsRegistered(std::string_view modNamespace) const;

    // Typed get/set
    Result<void> SetBool(std::string_view mod, std::string_view key, bool val);
    bool GetBool(std::string_view mod, std::string_view key, bool def = false) const;

    Result<void> SetInt(std::string_view mod, std::string_view key, int64_t val);
    int64_t GetInt(std::string_view mod, std::string_view key, int64_t def = 0) const;

    Result<void> SetFloat(std::string_view mod, std::string_view key, float val);
    float GetFloat(std::string_view mod, std::string_view key, float def = 0.0f) const;

    Result<void> SetString(std::string_view mod, std::string_view key, std::string_view val);
    std::string GetString(std::string_view mod, std::string_view key, std::string_view def = "") const;

    Result<void> SetBytes(std::string_view mod, std::string_view key, std::span<const uint8_t> val);
    std::vector<uint8_t> GetBytes(std::string_view mod, std::string_view key) const;

    // Management
    bool Has(std::string_view mod, std::string_view key) const;
    Result<void> Delete(std::string_view mod, std::string_view key);
    Result<void> ClearAll(std::string_view mod);

    // Versioning
    Result<void> SetVersion(std::string_view mod, uint32_t version);
    uint32_t GetVersion(std::string_view mod) const;

    // Serialization
    std::vector<uint8_t> SerializeAll() const;
    Result<void> DeserializeAll(std::span<const uint8_t> data);
    void RevertAll();

private:
    using Value = std::variant<bool, int64_t, float, std::string, std::vector<uint8_t>>;

    struct ModStore {
        uint32_t version{0};
        std::unordered_map<std::string, Value> entries;
        size_t totalSize{0};
    };

    static size_t EstimateValueSize(const Value& v);

    CosaveManager& m_mgr;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ModStore> m_stores;
};

}  // namespace ohui::cosave
