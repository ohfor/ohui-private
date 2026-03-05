#include "ohui/dsl/DSLRuntimeEngine.h"
#include "ohui/core/Log.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace ohui::dsl {

// --- Easing functions ---

static float EaseIn(float t) { return t * t; }
static float EaseOut(float t) { return t * (2.0f - t); }
static float EaseInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

static float ApplyEasing(EasingFunction fn, float t) {
    switch (fn) {
        case EasingFunction::EaseIn: return EaseIn(t);
        case EasingFunction::EaseOut: return EaseOut(t);
        case EasingFunction::EaseInOut: return EaseInOut(t);
        default: return t;  // Linear
    }
}

// --- AnimationState ---

float AnimationState::CurrentValue() const {
    if (durationMs <= 0.0f) return endValue;
    float t = std::clamp(elapsedMs / durationMs, 0.0f, 1.0f);
    float eased = ApplyEasing(easing, t);
    return startValue + (endValue - startValue) * eased;
}

// --- DSLRuntimeEngine ---

DSLRuntimeEngine::DSLRuntimeEngine(TokenStore& tokens, binding::DataBindingEngine& bindings)
    : m_tokens(tokens), m_bindings(bindings) {}

Result<void> DSLRuntimeEngine::LoadWidget(const WidgetDef& def) {
    WidgetRuntime runtime;
    runtime.def = &def;

    if (def.rootComponent) {
        std::function<void(const ComponentNode&)> collectAnimations =
            [&](const ComponentNode& node) {
                for (const auto& anim : node.animations) {
                    AnimationState state;
                    state.property = anim.property;
                    state.durationMs = anim.durationMs;
                    state.easing = anim.easing;
                    state.active = false;
                    runtime.animations.push_back(std::move(state));
                }
                for (const auto& child : node.children) {
                    collectAnimations(*child);
                }
            };
        collectAnimations(*def.rootComponent);
    }

    m_widgets[def.name] = std::move(runtime);
    return {};
}

Result<void> DSLRuntimeEngine::ApplySkin(const SkinDef& skin) {
    auto it = m_widgets.find(skin.widgetName);
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found for skin: " + skin.widgetName});
    }

    it->second.skin = &skin;

    if (!skin.tokens.empty()) {
        std::string uss = ":root {\n";
        for (const auto& tok : skin.tokens) {
            uss += "  " + tok.name + ": " + tok.value + ";\n";
        }
        uss += "}\n";
        m_tokens.LoadSkinTokens(uss);
    }

    return {};
}

void DSLRuntimeEngine::ClearSkin(std::string_view widgetName) {
    auto it = m_widgets.find(std::string(widgetName));
    if (it != m_widgets.end()) {
        it->second.skin = nullptr;
        m_tokens.ClearSkinTokens();
    }
}

Result<void> DSLRuntimeEngine::BindInstance(const WidgetInstance& instance) {
    auto it = m_widgets.find(instance.widgetTypeName);
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found for instance: " + instance.widgetTypeName});
    }

    for (const auto& prop : instance.properties) {
        it->second.instance.propertyValues[prop.name] = prop.value;
    }
    return {};
}

size_t DSLRuntimeEngine::CountNodes(const ComponentNode& node) {
    size_t count = 1;
    for (const auto& child : node.children) {
        count += CountNodes(*child);
    }
    return count;
}

Result<DrawCallList> DSLRuntimeEngine::Evaluate(std::string_view widgetName,
                                                 const widget::ViewportRect& viewport,
                                                 float deltaTime) {
    auto it = m_widgets.find(std::string(widgetName));
    if (it == m_widgets.end()) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not loaded: " + std::string(widgetName)});
    }

    auto& runtime = it->second;

    // Advance animations
    for (auto& anim : runtime.animations) {
        if (anim.active) {
            anim.elapsedMs += deltaTime * 1000.0f;
            if (anim.elapsedMs >= anim.durationMs) {
                anim.active = false;
            }
        }
    }

    // Select component tree
    const ComponentNode* rootNode = nullptr;
    if (runtime.skin && runtime.skin->templateRoot) {
        rootNode = runtime.skin->templateRoot.get();
    } else if (runtime.def && runtime.def->rootComponent) {
        rootNode = runtime.def->rootComponent.get();
    }

    DrawCallList result;
    if (!rootNode) return result;

    // Build layout tree in a flat vector.
    // Pre-allocate all nodes so the vector never reallocates (InsertChild takes references).
    size_t totalNodes = CountNodes(*rootNode);
    std::vector<layout::LayoutNode> nodes(totalNodes);

    // Set root node dimensions to viewport
    nodes[0].SetWidth(viewport.width);
    nodes[0].SetHeight(viewport.height);
    AssignLayoutProperties(nodes[0], *rootNode, runtime);

    // Build tree recursively using a counter to assign indices
    size_t nextIndex = 1;
    std::function<void(const ComponentNode&, size_t)> buildTree =
        [&](const ComponentNode& component, size_t parentIdx) {
            for (size_t i = 0; i < component.children.size(); ++i) {
                size_t childIdx = nextIndex++;
                AssignLayoutProperties(nodes[childIdx], *component.children[i], runtime);
                nodes[parentIdx].InsertChild(nodes[childIdx], i);
                buildTree(*component.children[i], childIdx);
            }
        };
    buildTree(*rootNode, 0);

    // Calculate layout
    layout::CalculateLayout(nodes[0], viewport.width, viewport.height);

    // Emit draw calls by walking component tree in parallel with layout nodes
    size_t emitIndex = 0;
    EmitDrawCalls(*rootNode, runtime, nodes[0].GetLayoutRect(),
                  viewport.left, viewport.top, result, nodes, emitIndex);

    // Sort by z-index
    std::sort(result.calls.begin(), result.calls.end(),
              [](const DrawCall& a, const DrawCall& b) { return a.zIndex < b.zIndex; });

    return result;
}

bool DSLRuntimeEngine::HasWidget(std::string_view name) const {
    return m_widgets.contains(std::string(name));
}

// --- Layout properties ---

void DSLRuntimeEngine::AssignLayoutProperties(layout::LayoutNode& ln,
                                               const ComponentNode& node,
                                               const WidgetRuntime& runtime) {
    for (const auto& prop : node.properties) {
        if (prop.name == "width") {
            float f = ResolveExprToFloat(prop.value, runtime);
            if (f > 0) ln.SetWidth(f);
        } else if (prop.name == "height") {
            float f = ResolveExprToFloat(prop.value, runtime);
            if (f > 0) ln.SetHeight(f);
        } else if (prop.name == "flexDirection" || prop.name == "flex-direction") {
            auto val = ResolveExprToString(prop.value, runtime);
            if (val == "row") ln.SetFlexDirection(layout::FlexDirection::Row);
            else if (val == "column") ln.SetFlexDirection(layout::FlexDirection::Column);
        } else if (prop.name == "flexGrow" || prop.name == "flex-grow") {
            ln.SetFlexGrow(ResolveExprToFloat(prop.value, runtime));
        } else if (prop.name == "padding") {
            ln.SetPadding(layout::Edge::All, ResolveExprToFloat(prop.value, runtime));
        } else if (prop.name == "margin") {
            ln.SetMargin(layout::Edge::All, ResolveExprToFloat(prop.value, runtime));
        }
    }
}

// --- Draw call emission ---

void DSLRuntimeEngine::EmitDrawCalls(const ComponentNode& node,
                                      const WidgetRuntime& runtime,
                                      const layout::LayoutRect& rect,
                                      float parentX, float parentY,
                                      DrawCallList& output,
                                      std::vector<layout::LayoutNode>& nodes,
                                      size_t& nodeIndex) {
    float absX = parentX + rect.left;
    float absY = parentY + rect.top;

    auto findProp = [&](const std::string& name) -> const PropertyAssignment* {
        for (const auto& pa : node.properties) {
            if (pa.name == name) return &pa;
        }
        return nullptr;
    };

    if (node.typeName == "Panel") {
        DrawRect dr;
        dr.x = absX;
        dr.y = absY;
        dr.width = rect.width;
        dr.height = rect.height;

        if (auto* p = findProp("fillColor")) {
            dr.fillColor = ParseColor(ResolveExprToString(p->value, runtime));
        } else if (auto* p2 = findProp("backgroundColor")) {
            dr.fillColor = ParseColor(ResolveExprToString(p2->value, runtime));
        } else if (auto* p3 = findProp("color")) {
            dr.fillColor = ParseColor(ResolveExprToString(p3->value, runtime));
        }
        if (auto* p = findProp("opacity")) {
            dr.opacity = ResolveExprToFloat(p->value, runtime);
        }
        if (auto* p = findProp("borderWidth")) {
            dr.borderWidth = ResolveExprToFloat(p->value, runtime);
        }
        if (auto* p = findProp("borderColor")) {
            dr.borderColor = ParseColor(ResolveExprToString(p->value, runtime));
        }
        if (auto* p = findProp("borderRadius")) {
            dr.borderRadius = ResolveExprToFloat(p->value, runtime);
        }

        output.calls.push_back({DrawCallType::Rect, dr, 0});

    } else if (node.typeName == "Label") {
        DrawText dt;
        dt.x = absX;
        dt.y = absY;
        dt.width = rect.width;
        dt.height = rect.height;

        if (auto* p = findProp("text")) {
            dt.text = ResolveExprToString(p->value, runtime);
        }
        if (auto* p = findProp("fontSize")) {
            dt.fontSize = ResolveExprToFloat(p->value, runtime);
        }
        if (auto* p = findProp("fontFamily")) {
            dt.fontFamily = ResolveExprToString(p->value, runtime);
        }
        if (auto* p = findProp("color")) {
            dt.color = ParseColor(ResolveExprToString(p->value, runtime));
        }
        if (auto* p = findProp("textAlign")) {
            auto val = ResolveExprToString(p->value, runtime);
            if (val == "center") dt.align = TextAlign::Center;
            else if (val == "right") dt.align = TextAlign::Right;
        }
        if (auto* p = findProp("opacity")) {
            dt.opacity = ResolveExprToFloat(p->value, runtime);
        }

        output.calls.push_back({DrawCallType::Text, dt, 0});

    } else if (node.typeName == "ValueBar") {
        // Background rect
        DrawRect bg;
        bg.x = absX;
        bg.y = absY;
        bg.width = rect.width;
        bg.height = rect.height;
        if (auto* p = findProp("backgroundColor")) {
            bg.fillColor = ParseColor(ResolveExprToString(p->value, runtime));
        }
        output.calls.push_back({DrawCallType::Rect, bg, 0});

        // Fill rect
        float value = 0.0f;
        float maxValue = 100.0f;
        if (auto* p = findProp("value")) {
            value = ResolveExprToFloat(p->value, runtime);
        }
        if (auto* p = findProp("maxValue")) {
            maxValue = ResolveExprToFloat(p->value, runtime);
        }
        float ratio = (maxValue > 0.0f) ? std::clamp(value / maxValue, 0.0f, 1.0f) : 0.0f;

        DrawRect fill;
        fill.x = absX;
        fill.y = absY;
        fill.width = rect.width * ratio;
        fill.height = rect.height;
        if (auto* p = findProp("fillColor")) {
            fill.fillColor = ParseColor(ResolveExprToString(p->value, runtime));
        }
        if (auto* p = findProp("opacity")) {
            fill.opacity = ResolveExprToFloat(p->value, runtime);
        }
        output.calls.push_back({DrawCallType::Rect, fill, 0});

    } else if (node.typeName == "Icon") {
        DrawIcon di;
        di.x = absX;
        di.y = absY;
        di.width = rect.width;
        di.height = rect.height;
        if (auto* p = findProp("source")) {
            di.source = ResolveExprToString(p->value, runtime);
        }
        if (auto* p = findProp("tint")) {
            di.tint = ParseColor(ResolveExprToString(p->value, runtime));
        }
        if (auto* p = findProp("opacity")) {
            di.opacity = ResolveExprToFloat(p->value, runtime);
        }
        output.calls.push_back({DrawCallType::Icon, di, 0});

    } else if (node.typeName == "Image") {
        DrawImage dim;
        dim.x = absX;
        dim.y = absY;
        dim.width = rect.width;
        dim.height = rect.height;
        if (auto* p = findProp("source")) {
            dim.source = ResolveExprToString(p->value, runtime);
        }
        if (auto* p = findProp("opacity")) {
            dim.opacity = ResolveExprToFloat(p->value, runtime);
        }
        output.calls.push_back({DrawCallType::Image, dim, 0});
    }

    // Recurse into children
    for (const auto& child : node.children) {
        ++nodeIndex;
        auto childRect = nodes[nodeIndex].GetLayoutRect();
        EmitDrawCalls(*child, runtime, childRect, absX, absY, output, nodes, nodeIndex);
    }
}

// --- Expression resolution ---

std::string DSLRuntimeEngine::ResolveExprToString(const Expr& expr, const WidgetRuntime& runtime) {
    switch (expr.kind) {
        case ExprKind::Literal:
            return expr.value;

        case ExprKind::PropertyRef: {
            auto it = runtime.instance.propertyValues.find(expr.value);
            if (it != runtime.instance.propertyValues.end()) {
                return ResolveExprToString(it->second, runtime);
            }
            return expr.value;
        }

        case ExprKind::Bind: {
            const auto* val = m_bindings.GetCurrentValue(expr.value);
            if (!val) return "";
            return std::visit([](const auto& v) -> std::string {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) return v;
                else if constexpr (std::is_same_v<T, bool>) return v ? "true" : "false";
                else if constexpr (std::is_same_v<T, float>) return std::to_string(v);
                else if constexpr (std::is_same_v<T, int64_t>) return std::to_string(v);
                else return "";
            }, *val);
        }

        case ExprKind::Token: {
            auto resolved = m_tokens.Resolve(expr.value);
            return resolved.value_or("");
        }

        case ExprKind::Var: {
            std::string varExpr = "var(" + expr.value;
            if (!expr.fallback.empty()) {
                varExpr += ", " + expr.fallback;
            }
            varExpr += ")";
            return m_tokens.ResolveVar(varExpr);
        }
    }
    return "";
}

float DSLRuntimeEngine::ResolveExprToFloat(const Expr& expr, const WidgetRuntime& runtime) {
    std::string str = ResolveExprToString(expr, runtime);
    if (str.empty()) return 0.0f;

    size_t numEnd = 0;
    bool hasDot = false;
    if (!str.empty() && (str[0] == '-' || str[0] == '+')) numEnd = 1;
    while (numEnd < str.size()) {
        char c = str[numEnd];
        if (c >= '0' && c <= '9') { ++numEnd; continue; }
        if (c == '.' && !hasDot) { hasDot = true; ++numEnd; continue; }
        break;
    }
    if (numEnd == 0 || (numEnd == 1 && (str[0] == '-' || str[0] == '+'))) return 0.0f;

    try {
        return std::stof(str.substr(0, numEnd));
    } catch (...) {
        return 0.0f;
    }
}

// --- Color parsing ---

Color DSLRuntimeEngine::ParseColor(const std::string& value) {
    if (value.empty()) return {};

    if (value[0] == '#' && (value.size() == 7 || value.size() == 9)) {
        auto hexByte = [&](size_t pos) -> uint8_t {
            auto toNib = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
                if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
                if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
                return 0;
            };
            return static_cast<uint8_t>(toNib(value[pos]) * 16 + toNib(value[pos + 1]));
        };
        Color c;
        c.r = hexByte(1);
        c.g = hexByte(3);
        c.b = hexByte(5);
        c.a = (value.size() == 9) ? hexByte(7) : 255;
        return c;
    }

    if (value == "red") return {255, 0, 0, 255};
    if (value == "green") return {0, 128, 0, 255};
    if (value == "blue") return {0, 0, 255, 255};
    if (value == "white") return {255, 255, 255, 255};
    if (value == "black") return {0, 0, 0, 255};

    return {};
}

}  // namespace ohui::dsl
