#pragma once

#include "ohui/mcm/MCMRegistrationEngine.h"
#include "ohui/core/Result.h"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::mcm {

enum class MCMSortMode : uint8_t {
    Manual,
    Alphabetical,
    MostRecentlyUsed,
};

struct MCMListEntry {
    std::string modName;
    std::string displayName;
    bool hidden{false};
    int32_t manualOrder{0};
    uint64_t lastOpenedTimestamp{0};
};

class MCMListManager {
public:
    explicit MCMListManager(MCMRegistrationEngine& registration);

    // --- Visibility ---
    Result<void> SetHidden(std::string_view modName, bool hidden);
    bool IsHidden(std::string_view modName) const;

    // --- Rename ---
    Result<void> SetDisplayName(std::string_view modName, std::string_view displayName);
    Result<void> ClearDisplayName(std::string_view modName);
    std::string GetDisplayName(std::string_view modName) const;

    // --- Reorder ---
    Result<void> SetManualOrder(std::string_view modName, int32_t position);
    Result<void> SwapOrder(std::string_view modA, std::string_view modB);
    Result<void> MoveToPosition(std::string_view modName, int32_t position);

    // --- Sort ---
    void SetSortMode(MCMSortMode mode);
    MCMSortMode GetSortMode() const;

    // --- Timestamp (MRU) ---
    void RecordOpen(std::string_view modName);

    // --- Sorted listing ---
    std::vector<MCMListEntry> GetSortedList(bool includeHidden = false) const;

    // --- Cosave serialization (BlockType::MCMListConfig = 0x0006) ---
    std::vector<uint8_t> Serialize() const;
    Result<void> Deserialize(std::span<const uint8_t> data);

    // --- Query ---
    size_t EntryCount() const;
    bool HasEntry(std::string_view modName) const;
    const MCMListEntry* GetEntry(std::string_view modName) const;

    // --- Sync with registration engine ---
    void SyncWithRegistry();

private:
    MCMRegistrationEngine& m_registration;
    std::unordered_map<std::string, MCMListEntry> m_entries;
    MCMSortMode m_sortMode{MCMSortMode::Alphabetical};
};

}  // namespace ohui::mcm
