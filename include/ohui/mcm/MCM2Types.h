#pragma once

#include "ohui/dsl/WidgetAST.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace ohui::mcm {

struct MCM2Version {
    uint16_t major{0};
    uint16_t minor{0};
    uint16_t patch{0};
};

enum class MCM2ControlType {
    Toggle, Slider, Dropdown, KeyBind, Colour, Text, Header, Empty
};

struct MCM2ToggleProps {
    bool defaultValue{false};
};

struct MCM2SliderProps {
    float minValue{0.0f};
    float maxValue{100.0f};
    float stepSize{1.0f};
    float defaultValue{0.0f};
    std::string formatString;
};

struct MCM2DropdownProps {
    std::vector<std::string> options;
    std::string defaultValue;
};

struct MCM2KeyBindProps {
    int32_t defaultValue{0};
};

struct MCM2ColourProps {
    int32_t defaultValue{0x000000};
};

struct MCM2TextProps {
    std::string defaultValue;
};

struct MCM2HeaderProps {};
struct MCM2EmptyProps {};

using MCM2ControlProps = std::variant<
    MCM2ToggleProps, MCM2SliderProps, MCM2DropdownProps,
    MCM2KeyBindProps, MCM2ColourProps, MCM2TextProps,
    MCM2HeaderProps, MCM2EmptyProps>;

struct MCM2ControlDef {
    MCM2ControlType type{MCM2ControlType::Empty};
    std::string id;
    std::string label;
    std::string description;
    std::string conditionExpr;
    std::string onChange;
    MCM2ControlProps properties;
    dsl::SourceLocation location;
};

struct MCM2SectionDef {
    std::string id;
    std::string displayName;
    bool collapsed{false};
    std::vector<MCM2ControlDef> controls;
    dsl::SourceLocation location;
};

struct MCM2PageDef {
    std::string id;
    std::string displayName;
    std::vector<MCM2SectionDef> sections;
    dsl::SourceLocation location;
};

struct MCM2Definition {
    std::string id;
    std::string displayName;
    MCM2Version version;
    std::vector<MCM2PageDef> pages;
    dsl::SourceLocation location;
};

struct MCM2ParseResult {
    MCM2Definition definition;
    std::vector<dsl::ParseDiagnostic> diagnostics;
    bool hasErrors{false};
};

}  // namespace ohui::mcm
