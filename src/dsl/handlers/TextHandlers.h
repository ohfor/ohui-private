#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- Caption (defaults to font-size-sm and text-secondary) ---
class CaptionHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- RichText (inline formatting: <b>, <i>, <color=#RRGGBB>) ---
class RichTextHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- LoreText (defaults to font-family-lore, supports inline formatting) ---
class LoreTextHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- StatValue (label + value + optional unit) ---
class StatValueHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- StatDelta (label + old → new with color coding) ---
class StatDeltaHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- DeltaList (vertical stack of StatDeltas) ---
class DeltaListHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
