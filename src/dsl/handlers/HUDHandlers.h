#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- ResourceBar: health/stamina/magicka bar with damage flash + regen shimmer ---
class ResourceBarHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- ShoutMeter: 3-segment cooldown bar ---
class ShoutMeterHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- CompassRose: horizontal compass strip with cardinal markers ---
class CompassRoseHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- DetectionMeter: Hidden/Caution/Detected state indicator ---
class DetectionMeterHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- StealthEye: eye icon with fill proportional to detection ---
class StealthEyeHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- NotificationToast: fading text notification ---
class NotificationToastHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
