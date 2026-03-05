#pragma once

#include "ohui/mcm/MCM2Types.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>

namespace ohui::mcm {

class MCM2DefinitionParser {
public:
    Result<MCM2ParseResult> Parse(std::string_view input,
                                   std::string_view filename = "") const;
    static std::string Serialize(const MCM2Definition& def);
};

}  // namespace ohui::mcm
