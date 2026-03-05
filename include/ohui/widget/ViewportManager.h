#pragma once

#include "ohui/widget/IViewportWidget.h"
#include "ohui/widget/WidgetRegistry.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace ohui::widget {

class ViewportManager {
public:
    explicit ViewportManager(WidgetRegistry& registry);

    Result<void> Register(IViewportWidget* widget);
    Result<void> Unregister(std::string_view widgetId);

    void UpdateAll();
    void NotifyEditModeChanged(bool active);
    void SyncWidgetState(std::string_view widgetId);

    size_t ViewportCount() const;
    bool HasViewport(std::string_view widgetId) const;

private:
    struct ViewportEntry {
        IViewportWidget* widget{nullptr};
        ViewportRect lastRect;
        bool lastVisible{true};
    };
    WidgetRegistry& m_registry;
    std::unordered_map<std::string, ViewportEntry> m_viewports;
    bool m_editModeActive{false};
};

}  // namespace ohui::widget
