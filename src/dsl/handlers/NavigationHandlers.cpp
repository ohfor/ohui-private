#include "NavigationHandlers.h"

#include <sstream>
#include <vector>

namespace ohui::dsl {

namespace {

std::vector<std::string> SplitString(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(token.substr(start, end - start + 1));
        } else if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

}  // anonymous namespace

// --- TabBarHandler ---

void TabBarHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    Color accentColor{0x80, 0x90, 0xA0, 255};
    Color secondaryColor{0xA0, 0xA0, 0xA0, 255};
    float fontSize = 14.0f;
    int activeIndex = 0;

    std::vector<std::string> tabs;
    if (auto* p = ctx.FindProp("tabs")) {
        tabs = SplitString(ctx.resolveString(p->value), ',');
    }
    if (auto* p = ctx.FindProp("activeIndex")) {
        activeIndex = static_cast<int>(ctx.resolveFloat(p->value));
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        fontSize = ctx.resolveFloat(p->value);
    }

    if (tabs.empty()) return;

    float tabWidth = ctx.rect.width / static_cast<float>(tabs.size());

    for (size_t i = 0; i < tabs.size(); ++i) {
        bool isActive = (static_cast<int>(i) == activeIndex);
        float x = ctx.absX + tabWidth * static_cast<float>(i);

        // Tab background
        DrawRect bg;
        bg.x = x;
        bg.y = ctx.absY;
        bg.width = tabWidth;
        bg.height = ctx.rect.height;
        bg.fillColor = isActive ? Color{0x2A, 0x2A, 0x2A, 0xE6} : Color{0x1A, 0x1A, 0x1A, 0xE6};
        output.calls.push_back({DrawCallType::Rect, bg, 0});

        // Tab label
        DrawText dt;
        dt.x = x;
        dt.y = ctx.absY;
        dt.width = tabWidth;
        dt.height = ctx.rect.height;
        dt.text = tabs[i];
        dt.fontSize = fontSize;
        dt.color = isActive ? accentColor : secondaryColor;
        dt.align = TextAlign::Center;
        output.calls.push_back({DrawCallType::Text, dt, 0});

        // Active indicator (underline)
        if (isActive) {
            DrawLine indicator;
            indicator.x1 = x;
            indicator.y1 = ctx.absY + ctx.rect.height - 2.0f;
            indicator.x2 = x + tabWidth;
            indicator.y2 = ctx.absY + ctx.rect.height - 2.0f;
            indicator.color = accentColor;
            indicator.thickness = 2.0f;
            output.calls.push_back({DrawCallType::Line, indicator, 0});
        }
    }
}

// --- BreadcrumbHandler ---

void BreadcrumbHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    Color primaryColor{0xE8, 0xE8, 0xE8, 255};
    Color secondaryColor{0xA0, 0xA0, 0xA0, 255};
    float fontSize = 14.0f;

    std::vector<std::string> levels;
    std::string separator = ">";

    if (auto* p = ctx.FindProp("levels")) {
        levels = SplitString(ctx.resolveString(p->value), ',');
    }
    if (auto* p = ctx.FindProp("separator")) {
        separator = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        fontSize = ctx.resolveFloat(p->value);
    }

    float xOffset = 0.0f;
    float charWidth = fontSize * 0.5f;

    for (size_t i = 0; i < levels.size(); ++i) {
        bool isLast = (i == levels.size() - 1);
        Color color = isLast ? primaryColor : secondaryColor;

        // Level text
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = static_cast<float>(levels[i].size()) * charWidth;
        dt.height = ctx.rect.height;
        dt.text = levels[i];
        dt.fontSize = fontSize;
        dt.color = color;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += dt.width;

        // Separator (except after last)
        if (!isLast) {
            DrawText sep;
            sep.x = ctx.absX + xOffset;
            sep.y = ctx.absY;
            sep.width = static_cast<float>(separator.size()) * charWidth + charWidth;
            sep.height = ctx.rect.height;
            sep.text = " " + separator + " ";
            sep.fontSize = fontSize;
            sep.color = secondaryColor;
            output.calls.push_back({DrawCallType::Text, sep, 0});
            xOffset += sep.width;
        }
    }
}

// --- PaginationHandler ---

void PaginationHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    Color accentColor{0x80, 0x90, 0xA0, 255};
    Color secondaryColor{0xA0, 0xA0, 0xA0, 255};
    Color disabledColor{0x60, 0x60, 0x60, 255};
    float fontSize = 14.0f;

    int totalPages = 1;
    int currentPage = 1;

    if (auto* p = ctx.FindProp("totalPages")) {
        totalPages = std::max(1, static_cast<int>(ctx.resolveFloat(p->value)));
    }
    if (auto* p = ctx.FindProp("currentPage")) {
        currentPage = std::max(1, static_cast<int>(ctx.resolveFloat(p->value)));
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        fontSize = ctx.resolveFloat(p->value);
    }

    float itemWidth = 24.0f;
    float xOffset = 0.0f;

    // Prev arrow
    {
        DrawText dt;
        dt.x = ctx.absX;
        dt.y = ctx.absY;
        dt.width = itemWidth;
        dt.height = ctx.rect.height;
        dt.text = "<";
        dt.fontSize = fontSize;
        dt.color = (currentPage <= 1) ? disabledColor : secondaryColor;
        dt.align = TextAlign::Center;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += itemWidth;
    }

    // Page numbers
    for (int i = 1; i <= totalPages; ++i) {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = itemWidth;
        dt.height = ctx.rect.height;
        dt.text = std::to_string(i);
        dt.fontSize = fontSize;
        dt.color = (i == currentPage) ? accentColor : secondaryColor;
        dt.align = TextAlign::Center;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += itemWidth;
    }

    // Next arrow
    {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = itemWidth;
        dt.height = ctx.rect.height;
        dt.text = ">";
        dt.fontSize = fontSize;
        dt.color = (currentPage >= totalPages) ? disabledColor : secondaryColor;
        dt.align = TextAlign::Center;
        output.calls.push_back({DrawCallType::Text, dt, 0});
    }
}

}  // namespace ohui::dsl
