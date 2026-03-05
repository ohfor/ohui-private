#include "ListHandlers.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace ohui::dsl {

namespace {

std::vector<std::string> SplitCSV(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
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

bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    if (haystack.size() < needle.size()) return false;
    auto toLower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
    };
    for (size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < needle.size(); ++j) {
            if (toLower(haystack[i + j]) != toLower(needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

}  // anonymous namespace

// --- ListEntryHandler ---

void ListEntryHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float iconSize = 24.0f;
    float padding = 8.0f;
    float xCursor = ctx.absX + padding;
    float entryHeight = ctx.rect.height > 0 ? ctx.rect.height : 48.0f;

    // Optional icon
    std::string iconSource;
    if (auto* p = ctx.FindProp("icon")) {
        iconSource = ctx.resolveString(p->value);
    }

    if (!iconSource.empty()) {
        DrawIcon di;
        di.x = xCursor;
        di.y = ctx.absY + (entryHeight - iconSize) * 0.5f;
        di.width = iconSize;
        di.height = iconSize;
        di.source = iconSource;
        di.tint = {0xE8, 0xE8, 0xE8, 255};
        output.calls.push_back({DrawCallType::Icon, di, 0});
        xCursor += iconSize + padding;
    }

    // Primary text
    std::string primaryText;
    if (auto* p = ctx.FindProp("primaryText")) {
        primaryText = ctx.resolveString(p->value);
    }

    float textAreaWidth = ctx.rect.width - (xCursor - ctx.absX) - padding;

    DrawText primary;
    primary.x = xCursor;
    primary.y = ctx.absY + 4.0f;
    primary.width = textAreaWidth;
    primary.height = entryHeight * 0.5f;
    primary.text = primaryText;
    primary.fontSize = 14.0f;
    primary.color = {0xE8, 0xE8, 0xE8, 255};
    output.calls.push_back({DrawCallType::Text, primary, 0});

    // Optional secondary text
    std::string secondaryText;
    if (auto* p = ctx.FindProp("secondaryText")) {
        secondaryText = ctx.resolveString(p->value);
    }

    if (!secondaryText.empty()) {
        DrawText secondary;
        secondary.x = xCursor;
        secondary.y = ctx.absY + entryHeight * 0.5f;
        secondary.width = textAreaWidth;
        secondary.height = entryHeight * 0.5f - 4.0f;
        secondary.text = secondaryText;
        secondary.fontSize = 12.0f;
        secondary.color = {0xA0, 0xA0, 0xA0, 255};  // text-secondary
        output.calls.push_back({DrawCallType::Text, secondary, 0});
    }

    // Indicator dots at right edge
    int indicators = 0;
    if (auto* p = ctx.FindProp("indicators")) {
        indicators = static_cast<int>(ctx.resolveFloat(p->value));
    }

    if (indicators > 0) {
        float dotSize = 6.0f;
        float dotGap = 4.0f;
        float totalDotsWidth = static_cast<float>(indicators) * dotSize +
                               static_cast<float>(indicators - 1) * dotGap;
        float dotX = ctx.absX + ctx.rect.width - padding - totalDotsWidth;
        float dotY = ctx.absY + (entryHeight - dotSize) * 0.5f;

        for (int i = 0; i < indicators; ++i) {
            DrawRect dot;
            dot.x = dotX + static_cast<float>(i) * (dotSize + dotGap);
            dot.y = dotY;
            dot.width = dotSize;
            dot.height = dotSize;
            dot.borderRadius = dotSize * 0.5f;
            dot.fillColor = {0x80, 0x90, 0xA0, 255};  // accent color
            output.calls.push_back({DrawCallType::Rect, dot, 0});
        }
    }
}

// --- ListEntryCompactHandler ---

void ListEntryCompactHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float iconSize = 16.0f;
    float padding = 4.0f;
    float xCursor = ctx.absX + padding;
    float entryHeight = ctx.rect.height > 0 ? ctx.rect.height : 28.0f;

    // Optional icon
    std::string iconSource;
    if (auto* p = ctx.FindProp("icon")) {
        iconSource = ctx.resolveString(p->value);
    }

    if (!iconSource.empty()) {
        DrawIcon di;
        di.x = xCursor;
        di.y = ctx.absY + (entryHeight - iconSize) * 0.5f;
        di.width = iconSize;
        di.height = iconSize;
        di.source = iconSource;
        di.tint = {0xE8, 0xE8, 0xE8, 255};
        output.calls.push_back({DrawCallType::Icon, di, 0});
        xCursor += iconSize + padding;
    }

    // Primary text only (no secondary in compact mode)
    std::string primaryText;
    if (auto* p = ctx.FindProp("primaryText")) {
        primaryText = ctx.resolveString(p->value);
    }

    DrawText primary;
    primary.x = xCursor;
    primary.y = ctx.absY + 2.0f;
    primary.width = ctx.rect.width - (xCursor - ctx.absX) - padding;
    primary.height = entryHeight - 4.0f;
    primary.text = primaryText;
    primary.fontSize = 12.0f;
    primary.color = {0xE8, 0xE8, 0xE8, 255};
    output.calls.push_back({DrawCallType::Text, primary, 0});
}

// --- ScrollListHandler ---

void ScrollListHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    int itemCount = 0;
    float itemHeight = 48.0f;
    float scrollOffset = 0.0f;
    float viewportHeight = ctx.rect.height;

    if (auto* p = ctx.FindProp("itemCount")) {
        itemCount = static_cast<int>(ctx.resolveFloat(p->value));
    }
    if (auto* p = ctx.FindProp("itemHeight")) {
        itemHeight = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("scrollOffset")) {
        scrollOffset = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("viewportHeight")) {
        viewportHeight = ctx.resolveFloat(p->value);
    }

    if (itemCount <= 0 || itemHeight <= 0.0f) return;

    int firstVisible = static_cast<int>(std::floor(scrollOffset / itemHeight));
    int lastVisible = static_cast<int>(std::ceil((scrollOffset + viewportHeight) / itemHeight));

    // Clamp to valid range
    firstVisible = std::max(0, firstVisible);
    lastVisible = std::min(itemCount, lastVisible);

    for (int i = firstVisible; i < lastVisible; ++i) {
        float itemY = ctx.absY + static_cast<float>(i) * itemHeight - scrollOffset;

        DrawRect itemRect;
        itemRect.x = ctx.absX;
        itemRect.y = itemY;
        itemRect.width = ctx.rect.width;
        itemRect.height = itemHeight;
        itemRect.fillColor = (i % 2 == 0)
            ? Color{0x1A, 0x1A, 0x1A, 0xE6}    // even row
            : Color{0x20, 0x20, 0x20, 0xE6};    // odd row
        output.calls.push_back({DrawCallType::Rect, itemRect, 0});

        DrawText itemText;
        itemText.x = ctx.absX + 8.0f;
        itemText.y = itemY;
        itemText.width = ctx.rect.width - 16.0f;
        itemText.height = itemHeight;
        itemText.text = "Item " + std::to_string(i);
        itemText.fontSize = 14.0f;
        itemText.color = {0xE8, 0xE8, 0xE8, 255};
        output.calls.push_back({DrawCallType::Text, itemText, 0});
    }
}

// --- FacetedListHandler ---

void FacetedListHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float padding = 8.0f;
    float searchHeight = 30.0f;
    float chipHeight = 24.0f;
    float chipGap = 4.0f;

    // Background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x1A, 0x1A, 0x1A, 0xE6};
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    float yPos = ctx.absY + padding;

    // Search field text
    std::string searchText;
    if (auto* p = ctx.FindProp("searchText")) {
        searchText = ctx.resolveString(p->value);
    }

    DrawRect searchBg;
    searchBg.x = ctx.absX + padding;
    searchBg.y = yPos;
    searchBg.width = ctx.rect.width - padding * 2;
    searchBg.height = searchHeight;
    searchBg.fillColor = {0x0D, 0x0D, 0x0D, 255};
    searchBg.borderWidth = 1.0f;
    searchBg.borderColor = {0x40, 0x40, 0x40, 255};
    output.calls.push_back({DrawCallType::Rect, searchBg, 0});

    DrawText searchDt;
    searchDt.x = ctx.absX + padding + 4.0f;
    searchDt.y = yPos;
    searchDt.width = ctx.rect.width - padding * 2 - 8.0f;
    searchDt.height = searchHeight;
    searchDt.text = searchText.empty() ? "Search..." : searchText;
    searchDt.fontSize = 14.0f;
    searchDt.color = searchText.empty()
        ? Color{0x60, 0x60, 0x60, 255}
        : Color{0xE8, 0xE8, 0xE8, 255};
    output.calls.push_back({DrawCallType::Text, searchDt, 0});

    yPos += searchHeight + chipGap;

    // Preset buttons (if presets property set)
    std::vector<std::string> presets;
    if (auto* p = ctx.FindProp("presets")) {
        presets = SplitCSV(ctx.resolveString(p->value));
    }

    if (!presets.empty()) {
        float presetX = ctx.absX + padding;
        for (const auto& preset : presets) {
            float chipWidth = static_cast<float>(preset.size()) * 8.0f + 16.0f;

            DrawRect presetBg;
            presetBg.x = presetX;
            presetBg.y = yPos;
            presetBg.width = chipWidth;
            presetBg.height = chipHeight;
            presetBg.borderRadius = chipHeight * 0.5f;
            presetBg.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};
            output.calls.push_back({DrawCallType::Rect, presetBg, 0});

            DrawText presetDt;
            presetDt.x = presetX + 8.0f;
            presetDt.y = yPos;
            presetDt.width = chipWidth - 16.0f;
            presetDt.height = chipHeight;
            presetDt.text = preset;
            presetDt.fontSize = 12.0f;
            presetDt.color = {0xE8, 0xE8, 0xE8, 255};
            output.calls.push_back({DrawCallType::Text, presetDt, 0});

            presetX += chipWidth + chipGap;
        }
        yPos += chipHeight + chipGap;
    }

    // Active filter chips
    std::vector<std::string> activeFilters;
    if (auto* p = ctx.FindProp("activeFilters")) {
        activeFilters = SplitCSV(ctx.resolveString(p->value));
    }

    if (!activeFilters.empty()) {
        float chipX = ctx.absX + padding;
        for (const auto& filter : activeFilters) {
            float chipWidth = static_cast<float>(filter.size()) * 8.0f + 16.0f;

            DrawRect chipBg;
            chipBg.x = chipX;
            chipBg.y = yPos;
            chipBg.width = chipWidth;
            chipBg.height = chipHeight;
            chipBg.borderRadius = chipHeight * 0.5f;
            chipBg.fillColor = {0x80, 0x90, 0xA0, 255};  // accent color
            output.calls.push_back({DrawCallType::Rect, chipBg, 0});

            DrawText chipDt;
            chipDt.x = chipX + 8.0f;
            chipDt.y = yPos;
            chipDt.width = chipWidth - 16.0f;
            chipDt.height = chipHeight;
            chipDt.text = filter;
            chipDt.fontSize = 12.0f;
            chipDt.color = {0xFF, 0xFF, 0xFF, 255};
            output.calls.push_back({DrawCallType::Text, chipDt, 0});

            chipX += chipWidth + chipGap;
        }
        yPos += chipHeight + chipGap;
    }

    // Items list (filtered by search and activeFilters)
    std::vector<std::string> items;
    if (auto* p = ctx.FindProp("items")) {
        items = SplitCSV(ctx.resolveString(p->value));
    }

    // Apply search filter
    std::vector<std::string> filtered;
    for (const auto& item : items) {
        if (!searchText.empty() && !ContainsCaseInsensitive(item, searchText)) {
            continue;
        }
        // Apply active filter chips (AND-combined)
        bool passesAllFilters = true;
        for (const auto& filter : activeFilters) {
            if (!ContainsCaseInsensitive(item, filter)) {
                passesAllFilters = false;
                break;
            }
        }
        if (passesAllFilters) {
            filtered.push_back(item);
        }
    }

    float itemHeight = 32.0f;
    for (size_t i = 0; i < filtered.size(); ++i) {
        float itemY = yPos + static_cast<float>(i) * itemHeight;

        DrawRect itemBg;
        itemBg.x = ctx.absX + padding;
        itemBg.y = itemY;
        itemBg.width = ctx.rect.width - padding * 2;
        itemBg.height = itemHeight;
        itemBg.fillColor = (i % 2 == 0)
            ? Color{0x1A, 0x1A, 0x1A, 0xE6}
            : Color{0x20, 0x20, 0x20, 0xE6};
        output.calls.push_back({DrawCallType::Rect, itemBg, 0});

        DrawText itemDt;
        itemDt.x = ctx.absX + padding + 8.0f;
        itemDt.y = itemY;
        itemDt.width = ctx.rect.width - padding * 2 - 16.0f;
        itemDt.height = itemHeight;
        itemDt.text = filtered[i];
        itemDt.fontSize = 14.0f;
        itemDt.color = {0xE8, 0xE8, 0xE8, 255};
        output.calls.push_back({DrawCallType::Text, itemDt, 0});
    }
}

}  // namespace ohui::dsl
