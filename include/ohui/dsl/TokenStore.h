#pragma once

#include "ohui/core/Result.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ohui::dsl {

class TokenStore {
public:
    Result<void> LoadBaseTokens(std::string_view uss);
    Result<void> LoadSkinTokens(std::string_view uss);
    void ClearSkinTokens();

    std::optional<std::string> Resolve(std::string_view key) const;
    std::string ResolveVar(std::string_view varExpr) const;
    std::string ResolveAllVars(std::string_view value) const;

    bool HasToken(std::string_view key) const;
    size_t TokenCount() const;

private:
    std::unordered_map<std::string, std::string> m_baseTokens;
    std::unordered_map<std::string, std::string> m_skinTokens;
};

}  // namespace ohui::dsl
