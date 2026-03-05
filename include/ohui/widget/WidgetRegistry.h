#pragma once

#include "ohui/widget/WidgetTypes.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::widget {

class WidgetRegistry {
public:
    Result<void> Register(const WidgetManifest& manifest);
    Result<void> Activate(std::string_view id);
    Result<void> Deactivate(std::string_view id);
    Result<void> Move(std::string_view id, Vec2 position);
    Result<void> Resize(std::string_view id, Vec2 size);
    Result<void> SetVisible(std::string_view id, bool visible);

    const WidgetState* GetWidgetState(std::string_view id) const;
    const WidgetManifest* GetManifest(std::string_view id) const;
    std::vector<const WidgetState*> GetAllWidgets() const;
    std::vector<const WidgetState*> GetActiveWidgets() const;
    size_t WidgetCount() const;
    bool HasWidget(std::string_view id) const;

private:
    struct WidgetEntry {
        WidgetManifest manifest;
        WidgetState state;
    };
    std::unordered_map<std::string, WidgetEntry> m_widgets;
};

}  // namespace ohui::widget
