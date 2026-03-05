#include "ohui/widget/ModWidgetAPI.h"
#include "ohui/core/Log.h"

namespace ohui::widget {

ModWidgetAPI::ModWidgetAPI(WidgetRegistry& registry)
    : m_registry(registry) {}

void ModWidgetAPI::OpenRegistrationWindow() {
    m_windowState = RegistrationWindow::Open;
}

void ModWidgetAPI::LockRegistrationWindow() {
    m_windowState = RegistrationWindow::Locked;
}

RegistrationWindow ModWidgetAPI::GetRegistrationWindowState() const {
    return m_windowState;
}

Result<void> ModWidgetAPI::RegisterWidget(const ModWidgetManifest& manifest) {
    if (m_windowState != RegistrationWindow::Open) {
        return std::unexpected(Error{ErrorCode::RegistrationWindowClosed,
            "Registration window is not open"});
    }

    if (manifest.modId.empty()) {
        return std::unexpected(Error{ErrorCode::InvalidModId,
            "Mod ID cannot be empty"});
    }

    // Supersede semantics: if widget already exists, unregister the old one
    if (m_modWidgets.contains(manifest.widget.id)) {
        ohui::log::debug("ModWidgetAPI: superseding widget '{}' from mod '{}'",
                         manifest.widget.id, manifest.modId);
        (void)m_registry.Unregister(manifest.widget.id);
        m_modWidgets.erase(manifest.widget.id);
    }

    auto result = m_registry.Register(manifest.widget);
    if (!result.has_value()) {
        return result;
    }

    ModWidgetEntry entry;
    entry.modId = manifest.modId;
    entry.renderFn = manifest.renderFn;
    m_modWidgets[manifest.widget.id] = std::move(entry);

    return {};
}

Result<void> ModWidgetAPI::UnregisterWidget(std::string_view widgetId) {
    auto it = m_modWidgets.find(std::string(widgetId));
    if (it == m_modWidgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Mod widget not found: " + std::string(widgetId)});
    }

    (void)m_registry.Unregister(widgetId);
    m_modWidgets.erase(it);
    return {};
}

bool ModWidgetAPI::IsModWidget(std::string_view widgetId) const {
    return m_modWidgets.contains(std::string(widgetId));
}

std::string_view ModWidgetAPI::GetOwnerMod(std::string_view widgetId) const {
    auto it = m_modWidgets.find(std::string(widgetId));
    if (it == m_modWidgets.end()) return {};
    return it->second.modId;
}

std::vector<std::string> ModWidgetAPI::GetWidgetsByMod(std::string_view modId) const {
    std::vector<std::string> result;
    for (const auto& [id, entry] : m_modWidgets) {
        if (entry.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

size_t ModWidgetAPI::ModWidgetCount() const {
    return m_modWidgets.size();
}

const WidgetRenderCallback* ModWidgetAPI::GetRenderCallback(std::string_view widgetId) const {
    auto it = m_modWidgets.find(std::string(widgetId));
    if (it == m_modWidgets.end()) return nullptr;
    return &it->second.renderFn;
}

}  // namespace ohui::widget
