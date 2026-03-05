#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- TimerBar (ValueBar with threshold color changes) ---
class TimerBarHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- CountBadge (pill/circle with count text) ---
class CountBadgeHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
