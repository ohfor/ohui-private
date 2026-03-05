#pragma once

#include "ohui/dsl/Selector.h"
#include "ohui/dsl/USSProperty.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ohui::dsl {

struct USSRule {
    Selector selector;
    std::unordered_map<std::string, USSProperty> properties;
};

struct USSRuleSet {
    std::vector<USSRule> rules;  // cascade order = source order
    std::unordered_map<std::string, std::string> customProperties;  // --token declarations
};

}  // namespace ohui::dsl
