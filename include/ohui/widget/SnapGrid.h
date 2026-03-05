#pragma once

#include "ohui/widget/WidgetTypes.h"

#include <cmath>

namespace ohui::widget {

inline float Snap(float value, float gridSize) {
    if (gridSize <= 0.0f) return value;
    return std::round(value / gridSize) * gridSize;
}

inline Vec2 SnapPosition(Vec2 pos, float gridSize) {
    return {Snap(pos.x, gridSize), Snap(pos.y, gridSize)};
}

}  // namespace ohui::widget
