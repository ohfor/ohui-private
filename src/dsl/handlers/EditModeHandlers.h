#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- WidgetBoundingBox: shows selection border + resize handles + label ---
class WidgetBoundingBoxHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- AlignmentGuide: emits DrawLines when widget edges align ---
class AlignmentGuideHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- EditModeToolbar: fixed-position panel with coordinate text + toggles ---
class EditModeToolbarHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- GridOverlay: grid of DrawLines at configurable spacing ---
class GridOverlayHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
