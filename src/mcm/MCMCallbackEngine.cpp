#include "ohui/mcm/MCMCallbackEngine.h"

namespace ohui::mcm {

MCMCallbackEngine::MCMCallbackEngine(MCMRegistrationEngine& registration)
    : m_registration(registration) {}

// --- Private helpers ---

Result<MCMCallbackDescriptor> MCMCallbackEngine::MakeOptionDescriptor(
    MCMCallbackType type, std::string_view modName, int32_t optionId) {
    if (!m_registration.HasMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    if (!m_registration.GetOption(modName, optionId))
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});

    MCMCallbackDescriptor desc;
    desc.type = type;
    desc.modName = std::string(modName);
    desc.optionId = optionId;
    return desc;
}

// --- Interaction -> descriptor resolution ---

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveOptionSelect(
    std::string_view modName, int32_t optionId) {
    return MakeOptionDescriptor(MCMCallbackType::OnOptionSelect, modName, optionId);
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveOptionDefault(
    std::string_view modName, int32_t optionId) {
    return MakeOptionDescriptor(MCMCallbackType::OnOptionDefault, modName, optionId);
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveOptionHighlight(
    std::string_view modName, int32_t optionId) {
    return MakeOptionDescriptor(MCMCallbackType::OnOptionHighlight, modName, optionId);
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveSliderOpen(
    std::string_view modName, int32_t optionId) {
    return MakeOptionDescriptor(MCMCallbackType::OnOptionSliderOpen, modName, optionId);
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveSliderAccept(
    std::string_view modName, int32_t optionId, float value) {
    auto result = MakeOptionDescriptor(MCMCallbackType::OnOptionSliderAccept, modName, optionId);
    if (result.has_value())
        result.value().floatValue = value;
    return result;
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveMenuOpen(
    std::string_view modName, int32_t optionId) {
    return MakeOptionDescriptor(MCMCallbackType::OnOptionMenuOpen, modName, optionId);
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveMenuAccept(
    std::string_view modName, int32_t optionId, int32_t index) {
    auto result = MakeOptionDescriptor(MCMCallbackType::OnOptionMenuAccept, modName, optionId);
    if (result.has_value())
        result.value().intValue = index;
    return result;
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveColorOpen(
    std::string_view modName, int32_t optionId) {
    return MakeOptionDescriptor(MCMCallbackType::OnOptionColorOpen, modName, optionId);
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveColorAccept(
    std::string_view modName, int32_t optionId, int32_t color) {
    auto result = MakeOptionDescriptor(MCMCallbackType::OnOptionColorAccept, modName, optionId);
    if (result.has_value())
        result.value().intValue = color;
    return result;
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveKeyMapChange(
    std::string_view modName, int32_t optionId, int32_t keyCode) {
    auto result = MakeOptionDescriptor(MCMCallbackType::OnOptionKeyMapChange, modName, optionId);
    if (result.has_value())
        result.value().intValue = keyCode;
    return result;
}

// --- Lifecycle event resolution ---

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveConfigInit(
    std::string_view modName) {
    if (!m_registration.HasMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    MCMCallbackDescriptor desc;
    desc.type = MCMCallbackType::OnConfigInit;
    desc.modName = std::string(modName);
    return desc;
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveConfigOpen(
    std::string_view modName) {
    if (!m_registration.HasMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    MCMCallbackDescriptor desc;
    desc.type = MCMCallbackType::OnConfigOpen;
    desc.modName = std::string(modName);
    return desc;
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolveConfigClose(
    std::string_view modName) {
    if (!m_registration.HasMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    MCMCallbackDescriptor desc;
    desc.type = MCMCallbackType::OnConfigClose;
    desc.modName = std::string(modName);
    return desc;
}

Result<MCMCallbackDescriptor> MCMCallbackEngine::ResolvePageReset(
    std::string_view modName, int32_t pageIndex) {
    if (!m_registration.HasMod(modName))
        return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    if (!m_registration.GetPage(modName, pageIndex))
        return std::unexpected(Error{ErrorCode::PageNotFound, "Page not found"});

    MCMCallbackDescriptor desc;
    desc.type = MCMCallbackType::OnPageReset;
    desc.modName = std::string(modName);
    desc.pageIndex = pageIndex;
    return desc;
}

// --- Value change with history ---

Result<MCMValueChange> MCMCallbackEngine::ApplyToggleChange(
    std::string_view modName, int32_t optionId, bool newValue) {
    auto* opt = m_registration.GetOption(modName, optionId);
    if (!opt) {
        if (!m_registration.HasMod(modName))
            return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});
    }

    MCMValueChange change;
    change.modName = std::string(modName);
    change.optionId = optionId;
    change.optionType = opt->type;
    change.oldBool = opt->boolValue;
    change.newBool = newValue;

    auto result = m_registration.SetToggleOptionValue(modName, optionId, newValue);
    if (!result.has_value())
        return std::unexpected(result.error());

    m_changeHistory.push_back(change);
    return change;
}

Result<MCMValueChange> MCMCallbackEngine::ApplySliderChange(
    std::string_view modName, int32_t optionId, float newValue) {
    auto* opt = m_registration.GetOption(modName, optionId);
    if (!opt) {
        if (!m_registration.HasMod(modName))
            return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});
    }

    MCMValueChange change;
    change.modName = std::string(modName);
    change.optionId = optionId;
    change.optionType = opt->type;
    change.oldFloat = opt->floatValue;
    change.newFloat = newValue;

    auto result = m_registration.SetSliderOptionValue(modName, optionId, newValue);
    if (!result.has_value())
        return std::unexpected(result.error());

    m_changeHistory.push_back(change);
    return change;
}

Result<MCMValueChange> MCMCallbackEngine::ApplyMenuChange(
    std::string_view modName, int32_t optionId, int32_t newIndex) {
    auto* opt = m_registration.GetOption(modName, optionId);
    if (!opt) {
        if (!m_registration.HasMod(modName))
            return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});
    }

    MCMValueChange change;
    change.modName = std::string(modName);
    change.optionId = optionId;
    change.optionType = opt->type;
    change.oldInt = opt->intValue;
    change.newInt = newIndex;

    auto result = m_registration.SetMenuOptionValue(modName, optionId,
                                                     std::to_string(newIndex));
    if (!result.has_value())
        return std::unexpected(result.error());

    m_changeHistory.push_back(change);
    return change;
}

Result<MCMValueChange> MCMCallbackEngine::ApplyColorChange(
    std::string_view modName, int32_t optionId, int32_t newColor) {
    auto* opt = m_registration.GetOption(modName, optionId);
    if (!opt) {
        if (!m_registration.HasMod(modName))
            return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});
    }

    MCMValueChange change;
    change.modName = std::string(modName);
    change.optionId = optionId;
    change.optionType = opt->type;
    change.oldInt = opt->intValue;
    change.newInt = newColor;

    auto result = m_registration.SetColorOptionValue(modName, optionId, newColor);
    if (!result.has_value())
        return std::unexpected(result.error());

    m_changeHistory.push_back(change);
    return change;
}

Result<MCMValueChange> MCMCallbackEngine::ApplyKeyMapChange(
    std::string_view modName, int32_t optionId, int32_t newKeyCode) {
    auto* opt = m_registration.GetOption(modName, optionId);
    if (!opt) {
        if (!m_registration.HasMod(modName))
            return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});
        return std::unexpected(Error{ErrorCode::OptionNotFound, "Option not found"});
    }

    MCMValueChange change;
    change.modName = std::string(modName);
    change.optionId = optionId;
    change.optionType = opt->type;
    change.oldInt = opt->intValue;
    change.newInt = newKeyCode;

    auto result = m_registration.SetKeyMapOptionValue(modName, optionId, newKeyCode);
    if (!result.has_value())
        return std::unexpected(result.error());

    m_changeHistory.push_back(change);
    return change;
}

// --- Change history ---

const std::vector<MCMValueChange>& MCMCallbackEngine::GetChangeHistory() const {
    return m_changeHistory;
}

void MCMCallbackEngine::ClearChangeHistory() {
    m_changeHistory.clear();
}

}  // namespace ohui::mcm
