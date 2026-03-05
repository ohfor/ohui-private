#pragma once

#include "ohui/mcm/MCM2Types.h"
#include "ohui/mcm/MCM2ConditionTypes.h"
#include "ohui/cosave/PersistenceAPI.h"
#include "ohui/core/Result.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::mcm {

class MCM2PersistenceManager {
public:
    static constexpr uint32_t kDefaultOrphanSessionLimit = 5;
    static constexpr std::string_view kLastSeenKey = "_ohui.lastSeenSession";

    explicit MCM2PersistenceManager(cosave::PersistenceAPI& persistence);

    Result<void> RegisterDefinition(const MCM2Definition& definition);
    void ApplyDefaults(const MCM2Definition& definition);

    // Typed get/set
    Result<void> SetBool(std::string_view mcmId, std::string_view controlId, bool value);
    bool GetBool(std::string_view mcmId, std::string_view controlId, bool def = false) const;
    Result<void> SetFloat(std::string_view mcmId, std::string_view controlId, float value);
    float GetFloat(std::string_view mcmId, std::string_view controlId, float def = 0.0f) const;
    Result<void> SetInt(std::string_view mcmId, std::string_view controlId, int64_t value);
    int64_t GetInt(std::string_view mcmId, std::string_view controlId, int64_t def = 0) const;
    Result<void> SetString(std::string_view mcmId, std::string_view controlId, std::string_view value);
    std::string GetString(std::string_view mcmId, std::string_view controlId, std::string_view def = "") const;

    // Read all values for condition engine integration
    std::unordered_map<std::string, ConditionValue> GetAllValues(const MCM2Definition& def) const;

    // Session management
    void BeginSession();
    uint64_t CurrentSession() const;

    // Orphan pruning
    Result<size_t> PruneOrphanedEntries(uint32_t sessionLimit = kDefaultOrphanSessionLimit);

    bool IsRegistered(std::string_view mcmId) const;

private:
    cosave::PersistenceAPI& m_persistence;
    uint64_t m_currentSession{0};
    std::vector<std::string> m_registeredMcmIds;

    std::string MakeNamespace(std::string_view mcmId) const;
    void TouchLastSeen(std::string_view mcmId);
    void StoreControlDefault(std::string_view mcmId, const MCM2ControlDef& control);
};

}  // namespace ohui::mcm
