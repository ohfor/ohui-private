#include "ohui/mcm/MCMRegistrationEngine.h"
#include <algorithm>

namespace ohui::mcm {

// --- Private helpers ---

MCMModData* MCMRegistrationEngine::FindMod(std::string_view modName) {
    auto it = m_mods.find(std::string(modName));
    return it != m_mods.end() ? &it->second : nullptr;
}

const MCMModData* MCMRegistrationEngine::FindMod(std::string_view modName) const {
    auto it = m_mods.find(std::string(modName));
    return it != m_mods.end() ? &it->second : nullptr;
}

MCMOptionData* MCMRegistrationEngine::FindOption(std::string_view modName, int32_t optionId) {
    auto* mod = FindMod(modName);
    if (!mod) return nullptr;

    int32_t pageIdx = DecodePageIndex(optionId);
    int32_t optIdx = DecodeOptionIndex(optionId);

    if (pageIdx < 0 || pageIdx >= static_cast<int32_t>(mod->pages.size()))
        return nullptr;

    auto& page = mod->pages[pageIdx];
    if (optIdx < 0 || optIdx >= static_cast<int32_t>(page.options.size()))
        return nullptr;

    return &page.options[optIdx];
}

Result<int32_t> MCMRegistrationEngine::AddOption(std::string_view modName, MCMOptionData option) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    if (mod->state != MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Not in page build mode"});

    auto& page = mod->pages[mod->activeBuildPageIndex];
    if (static_cast<int32_t>(page.options.size()) >= kMaxOptionsPerPage)
        return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Max options per page exceeded"});

    int32_t optionIndex = static_cast<int32_t>(page.options.size());
    int32_t encodedId = EncodeOptionId(mod->activeBuildPageIndex, optionIndex);
    option.id = encodedId;

    page.options.push_back(std::move(option));
    return encodedId;
}

// --- Registration protocol ---

Result<void> MCMRegistrationEngine::RegisterMod(std::string_view modName) {
    std::string key(modName);
    if (m_mods.contains(key))
        return std::unexpected(Error{ErrorCode::DuplicateRegistration, "Mod already registered"});

    MCMModData mod;
    mod.modName = key;
    m_mods[key] = std::move(mod);
    return {};
}

Result<void> MCMRegistrationEngine::UnregisterMod(std::string_view modName) {
    std::string key(modName);
    auto it = m_mods.find(key);
    if (it == m_mods.end())
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    m_mods.erase(it);
    m_sliderDialogs.erase(key);
    m_menuDialogs.erase(key);
    m_infoTexts.erase(key);
    return {};
}

Result<int32_t> MCMRegistrationEngine::AddPage(std::string_view modName, std::string_view pageName) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    MCMPageData page;
    page.name = std::string(pageName);
    page.pageIndex = static_cast<int32_t>(mod->pages.size());
    mod->pages.push_back(std::move(page));
    return mod->pages.back().pageIndex;
}

// --- Page build protocol ---

Result<void> MCMRegistrationEngine::BeginPageBuild(std::string_view modName, int32_t pageIndex) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Already in page build mode"});

    if (pageIndex < 0 || pageIndex >= static_cast<int32_t>(mod->pages.size()))
        return std::unexpected(Error{ErrorCode::PageNotFound, "Page index out of range"});

    mod->state = MCMModState::Building;
    mod->activeBuildPageIndex = pageIndex;
    mod->pages[pageIndex].options.clear();
    return {};
}

Result<void> MCMRegistrationEngine::EndPageBuild(std::string_view modName) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    if (mod->state != MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Not in page build mode"});

    mod->state = MCMModState::Ready;
    mod->activeBuildPageIndex = -1;
    return {};
}

// --- Option registration ---

Result<int32_t> MCMRegistrationEngine::AddToggleOption(std::string_view modName,
                                                        std::string_view name,
                                                        bool value,
                                                        MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Toggle;
    opt.name = std::string(name);
    opt.boolValue = value;
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddSliderOption(std::string_view modName,
                                                        std::string_view name,
                                                        float value,
                                                        std::string_view formatString,
                                                        MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Slider;
    opt.name = std::string(name);
    opt.floatValue = value;
    opt.formatString = std::string(formatString);
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddMenuOption(std::string_view modName,
                                                      std::string_view name,
                                                      std::string_view value,
                                                      MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Menu;
    opt.name = std::string(name);
    opt.stringValue = std::string(value);
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddTextOption(std::string_view modName,
                                                      std::string_view name,
                                                      std::string_view value,
                                                      MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Text;
    opt.name = std::string(name);
    opt.stringValue = std::string(value);
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddColorOption(std::string_view modName,
                                                       std::string_view name,
                                                       int32_t color,
                                                       MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Color;
    opt.name = std::string(name);
    opt.intValue = color;
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddKeyMapOption(std::string_view modName,
                                                        std::string_view name,
                                                        int32_t keyCode,
                                                        MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::KeyMap;
    opt.name = std::string(name);
    opt.intValue = keyCode;
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddHeaderOption(std::string_view modName,
                                                        std::string_view name,
                                                        MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Header;
    opt.name = std::string(name);
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

Result<int32_t> MCMRegistrationEngine::AddEmptyOption(std::string_view modName,
                                                       MCMOptionFlag flags) {
    MCMOptionData opt;
    opt.type = MCMOptionType::Empty;
    opt.flags = flags;
    return AddOption(modName, std::move(opt));
}

// --- Value setters ---

Result<void> MCMRegistrationEngine::SetToggleOptionValue(std::string_view modName,
                                                          int32_t optionId,
                                                          bool value) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->boolValue = value;
    return {};
}

Result<void> MCMRegistrationEngine::SetSliderOptionValue(std::string_view modName,
                                                          int32_t optionId,
                                                          float value,
                                                          std::string_view formatString) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->floatValue = value;
    if (!formatString.empty())
        opt->formatString = std::string(formatString);
    return {};
}

Result<void> MCMRegistrationEngine::SetMenuOptionValue(std::string_view modName,
                                                        int32_t optionId,
                                                        std::string_view value) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->stringValue = std::string(value);
    return {};
}

Result<void> MCMRegistrationEngine::SetTextOptionValue(std::string_view modName,
                                                        int32_t optionId,
                                                        std::string_view value) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->stringValue = std::string(value);
    return {};
}

Result<void> MCMRegistrationEngine::SetColorOptionValue(std::string_view modName,
                                                         int32_t optionId,
                                                         int32_t color) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->intValue = color;
    return {};
}

Result<void> MCMRegistrationEngine::SetKeyMapOptionValue(std::string_view modName,
                                                          int32_t optionId,
                                                          int32_t keyCode) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->intValue = keyCode;
    return {};
}

Result<void> MCMRegistrationEngine::SetOptionFlags(std::string_view modName,
                                                    int32_t optionId,
                                                    MCMOptionFlag flags) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    if (mod->state == MCMModState::Building)
        return std::unexpected(Error{ErrorCode::InvalidState, "Cannot set values during build"});

    auto* opt = FindOption(modName, optionId);
    if (!opt)
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    opt->flags = flags;
    return {};
}

// --- Dialog transient state ---

Result<void> MCMRegistrationEngine::SetSliderDialogParams(std::string_view modName,
                                                           const SliderDialogParams& params) {
    if (!FindMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    m_sliderDialogs[std::string(modName)] = params;
    return {};
}

Result<void> MCMRegistrationEngine::SetMenuDialogParams(std::string_view modName,
                                                         const MenuDialogParams& params) {
    if (!FindMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    m_menuDialogs[std::string(modName)] = params;
    return {};
}

Result<void> MCMRegistrationEngine::SetInfoText(std::string_view modName,
                                                 std::string_view text) {
    if (!FindMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
    m_infoTexts[std::string(modName)] = InfoTextData{std::string(text)};
    return {};
}

const SliderDialogParams* MCMRegistrationEngine::GetSliderDialogParams(
    std::string_view modName) const {
    auto it = m_sliderDialogs.find(std::string(modName));
    return it != m_sliderDialogs.end() ? &it->second : nullptr;
}

const MenuDialogParams* MCMRegistrationEngine::GetMenuDialogParams(
    std::string_view modName) const {
    auto it = m_menuDialogs.find(std::string(modName));
    return it != m_menuDialogs.end() ? &it->second : nullptr;
}

const InfoTextData* MCMRegistrationEngine::GetInfoText(
    std::string_view modName) const {
    auto it = m_infoTexts.find(std::string(modName));
    return it != m_infoTexts.end() ? &it->second : nullptr;
}

// --- ForcePageReset ---

Result<void> MCMRegistrationEngine::ForcePageReset(std::string_view modName,
                                                    int32_t pageIndex) {
    auto* mod = FindMod(modName);
    if (!mod)
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    if (pageIndex < 0 || pageIndex >= static_cast<int32_t>(mod->pages.size()))
        return std::unexpected(Error{ErrorCode::PageNotFound, "Page index out of range"});

    mod->pages[pageIndex].options.clear();
    mod->state = MCMModState::Registered;
    mod->activeBuildPageIndex = -1;
    return {};
}

// --- Query API ---

bool MCMRegistrationEngine::HasMod(std::string_view modName) const {
    return m_mods.contains(std::string(modName));
}

const MCMModData* MCMRegistrationEngine::GetMod(std::string_view modName) const {
    return FindMod(modName);
}

const MCMPageData* MCMRegistrationEngine::GetPage(std::string_view modName,
                                                   int32_t pageIndex) const {
    auto* mod = FindMod(modName);
    if (!mod) return nullptr;
    if (pageIndex < 0 || pageIndex >= static_cast<int32_t>(mod->pages.size()))
        return nullptr;
    return &mod->pages[pageIndex];
}

const MCMOptionData* MCMRegistrationEngine::GetOption(std::string_view modName,
                                                       int32_t optionId) const {
    auto* mod = FindMod(modName);
    if (!mod) return nullptr;

    int32_t pageIdx = DecodePageIndex(optionId);
    int32_t optIdx = DecodeOptionIndex(optionId);

    if (pageIdx < 0 || pageIdx >= static_cast<int32_t>(mod->pages.size()))
        return nullptr;

    const auto& page = mod->pages[pageIdx];
    if (optIdx < 0 || optIdx >= static_cast<int32_t>(page.options.size()))
        return nullptr;

    return &page.options[optIdx];
}

std::vector<std::string> MCMRegistrationEngine::GetModNames() const {
    std::vector<std::string> names;
    names.reserve(m_mods.size());
    for (const auto& [key, _] : m_mods) {
        names.push_back(key);
    }
    std::sort(names.begin(), names.end());
    return names;
}

size_t MCMRegistrationEngine::ModCount() const {
    return m_mods.size();
}

}  // namespace ohui::mcm
