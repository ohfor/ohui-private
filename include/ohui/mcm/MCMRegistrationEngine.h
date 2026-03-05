#pragma once

#include "ohui/mcm/MCMTypes.h"
#include "ohui/core/Result.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::mcm {

class MCMRegistrationEngine {
public:
    // --- Registration protocol ---
    Result<void> RegisterMod(std::string_view modName);
    Result<void> UnregisterMod(std::string_view modName);
    Result<int32_t> AddPage(std::string_view modName, std::string_view pageName);

    // --- Page build protocol ---
    Result<void> BeginPageBuild(std::string_view modName, int32_t pageIndex);
    Result<void> EndPageBuild(std::string_view modName);

    // --- Option registration (only valid between Begin/EndPageBuild) ---
    Result<int32_t> AddToggleOption(std::string_view modName, std::string_view name,
                                     bool value, MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddSliderOption(std::string_view modName, std::string_view name,
                                     float value, std::string_view formatString = "",
                                     MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddMenuOption(std::string_view modName, std::string_view name,
                                   std::string_view value,
                                   MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddTextOption(std::string_view modName, std::string_view name,
                                   std::string_view value,
                                   MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddColorOption(std::string_view modName, std::string_view name,
                                    int32_t color,
                                    MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddKeyMapOption(std::string_view modName, std::string_view name,
                                     int32_t keyCode,
                                     MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddHeaderOption(std::string_view modName, std::string_view name,
                                     MCMOptionFlag flags = MCMOptionFlag::None);
    Result<int32_t> AddEmptyOption(std::string_view modName,
                                    MCMOptionFlag flags = MCMOptionFlag::None);

    // --- Value setters (valid any time after registration, not during build) ---
    Result<void> SetToggleOptionValue(std::string_view modName, int32_t optionId,
                                       bool value);
    Result<void> SetSliderOptionValue(std::string_view modName, int32_t optionId,
                                       float value, std::string_view formatString = "");
    Result<void> SetMenuOptionValue(std::string_view modName, int32_t optionId,
                                     std::string_view value);
    Result<void> SetTextOptionValue(std::string_view modName, int32_t optionId,
                                     std::string_view value);
    Result<void> SetColorOptionValue(std::string_view modName, int32_t optionId,
                                      int32_t color);
    Result<void> SetKeyMapOptionValue(std::string_view modName, int32_t optionId,
                                       int32_t keyCode);
    Result<void> SetOptionFlags(std::string_view modName, int32_t optionId,
                                 MCMOptionFlag flags);

    // --- Dialog transient state ---
    Result<void> SetSliderDialogParams(std::string_view modName,
                                        const SliderDialogParams& params);
    Result<void> SetMenuDialogParams(std::string_view modName,
                                      const MenuDialogParams& params);
    Result<void> SetInfoText(std::string_view modName, std::string_view text);

    const SliderDialogParams* GetSliderDialogParams(std::string_view modName) const;
    const MenuDialogParams* GetMenuDialogParams(std::string_view modName) const;
    const InfoTextData* GetInfoText(std::string_view modName) const;

    // --- ForcePageReset ---
    Result<void> ForcePageReset(std::string_view modName, int32_t pageIndex);

    // --- Query API ---
    bool HasMod(std::string_view modName) const;
    const MCMModData* GetMod(std::string_view modName) const;
    const MCMPageData* GetPage(std::string_view modName, int32_t pageIndex) const;
    const MCMOptionData* GetOption(std::string_view modName, int32_t optionId) const;
    std::vector<std::string> GetModNames() const;
    size_t ModCount() const;

private:
    MCMModData* FindMod(std::string_view modName);
    const MCMModData* FindMod(std::string_view modName) const;
    MCMOptionData* FindOption(std::string_view modName, int32_t optionId);
    Result<int32_t> AddOption(std::string_view modName, MCMOptionData option);

    std::unordered_map<std::string, MCMModData> m_mods;
    std::unordered_map<std::string, SliderDialogParams> m_sliderDialogs;
    std::unordered_map<std::string, MenuDialogParams> m_menuDialogs;
    std::unordered_map<std::string, InfoTextData> m_infoTexts;
};

}  // namespace ohui::mcm
