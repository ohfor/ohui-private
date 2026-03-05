#include "EditModeHandlers.h"

namespace ohui::dsl {

// --- WidgetBoundingBoxHandler ---

void WidgetBoundingBoxHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    bool selected = false;
    if (auto* p = ctx.FindProp("selected")) {
        auto val = ctx.resolveString(p->value);
        selected = (val == "true" || val == "1");
    }

    Color borderColor = selected ? Color{0x80, 0x90, 0xA0, 255} : Color{0x40, 0x40, 0x40, 255};
    float borderWidth = selected ? 2.0f : 1.0f;

    // Border DrawRect around the widget (no fill, just border)
    DrawRect border;
    border.x = ctx.absX;
    border.y = ctx.absY;
    border.width = ctx.rect.width;
    border.height = ctx.rect.height;
    border.fillColor = {0, 0, 0, 0};  // transparent fill
    border.borderColor = borderColor;
    border.borderWidth = borderWidth;
    output.calls.push_back({DrawCallType::Rect, border, 0});

    // 4 corner handle DrawRects (6x6 squares)
    float handleSize = 6.0f;
    float halfHandle = 3.0f;

    // Top-left
    DrawRect tlHandle;
    tlHandle.x = ctx.absX - halfHandle;
    tlHandle.y = ctx.absY - halfHandle;
    tlHandle.width = handleSize;
    tlHandle.height = handleSize;
    tlHandle.fillColor = borderColor;
    output.calls.push_back({DrawCallType::Rect, tlHandle, 0});

    // Top-right
    DrawRect trHandle;
    trHandle.x = ctx.absX + ctx.rect.width - halfHandle;
    trHandle.y = ctx.absY - halfHandle;
    trHandle.width = handleSize;
    trHandle.height = handleSize;
    trHandle.fillColor = borderColor;
    output.calls.push_back({DrawCallType::Rect, trHandle, 0});

    // Bottom-left
    DrawRect blHandle;
    blHandle.x = ctx.absX - halfHandle;
    blHandle.y = ctx.absY + ctx.rect.height - halfHandle;
    blHandle.width = handleSize;
    blHandle.height = handleSize;
    blHandle.fillColor = borderColor;
    output.calls.push_back({DrawCallType::Rect, blHandle, 0});

    // Bottom-right
    DrawRect brHandle;
    brHandle.x = ctx.absX + ctx.rect.width - halfHandle;
    brHandle.y = ctx.absY + ctx.rect.height - halfHandle;
    brHandle.width = handleSize;
    brHandle.height = handleSize;
    brHandle.fillColor = borderColor;
    output.calls.push_back({DrawCallType::Rect, brHandle, 0});

    // Optional label above the widget
    if (auto* p = ctx.FindProp("label")) {
        std::string label = ctx.resolveString(p->value);
        DrawText dt;
        dt.x = ctx.absX;
        dt.y = ctx.absY - 16.0f;
        dt.width = ctx.rect.width;
        dt.height = 16.0f;
        dt.text = label;
        dt.fontSize = 10.0f;
        dt.color = borderColor;
        output.calls.push_back({DrawCallType::Text, dt, 0});
    }
}

// --- AlignmentGuideHandler ---

void AlignmentGuideHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    auto* axisProp = ctx.FindProp("axis");
    auto* positionProp = ctx.FindProp("position");

    if (!axisProp || !positionProp) return;

    std::string axis = ctx.resolveString(axisProp->value);
    float position = ctx.resolveFloat(positionProp->value);

    Color guideColor{0x80, 0x90, 0xA0, 128};

    DrawLine line;
    line.color = guideColor;
    line.thickness = 1.0f;

    if (axis == "horizontal") {
        line.x1 = ctx.absX;
        line.y1 = position;
        line.x2 = ctx.absX + ctx.rect.width;
        line.y2 = position;
    } else if (axis == "vertical") {
        line.x1 = position;
        line.y1 = ctx.absY;
        line.x2 = position;
        line.y2 = ctx.absY + ctx.rect.height;
    } else {
        return;  // unknown axis, emit nothing
    }

    output.calls.push_back({DrawCallType::Line, line, 0});
}

// --- EditModeToolbarHandler ---

void EditModeToolbarHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Panel background
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    bg.fillColor = {0x2A, 0x2A, 0x2A, 230};  // surface-raised
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Coordinate text
    if (auto* p = ctx.FindProp("coordText")) {
        std::string coordText = ctx.resolveString(p->value);
        DrawText dt;
        dt.x = ctx.absX + 8.0f;
        dt.y = ctx.absY;
        dt.width = ctx.rect.width * 0.5f;
        dt.height = ctx.rect.height;
        dt.text = coordText;
        dt.fontSize = 12.0f;
        dt.color = {0xE8, 0xE8, 0xE8, 255};
        output.calls.push_back({DrawCallType::Text, dt, 0});
    }

    // Snap toggle state
    bool snapEnabled = false;
    if (auto* p = ctx.FindProp("snapEnabled")) {
        auto val = ctx.resolveString(p->value);
        snapEnabled = (val == "true" || val == "1");
    }

    DrawText snapText;
    snapText.x = ctx.absX + ctx.rect.width * 0.5f;
    snapText.y = ctx.absY;
    snapText.width = ctx.rect.width * 0.5f;
    snapText.height = ctx.rect.height;
    snapText.text = snapEnabled ? "Snap: ON" : "Snap: OFF";
    snapText.fontSize = 12.0f;
    snapText.color = snapEnabled ? Color{0x80, 0x90, 0xA0, 255} : Color{0x60, 0x60, 0x60, 255};
    output.calls.push_back({DrawCallType::Text, snapText, 0});
}

// --- GridOverlayHandler ---

void GridOverlayHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    bool visible = true;
    if (auto* p = ctx.FindProp("visible")) {
        auto val = ctx.resolveString(p->value);
        visible = (val == "true" || val == "1");
    }

    if (!visible) return;

    float spacing = 16.0f;
    if (auto* p = ctx.FindProp("spacing")) {
        spacing = ctx.resolveFloat(p->value);
    }

    if (spacing <= 0.0f) return;

    Color gridColor{0x40, 0x40, 0x40, 64};

    // Vertical lines
    for (float x = spacing; x < ctx.rect.width; x += spacing) {
        DrawLine line;
        line.x1 = ctx.absX + x;
        line.y1 = ctx.absY;
        line.x2 = ctx.absX + x;
        line.y2 = ctx.absY + ctx.rect.height;
        line.color = gridColor;
        line.thickness = 1.0f;
        output.calls.push_back({DrawCallType::Line, line, 0});
    }

    // Horizontal lines
    for (float y = spacing; y < ctx.rect.height; y += spacing) {
        DrawLine line;
        line.x1 = ctx.absX;
        line.y1 = ctx.absY + y;
        line.x2 = ctx.absX + ctx.rect.width;
        line.y2 = ctx.absY + y;
        line.color = gridColor;
        line.thickness = 1.0f;
        output.calls.push_back({DrawCallType::Line, line, 0});
    }
}

}  // namespace ohui::dsl
