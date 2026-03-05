#include "ohui/widget/ViewportManager.h"
#include "ohui/core/Log.h"

namespace ohui::widget {

ViewportManager::ViewportManager(WidgetRegistry& registry)
    : m_registry(registry) {}

Result<void> ViewportManager::Register(IViewportWidget* widget) {
    const auto& manifest = widget->GetManifest();

    if (m_viewports.contains(manifest.id)) {
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Viewport already registered: " + manifest.id});
    }

    auto regResult = m_registry.Register(manifest);
    if (!regResult.has_value()) {
        return std::unexpected(regResult.error());
    }

    ViewportRect initialRect;
    initialRect.left = manifest.defaultPosition.x;
    initialRect.top = manifest.defaultPosition.y;
    initialRect.width = manifest.defaultSize.x;
    initialRect.height = manifest.defaultSize.y;

    m_viewports[manifest.id] = ViewportEntry{widget, initialRect, manifest.defaultVisible};
    return {};
}

Result<void> ViewportManager::Unregister(std::string_view widgetId) {
    auto it = m_viewports.find(std::string(widgetId));
    if (it == m_viewports.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Viewport not found: " + std::string(widgetId)});
    }

    m_viewports.erase(it);
    return {};
}

void ViewportManager::UpdateAll() {
    for (auto& [id, entry] : m_viewports) {
        const auto* state = m_registry.GetWidgetState(id);
        if (!state || !state->active || !state->visible) continue;

        ViewportRect rect;
        rect.left = state->position.x;
        rect.top = state->position.y;
        rect.width = state->size.x;
        rect.height = state->size.y;

        entry.widget->Render(rect);
    }
}

void ViewportManager::NotifyEditModeChanged(bool active) {
    m_editModeActive = active;
    auto type = active ? ViewportNotification::EditModeEntered
                       : ViewportNotification::EditModeExited;

    for (auto& [id, entry] : m_viewports) {
        const auto* state = m_registry.GetWidgetState(id);
        ViewportRect rect = entry.lastRect;
        if (state) {
            rect.left = state->position.x;
            rect.top = state->position.y;
            rect.width = state->size.x;
            rect.height = state->size.y;
        }

        ViewportEvent event;
        event.type = type;
        event.viewport = rect;
        event.visible = state ? state->visible : true;
        entry.widget->OnViewportEvent(event);
    }
}

void ViewportManager::SyncWidgetState(std::string_view widgetId) {
    auto it = m_viewports.find(std::string(widgetId));
    if (it == m_viewports.end()) return;

    const auto* state = m_registry.GetWidgetState(widgetId);
    if (!state) return;

    auto& entry = it->second;

    ViewportRect newRect;
    newRect.left = state->position.x;
    newRect.top = state->position.y;
    newRect.width = state->size.x;
    newRect.height = state->size.y;

    bool visChanged = (state->visible != entry.lastVisible);
    bool sizeChanged = (newRect.width != entry.lastRect.width ||
                        newRect.height != entry.lastRect.height);
    bool posChanged = (newRect.left != entry.lastRect.left ||
                       newRect.top != entry.lastRect.top);

    entry.lastRect = newRect;
    entry.lastVisible = state->visible;

    ViewportEvent event;
    event.viewport = newRect;
    event.visible = state->visible;

    if (visChanged) {
        event.type = ViewportNotification::VisibilityChanged;
        entry.widget->OnViewportEvent(event);
    }
    if (sizeChanged) {
        event.type = ViewportNotification::Resized;
        entry.widget->OnViewportEvent(event);
    }
    if (posChanged) {
        event.type = ViewportNotification::Moved;
        entry.widget->OnViewportEvent(event);
    }
}

size_t ViewportManager::ViewportCount() const {
    return m_viewports.size();
}

bool ViewportManager::HasViewport(std::string_view widgetId) const {
    return m_viewports.contains(std::string(widgetId));
}

}  // namespace ohui::widget
