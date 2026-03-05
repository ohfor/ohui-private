#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- Line ---
class LineHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- Divider (alias for horizontal line) ---
class DividerHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- Viewport placeholder ---
class ViewportPlaceholderHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
