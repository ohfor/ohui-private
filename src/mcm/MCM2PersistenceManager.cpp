#include "ohui/mcm/MCM2PersistenceManager.h"
#include "ohui/core/Log.h"

namespace ohui::mcm {

MCM2PersistenceManager::MCM2PersistenceManager(cosave::PersistenceAPI& persistence)
    : m_persistence(persistence) {}

std::string MCM2PersistenceManager::MakeNamespace(std::string_view mcmId) const {
    return "mcm2." + std::string(mcmId);
}

Result<void> MCM2PersistenceManager::RegisterDefinition(const MCM2Definition& definition) {
    auto ns = MakeNamespace(definition.id);

    // Register namespace if not already registered (idempotent)
    if (!m_persistence.IsRegistered(ns)) {
        auto result = m_persistence.Register(ns);
        if (!result.has_value()) return result;
    }

    // Track registration (avoid duplicates in our list)
    bool alreadyTracked = false;
    for (const auto& id : m_registeredMcmIds) {
        if (id == definition.id) { alreadyTracked = true; break; }
    }
    if (!alreadyTracked) {
        m_registeredMcmIds.push_back(definition.id);
    }

    TouchLastSeen(definition.id);
    return {};
}

void MCM2PersistenceManager::ApplyDefaults(const MCM2Definition& definition) {
    auto ns = MakeNamespace(definition.id);

    for (const auto& page : definition.pages) {
        for (const auto& section : page.sections) {
            for (const auto& control : section.controls) {
                if (control.id.empty()) continue;
                // Only set default if key doesn't already exist
                if (!m_persistence.Has(ns, control.id)) {
                    StoreControlDefault(definition.id, control);
                }
            }
        }
    }
}

void MCM2PersistenceManager::StoreControlDefault(std::string_view mcmId, const MCM2ControlDef& control) {
    std::visit([this, &mcmId, &control](const auto& props) {
        using T = std::decay_t<decltype(props)>;
        if constexpr (std::is_same_v<T, MCM2ToggleProps>) {
            SetBool(mcmId, control.id, props.defaultValue);
        } else if constexpr (std::is_same_v<T, MCM2SliderProps>) {
            SetFloat(mcmId, control.id, props.defaultValue);
        } else if constexpr (std::is_same_v<T, MCM2DropdownProps>) {
            // Store index of default option
            int64_t idx = 0;
            for (size_t i = 0; i < props.options.size(); ++i) {
                if (props.options[i] == props.defaultValue) {
                    idx = static_cast<int64_t>(i);
                    break;
                }
            }
            SetInt(mcmId, control.id, idx);
        } else if constexpr (std::is_same_v<T, MCM2KeyBindProps>) {
            SetInt(mcmId, control.id, props.defaultValue);
        } else if constexpr (std::is_same_v<T, MCM2ColourProps>) {
            SetInt(mcmId, control.id, props.defaultValue);
        } else if constexpr (std::is_same_v<T, MCM2TextProps>) {
            SetString(mcmId, control.id, props.defaultValue);
        }
        // Header and Empty have no persistence
    }, control.properties);
}

// --- Typed get/set ---

Result<void> MCM2PersistenceManager::SetBool(std::string_view mcmId, std::string_view controlId, bool value) {
    auto ns = MakeNamespace(mcmId);
    if (!m_persistence.IsRegistered(ns)) {
        return std::unexpected(Error{ErrorCode::NotRegistered, "MCM not registered: " + std::string(mcmId)});
    }
    return m_persistence.SetBool(ns, controlId, value);
}

bool MCM2PersistenceManager::GetBool(std::string_view mcmId, std::string_view controlId, bool def) const {
    return m_persistence.GetBool(MakeNamespace(mcmId), controlId, def);
}

Result<void> MCM2PersistenceManager::SetFloat(std::string_view mcmId, std::string_view controlId, float value) {
    auto ns = MakeNamespace(mcmId);
    if (!m_persistence.IsRegistered(ns)) {
        return std::unexpected(Error{ErrorCode::NotRegistered, "MCM not registered: " + std::string(mcmId)});
    }
    return m_persistence.SetFloat(ns, controlId, value);
}

float MCM2PersistenceManager::GetFloat(std::string_view mcmId, std::string_view controlId, float def) const {
    return m_persistence.GetFloat(MakeNamespace(mcmId), controlId, def);
}

Result<void> MCM2PersistenceManager::SetInt(std::string_view mcmId, std::string_view controlId, int64_t value) {
    auto ns = MakeNamespace(mcmId);
    if (!m_persistence.IsRegistered(ns)) {
        return std::unexpected(Error{ErrorCode::NotRegistered, "MCM not registered: " + std::string(mcmId)});
    }
    return m_persistence.SetInt(ns, controlId, value);
}

int64_t MCM2PersistenceManager::GetInt(std::string_view mcmId, std::string_view controlId, int64_t def) const {
    return m_persistence.GetInt(MakeNamespace(mcmId), controlId, def);
}

Result<void> MCM2PersistenceManager::SetString(std::string_view mcmId, std::string_view controlId, std::string_view value) {
    auto ns = MakeNamespace(mcmId);
    if (!m_persistence.IsRegistered(ns)) {
        return std::unexpected(Error{ErrorCode::NotRegistered, "MCM not registered: " + std::string(mcmId)});
    }
    return m_persistence.SetString(ns, controlId, value);
}

std::string MCM2PersistenceManager::GetString(std::string_view mcmId, std::string_view controlId, std::string_view def) const {
    return m_persistence.GetString(MakeNamespace(mcmId), controlId, def);
}

// --- GetAllValues ---

std::unordered_map<std::string, ConditionValue> MCM2PersistenceManager::GetAllValues(
    const MCM2Definition& def) const {
    std::unordered_map<std::string, ConditionValue> values;
    auto ns = MakeNamespace(def.id);

    for (const auto& page : def.pages) {
        for (const auto& section : page.sections) {
            for (const auto& control : section.controls) {
                if (control.id.empty()) continue;

                std::visit([&](const auto& props) {
                    using T = std::decay_t<decltype(props)>;
                    if constexpr (std::is_same_v<T, MCM2ToggleProps>) {
                        values[control.id] = ConditionValue{
                            m_persistence.GetBool(ns, control.id, props.defaultValue)};
                    } else if constexpr (std::is_same_v<T, MCM2SliderProps>) {
                        values[control.id] = ConditionValue{
                            static_cast<double>(m_persistence.GetFloat(ns, control.id, props.defaultValue))};
                    } else if constexpr (std::is_same_v<T, MCM2DropdownProps>) {
                        int64_t idx = 0;
                        for (size_t i = 0; i < props.options.size(); ++i) {
                            if (props.options[i] == props.defaultValue) {
                                idx = static_cast<int64_t>(i);
                                break;
                            }
                        }
                        values[control.id] = ConditionValue{
                            m_persistence.GetInt(ns, control.id, idx)};
                    } else if constexpr (std::is_same_v<T, MCM2KeyBindProps>) {
                        values[control.id] = ConditionValue{
                            m_persistence.GetInt(ns, control.id, props.defaultValue)};
                    } else if constexpr (std::is_same_v<T, MCM2ColourProps>) {
                        values[control.id] = ConditionValue{
                            m_persistence.GetInt(ns, control.id, props.defaultValue)};
                    } else if constexpr (std::is_same_v<T, MCM2TextProps>) {
                        values[control.id] = ConditionValue{
                            m_persistence.GetString(ns, control.id, props.defaultValue)};
                    }
                }, control.properties);
            }
        }
    }

    return values;
}

// --- Session management ---

void MCM2PersistenceManager::BeginSession() {
    ++m_currentSession;
}

uint64_t MCM2PersistenceManager::CurrentSession() const {
    return m_currentSession;
}

void MCM2PersistenceManager::TouchLastSeen(std::string_view mcmId) {
    auto ns = MakeNamespace(mcmId);
    m_persistence.SetInt(ns, std::string(kLastSeenKey), static_cast<int64_t>(m_currentSession));
}

// --- Orphan pruning ---

Result<size_t> MCM2PersistenceManager::PruneOrphanedEntries(uint32_t sessionLimit) {
    size_t pruned = 0;

    // Check all registered MCM IDs
    std::vector<std::string> toRemove;
    for (const auto& mcmId : m_registeredMcmIds) {
        auto ns = MakeNamespace(mcmId);
        auto lastSeen = static_cast<uint64_t>(m_persistence.GetInt(ns, std::string(kLastSeenKey), 0));

        if (m_currentSession > lastSeen &&
            (m_currentSession - lastSeen) >= sessionLimit) {
            toRemove.push_back(mcmId);
        }
    }

    for (const auto& mcmId : toRemove) {
        auto ns = MakeNamespace(mcmId);
        m_persistence.ClearAll(ns);
        ++pruned;

        // Remove from tracked list
        std::erase(m_registeredMcmIds, mcmId);
    }

    return pruned;
}

bool MCM2PersistenceManager::IsRegistered(std::string_view mcmId) const {
    auto ns = MakeNamespace(mcmId);
    return m_persistence.IsRegistered(ns);
}

}  // namespace ohui::mcm
