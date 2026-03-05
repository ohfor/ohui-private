#pragma once

#include <string>

namespace ohui::widget {

struct Vec2 {
    float x{0.0f};
    float y{0.0f};
    bool operator==(const Vec2&) const = default;
};

struct WidgetManifest {
    std::string id;
    std::string displayName;
    Vec2 defaultPosition;
    Vec2 defaultSize;
    Vec2 minSize;
    Vec2 maxSize;  // {0,0} = unconstrained
    bool defaultVisible{true};
};

struct WidgetState {
    std::string id;
    Vec2 position;
    Vec2 size;
    bool visible{false};
    bool active{false};
};

}  // namespace ohui::widget
