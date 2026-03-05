#include "ContainerHandlers.h"

#include <algorithm>
#include <cmath>

namespace ohui::dsl {

// --- SplitPanelHandler ---

void SplitPanelHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Background panel
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("fillColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    } else if (auto* p2 = ctx.FindProp("backgroundColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p2->value));
    }
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Divider line between panels
    std::string orientation = "horizontal";
    if (auto* p = ctx.FindProp("orientation")) {
        orientation = ctx.resolveString(p->value);
    }

    float ratio = 0.5f;
    if (auto* p = ctx.FindProp("ratio")) {
        ratio = std::clamp(ctx.resolveFloat(p->value), 0.0f, 1.0f);
    }

    DrawLine divider;
    Color dividerColor{0x40, 0x40, 0x40, 255};  // border-default
    if (auto* p = ctx.FindProp("dividerColor")) {
        dividerColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    divider.color = dividerColor;
    divider.thickness = 1.0f;

    if (orientation == "vertical") {
        float splitY = ctx.absY + ctx.rect.height * ratio;
        divider.x1 = ctx.absX;
        divider.y1 = splitY;
        divider.x2 = ctx.absX + ctx.rect.width;
        divider.y2 = splitY;
    } else {
        float splitX = ctx.absX + ctx.rect.width * ratio;
        divider.x1 = splitX;
        divider.y1 = ctx.absY;
        divider.x2 = splitX;
        divider.y2 = ctx.absY + ctx.rect.height;
    }
    output.calls.push_back({DrawCallType::Line, divider, 0});
}

// --- ScrollPanelHandler ---

void ScrollPanelHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Background with clip rect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("fillColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    } else if (auto* p2 = ctx.FindProp("backgroundColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p2->value));
    }

    ClipRect clip{ctx.absX, ctx.absY, ctx.rect.width, ctx.rect.height};
    output.calls.push_back({DrawCallType::Rect, bg, 0, clip});
}

// --- ModalHandler ---

void ModalHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Dimming overlay
    DrawRect overlay;
    overlay.x = 0;
    overlay.y = 0;
    overlay.width = 10000.0f;  // Full screen (renderer clips to viewport)
    overlay.height = 10000.0f;
    overlay.fillColor = {0, 0, 0, 0xCC};  // --color-surface-overlay
    overlay.opacity = 1.0f;
    output.calls.push_back({DrawCallType::Rect, overlay, 100});

    // Modal content background (centered)
    DrawRect content;
    content.x = ctx.absX;
    content.y = ctx.absY;
    content.width = ctx.rect.width;
    content.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("fillColor")) {
        content.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    } else {
        content.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};  // --color-surface-raised
    }
    if (auto* p = ctx.FindProp("borderRadius")) {
        content.borderRadius = ctx.resolveFloat(p->value);
    }
    output.calls.push_back({DrawCallType::Rect, content, 101});
}

// --- TooltipHandler ---

void TooltipHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    float anchorX = ctx.absX;
    float anchorY = ctx.absY;
    float tooltipW = ctx.rect.width;
    float tooltipH = ctx.rect.height;

    if (auto* p = ctx.FindProp("anchorX")) anchorX = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("anchorY")) anchorY = ctx.resolveFloat(p->value);

    float screenW = 1280.0f, screenH = 720.0f;
    if (auto* p = ctx.FindProp("screenWidth")) screenW = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("screenHeight")) screenH = ctx.resolveFloat(p->value);

    float gap = 4.0f;

    // Default: below anchor
    float tipX = anchorX;
    float tipY = anchorY + gap;

    // Reposition if too close to bottom
    if (tipY + tooltipH > screenH) {
        tipY = anchorY - tooltipH - gap;
    }
    // Reposition if too close to right
    if (tipX + tooltipW > screenW) {
        tipX = screenW - tooltipW;
    }
    // Clamp to screen
    tipX = std::max(0.0f, tipX);
    tipY = std::max(0.0f, tipY);

    DrawRect bg;
    bg.x = tipX;
    bg.y = tipY;
    bg.width = tooltipW;
    bg.height = tooltipH;
    bg.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};  // surface-raised
    bg.borderRadius = 4.0f;
    if (auto* p = ctx.FindProp("fillColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    output.calls.push_back({DrawCallType::Rect, bg, 200});

    // Text content
    if (auto* p = ctx.FindProp("text")) {
        DrawText dt;
        dt.x = tipX + 4.0f;
        dt.y = tipY + 2.0f;
        dt.width = tooltipW - 8.0f;
        dt.height = tooltipH - 4.0f;
        dt.text = ctx.resolveString(p->value);
        dt.fontSize = 12.0f;
        dt.color = {0xE8, 0xE8, 0xE8, 255};
        output.calls.push_back({DrawCallType::Text, dt, 200});
    }
}

// --- DrawerHandler ---

void DrawerHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    std::string edge = "left";
    bool open = false;

    if (auto* p = ctx.FindProp("edge")) edge = ctx.resolveString(p->value);
    if (auto* p = ctx.FindProp("open")) {
        auto val = ctx.resolveString(p->value);
        open = (val == "true" || val == "1");
    }

    if (!open) return;  // Closed drawer emits nothing

    float screenW = 1280.0f, screenH = 720.0f;
    if (auto* p = ctx.FindProp("screenWidth")) screenW = ctx.resolveFloat(p->value);
    if (auto* p = ctx.FindProp("screenHeight")) screenH = ctx.resolveFloat(p->value);

    float drawerW = ctx.rect.width;
    float drawerH = ctx.rect.height;

    float x = 0, y = 0;
    if (edge == "left") {
        x = 0;
        y = ctx.absY;
    } else if (edge == "right") {
        x = screenW - drawerW;
        y = ctx.absY;
    } else if (edge == "top") {
        x = ctx.absX;
        y = 0;
    } else if (edge == "bottom") {
        x = ctx.absX;
        y = screenH - drawerH;
    }

    DrawRect bg;
    bg.x = x;
    bg.y = y;
    bg.width = drawerW;
    bg.height = drawerH;
    bg.fillColor = {0x2A, 0x2A, 0x2A, 0xE6};  // surface-raised
    if (auto* p = ctx.FindProp("fillColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    output.calls.push_back({DrawCallType::Rect, bg, 50});
}

}  // namespace ohui::dsl
