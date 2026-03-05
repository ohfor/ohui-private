#pragma once

#include "ohui/dsl/USSRule.h"
#include "ohui/core/Result.h"

#include <string_view>

namespace ohui::dsl {

class USSParser {
public:
    Result<USSRuleSet> Parse(std::string_view input) const;
};

}  // namespace ohui::dsl
