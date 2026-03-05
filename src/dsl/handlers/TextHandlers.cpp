#include "TextHandlers.h"

#include <string>
#include <vector>

namespace ohui::dsl {

// --- CaptionHandler ---

void CaptionHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawText dt;
    dt.x = ctx.absX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width;
    dt.height = ctx.rect.height;
    dt.fontSize = 12.0f;  // --font-size-sm default
    dt.color = {0xA0, 0xA0, 0xA0, 255};  // --color-text-secondary default

    if (auto* p = ctx.FindProp("text")) {
        dt.text = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        dt.fontSize = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("color")) {
        dt.color = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("textAlign")) {
        auto val = ctx.resolveString(p->value);
        if (val == "center") dt.align = TextAlign::Center;
        else if (val == "right") dt.align = TextAlign::Right;
    }
    if (auto* p = ctx.FindProp("opacity")) {
        dt.opacity = ctx.resolveFloat(p->value);
    }

    output.calls.push_back({DrawCallType::Text, dt, 0});
}

// --- RichText helpers ---

namespace {

struct TextSegment {
    std::string text;
    bool bold{false};
    bool italic{false};
    Color color{0xE8, 0xE8, 0xE8, 255};  // default primary
    bool hasCustomColor{false};
};

std::vector<TextSegment> ParseRichText(const std::string& input, Color defaultColor) {
    std::vector<TextSegment> segments;

    struct State {
        bool bold{false};
        bool italic{false};
        Color color{};
        bool hasCustomColor{false};
    };
    std::vector<State> stateStack;
    State current;
    current.color = defaultColor;

    std::string buffer;
    size_t i = 0;

    auto flushBuffer = [&]() {
        if (!buffer.empty()) {
            TextSegment seg;
            seg.text = buffer;
            seg.bold = current.bold;
            seg.italic = current.italic;
            seg.color = current.color;
            seg.hasCustomColor = current.hasCustomColor;
            segments.push_back(std::move(seg));
            buffer.clear();
        }
    };

    while (i < input.size()) {
        if (input[i] == '<') {
            // Check for tags
            if (i + 2 < input.size() && input[i + 1] == 'b' && input[i + 2] == '>') {
                flushBuffer();
                stateStack.push_back(current);
                current.bold = true;
                i += 3;
                continue;
            }
            if (i + 3 < input.size() && input.substr(i, 4) == "</b>") {
                flushBuffer();
                if (!stateStack.empty()) {
                    current = stateStack.back();
                    stateStack.pop_back();
                }
                i += 4;
                continue;
            }
            if (i + 2 < input.size() && input[i + 1] == 'i' && input[i + 2] == '>') {
                flushBuffer();
                stateStack.push_back(current);
                current.italic = true;
                i += 3;
                continue;
            }
            if (i + 3 < input.size() && input.substr(i, 4) == "</i>") {
                flushBuffer();
                if (!stateStack.empty()) {
                    current = stateStack.back();
                    stateStack.pop_back();
                }
                i += 4;
                continue;
            }
            // <color=#RRGGBB>
            if (i + 7 < input.size() && input.substr(i, 7) == "<color=") {
                size_t closePos = input.find('>', i + 7);
                if (closePos != std::string::npos) {
                    flushBuffer();
                    std::string colorStr = input.substr(i + 7, closePos - (i + 7));
                    stateStack.push_back(current);
                    // Parse hex color
                    if (colorStr.size() >= 7 && colorStr[0] == '#') {
                        auto hexByte = [&](size_t pos) -> uint8_t {
                            auto toNib = [](char c) -> uint8_t {
                                if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
                                if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
                                if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
                                return 0;
                            };
                            return static_cast<uint8_t>(toNib(colorStr[pos]) * 16 + toNib(colorStr[pos + 1]));
                        };
                        current.color.r = hexByte(1);
                        current.color.g = hexByte(3);
                        current.color.b = hexByte(5);
                        current.color.a = 255;
                        current.hasCustomColor = true;
                    }
                    i = closePos + 1;
                    continue;
                }
            }
            if (i + 7 < input.size() && input.substr(i, 8) == "</color>") {
                flushBuffer();
                if (!stateStack.empty()) {
                    current = stateStack.back();
                    stateStack.pop_back();
                }
                i += 8;
                continue;
            }
        }
        buffer += input[i];
        ++i;
    }
    flushBuffer();

    return segments;
}

}  // anonymous namespace

// --- RichTextHandler ---

void RichTextHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string text;
    float fontSize = 14.0f;
    Color defaultColor{0xE8, 0xE8, 0xE8, 255};
    float opacity = 1.0f;

    if (auto* p = ctx.FindProp("text")) {
        text = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        fontSize = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("color")) {
        defaultColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("opacity")) {
        opacity = ctx.resolveFloat(p->value);
    }

    auto segments = ParseRichText(text, defaultColor);
    float xOffset = 0.0f;

    for (const auto& seg : segments) {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = ctx.rect.width - xOffset;
        dt.height = ctx.rect.height;
        dt.text = seg.text;
        dt.fontSize = fontSize;
        dt.color = seg.color;
        dt.opacity = opacity;

        if (seg.bold && seg.italic) {
            dt.fontFamily = "bold-italic";
        } else if (seg.bold) {
            dt.fontFamily = "bold";
        } else if (seg.italic) {
            dt.fontFamily = "italic";
        }

        output.calls.push_back({DrawCallType::Text, dt, 0});
        // Approximate advance: character count * fontSize * 0.5
        xOffset += static_cast<float>(seg.text.size()) * fontSize * 0.5f;
    }
}

// --- LoreTextHandler ---

void LoreTextHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string text;
    float fontSize = 14.0f;
    Color defaultColor{0xD4, 0xA5, 0x74, 255};  // --color-text-lore
    std::string fontFamily = "Garamond";  // --font-family-lore
    float opacity = 1.0f;

    if (auto* p = ctx.FindProp("text")) {
        text = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        fontSize = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("color")) {
        defaultColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("fontFamily")) {
        fontFamily = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("opacity")) {
        opacity = ctx.resolveFloat(p->value);
    }

    auto segments = ParseRichText(text, defaultColor);
    float xOffset = 0.0f;

    for (const auto& seg : segments) {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = ctx.rect.width - xOffset;
        dt.height = ctx.rect.height;
        dt.text = seg.text;
        dt.fontSize = fontSize;
        dt.fontFamily = fontFamily;
        dt.color = seg.color;
        dt.opacity = opacity;

        if (seg.bold) dt.fontFamily = fontFamily + "-bold";
        if (seg.italic) dt.fontFamily = fontFamily + "-italic";

        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += static_cast<float>(seg.text.size()) * fontSize * 0.5f;
    }
}

// --- StatValueHandler ---

void StatValueHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    Color labelColor{0xA0, 0xA0, 0xA0, 255};  // text-secondary
    Color valueColor{0xE8, 0xE8, 0xE8, 255};  // text-primary
    float fontSize = 14.0f;

    std::string label, value, unit;
    if (auto* p = ctx.FindProp("label")) label = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("unit")) unit = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("fontSize")) fontSize = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("labelColor")) labelColor = ctx.parseColor(ctx.resolveString(p->value));
    if (auto* p = ctx.FindProp("valueColor")) valueColor = ctx.parseColor(ctx.resolveString(p->value));

    float xOffset = 0.0f;

    // Label text
    if (!label.empty()) {
        DrawText dt;
        dt.x = ctx.absX;
        dt.y = ctx.absY;
        dt.width = ctx.rect.width * 0.5f;
        dt.height = ctx.rect.height;
        dt.text = label;
        dt.fontSize = fontSize;
        dt.color = labelColor;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset = ctx.rect.width * 0.5f;
    }

    // Value text
    {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = ctx.rect.width - xOffset;
        dt.height = ctx.rect.height;
        dt.text = value;
        dt.fontSize = fontSize;
        dt.color = valueColor;
        dt.align = TextAlign::Right;
        output.calls.push_back({DrawCallType::Text, dt, 0});
    }

    // Unit text (if present)
    if (!unit.empty()) {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = ctx.rect.width - xOffset;
        dt.height = ctx.rect.height;
        dt.text = unit;
        dt.fontSize = fontSize * 0.8f;
        dt.color = labelColor;
        output.calls.push_back({DrawCallType::Text, dt, 0});
    }
}

// --- StatDeltaHandler ---

void StatDeltaHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    Color positiveColor{0x4C, 0xAF, 0x50, 255};   // --color-text-positive
    Color negativeColor{0xF4, 0x43, 0x36, 255};    // --color-text-negative
    Color secondaryColor{0xA0, 0xA0, 0xA0, 255};   // --color-text-secondary

    float fontSize = 14.0f;
    std::string label, unit;
    float oldValue = 0.0f, newValue = 0.0f;

    if (auto* p = ctx.FindProp("label")) label = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("oldValue")) oldValue = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("newValue")) newValue = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("unit")) unit = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("fontSize")) fontSize = ctx.resolveFloat(p->value);

    float delta = newValue - oldValue;
    Color deltaColor = secondaryColor;
    if (delta > 0.0f) deltaColor = positiveColor;
    else if (delta < 0.0f) deltaColor = negativeColor;

    float xOffset = 0.0f;
    float segWidth = ctx.rect.width / 4.0f;

    // Label
    if (!label.empty()) {
        DrawText dt;
        dt.x = ctx.absX;
        dt.y = ctx.absY;
        dt.width = segWidth;
        dt.height = ctx.rect.height;
        dt.text = label;
        dt.fontSize = fontSize;
        dt.color = secondaryColor;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += segWidth;
    }

    // Old value
    {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = segWidth;
        dt.height = ctx.rect.height;
        dt.text = std::to_string(static_cast<int>(oldValue));
        dt.fontSize = fontSize;
        dt.color = secondaryColor;
        dt.align = TextAlign::Right;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += segWidth;
    }

    // Arrow
    {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = segWidth;
        dt.height = ctx.rect.height;
        dt.text = "\xe2\x86\x92";  // → UTF-8
        dt.fontSize = fontSize;
        dt.color = deltaColor;
        dt.align = TextAlign::Center;
        output.calls.push_back({DrawCallType::Text, dt, 0});
        xOffset += segWidth;
    }

    // New value
    {
        DrawText dt;
        dt.x = ctx.absX + xOffset;
        dt.y = ctx.absY;
        dt.width = segWidth;
        dt.height = ctx.rect.height;
        dt.text = std::to_string(static_cast<int>(newValue));
        dt.fontSize = fontSize;
        dt.color = deltaColor;
        output.calls.push_back({DrawCallType::Text, dt, 0});
    }
}

// --- DeltaListHandler ---

void DeltaListHandler::Emit(const ComponentContext& /*ctx*/, DrawCallList& /*output*/) {
    // DeltaList is purely a container — it sets flexDirection column.
    // No direct draw calls; children are StatDeltas rendered by the engine.
    // Emit nothing; children are handled by the engine's recursive tree walk.
}

}  // namespace ohui::dsl
