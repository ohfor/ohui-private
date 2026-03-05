#include "ohui/widget/WidgetRegistry.h"
#include "ohui/core/Log.h"

#include <algorithm>

namespace ohui::widget {

Result<void> WidgetRegistry::Register(const WidgetManifest& manifest) {
    if (m_widgets.contains(manifest.id)) {
        ohui::log::debug("WidgetRegistry: duplicate registration for '{}'", manifest.id);
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Widget already registered: " + manifest.id});
    }

    WidgetState state;
    state.id = manifest.id;
    state.position = manifest.defaultPosition;
    state.size = manifest.defaultSize;
    state.visible = manifest.defaultVisible;
    state.active = false;

    m_widgets[manifest.id] = WidgetEntry{manifest, state};
    return {};
}

Result<void> WidgetRegistry::Unregister(std::string_view id) {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }
    m_widgets.erase(it);
    return {};
}

Result<void> WidgetRegistry::Activate(std::string_view id) {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }
    it->second.state.active = true;
    return {};
}

Result<void> WidgetRegistry::Deactivate(std::string_view id) {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }
    it->second.state.active = false;
    return {};
}

Result<void> WidgetRegistry::Move(std::string_view id, Vec2 position) {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }
    it->second.state.position = position;
    return {};
}

Result<void> WidgetRegistry::Resize(std::string_view id, Vec2 size) {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }

    const auto& manifest = it->second.manifest;

    // Clamp to minimum
    if (manifest.minSize.x > 0.0f) size.x = std::max(size.x, manifest.minSize.x);
    if (manifest.minSize.y > 0.0f) size.y = std::max(size.y, manifest.minSize.y);

    // Clamp to maximum (0 = unconstrained)
    if (manifest.maxSize.x > 0.0f) size.x = std::min(size.x, manifest.maxSize.x);
    if (manifest.maxSize.y > 0.0f) size.y = std::min(size.y, manifest.maxSize.y);

    it->second.state.size = size;
    return {};
}

Result<void> WidgetRegistry::SetVisible(std::string_view id, bool visible) {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }
    it->second.state.visible = visible;
    return {};
}

const WidgetState* WidgetRegistry::GetWidgetState(std::string_view id) const {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) return nullptr;
    return &it->second.state;
}

const WidgetManifest* WidgetRegistry::GetManifest(std::string_view id) const {
    auto it = m_widgets.find(std::string(id));
    if (it == m_widgets.end()) return nullptr;
    return &it->second.manifest;
}

std::vector<const WidgetState*> WidgetRegistry::GetAllWidgets() const {
    std::vector<const WidgetState*> result;
    result.reserve(m_widgets.size());
    for (const auto& [id, entry] : m_widgets) {
        result.push_back(&entry.state);
    }
    return result;
}

std::vector<const WidgetState*> WidgetRegistry::GetActiveWidgets() const {
    std::vector<const WidgetState*> result;
    for (const auto& [id, entry] : m_widgets) {
        if (entry.state.active) {
            result.push_back(&entry.state);
        }
    }
    return result;
}

size_t WidgetRegistry::WidgetCount() const {
    return m_widgets.size();
}

bool WidgetRegistry::HasWidget(std::string_view id) const {
    return m_widgets.contains(std::string(id));
}

}  // namespace ohui::widget
