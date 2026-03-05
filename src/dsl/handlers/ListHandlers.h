#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

// --- ListEntry (icon + primary text + secondary text + indicator dots) ---
class ListEntryHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- ListEntryCompact (smaller height, primary text + optional icon) ---
class ListEntryCompactHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- ScrollList (virtual scrolling: only emit visible items) ---
class ScrollListHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

// --- FacetedList (search + filter chips + scroll list) ---
class FacetedListHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
