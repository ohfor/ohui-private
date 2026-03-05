#pragma once

#include "ohui/core/Result.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ohui::dsl {

struct SourceLocation {
    std::string file;
    uint32_t line{1};
    uint32_t column{1};
};

struct VersionConstraint {
    uint16_t major{0};
    uint16_t minor{0};
};

enum class PropertyType { Float, Int, Bool, String, Color };

struct PropertyDecl {
    std::string name;
    PropertyType type;
    SourceLocation location;
};

enum class ExprKind { Literal, PropertyRef, Bind, Token, Var };

struct Expr {
    ExprKind kind;
    std::string value;
    std::string fallback;
    SourceLocation location;
};

enum class EasingFunction { Linear, EaseIn, EaseOut, EaseInOut };

struct AnimateBlock {
    std::string property;
    float durationMs{0.0f};
    EasingFunction easing{EasingFunction::Linear};
    SourceLocation location;
};

struct PropertyAssignment {
    std::string name;
    Expr value;
    SourceLocation location;
};

struct ComponentNode {
    std::string typeName;
    std::vector<PropertyAssignment> properties;
    std::vector<AnimateBlock> animations;
    std::vector<std::unique_ptr<ComponentNode>> children;
    SourceLocation location;
};

struct WidgetDef {
    std::string name;
    std::optional<VersionConstraint> requiresVersion;
    std::vector<PropertyDecl> propertyDecls;
    std::unique_ptr<ComponentNode> rootComponent;
    SourceLocation location;
};

struct TokenOverride {
    std::string name;
    std::string value;
};

struct SkinDef {
    std::string widgetName;
    std::string skinName;
    std::optional<VersionConstraint> requiresVersion;
    std::vector<TokenOverride> tokens;
    std::unique_ptr<ComponentNode> templateRoot;
    SourceLocation location;
};

struct InstanceProp {
    std::string name;
    Expr value;
    SourceLocation location;
};

struct UpdateDecl {
    std::string mode;
    std::vector<std::string> watchBindings;
};

struct WidgetInstance {
    std::string widgetTypeName;
    std::vector<InstanceProp> properties;
    std::optional<UpdateDecl> updateDecl;
    SourceLocation location;
};

struct WidgetFileAST {
    std::vector<WidgetDef> widgets;
    std::vector<SkinDef> skins;
    std::vector<WidgetInstance> instances;
};

struct ParseDiagnostic {
    ErrorCode code;
    std::string message;
    SourceLocation location;
};

struct ParseResult {
    WidgetFileAST ast;
    std::vector<ParseDiagnostic> diagnostics;
    bool hasErrors{false};
};

}  // namespace ohui::dsl
