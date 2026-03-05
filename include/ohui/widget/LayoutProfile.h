#pragma once

#include "ohui/widget/WidgetTypes.h"

#include <string>
#include <unordered_map>

namespace ohui::widget {

struct WidgetLayoutEntry {
    Vec2 position;
    Vec2 size;
    bool visible{true};
    bool operator==(const WidgetLayoutEntry&) const = default;
};

struct LayoutProfile {
    std::string name;
    std::unordered_map<std::string, WidgetLayoutEntry> widgets;
};

}  // namespace ohui::widget
