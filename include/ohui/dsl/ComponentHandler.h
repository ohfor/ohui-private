#pragma once

#include "ohui/dsl/DrawCall.h"
#include "ohui/dsl/WidgetAST.h"
#include "ohui/layout/YogaWrapper.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::dsl {

class ComponentStateStore;

struct ComponentContext {
    const ComponentNode& node;
    layout::LayoutRect rect;
    float absX{};
    float absY{};

    std::function<std::string(const Expr&)> resolveString;
    std::function<float(const Expr&)> resolveFloat;
    std::function<Color(const std::string&)> parseColor;
    ComponentStateStore* stateStore{nullptr};

    // Find a property by name on the current node
    const PropertyAssignment* FindProp(const std::string& name) const {
        for (const auto& pa : node.properties) {
            if (pa.name == name) return &pa;
        }
        return nullptr;
    }
};

class IComponentHandler {
public:
    virtual ~IComponentHandler() = default;
    virtual void Emit(const ComponentContext& ctx, DrawCallList& output) = 0;
};

class ComponentRegistry {
public:
    void Register(std::string_view typeName, std::unique_ptr<IComponentHandler> handler);
    IComponentHandler* Get(std::string_view typeName) const;
    bool Has(std::string_view typeName) const;
    size_t Count() const;
    std::vector<std::string> GetTypeNames() const;

    void RegisterBuiltins();

private:
    std::unordered_map<std::string, std::unique_ptr<IComponentHandler>> m_handlers;
};

}  // namespace ohui::dsl
