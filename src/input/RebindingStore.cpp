#include "ohui/input/RebindingStore.h"
#include <cstring>

namespace ohui::input {

static std::string MakeKey(std::string_view contextId, std::string_view actionId) {
    std::string key;
    key.reserve(contextId.size() + 1 + actionId.size());
    key.append(contextId);
    key.push_back('.');
    key.append(actionId);
    return key;
}

Result<void> RebindingStore::SetKeyBinding(std::string_view contextId,
                                            std::string_view actionId,
                                            KeyCode key) {
    auto mapKey = MakeKey(contextId, actionId);
    auto it = m_rebindings.find(mapKey);
    if (it != m_rebindings.end()) {
        it->second.key = key;
    } else {
        RebindEntry entry;
        entry.contextId = std::string(contextId);
        entry.actionId = std::string(actionId);
        entry.key = key;
        m_rebindings[mapKey] = std::move(entry);
    }
    return {};
}

Result<void> RebindingStore::SetButtonBinding(std::string_view contextId,
                                               std::string_view actionId,
                                               GamepadButton button) {
    auto mapKey = MakeKey(contextId, actionId);
    auto it = m_rebindings.find(mapKey);
    if (it != m_rebindings.end()) {
        it->second.button = button;
    } else {
        RebindEntry entry;
        entry.contextId = std::string(contextId);
        entry.actionId = std::string(actionId);
        entry.button = button;
        m_rebindings[mapKey] = std::move(entry);
    }
    return {};
}

bool RebindingStore::HasKeyConflict(std::string_view contextId,
                                     std::string_view actionId,
                                     KeyCode key) const {
    if (key == KeyCode::None) return false;
    for (const auto& [mapKey, entry] : m_rebindings) {
        if (entry.contextId == contextId && entry.actionId != actionId && entry.key == key) {
            return true;
        }
    }
    return false;
}

bool RebindingStore::HasButtonConflict(std::string_view contextId,
                                        std::string_view actionId,
                                        GamepadButton button) const {
    if (button == GamepadButton::None) return false;
    for (const auto& [mapKey, entry] : m_rebindings) {
        if (entry.contextId == contextId && entry.actionId != actionId && entry.button == button) {
            return true;
        }
    }
    return false;
}

std::string RebindingStore::GetConflictingAction(std::string_view contextId, KeyCode key) const {
    if (key == KeyCode::None) return "";
    for (const auto& [mapKey, entry] : m_rebindings) {
        if (entry.contextId == contextId && entry.key == key) {
            return entry.actionId;
        }
    }
    return "";
}

std::string RebindingStore::GetConflictingAction(std::string_view contextId,
                                                  GamepadButton button) const {
    if (button == GamepadButton::None) return "";
    for (const auto& [mapKey, entry] : m_rebindings) {
        if (entry.contextId == contextId && entry.button == button) {
            return entry.actionId;
        }
    }
    return "";
}

void RebindingStore::ApplyToContext(InputContext& context) const {
    for (const auto& [mapKey, entry] : m_rebindings) {
        if (entry.contextId == context.GetId()) {
            if (entry.key != KeyCode::None) {
                context.SetKeyBinding(entry.actionId, entry.key);
            }
            if (entry.button != GamepadButton::None) {
                context.SetButtonBinding(entry.actionId, entry.button);
            }
        }
    }
}

void RebindingStore::ResetContext(std::string_view contextId) {
    std::erase_if(m_rebindings, [&](const auto& pair) {
        return pair.second.contextId == contextId;
    });
}

void RebindingStore::ResetAll() {
    m_rebindings.clear();
}

bool RebindingStore::HasRebinding(std::string_view contextId,
                                   std::string_view actionId) const {
    return m_rebindings.contains(MakeKey(contextId, actionId));
}

const RebindEntry* RebindingStore::GetRebinding(std::string_view contextId,
                                                  std::string_view actionId) const {
    auto it = m_rebindings.find(MakeKey(contextId, actionId));
    return it != m_rebindings.end() ? &it->second : nullptr;
}

size_t RebindingStore::RebindingCount() const { return m_rebindings.size(); }

// --- Serialization helpers ---

static void WriteU16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void WriteString(std::vector<uint8_t>& buf, const std::string& str) {
    WriteU16(buf, static_cast<uint16_t>(str.size()));
    buf.insert(buf.end(), str.begin(), str.end());
}

static bool ReadU16(std::span<const uint8_t>& data, uint16_t& out) {
    if (data.size() < 2) return false;
    out = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    data = data.subspan(2);
    return true;
}

static bool ReadString(std::span<const uint8_t>& data, std::string& out) {
    uint16_t len;
    if (!ReadU16(data, len)) return false;
    if (data.size() < len) return false;
    out.assign(reinterpret_cast<const char*>(data.data()), len);
    data = data.subspan(len);
    return true;
}

std::vector<uint8_t> RebindingStore::Serialize() const {
    std::vector<uint8_t> buf;
    WriteU16(buf, static_cast<uint16_t>(m_rebindings.size()));
    for (const auto& [mapKey, entry] : m_rebindings) {
        WriteString(buf, entry.contextId);
        WriteString(buf, entry.actionId);
        WriteU16(buf, static_cast<uint16_t>(entry.key));
        WriteU16(buf, static_cast<uint16_t>(entry.button));
    }
    return buf;
}

Result<void> RebindingStore::Deserialize(std::span<const uint8_t> data) {
    m_rebindings.clear();
    if (data.empty()) return {};

    uint16_t count;
    if (!ReadU16(data, count)) {
        return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read rebinding count"});
    }

    for (uint16_t i = 0; i < count; ++i) {
        RebindEntry entry;
        if (!ReadString(data, entry.contextId) ||
            !ReadString(data, entry.actionId)) {
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read rebinding entry"});
        }
        uint16_t keyVal, buttonVal;
        if (!ReadU16(data, keyVal) || !ReadU16(data, buttonVal)) {
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read rebinding values"});
        }
        entry.key = static_cast<KeyCode>(keyVal);
        entry.button = static_cast<GamepadButton>(buttonVal);
        m_rebindings[MakeKey(entry.contextId, entry.actionId)] = std::move(entry);
    }
    return {};
}

}  // namespace ohui::input
