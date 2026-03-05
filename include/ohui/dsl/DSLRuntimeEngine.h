#pragma once

#include "ohui/dsl/DrawCall.h"
#include "ohui/dsl/WidgetAST.h"
#include "ohui/dsl/TokenStore.h"
#include "ohui/binding/DataBindingEngine.h"
#include "ohui/widget/IViewportWidget.h"
#include "ohui/layout/YogaWrapper.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::dsl {

struct AnimationState {
    std::string property;
    float startValue{0};
    float endValue{0};
    float durationMs{0};
    float elapsedMs{0};
    EasingFunction easing{EasingFunction::Linear};
    bool active{false};
    float CurrentValue() const;
};

class DSLRuntimeEngine {
public:
    DSLRuntimeEngine(TokenStore& tokens, binding::DataBindingEngine& bindings);

    Result<void> LoadWidget(const WidgetDef& def);
    Result<void> ApplySkin(const SkinDef& skin);
    void ClearSkin(std::string_view widgetName);
    Result<void> BindInstance(const WidgetInstance& instance);

    Result<DrawCallList> Evaluate(std::string_view widgetName,
                                  const widget::ViewportRect& viewport,
                                  float deltaTime);
    bool HasWidget(std::string_view name) const;

private:
    struct InstanceBinding {
        std::unordered_map<std::string, Expr> propertyValues;
    };

    struct WidgetRuntime {
        const WidgetDef* def{nullptr};
        const SkinDef* skin{nullptr};
        InstanceBinding instance;
        std::vector<AnimationState> animations;
    };

    // Layout tree built per-evaluate, stored in a flat vector.
    // Index 0 is the root. Children follow in pre-order.
    struct LayoutEntry {
        const ComponentNode* component{nullptr};
        size_t layoutIndex{0};   // index into the LayoutNode vector
        size_t parentIndex{0};   // index of parent LayoutEntry (0 for root)
    };

    static size_t CountNodes(const ComponentNode& node);
    void AssignLayoutProperties(layout::LayoutNode& ln, const ComponentNode& node,
                                const WidgetRuntime& runtime);
    void EmitDrawCalls(const ComponentNode& node, const WidgetRuntime& runtime,
                       const layout::LayoutRect& rect, float parentX, float parentY,
                       DrawCallList& output,
                       std::vector<layout::LayoutNode>& nodes, size_t& nodeIndex);

    std::string ResolveExprToString(const Expr& expr, const WidgetRuntime& runtime);
    float ResolveExprToFloat(const Expr& expr, const WidgetRuntime& runtime);
    Color ParseColor(const std::string& value);

    TokenStore& m_tokens;
    binding::DataBindingEngine& m_bindings;
    std::unordered_map<std::string, WidgetRuntime> m_widgets;
};

}  // namespace ohui::dsl
