#include "IndicatorHandlers.h"

#include <algorithm>

namespace ohui::dsl {

// --- StatusBadgeHandler ---

void StatusBadgeHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    Color badgeColor{0x80, 0x90, 0xA0, 255};  // accent-primary default
    if (auto* p = ctx.FindProp("color")) {
        badgeColor = ctx.parseColor(ctx.resolveString(p->value));
    }

    // Pill background
    DrawRect pill;
    pill.x = ctx.absX;
    pill.y = ctx.absY;
    pill.width = ctx.rect.width;
    pill.height = ctx.rect.height;
    pill.fillColor = badgeColor;
    pill.borderRadius = 9999.0f;  // radius-pill token
    output.calls.push_back({DrawCallType::Rect, pill, 0});

    // Optional icon
    if (auto* p = ctx.FindProp("icon")) {
        std::string iconSrc = ctx.resolveString(p->value);
        DrawIcon di;
        di.x = ctx.absX + 4.0f;
        di.y = ctx.absY + (ctx.rect.height - 12.0f) * 0.5f;
        di.width = 12.0f;
        di.height = 12.0f;
        di.source = iconSrc;
        di.tint = {255, 255, 255, 255};
        output.calls.push_back({DrawCallType::Icon, di, 0});
    }

    // Text
    std::string text;
    if (auto* p = ctx.FindProp("text")) {
        text = ctx.resolveString(p->value);
    }

    DrawText dt;
    float textOffsetX = ctx.FindProp("icon") ? 20.0f : 4.0f;
    dt.x = ctx.absX + textOffsetX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width - textOffsetX - 4.0f;
    dt.height = ctx.rect.height;
    dt.text = text;
    dt.fontSize = 12.0f;
    dt.color = {255, 255, 255, 255};
    dt.align = TextAlign::Center;
    output.calls.push_back({DrawCallType::Text, dt, 0});
}

// --- IndicatorDotHandler ---

void IndicatorDotHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float size = 8.0f;
    if (auto* p = ctx.FindProp("size")) {
        size = ctx.resolveFloat(p->value);
    }

    Color dotColor{0x80, 0x90, 0xA0, 255};  // accent-primary default
    if (auto* p = ctx.FindProp("color")) {
        dotColor = ctx.parseColor(ctx.resolveString(p->value));
    }

    DrawRect dot;
    dot.x = ctx.absX;
    dot.y = ctx.absY;
    dot.width = size;
    dot.height = size;
    dot.fillColor = dotColor;
    dot.borderRadius = size * 0.5f;  // circle
    output.calls.push_back({DrawCallType::Rect, dot, 0});
}

// --- AlertBannerHandler ---

void AlertBannerHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string severity = "warning";
    if (auto* p = ctx.FindProp("severity")) {
        severity = ctx.resolveString(p->value);
    }

    Color bannerColor;
    if (severity == "critical") {
        bannerColor = {0xFF, 0x17, 0x44, 255};
    } else {
        bannerColor = {0xFF, 0xC1, 0x07, 255};  // warning default
    }

    // Background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = bannerColor;
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    float contentX = ctx.absX + 4.0f;

    // Optional icon
    if (auto* p = ctx.FindProp("icon")) {
        std::string iconSrc = ctx.resolveString(p->value);
        DrawIcon di;
        di.x = contentX;
        di.y = ctx.absY + (ctx.rect.height - 16.0f) * 0.5f;
        di.width = 16.0f;
        di.height = 16.0f;
        di.source = iconSrc;
        di.tint = {255, 255, 255, 255};
        output.calls.push_back({DrawCallType::Icon, di, 0});
        contentX += 20.0f;
    }

    // Text
    std::string text;
    if (auto* p = ctx.FindProp("text")) {
        text = ctx.resolveString(p->value);
    }

    DrawText dt;
    dt.x = contentX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width - (contentX - ctx.absX) - 4.0f;
    dt.height = ctx.rect.height;
    dt.text = text;
    dt.fontSize = 14.0f;
    dt.color = {255, 255, 255, 255};
    output.calls.push_back({DrawCallType::Text, dt, 0});

    // Optional dismiss button
    bool dismissable = false;
    if (auto* p = ctx.FindProp("dismissable")) {
        auto val = ctx.resolveString(p->value);
        dismissable = (val == "true" || val == "1");
    }
    if (dismissable) {
        DrawIcon closeBtn;
        closeBtn.x = ctx.absX + ctx.rect.width - 20.0f;
        closeBtn.y = ctx.absY + (ctx.rect.height - 12.0f) * 0.5f;
        closeBtn.width = 12.0f;
        closeBtn.height = 12.0f;
        closeBtn.source = "icons/close.png";
        closeBtn.tint = {255, 255, 255, 255};
        output.calls.push_back({DrawCallType::Icon, closeBtn, 0});
    }
}

// --- CompletionRingHandler ---

void CompletionRingHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float value = 0.0f;
    float maxValue = 100.0f;
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("maxValue")) maxValue = ctx.resolveFloat(p->value);

    float ratio = (maxValue > 0.0f) ? std::clamp(value / maxValue, 0.0f, 1.0f) : 0.0f;

    float size = std::min(ctx.rect.width, ctx.rect.height);

    // Background circle
    DrawRect bgCircle;
    bgCircle.x = ctx.absX;
    bgCircle.y = ctx.absY;
    bgCircle.width = size;
    bgCircle.height = size;
    bgCircle.fillColor = {0x40, 0x40, 0x40, 255};
    bgCircle.borderRadius = size * 0.5f;
    output.calls.push_back({DrawCallType::Rect, bgCircle, 0});

    // Fill representing completion ratio
    DrawRect fill;
    fill.x = ctx.absX;
    fill.y = ctx.absY;
    fill.width = size * ratio;
    fill.height = size;
    fill.fillColor = {0x80, 0x90, 0xA0, 255};  // accent-primary
    fill.borderRadius = size * 0.5f;
    output.calls.push_back({DrawCallType::Rect, fill, 0});
}

}  // namespace ohui::dsl
