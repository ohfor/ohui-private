#pragma once

#include "ohui/mcm/MCMTypes.h"
#include "ohui/mcm/MCMRegistrationEngine.h"
#include "ohui/core/Result.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ohui::mcm {

enum class MCMCallbackType {
    // Option interaction
    OnOptionSelect, OnOptionDefault, OnOptionHighlight,
    OnOptionSliderOpen, OnOptionSliderAccept,
    OnOptionMenuOpen, OnOptionMenuAccept,
    OnOptionColorOpen, OnOptionColorAccept,
    OnOptionKeyMapChange,
    // Lifecycle
    OnConfigInit, OnConfigOpen, OnConfigClose, OnPageReset,
};

struct MCMCallbackDescriptor {
    MCMCallbackType type;
    std::string modName;
    int32_t optionId{-1};
    int32_t pageIndex{-1};

    // Payload for accept callbacks
    float floatValue{0.0f};
    int32_t intValue{0};
};

struct MCMValueChange {
    std::string modName;
    int32_t optionId;
    MCMOptionType optionType;

    bool oldBool{false};         bool newBool{false};
    float oldFloat{0.0f};        float newFloat{0.0f};
    int32_t oldInt{0};           int32_t newInt{0};
    std::string oldString;       std::string newString;
};

class MCMCallbackEngine {
public:
    explicit MCMCallbackEngine(MCMRegistrationEngine& registration);

    // --- Interaction -> descriptor resolution ---
    Result<MCMCallbackDescriptor> ResolveOptionSelect(std::string_view modName,
                                                       int32_t optionId);
    Result<MCMCallbackDescriptor> ResolveOptionDefault(std::string_view modName,
                                                        int32_t optionId);
    Result<MCMCallbackDescriptor> ResolveOptionHighlight(std::string_view modName,
                                                          int32_t optionId);
    Result<MCMCallbackDescriptor> ResolveSliderOpen(std::string_view modName,
                                                     int32_t optionId);
    Result<MCMCallbackDescriptor> ResolveSliderAccept(std::string_view modName,
                                                       int32_t optionId, float value);
    Result<MCMCallbackDescriptor> ResolveMenuOpen(std::string_view modName,
                                                    int32_t optionId);
    Result<MCMCallbackDescriptor> ResolveMenuAccept(std::string_view modName,
                                                      int32_t optionId, int32_t index);
    Result<MCMCallbackDescriptor> ResolveColorOpen(std::string_view modName,
                                                     int32_t optionId);
    Result<MCMCallbackDescriptor> ResolveColorAccept(std::string_view modName,
                                                       int32_t optionId, int32_t color);
    Result<MCMCallbackDescriptor> ResolveKeyMapChange(std::string_view modName,
                                                        int32_t optionId, int32_t keyCode);

    // --- Lifecycle event resolution ---
    Result<MCMCallbackDescriptor> ResolveConfigInit(std::string_view modName);
    Result<MCMCallbackDescriptor> ResolveConfigOpen(std::string_view modName);
    Result<MCMCallbackDescriptor> ResolveConfigClose(std::string_view modName);
    Result<MCMCallbackDescriptor> ResolvePageReset(std::string_view modName,
                                                     int32_t pageIndex);

    // --- Value change with history ---
    Result<MCMValueChange> ApplyToggleChange(std::string_view modName,
                                              int32_t optionId, bool newValue);
    Result<MCMValueChange> ApplySliderChange(std::string_view modName,
                                              int32_t optionId, float newValue);
    Result<MCMValueChange> ApplyMenuChange(std::string_view modName,
                                            int32_t optionId, int32_t newIndex);
    Result<MCMValueChange> ApplyColorChange(std::string_view modName,
                                             int32_t optionId, int32_t newColor);
    Result<MCMValueChange> ApplyKeyMapChange(std::string_view modName,
                                              int32_t optionId, int32_t newKeyCode);

    // --- Change history ---
    const std::vector<MCMValueChange>& GetChangeHistory() const;
    void ClearChangeHistory();

private:
    MCMRegistrationEngine& m_registration;
    std::vector<MCMValueChange> m_changeHistory;

    Result<MCMCallbackDescriptor> MakeOptionDescriptor(MCMCallbackType type,
                                                        std::string_view modName,
                                                        int32_t optionId);
};

}  // namespace ohui::mcm
