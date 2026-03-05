#pragma once

#include "ohui/dsl/WidgetAST.h"
#include "ohui/core/Result.h"

#include <string_view>

namespace ohui::dsl {

class WidgetParser {
public:
    Result<ParseResult> Parse(std::string_view input, std::string_view filename = "") const;
};

}  // namespace ohui::dsl
