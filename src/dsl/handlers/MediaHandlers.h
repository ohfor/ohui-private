#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- Portrait: viewport placeholder with fixed square aspect ratio ---
class PortraitHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- CharacterViewport: 3D character viewport placeholder ---
class CharacterViewportHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- SceneViewport: 3D scene viewport placeholder ---
class SceneViewportHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- MapViewport: map viewport placeholder ---
class MapViewportHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
