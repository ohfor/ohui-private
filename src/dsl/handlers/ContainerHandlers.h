#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- SplitPanel (split children with ratio) ---
class SplitPanelHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- ScrollPanel (clipped scrollable area) ---
class ScrollPanelHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- Modal (overlay + centered content) ---
class ModalHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- Tooltip (repositioning based on anchor/screen bounds) ---
class TooltipHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- Drawer (slide-in panel from edge) ---
class DrawerHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
