#include "HUDHandlers.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ohui::dsl {

// --- ResourceBarHandler ---

void ResourceBarHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float value = 0.0f;
    float maxValue = 100.0f;
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("maxValue")) maxValue = ctx.resolveFloat(p->value);

    std::string resourceType = "default";
    if (auto* p = ctx.FindProp("resourceType")) {
        resourceType = ctx.resolveString(p->value);
    }

    float previousValue = -1.0f;
    bool hasPreviousValue = false;
    if (auto* p = ctx.FindProp("previousValue")) {
        previousValue = ctx.resolveFloat(p->value);
        hasPreviousValue = true;
    }

    float deltaTime = -1.0f;
    bool hasDeltaTime = false;
    if (auto* p = ctx.FindProp("deltaTime")) {
        deltaTime = ctx.resolveFloat(p->value);
        hasDeltaTime = true;
    }

    float ratio = (maxValue > 0.0f) ? std::clamp(value / maxValue, 0.0f, 1.0f) : 0.0f;

    // Background DrawRect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x1A, 0x1A, 0x1A, 230};
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Fill DrawRect
    Color fillColor;
    if (resourceType == "health") {
        fillColor = {0x8B, 0x00, 0x00, 255};
    } else if (resourceType == "stamina") {
        fillColor = {0x2E, 0x7D, 0x32, 255};
    } else if (resourceType == "magicka") {
        fillColor = {0x15, 0x65, 0xC0, 255};
    } else if (resourceType == "xp") {
        fillColor = {0xD4, 0xA0, 0x17, 255};
    } else {
        fillColor = {0x80, 0x90, 0xA0, 255};  // accent default
    }

    float fillWidth = ctx.rect.width * ratio;

    DrawRect fill;
    fill.x = ctx.absX;
    fill.y = ctx.absY;
    fill.width = fillWidth;
    fill.height = ctx.rect.height;
    fill.fillColor = fillColor;
    output.calls.push_back({DrawCallType::Rect, fill, 0});

    // Damage flash: previousValue > value
    if (hasPreviousValue && previousValue > value) {
        float prevRatio = (maxValue > 0.0f) ? std::clamp(previousValue / maxValue, 0.0f, 1.0f) : 0.0f;
        float flashWidth = (prevRatio - ratio) * ctx.rect.width;

        float flashOpacity = 1.0f;
        if (hasDeltaTime) {
            flashOpacity = std::max(0.0f, 1.0f - deltaTime * 2.0f);
        }

        DrawRect flash;
        flash.x = ctx.absX + fillWidth;
        flash.y = ctx.absY;
        flash.width = flashWidth;
        flash.height = ctx.rect.height;
        flash.fillColor = {0xFF, 0x00, 0x00, 180};
        flash.opacity = flashOpacity;
        output.calls.push_back({DrawCallType::Rect, flash, 0});
    }

    // Regen shimmer: previousValue < value
    if (hasPreviousValue && previousValue < value) {
        float shimmerWidth = fillWidth * 0.3f;
        float shimmerOpacity = 0.3f;
        if (hasDeltaTime) {
            shimmerOpacity = 0.3f * (1.0f - deltaTime);
        }

        DrawRect shimmer;
        shimmer.x = ctx.absX + fillWidth - shimmerWidth;
        shimmer.y = ctx.absY;
        shimmer.width = shimmerWidth;
        shimmer.height = ctx.rect.height;
        shimmer.fillColor = {0xFF, 0xFF, 0xFF, 40};
        shimmer.opacity = shimmerOpacity;
        output.calls.push_back({DrawCallType::Rect, shimmer, 0});
    }
}

// --- ShoutMeterHandler ---

void ShoutMeterHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float value = 0.0f;
    float maxValue = 100.0f;
    int segments = 3;
    if (auto* p = ctx.FindProp("value")) value = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("maxValue")) maxValue = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("segments")) segments = static_cast<int>(ctx.resolveFloat(p->value));

    float ratio = (maxValue > 0.0f) ? std::clamp(value / maxValue, 0.0f, 1.0f) : 0.0f;

    // Background DrawRect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x1A, 0x1A, 0x1A, 230};
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Fill DrawRect
    DrawRect fill;
    fill.x = ctx.absX;
    fill.y = ctx.absY;
    fill.width = ctx.rect.width * ratio;
    fill.height = ctx.rect.height;
    fill.fillColor = {0x80, 0x90, 0xA0, 255};  // accent primary
    output.calls.push_back({DrawCallType::Rect, fill, 0});

    // Segment dividers: (segments-1) vertical lines
    for (int i = 1; i < segments; ++i) {
        float dividerX = ctx.absX + (ctx.rect.width / static_cast<float>(segments)) * static_cast<float>(i);
        DrawLine divider;
        divider.x1 = dividerX;
        divider.y1 = ctx.absY;
        divider.x2 = dividerX;
        divider.y2 = ctx.absY + ctx.rect.height;
        divider.color = {0x1A, 0x1A, 0x1A, 255};
        divider.thickness = 2.0f;
        output.calls.push_back({DrawCallType::Line, divider, 0});
    }
}

// --- CompassRoseHandler ---

void CompassRoseHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float heading = 0.0f;
    if (auto* p = ctx.FindProp("heading")) heading = ctx.resolveFloat(p->value);

    float width = ctx.rect.width;

    float questMarkerBearing = -1.0f;
    bool hasQuestMarker = false;
    if (auto* p = ctx.FindProp("questMarkerBearing")) {
        questMarkerBearing = ctx.resolveFloat(p->value);
        hasQuestMarker = true;
    }

    // Cardinal markers: N=0, E=90, S=180, W=270
    struct Cardinal {
        const char* label;
        float degree;
    };
    Cardinal cardinals[] = {
        {"N", 0.0f},
        {"E", 90.0f},
        {"S", 180.0f},
        {"W", 270.0f}
    };

    float visibleRange = width / 4.0f;

    for (const auto& cardinal : cardinals) {
        float offset = cardinal.degree - heading;
        // Normalize to -180..180
        while (offset > 180.0f) offset -= 360.0f;
        while (offset < -180.0f) offset += 360.0f;

        if (std::abs(offset) < visibleRange) {
            float textX = ctx.absX + width / 2.0f + offset * (width / 180.0f);
            Color textColor;
            if (cardinal.label[0] == 'N') {
                textColor = {0xE8, 0xE8, 0xE8, 255};  // primary (highlighted)
            } else {
                textColor = {0xA0, 0xA0, 0xA0, 255};  // secondary
            }

            DrawText dt;
            dt.x = textX;
            dt.y = ctx.absY;
            dt.width = 14.0f;
            dt.height = ctx.rect.height;
            dt.text = cardinal.label;
            dt.fontSize = 14.0f;
            dt.color = textColor;
            dt.align = TextAlign::Center;
            output.calls.push_back({DrawCallType::Text, dt, 0});
        }
    }

    // Quest marker icon
    if (hasQuestMarker) {
        float offset = questMarkerBearing - heading;
        while (offset > 180.0f) offset -= 360.0f;
        while (offset < -180.0f) offset += 360.0f;

        if (std::abs(offset) < visibleRange) {
            float iconX = ctx.absX + width / 2.0f + offset * (width / 180.0f);
            DrawIcon di;
            di.x = iconX;
            di.y = ctx.absY + (ctx.rect.height - 12.0f) * 0.5f;
            di.width = 12.0f;
            di.height = 12.0f;
            di.source = "icons/quest-marker.png";
            di.tint = {255, 255, 255, 255};
            output.calls.push_back({DrawCallType::Icon, di, 0});
        }
    }
}

// --- DetectionMeterHandler ---

void DetectionMeterHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string state = "hidden";
    if (auto* p = ctx.FindProp("state")) {
        state = ctx.resolveString(p->value);
    }

    Color textColor;
    Color bgColor;
    std::string label;

    if (state == "caution") {
        textColor = {0xFF, 0xC1, 0x07, 255};
        bgColor = {0xFF, 0xC1, 0x07, 30};
        label = "CAUTION";
    } else if (state == "detected") {
        textColor = {0xFF, 0x17, 0x44, 255};
        bgColor = {0xFF, 0x17, 0x44, 30};
        label = "DETECTED";
    } else {
        // hidden
        textColor = {0x60, 0x60, 0x60, 255};
        bgColor = {0, 0, 0, 0};
        label = "HIDDEN";
    }

    // Background DrawRect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = bgColor;
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Text DrawText
    DrawText dt;
    dt.x = ctx.absX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width;
    dt.height = ctx.rect.height;
    dt.text = label;
    dt.fontSize = 14.0f;
    dt.color = textColor;
    dt.align = TextAlign::Center;
    output.calls.push_back({DrawCallType::Text, dt, 0});
}

// --- StealthEyeHandler ---

void StealthEyeHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float detectionLevel = 0.0f;
    if (auto* p = ctx.FindProp("detectionLevel")) {
        detectionLevel = ctx.resolveFloat(p->value);
    }

    detectionLevel = std::clamp(detectionLevel, 0.0f, 1.0f);

    // Eye outline icon
    DrawIcon di;
    di.x = ctx.absX;
    di.y = ctx.absY;
    di.width = ctx.rect.width;
    di.height = ctx.rect.height;
    di.source = "icons/stealth-eye.png";
    di.tint = {255, 255, 255, 255};
    output.calls.push_back({DrawCallType::Icon, di, 0});

    // Fill mask (only if detectionLevel > 0)
    if (detectionLevel > 0.0f) {
        DrawRect fillMask;
        fillMask.x = ctx.absX;
        fillMask.y = ctx.absY;
        fillMask.width = ctx.rect.width * detectionLevel;
        fillMask.height = ctx.rect.height;
        fillMask.fillColor = {0xFF, 0x17, 0x44, 100};
        output.calls.push_back({DrawCallType::Rect, fillMask, 0});
    }
}

// --- NotificationToastHandler ---

void NotificationToastHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string text;
    if (auto* p = ctx.FindProp("text")) {
        text = ctx.resolveString(p->value);
    }

    float lifetime = 3.0f;
    float elapsed = 0.0f;
    if (auto* p = ctx.FindProp("lifetime")) lifetime = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("elapsed")) elapsed = ctx.resolveFloat(p->value);

    // Opacity calculation
    float fadeInDuration = 0.3f;
    float fadeOutDuration = 0.5f;
    float opacity = 1.0f;

    if (elapsed < fadeInDuration) {
        opacity = elapsed / fadeInDuration;
    } else if (elapsed >= lifetime) {
        opacity = 0.0f;
    } else if (elapsed > lifetime - fadeOutDuration) {
        opacity = (lifetime - elapsed) / fadeOutDuration;
    }
    opacity = std::clamp(opacity, 0.0f, 1.0f);

    // Background DrawRect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x2A, 0x2A, 0x2A, 230};
    bg.borderRadius = 4.0f;
    bg.opacity = opacity;
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Text DrawText
    DrawText dt;
    dt.x = ctx.absX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width;
    dt.height = ctx.rect.height;
    dt.text = text;
    dt.fontSize = 14.0f;
    dt.color = {0xE8, 0xE8, 0xE8, 255};
    dt.opacity = opacity;
    output.calls.push_back({DrawCallType::Text, dt, 0});
}

}  // namespace ohui::dsl
