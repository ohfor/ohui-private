#include "AtomHandlers.h"

namespace ohui::dsl {

// --- LineHandler ---

void LineHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawLine dl;

    if (auto* p = ctx.FindProp("x1")) dl.x1 = ctx.resolveFloat(p->value);
    else dl.x1 = ctx.absX;

    if (auto* p = ctx.FindProp("y1")) dl.y1 = ctx.resolveFloat(p->value);
    else dl.y1 = ctx.absY;

    if (auto* p = ctx.FindProp("x2")) dl.x2 = ctx.resolveFloat(p->value);
    else dl.x2 = ctx.absX + ctx.rect.width;

    if (auto* p = ctx.FindProp("y2")) dl.y2 = ctx.resolveFloat(p->value);
    else dl.y2 = ctx.absY;

    if (auto* p = ctx.FindProp("color")) {
        dl.color = ctx.parseColor(ctx.resolveString(p->value));
    } else {
        dl.color = {0x40, 0x40, 0x40, 255};  // border-default fallback
    }
    if (auto* p = ctx.FindProp("thickness")) {
        dl.thickness = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("opacity")) {
        dl.opacity = ctx.resolveFloat(p->value);
    }

    output.calls.push_back({DrawCallType::Line, dl, 0});
}

// --- DividerHandler ---

void DividerHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawLine dl;
    dl.x1 = ctx.absX;
    dl.y1 = ctx.absY + ctx.rect.height * 0.5f;
    dl.x2 = ctx.absX + ctx.rect.width;
    dl.y2 = ctx.absY + ctx.rect.height * 0.5f;
    dl.thickness = 1.0f;

    if (auto* p = ctx.FindProp("color")) {
        dl.color = ctx.parseColor(ctx.resolveString(p->value));
    } else {
        dl.color = {0x40, 0x40, 0x40, 255};  // --color-border-default
    }
    if (auto* p = ctx.FindProp("thickness")) {
        dl.thickness = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("opacity")) {
        dl.opacity = ctx.resolveFloat(p->value);
    }

    output.calls.push_back({DrawCallType::Line, dl, 0});
}

// --- ViewportPlaceholderHandler ---

void ViewportPlaceholderHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Viewport components set up layout dimensions but emit no real draw calls.
    // Emit a placeholder DrawRect with zero opacity for layout verification in tests.
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
