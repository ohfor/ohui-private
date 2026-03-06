#include "MessageFeedHandler.h"

#include <algorithm>
#include <cmath>

namespace ohui::dsl {

void MessageFeedHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    if (!m_stream) return;

    // Read properties from DSL
    int maxVisible = 5;
    float fadeInDuration = 0.3f;
    float fadeOutDuration = 0.5f;
    float defaultLifetime = 5.0f;
    float currentTime = 0.0f;
    float itemHeight = 24.0f;

    if (auto* p = ctx.FindProp("maxVisible"))
        maxVisible = static_cast<int>(ctx.resolveFloat(p->value));
    if (auto* p = ctx.FindProp("fadeInDuration"))
        fadeInDuration = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("fadeOutDuration"))
        fadeOutDuration = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("defaultLifetime"))
        defaultLifetime = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("currentTime"))
        currentTime = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("itemHeight"))
        itemHeight = ctx.resolveFloat(p->value);

    // Optional type filter
    std::string filterTypes;
    if (auto* p = ctx.FindProp("filterTypes"))
        filterTypes = ctx.resolveString(p->value);

    // Get recent messages from stream
    std::vector<const message::Message*> messages;
    if (filterTypes.empty()) {
        messages = m_stream->GetAllMessages(static_cast<size_t>(maxVisible * 2));
    } else {
        messages = m_stream->GetMessages(filterTypes, static_cast<size_t>(maxVisible * 2));
    }

    // Filter to non-expired and cap at maxVisible
    int drawn = 0;
    for (const auto* msg : messages) {
        if (drawn >= maxVisible) break;

        float lifetime = msg->lifetimeHint > 0.0f ? msg->lifetimeHint : defaultLifetime;
        float elapsed = currentTime - static_cast<float>(msg->realTime);

        // Skip expired messages
        if (elapsed >= lifetime) continue;

        // Compute per-message opacity
        float opacity = 1.0f;
        if (elapsed < fadeInDuration) {
            opacity = elapsed / fadeInDuration;
        } else if (elapsed > lifetime - fadeOutDuration) {
            opacity = (lifetime - elapsed) / fadeOutDuration;
        }
        opacity = std::clamp(opacity, 0.0f, 1.0f);

        float yOffset = ctx.absY + drawn * itemHeight;

        // Background rect
        DrawRect bg;
        bg.x = ctx.absX;
        bg.y = yOffset;
        bg.width = ctx.rect.width;
        bg.height = itemHeight;
        bg.fillColor = {0x2A, 0x2A, 0x2A, 180};
        bg.borderRadius = 2.0f;
        bg.opacity = opacity;
        output.calls.push_back({DrawCallType::Rect, bg, 0});

        // Text
        DrawText dt;
        dt.x = ctx.absX + 4.0f;
        dt.y = yOffset;
        dt.width = ctx.rect.width - 8.0f;
        dt.height = itemHeight;
        dt.text = msg->content;
        dt.fontSize = 13.0f;
        dt.color = {0xE8, 0xE8, 0xE8, 255};
        dt.opacity = opacity;
        output.calls.push_back({DrawCallType::Text, dt, 0});

        ++drawn;
    }
}

}  // namespace ohui::dsl
