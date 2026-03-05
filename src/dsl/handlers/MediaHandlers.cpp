#include "MediaHandlers.h"

#include <algorithm>

namespace ohui::dsl {

// --- PortraitHandler ---

void PortraitHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Portrait uses square aspect ratio: min(width, height)
    float size = std::min(ctx.rect.width, ctx.rect.height);

    DrawRect dr;
    dr.x = ctx.absX;
    dr.y = ctx.absY;
    dr.width = size;
    dr.height = size;
    dr.opacity = 0.0f;
    dr.fillColor = {0, 0, 0, 0};
    output.calls.push_back({DrawCallType::Rect, dr, 0});
}

// --- CharacterViewportHandler ---

void CharacterViewportHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawRect dr;
    dr.x = ctx.absX;
    dr.y = ctx.absY;
    dr.width = ctx.rect.width;
    dr.height = ctx.rect.height;
    dr.opacity = 0.0f;
    dr.fillColor = {0, 0, 0, 0};
    output.calls.push_back({DrawCallType::Rect, dr, 0});
}

// --- SceneViewportHandler ---

void SceneViewportHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawRect dr;
    dr.x = ctx.absX;
    dr.y = ctx.absY;
    dr.width = ctx.rect.width;
    dr.height = ctx.rect.height;
    dr.opacity = 0.0f;
    dr.fillColor = {0, 0, 0, 0};
    output.calls.push_back({DrawCallType::Rect, dr, 0});
}

// --- MapViewportHandler ---

void MapViewportHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawRect dr;
    dr.x = ctx.absX;
    dr.y = ctx.absY;
    dr.width = ctx.rect.width;
    dr.height = ctx.rect.height;
    dr.opacity = 0.0f;
    dr.fillColor = {0, 0, 0, 0};
    output.calls.push_back({DrawCallType::Rect, dr, 0});
}

}  // namespace ohui::dsl
