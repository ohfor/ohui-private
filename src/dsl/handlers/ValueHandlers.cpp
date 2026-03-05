#include "ValueHandlers.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace ohui::dsl {

// --- TimerBarHandler ---

void TimerBarHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float value = 0.0f, maxValue = 100.0f;
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("maxValue")) maxValue = ctx.resolveFloat(p->value);

    float ratio = (maxValue > 0.0f) ? std::clamp(value / maxValue, 0.0f, 1.0f) : 0.0f;

    // Parse thresholds and colors
    std::vector<float> thresholds;
    std::vector<Color> thresholdColors;

    if (auto* p = ctx.FindProp("thresholds")) {
        std::string s = ctx.resolveString(p->value);
        std::istringstream ss(s);
        std::string token;
        while (std::getline(ss, token, ',')) {
            try { thresholds.push_back(std::stof(token)); } catch (...) {}
        }
    }
    if (auto* p = ctx.FindProp("thresholdColors")) {
        std::string s = ctx.resolveString(p->value);
        std::istringstream ss(s);
        std::string token;
        while (std::getline(ss, token, ',')) {
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t");
            if (start != std::string::npos) token = token.substr(start);
            size_t end = token.find_last_not_of(" \t");
            if (end != std::string::npos) token = token.substr(0, end + 1);
            thresholdColors.push_back(ctx.parseColor(token));
        }
    }

    // Default fill color
    Color fillColor{0x80, 0x90, 0xA0, 255};  // accent-primary
    if (auto* p = ctx.FindProp("fillColor")) {
        fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }

    // Determine active color based on ratio vs thresholds
    Color activeColor = fillColor;
    for (size_t i = 0; i < thresholds.size() && i < thresholdColors.size(); ++i) {
        if (ratio <= thresholds[i]) {
            activeColor = thresholdColors[i];
        }
    }

    // Background rect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("backgroundColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Fill rect
    DrawRect fill;
    fill.x = ctx.absX;
    fill.y = ctx.absY;
    fill.width = ctx.rect.width * ratio;
    fill.height = ctx.rect.height;
    fill.fillColor = activeColor;
    if (auto* p = ctx.FindProp("opacity")) {
        fill.opacity = ctx.resolveFloat(p->value);
    }
    output.calls.push_back({DrawCallType::Rect, fill, 0});
}

// --- CountBadgeHandler ---

void CountBadgeHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    int count = 0;
    int maxDisplay = 99;
    std::string shape = "pill";

    if (auto* p = ctx.FindProp("count")) count = static_cast<int>(ctx.resolveFloat(p->value));
    if (auto* p = ctx.FindProp("maxDisplay")) maxDisplay = static_cast<int>(ctx.resolveFloat(p->value));
    if (auto* p = ctx.FindProp("shape")) shape = ctx.resolveString(p->value);

    std::string displayText;
    if (count > maxDisplay) {
        displayText = std::to_string(maxDisplay) + "+";
    } else {
        displayText = std::to_string(count);
    }

    // Background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x80, 0x90, 0xA0, 255};  // accent-primary
    if (auto* p = ctx.FindProp("color")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (shape == "pill") {
        bg.borderRadius = 9999.0f;  // --radius-pill
    } else if (shape == "circle") {
        bg.borderRadius = ctx.rect.height * 0.5f;
    }
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Count text
    DrawText dt;
    dt.x = ctx.absX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width;
    dt.height = ctx.rect.height;
    dt.text = displayText;
    dt.fontSize = 12.0f;
    dt.color = {255, 255, 255, 255};
    dt.align = TextAlign::Center;
    if (auto* p = ctx.FindProp("fontSize")) {
        dt.fontSize = ctx.resolveFloat(p->value);
    }
    output.calls.push_back({DrawCallType::Text, dt, 0});
}

}  // namespace ohui::dsl
