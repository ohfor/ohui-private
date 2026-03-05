#include "ohui/mcm/MCMListManager.h"
#include <algorithm>
#include <cstring>

namespace ohui::mcm {

MCMListManager::MCMListManager(MCMRegistrationEngine& registration)
    : m_registration(registration) {}

// --- Serialization helpers ---

static void WriteU8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

static void WriteU16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void WriteU64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
}

static void WriteString(std::vector<uint8_t>& buf, const std::string& str) {
    WriteU16(buf, static_cast<uint16_t>(str.size()));
    buf.insert(buf.end(), str.begin(), str.end());
}

static bool ReadU8(std::span<const uint8_t>& data, uint8_t& out) {
    if (data.empty()) return false;
    out = data[0];
    data = data.subspan(1);
    return true;
}

static bool ReadU16(std::span<const uint8_t>& data, uint16_t& out) {
    if (data.size() < 2) return false;
    out = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    data = data.subspan(2);
    return true;
}

static bool ReadU64(std::span<const uint8_t>& data, uint64_t& out) {
    if (data.size() < 8) return false;
    out = 0;
    for (int i = 0; i < 8; ++i) {
        out |= static_cast<uint64_t>(data[i]) << (i * 8);
    }
    data = data.subspan(8);
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

// --- Visibility ---

Result<void> MCMListManager::SetHidden(std::string_view modName, bool hidden) {
    auto it = m_entries.find(std::string(modName));
    if (it == m_entries.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not found in list"});
    it->second.hidden = hidden;
    return {};
}

bool MCMListManager::IsHidden(std::string_view modName) const {
    auto it = m_entries.find(std::string(modName));
    return it != m_entries.end() && it->second.hidden;
}

// --- Rename ---

Result<void> MCMListManager::SetDisplayName(std::string_view modName,
                                             std::string_view displayName) {
    auto it = m_entries.find(std::string(modName));
    if (it == m_entries.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not found in list"});
    it->second.displayName = std::string(displayName);
    return {};
}

Result<void> MCMListManager::ClearDisplayName(std::string_view modName) {
    auto it = m_entries.find(std::string(modName));
    if (it == m_entries.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not found in list"});
    it->second.displayName = it->second.modName;
    return {};
}

std::string MCMListManager::GetDisplayName(std::string_view modName) const {
    auto it = m_entries.find(std::string(modName));
    if (it == m_entries.end())
        return std::string(modName);
    return it->second.displayName;
}

// --- Reorder ---

Result<void> MCMListManager::SetManualOrder(std::string_view modName, int32_t position) {
    auto it = m_entries.find(std::string(modName));
    if (it == m_entries.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not found in list"});
    it->second.manualOrder = position;
    return {};
}

Result<void> MCMListManager::SwapOrder(std::string_view modA, std::string_view modB) {
    auto itA = m_entries.find(std::string(modA));
    auto itB = m_entries.find(std::string(modB));
    if (itA == m_entries.end() || itB == m_entries.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not found in list"});

    std::swap(itA->second.manualOrder, itB->second.manualOrder);
    return {};
}

Result<void> MCMListManager::MoveToPosition(std::string_view modName, int32_t position) {
    auto it = m_entries.find(std::string(modName));
    if (it == m_entries.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not found in list"});

    int32_t oldPosition = it->second.manualOrder;

    if (position == oldPosition)
        return {};

    // Shift entries between old and new positions
    for (auto& [key, entry] : m_entries) {
        if (key == modName) continue;

        if (oldPosition < position) {
            // Moving down: shift entries in (old, position] up by 1
            if (entry.manualOrder > oldPosition && entry.manualOrder <= position) {
                entry.manualOrder -= 1;
            }
        } else {
            // Moving up: shift entries in [position, old) down by 1
            if (entry.manualOrder >= position && entry.manualOrder < oldPosition) {
                entry.manualOrder += 1;
            }
        }
    }

    it->second.manualOrder = position;
    return {};
}

// --- Sort ---

void MCMListManager::SetSortMode(MCMSortMode mode) {
    m_sortMode = mode;
}

MCMSortMode MCMListManager::GetSortMode() const {
    return m_sortMode;
}

// --- Timestamp ---

void MCMListManager::RecordOpen(std::string_view modName) {
    auto it = m_entries.find(std::string(modName));
    if (it != m_entries.end()) {
        // Use a monotonic counter approach - find max and add 1
        uint64_t maxTimestamp = 0;
        for (const auto& [_, entry] : m_entries) {
            maxTimestamp = std::max(maxTimestamp, entry.lastOpenedTimestamp);
        }
        it->second.lastOpenedTimestamp = maxTimestamp + 1;
    }
}

// --- Sorted listing ---

std::vector<MCMListEntry> MCMListManager::GetSortedList(bool includeHidden) const {
    std::vector<MCMListEntry> result;
    result.reserve(m_entries.size());

    for (const auto& [_, entry] : m_entries) {
        if (!includeHidden && entry.hidden) continue;
        result.push_back(entry);
    }

    switch (m_sortMode) {
    case MCMSortMode::Alphabetical:
        std::sort(result.begin(), result.end(), [](const MCMListEntry& a, const MCMListEntry& b) {
            return a.displayName < b.displayName;
        });
        break;
    case MCMSortMode::Manual:
        std::sort(result.begin(), result.end(), [](const MCMListEntry& a, const MCMListEntry& b) {
            return a.manualOrder < b.manualOrder;
        });
        break;
    case MCMSortMode::MostRecentlyUsed:
        std::sort(result.begin(), result.end(), [](const MCMListEntry& a, const MCMListEntry& b) {
            return a.lastOpenedTimestamp > b.lastOpenedTimestamp;
        });
        break;
    }

    return result;
}

// --- Cosave serialization ---

std::vector<uint8_t> MCMListManager::Serialize() const {
    std::vector<uint8_t> buf;
    WriteU16(buf, static_cast<uint16_t>(m_entries.size()));
    WriteU8(buf, static_cast<uint8_t>(m_sortMode));

    for (const auto& [_, entry] : m_entries) {
        WriteString(buf, entry.modName);
        WriteString(buf, entry.displayName);
        WriteU8(buf, entry.hidden ? 1 : 0);
        WriteU16(buf, static_cast<uint16_t>(entry.manualOrder));
        WriteU64(buf, entry.lastOpenedTimestamp);
    }
    return buf;
}

Result<void> MCMListManager::Deserialize(std::span<const uint8_t> data) {
    m_entries.clear();
    m_sortMode = MCMSortMode::Alphabetical;
    if (data.empty()) return {};

    uint16_t count;
    if (!ReadU16(data, count))
        return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read entry count"});

    uint8_t sortModeVal;
    if (!ReadU8(data, sortModeVal))
        return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read sort mode"});
    m_sortMode = static_cast<MCMSortMode>(sortModeVal);

    for (uint16_t i = 0; i < count; ++i) {
        MCMListEntry entry;
        if (!ReadString(data, entry.modName) || !ReadString(data, entry.displayName))
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read entry strings"});

        uint8_t hiddenVal;
        if (!ReadU8(data, hiddenVal))
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read hidden flag"});
        entry.hidden = (hiddenVal != 0);

        uint16_t orderVal;
        if (!ReadU16(data, orderVal))
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read manual order"});
        entry.manualOrder = static_cast<int32_t>(orderVal);

        if (!ReadU64(data, entry.lastOpenedTimestamp))
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Failed to read timestamp"});

        m_entries[entry.modName] = std::move(entry);
    }
    return {};
}

// --- Query ---

size_t MCMListManager::EntryCount() const {
    return m_entries.size();
}

bool MCMListManager::HasEntry(std::string_view modName) const {
    return m_entries.contains(std::string(modName));
}

const MCMListEntry* MCMListManager::GetEntry(std::string_view modName) const {
    auto it = m_entries.find(std::string(modName));
    return it != m_entries.end() ? &it->second : nullptr;
}

// --- Sync ---

void MCMListManager::SyncWithRegistry() {
    auto modNames = m_registration.GetModNames();

    // Remove entries for unregistered mods
    std::erase_if(m_entries, [&](const auto& pair) {
        return !m_registration.HasMod(pair.first);
    });

    // Add entries for newly registered mods
    int32_t nextOrder = 0;
    if (!m_entries.empty()) {
        for (const auto& [_, entry] : m_entries) {
            nextOrder = std::max(nextOrder, entry.manualOrder + 1);
        }
    }

    for (const auto& name : modNames) {
        if (!m_entries.contains(name)) {
            MCMListEntry entry;
            entry.modName = name;
            entry.displayName = name;
            entry.manualOrder = nextOrder++;
            m_entries[name] = std::move(entry);
        }
    }
}

}  // namespace ohui::mcm
