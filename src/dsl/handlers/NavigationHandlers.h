#pragma once

#include "ohui/dsl/ComponentHandler.h"

namespace ohui::dsl {

class TabBarHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

class BreadcrumbHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

class PaginationHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;
};

}  // namespace ohui::dsl
