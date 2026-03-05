#pragma once

#include "ohui/core/RegistrationTypes.h"
#include "ohui/core/Result.h"
#include "ohui/dsl/DrawCall.h"
#include "ohui/widget/WidgetRegistry.h"
#include "ohui/widget/WidgetTypes.h"

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::widget {

using WidgetRenderCallback = std::function<void(const WidgetState& state,
                                                 dsl::DrawCallList& output)>;

struct ModWidgetManifest {
    std::string modId;
    WidgetManifest widget;
    WidgetRenderCallback renderFn;
};

class ModWidgetAPI {
public:
    explicit ModWidgetAPI(WidgetRegistry& registry);

    void OpenRegistrationWindow();
    void LockRegistrationWindow();
    RegistrationWindow GetRegistrationWindowState() const;

    Result<void> RegisterWidget(const ModWidgetManifest& manifest);
    Result<void> UnregisterWidget(std::string_view widgetId);

    bool IsModWidget(std::string_view widgetId) const;
    std::string_view GetOwnerMod(std::string_view widgetId) const;
    std::vector<std::string> GetWidgetsByMod(std::string_view modId) const;
    size_t ModWidgetCount() const;

    const WidgetRenderCallback* GetRenderCallback(std::string_view widgetId) const;

private:
    WidgetRegistry& m_registry;
    RegistrationWindow m_windowState{RegistrationWindow::Closed};
    struct ModWidgetEntry {
        std::string modId;
        WidgetRenderCallback renderFn;
    };
    std::unordered_map<std::string, ModWidgetEntry> m_modWidgets;
};

}  // namespace ohui::widget
