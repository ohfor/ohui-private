#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- StatusBadge: pill/rounded rect with optional icon + text ---
class StatusBadgeHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- IndicatorDot: small colored circle ---
class IndicatorDotHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- AlertBanner: full-width banner with icon + text + optional dismiss ---
class AlertBannerHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- CompletionRing: circle bg + filled segment ---
class CompletionRingHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
