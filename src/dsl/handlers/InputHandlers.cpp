#include "InputHandlers.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace ohui::dsl {

namespace {

std::vector<std::string> SplitItems(const std::string& s, char delim = ',') {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
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

// --- TextInputHandler ---

void TextInputHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string value, placeholder;
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("placeholder")) placeholder = ctx.resolveString(p->value);

    // Background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x0D, 0x0D, 0x0D, 255};  // surface-input
    bg.borderWidth = 1.0f;
    bg.borderColor = {0x40, 0x40, 0x40, 255};
    if (auto* p = ctx.FindProp("borderColor")) {
        bg.borderColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Text content or placeholder
    DrawText dt;
    dt.x = ctx.absX + 4.0f;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width - 8.0f;
    dt.height = ctx.rect.height;
    dt.fontSize = 14.0f;

    if (value.empty() && !placeholder.empty()) {
        dt.text = placeholder;
        dt.color = {0x60, 0x60, 0x60, 255};  // text-disabled
    } else {
        dt.text = value;
        dt.color = {0xE8, 0xE8, 0xE8, 255};  // text-primary
    }
    output.calls.push_back({DrawCallType::Text, dt, 0});

    // Cursor line
    float cursorX = ctx.absX + 4.0f + static_cast<float>(value.size()) * 7.0f;
    DrawLine cursor;
    cursor.x1 = cursorX;
    cursor.y1 = ctx.absY + 2.0f;
    cursor.x2 = cursorX;
    cursor.y2 = ctx.absY + ctx.rect.height - 2.0f;
    cursor.color = {0xE8, 0xE8, 0xE8, 255};
    cursor.thickness = 1.0f;
    output.calls.push_back({DrawCallType::Line, cursor, 0});
}

// --- SearchFieldHandler ---

void SearchFieldHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string value;
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveString(p->value);

    // Search icon
    DrawIcon icon;
    icon.x = ctx.absX + 4.0f;
    icon.y = ctx.absY + (ctx.rect.height - 16.0f) * 0.5f;
    icon.width = 16.0f;
    icon.height = 16.0f;
    icon.source = "icons/search.png";
    icon.tint = {0xA0, 0xA0, 0xA0, 255};
    output.calls.push_back({DrawCallType::Icon, icon, 0});

    // Background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x0D, 0x0D, 0x0D, 255};
    bg.borderWidth = 1.0f;
    bg.borderColor = {0x40, 0x40, 0x40, 255};
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Text
    DrawText dt;
    dt.x = ctx.absX + 24.0f;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width - 48.0f;
    dt.height = ctx.rect.height;
    dt.fontSize = 14.0f;

    std::string placeholder;
    if (auto* p = ctx.FindProp("placeholder")) placeholder = ctx.resolveString(p->value);

    if (value.empty() && !placeholder.empty()) {
        dt.text = placeholder;
        dt.color = {0x60, 0x60, 0x60, 255};
    } else {
        dt.text = value;
        dt.color = {0xE8, 0xE8, 0xE8, 255};
    }
    output.calls.push_back({DrawCallType::Text, dt, 0});

    // Clear button when text present
    if (!value.empty()) {
        DrawIcon clearBtn;
        clearBtn.x = ctx.absX + ctx.rect.width - 20.0f;
        clearBtn.y = ctx.absY + (ctx.rect.height - 12.0f) * 0.5f;
        clearBtn.width = 12.0f;
        clearBtn.height = 12.0f;
        clearBtn.source = "icons/clear.png";
        clearBtn.tint = {0xA0, 0xA0, 0xA0, 255};
        output.calls.push_back({DrawCallType::Icon, clearBtn, 0});
    }
}

// --- ToggleHandler ---

void ToggleHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    bool on = false;
    if (auto* p = ctx.FindProp("value")) {
        auto val = ctx.resolveString(p->value);
        on = (val == "true" || val == "1");
    }

    Color accentColor{0x80, 0x90, 0xA0, 255};
    Color secondaryColor{0xA0, 0xA0, 0xA0, 255};

    // Track
    DrawRect track;
    track.x = ctx.absX;
    track.y = ctx.absY;
    track.width = ctx.rect.width;
    track.height = ctx.rect.height;
    track.borderRadius = ctx.rect.height * 0.5f;
    track.fillColor = on ? accentColor : Color{0x40, 0x40, 0x40, 255};
    output.calls.push_back({DrawCallType::Rect, track, 0});

    // Thumb
    float thumbSize = ctx.rect.height - 4.0f;
    float thumbX = on ? (ctx.absX + ctx.rect.width - thumbSize - 2.0f) : (ctx.absX + 2.0f);

    DrawRect thumb;
    thumb.x = thumbX;
    thumb.y = ctx.absY + 2.0f;
    thumb.width = thumbSize;
    thumb.height = thumbSize;
    thumb.borderRadius = thumbSize * 0.5f;
    thumb.fillColor = {255, 255, 255, 255};
    output.calls.push_back({DrawCallType::Rect, thumb, 0});
}

// --- SliderHandler ---

void SliderHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float value = 50.0f;
    float minVal = 0.0f;
    float maxVal = 100.0f;

    if (auto* p = ctx.FindProp("value")) value = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("min")) minVal = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("max")) maxVal = ctx.resolveFloat(p->value);

    float range = maxVal - minVal;
    float ratio = (range > 0.0f) ? std::clamp((value - minVal) / range, 0.0f, 1.0f) : 0.0f;

    float trackHeight = 4.0f;
    float trackY = ctx.absY + (ctx.rect.height - trackHeight) * 0.5f;

    // Track background
    DrawRect trackBg;
    trackBg.x = ctx.absX;
    trackBg.y = trackY;
    trackBg.width = ctx.rect.width;
    trackBg.height = trackHeight;
    trackBg.borderRadius = 2.0f;
    trackBg.fillColor = {0x40, 0x40, 0x40, 255};
    output.calls.push_back({DrawCallType::Rect, trackBg, 0});

    // Track fill
    DrawRect trackFill;
    trackFill.x = ctx.absX;
    trackFill.y = trackY;
    trackFill.width = ctx.rect.width * ratio;
    trackFill.height = trackHeight;
    trackFill.borderRadius = 2.0f;
    trackFill.fillColor = {0x80, 0x90, 0xA0, 255};
    output.calls.push_back({DrawCallType::Rect, trackFill, 0});

    // Thumb
    float thumbSize = 12.0f;
    float thumbX = ctx.absX + ctx.rect.width * ratio - thumbSize * 0.5f;

    DrawRect thumb;
    thumb.x = thumbX;
    thumb.y = ctx.absY + (ctx.rect.height - thumbSize) * 0.5f;
    thumb.width = thumbSize;
    thumb.height = thumbSize;
    thumb.borderRadius = thumbSize * 0.5f;
    thumb.fillColor = {255, 255, 255, 255};
    output.calls.push_back({DrawCallType::Rect, thumb, 0});
}

// --- DropdownHandler ---

void DropdownHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    bool isOpen = false;
    std::string selectedValue;
    int selectedIndex = 0;

    if (auto* p = ctx.FindProp("open")) {
        auto val = ctx.resolveString(p->value);
        isOpen = (val == "true" || val == "1");
    }
    if (auto* p = ctx.FindProp("selectedValue")) selectedValue = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("selectedIndex")) selectedIndex = static_cast<int>(ctx.resolveFloat(p->value));

    std::vector<std::string> options;
    if (auto* p = ctx.FindProp("options")) {
        options = SplitItems(ctx.resolveString(p->value));
    }

    if (selectedValue.empty() && !options.empty() && selectedIndex >= 0 &&
        selectedIndex < static_cast<int>(options.size())) {
        selectedValue = options[selectedIndex];
    }

    // Closed state: selected value + chevron
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x0D, 0x0D, 0x0D, 255};
    bg.borderWidth = 1.0f;
    bg.borderColor = {0x40, 0x40, 0x40, 255};
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    DrawText dt;
    dt.x = ctx.absX + 4.0f;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width - 24.0f;
    dt.height = ctx.rect.height;
    dt.text = selectedValue;
    dt.fontSize = 14.0f;
    dt.color = {0xE8, 0xE8, 0xE8, 255};
    output.calls.push_back({DrawCallType::Text, dt, 0});

    // Chevron icon
    DrawIcon chevron;
    chevron.x = ctx.absX + ctx.rect.width - 20.0f;
    chevron.y = ctx.absY + (ctx.rect.height - 12.0f) * 0.5f;
    chevron.width = 12.0f;
    chevron.height = 12.0f;
    chevron.source = isOpen ? "icons/chevron-up.png" : "icons/chevron-down.png";
    chevron.tint = {0xA0, 0xA0, 0xA0, 255};
    output.calls.push_back({DrawCallType::Icon, chevron, 0});

    // Open state: option list
    if (isOpen && !options.empty()) {
        float optionH = ctx.rect.height;
        float listY = ctx.absY + ctx.rect.height;

        for (size_t i = 0; i < options.size(); ++i) {
            bool selected = (static_cast<int>(i) == selectedIndex);
            float y = listY + optionH * static_cast<float>(i);

            DrawRect optBg;
            optBg.x = ctx.absX;
            optBg.y = y;
            optBg.width = ctx.rect.width;
            optBg.height = optionH;
            optBg.fillColor = selected ? Color{0x2A, 0x2A, 0x2A, 0xE6} : Color{0x1A, 0x1A, 0x1A, 0xE6};
            output.calls.push_back({DrawCallType::Rect, optBg, 10});

            DrawText optText;
            optText.x = ctx.absX + 4.0f;
            optText.y = y;
            optText.width = ctx.rect.width - 8.0f;
            optText.height = optionH;
            optText.text = options[i];
            optText.fontSize = 14.0f;
            optText.color = selected ? Color{0x80, 0x90, 0xA0, 255} : Color{0xE8, 0xE8, 0xE8, 255};
            output.calls.push_back({DrawCallType::Text, optText, 10});
        }
    }
}

// --- StepperHandler ---

void StepperHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float value = 0.0f;
    float minVal = 0.0f;
    float maxVal = 100.0f;

    if (auto* p = ctx.FindProp("value")) value = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("min")) minVal = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("max")) maxVal = ctx.resolveFloat(p->value);

    Color normalColor{0xA0, 0xA0, 0xA0, 255};
    Color disabledColor{0x60, 0x60, 0x60, 255};

    float btnWidth = ctx.rect.height;  // Square buttons

    // Minus button
    DrawRect minusBg;
    minusBg.x = ctx.absX;
    minusBg.y = ctx.absY;
    minusBg.width = btnWidth;
    minusBg.height = ctx.rect.height;
    minusBg.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};
    output.calls.push_back({DrawCallType::Rect, minusBg, 0});

    DrawText minusText;
    minusText.x = ctx.absX;
    minusText.y = ctx.absY;
    minusText.width = btnWidth;
    minusText.height = ctx.rect.height;
    minusText.text = "-";
    minusText.fontSize = 14.0f;
    minusText.color = (value <= minVal) ? disabledColor : normalColor;
    minusText.align = TextAlign::Center;
    output.calls.push_back({DrawCallType::Text, minusText, 0});

    // Value display
    DrawText valueText;
    valueText.x = ctx.absX + btnWidth;
    valueText.y = ctx.absY;
    valueText.width = ctx.rect.width - btnWidth * 2;
    valueText.height = ctx.rect.height;
    valueText.text = std::to_string(static_cast<int>(value));
    valueText.fontSize = 14.0f;
    valueText.color = {0xE8, 0xE8, 0xE8, 255};
    valueText.align = TextAlign::Center;
    output.calls.push_back({DrawCallType::Text, valueText, 0});

    // Plus button
    DrawRect plusBg;
    plusBg.x = ctx.absX + ctx.rect.width - btnWidth;
    plusBg.y = ctx.absY;
    plusBg.width = btnWidth;
    plusBg.height = ctx.rect.height;
    plusBg.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};
    output.calls.push_back({DrawCallType::Rect, plusBg, 0});

    DrawText plusText;
    plusText.x = ctx.absX + ctx.rect.width - btnWidth;
    plusText.y = ctx.absY;
    plusText.width = btnWidth;
    plusText.height = ctx.rect.height;
    plusText.text = "+";
    plusText.fontSize = 14.0f;
    plusText.color = (value >= maxVal) ? disabledColor : normalColor;
    plusText.align = TextAlign::Center;
    output.calls.push_back({DrawCallType::Text, plusText, 0});
}

// --- ContextMenuHandler ---

void ContextMenuHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::vector<std::string> items;
    if (auto* p = ctx.FindProp("items")) {
        items = SplitItems(ctx.resolveString(p->value));
    }
    if (items.empty()) return;

    float itemHeight = 24.0f;
    float menuWidth = ctx.rect.width > 0 ? ctx.rect.width : 150.0f;

    // Overlay background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = menuWidth;
    bg.height = itemHeight * static_cast<float>(items.size());
    bg.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};
    bg.borderWidth = 1.0f;
    bg.borderColor = {0x40, 0x40, 0x40, 255};
    output.calls.push_back({DrawCallType::Rect, bg, 200});

    for (size_t i = 0; i < items.size(); ++i) {
        float y = ctx.absY + itemHeight * static_cast<float>(i);

        if (items[i] == "---") {
            // Separator
            DrawLine sep;
            sep.x1 = ctx.absX + 4.0f;
            sep.y1 = y + itemHeight * 0.5f;
            sep.x2 = ctx.absX + menuWidth - 4.0f;
            sep.y2 = y + itemHeight * 0.5f;
            sep.color = {0x40, 0x40, 0x40, 255};
            sep.thickness = 1.0f;
            output.calls.push_back({DrawCallType::Line, sep, 200});
        } else {
            DrawText dt;
            dt.x = ctx.absX + 8.0f;
            dt.y = y;
            dt.width = menuWidth - 16.0f;
            dt.height = itemHeight;
            dt.text = items[i];
            dt.fontSize = 14.0f;
            dt.color = {0xE8, 0xE8, 0xE8, 255};
            output.calls.push_back({DrawCallType::Text, dt, 200});
        }
    }
}

}  // namespace ohui::dsl
