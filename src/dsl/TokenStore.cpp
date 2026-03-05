#include "ohui/dsl/TokenStore.h"
#include "ohui/dsl/USSParser.h"
#include "ohui/core/Log.h"

namespace ohui::dsl {

Result<void> TokenStore::LoadBaseTokens(std::string_view uss) {
    USSParser parser;
    auto result = parser.Parse(uss);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    m_baseTokens.clear();
    for (const auto& [key, value] : result->customProperties) {
        m_baseTokens[key] = value;
    }
    return {};
}

Result<void> TokenStore::LoadSkinTokens(std::string_view uss) {
    USSParser parser;
    auto result = parser.Parse(uss);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    m_skinTokens.clear();
    for (const auto& [key, value] : result->customProperties) {
        m_skinTokens[key] = value;
    }
    return {};
}

void TokenStore::ClearSkinTokens() {
    m_skinTokens.clear();
}

std::optional<std::string> TokenStore::Resolve(std::string_view key) const {
    auto keyStr = std::string(key);

    // Skin overrides base
    auto sit = m_skinTokens.find(keyStr);
    if (sit != m_skinTokens.end()) {
        return sit->second;
    }

    auto bit = m_baseTokens.find(keyStr);
    if (bit != m_baseTokens.end()) {
        return bit->second;
    }

    return std::nullopt;
}

std::string TokenStore::ResolveVar(std::string_view varExpr) const {
    // Expected forms:
    //   var(--name)
    //   var(--name, fallback)

    // Strip "var(" prefix and ")" suffix
    auto expr = varExpr;
    if (expr.size() < 6 || expr.substr(0, 4) != "var(") {
        return std::string(varExpr);
    }

    // Find matching closing paren (start after the opening paren of "var(")
    size_t depth = 0;
    size_t closePos = std::string_view::npos;
    for (size_t i = 4; i < expr.size(); ++i) {
        if (expr[i] == '(') ++depth;
        else if (expr[i] == ')') {
            if (depth == 0) {
                closePos = i;
                break;
            }
            --depth;
        }
    }

    if (closePos == std::string_view::npos) {
        return std::string(varExpr);
    }

    // Inner content between var( and )
    auto inner = expr.substr(4, closePos - 4);

    // Split on first comma (respecting nested parens)
    std::string_view tokenName;
    std::string_view fallback;
    bool hasFallback = false;

    size_t commaPos = std::string_view::npos;
    depth = 0;
    for (size_t i = 0; i < inner.size(); ++i) {
        if (inner[i] == '(') ++depth;
        else if (inner[i] == ')') { if (depth > 0) --depth; }
        else if (inner[i] == ',' && depth == 0) {
            commaPos = i;
            break;
        }
    }

    if (commaPos != std::string_view::npos) {
        tokenName = inner.substr(0, commaPos);
        fallback = inner.substr(commaPos + 1);
        hasFallback = true;

        // Trim whitespace from fallback
        while (!fallback.empty() && (fallback.front() == ' ' || fallback.front() == '\t')) {
            fallback = fallback.substr(1);
        }
        while (!fallback.empty() && (fallback.back() == ' ' || fallback.back() == '\t')) {
            fallback = fallback.substr(0, fallback.size() - 1);
        }
    } else {
        tokenName = inner;
    }

    // Trim whitespace from token name
    while (!tokenName.empty() && (tokenName.front() == ' ' || tokenName.front() == '\t')) {
        tokenName = tokenName.substr(1);
    }
    while (!tokenName.empty() && (tokenName.back() == ' ' || tokenName.back() == '\t')) {
        tokenName = tokenName.substr(0, tokenName.size() - 1);
    }

    auto resolved = Resolve(tokenName);
    if (resolved.has_value()) {
        return *resolved;
    }

    if (hasFallback) {
        return std::string(fallback);
    }

    ohui::log::debug("TokenStore: undefined token '{}' with no fallback", tokenName);
    return "";
}

std::string TokenStore::ResolveAllVars(std::string_view value) const {
    std::string result;
    size_t pos = 0;

    while (pos < value.size()) {
        // Look for "var("
        auto varPos = value.find("var(", pos);
        if (varPos == std::string_view::npos) {
            result.append(value.substr(pos));
            break;
        }

        // Append text before var()
        result.append(value.substr(pos, varPos - pos));

        // Find matching closing paren (start after the opening paren of "var(")
        size_t depth = 0;
        size_t closePos = std::string_view::npos;
        for (size_t i = varPos + 4; i < value.size(); ++i) {
            if (value[i] == '(') ++depth;
            else if (value[i] == ')') {
                if (depth == 0) {
                    closePos = i;
                    break;
                }
                --depth;
            }
        }

        if (closePos == std::string_view::npos) {
            // No matching close paren, append rest as-is
            result.append(value.substr(varPos));
            break;
        }

        auto varExpr = value.substr(varPos, closePos - varPos + 1);
        result.append(ResolveVar(varExpr));
        pos = closePos + 1;
    }

    return result;
}

bool TokenStore::HasToken(std::string_view key) const {
    auto keyStr = std::string(key);
    return m_skinTokens.contains(keyStr) || m_baseTokens.contains(keyStr);
}

size_t TokenStore::TokenCount() const {
    // Count unique keys across both maps
    std::unordered_map<std::string, bool> seen;
    for (const auto& [k, v] : m_baseTokens) seen[k] = true;
    for (const auto& [k, v] : m_skinTokens) seen[k] = true;
    return seen.size();
}

}  // namespace ohui::dsl
