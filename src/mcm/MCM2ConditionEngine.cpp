#include "ohui/mcm/MCM2ConditionEngine.h"
#include "ohui/core/Log.h"

namespace ohui::mcm {

// --- Condition expression lexer ---

namespace {

enum class CondTokenType {
    Identifier, Number, String, Bool,
    Eq, Neq, Gt, Lt, Gte, Lte,
    And, Or, Not,
    OpenParen, CloseParen,
    Dot, Comma,
    EndOfFile, Error
};

struct CondToken {
    CondTokenType type;
    std::string text;
    size_t pos{0};
};

class CondLexer {
public:
    explicit CondLexer(std::string_view input) : m_input(input) {}

    CondToken Next() {
        SkipWhitespace();
        if (AtEnd()) return {CondTokenType::EndOfFile, "", m_pos};

        size_t start = m_pos;
        char c = Peek();

        // Two-character operators
        if (c == '=' && LookAhead() == '=') { m_pos += 2; return {CondTokenType::Eq, "==", start}; }
        if (c == '!' && LookAhead() == '=') { m_pos += 2; return {CondTokenType::Neq, "!=", start}; }
        if (c == '>' && LookAhead() == '=') { m_pos += 2; return {CondTokenType::Gte, ">=", start}; }
        if (c == '<' && LookAhead() == '=') { m_pos += 2; return {CondTokenType::Lte, "<=", start}; }
        if (c == '&' && LookAhead() == '&') { m_pos += 2; return {CondTokenType::And, "&&", start}; }
        if (c == '|' && LookAhead() == '|') { m_pos += 2; return {CondTokenType::Or, "||", start}; }

        // Single-character operators
        if (c == '>') { ++m_pos; return {CondTokenType::Gt, ">", start}; }
        if (c == '<') { ++m_pos; return {CondTokenType::Lt, "<", start}; }
        if (c == '!') { ++m_pos; return {CondTokenType::Not, "!", start}; }
        if (c == '(') { ++m_pos; return {CondTokenType::OpenParen, "(", start}; }
        if (c == ')') { ++m_pos; return {CondTokenType::CloseParen, ")", start}; }
        if (c == '.') { ++m_pos; return {CondTokenType::Dot, ".", start}; }
        if (c == ',') { ++m_pos; return {CondTokenType::Comma, ",", start}; }

        // String literal
        if (c == '"') return ReadString(start);

        // Number
        if (IsDigit(c) || (c == '-' && m_pos + 1 < m_input.size() && IsDigit(m_input[m_pos + 1]))) {
            return ReadNumber(start);
        }

        // Identifier or bool
        if (IsIdentStart(c)) return ReadIdentifier(start);

        ++m_pos;
        return {CondTokenType::Error, std::string(1, c), start};
    }

private:
    bool AtEnd() const { return m_pos >= m_input.size(); }
    char Peek() const { return AtEnd() ? '\0' : m_input[m_pos]; }
    char LookAhead() const { return (m_pos + 1 < m_input.size()) ? m_input[m_pos + 1] : '\0'; }

    void SkipWhitespace() {
        while (!AtEnd() && (m_input[m_pos] == ' ' || m_input[m_pos] == '\t' ||
               m_input[m_pos] == '\n' || m_input[m_pos] == '\r')) {
            ++m_pos;
        }
    }

    static bool IsDigit(char c) { return c >= '0' && c <= '9'; }
    static bool IsIdentStart(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    static bool IsIdentChar(char c) { return IsIdentStart(c) || IsDigit(c); }

    CondToken ReadString(size_t start) {
        ++m_pos; // skip opening quote
        std::string value;
        while (!AtEnd() && m_input[m_pos] != '"') {
            if (m_input[m_pos] == '\\' && m_pos + 1 < m_input.size()) {
                ++m_pos;
                value += m_input[m_pos++];
            } else {
                value += m_input[m_pos++];
            }
        }
        if (!AtEnd()) ++m_pos; // skip closing quote
        return {CondTokenType::String, std::move(value), start};
    }

    CondToken ReadNumber(size_t start) {
        std::string value;
        if (m_input[m_pos] == '-') value += m_input[m_pos++];
        while (!AtEnd() && IsDigit(m_input[m_pos])) value += m_input[m_pos++];
        if (!AtEnd() && m_input[m_pos] == '.') {
            value += m_input[m_pos++];
            while (!AtEnd() && IsDigit(m_input[m_pos])) value += m_input[m_pos++];
        }
        return {CondTokenType::Number, std::move(value), start};
    }

    CondToken ReadIdentifier(size_t start) {
        std::string value;
        while (!AtEnd() && IsIdentChar(m_input[m_pos])) value += m_input[m_pos++];
        if (value == "true" || value == "false") {
            return {CondTokenType::Bool, std::move(value), start};
        }
        return {CondTokenType::Identifier, std::move(value), start};
    }

    std::string_view m_input;
    size_t m_pos{0};
};

// --- Condition expression parser (recursive descent) ---

class CondExprParser {
public:
    explicit CondExprParser(std::string_view input) : m_lexer(input) {
        Advance();
    }

    Result<CompiledCondition> Parse() {
        CompiledCondition compiled;
        auto node = ParseOrExpr(compiled);
        if (!node.has_value()) return std::unexpected(node.error());

        if (m_current.type != CondTokenType::EndOfFile) {
            return std::unexpected(Error{ErrorCode::ParseError,
                "unexpected token '" + m_current.text + "' in condition"});
        }

        compiled.root = std::move(*node);
        return compiled;
    }

private:
    void Advance() { m_current = m_lexer.Next(); }

    bool Check(CondTokenType type) const { return m_current.type == type; }
    bool Match(CondTokenType type) {
        if (Check(type)) { Advance(); return true; }
        return false;
    }

    // or_expr := and_expr ('||' and_expr)*
    Result<ConditionNode> ParseOrExpr(CompiledCondition& compiled) {
        auto left = ParseAndExpr(compiled);
        if (!left.has_value()) return left;

        while (Check(CondTokenType::Or)) {
            Advance();
            auto right = ParseAndExpr(compiled);
            if (!right.has_value()) return right;

            auto logical = std::make_unique<ConditionLogical>();
            logical->left = std::move(*left);
            logical->op = LogicalOp::Or;
            logical->right = std::move(*right);
            left = ConditionNode{std::move(logical)};
        }
        return left;
    }

    // and_expr := comparison ('&&' comparison)*
    Result<ConditionNode> ParseAndExpr(CompiledCondition& compiled) {
        auto left = ParseComparison(compiled);
        if (!left.has_value()) return left;

        while (Check(CondTokenType::And)) {
            Advance();
            auto right = ParseComparison(compiled);
            if (!right.has_value()) return right;

            auto logical = std::make_unique<ConditionLogical>();
            logical->left = std::move(*left);
            logical->op = LogicalOp::And;
            logical->right = std::move(*right);
            left = ConditionNode{std::move(logical)};
        }
        return left;
    }

    // comparison := unary (('==' | '!=' | '>=' | '<=' | '>' | '<') unary)?
    Result<ConditionNode> ParseComparison(CompiledCondition& compiled) {
        auto left = ParseUnary(compiled);
        if (!left.has_value()) return left;

        std::optional<ConditionOp> op;
        if (Check(CondTokenType::Eq)) { op = ConditionOp::Eq; Advance(); }
        else if (Check(CondTokenType::Neq)) { op = ConditionOp::Neq; Advance(); }
        else if (Check(CondTokenType::Gt)) { op = ConditionOp::Gt; Advance(); }
        else if (Check(CondTokenType::Lt)) { op = ConditionOp::Lt; Advance(); }
        else if (Check(CondTokenType::Gte)) { op = ConditionOp::Gte; Advance(); }
        else if (Check(CondTokenType::Lte)) { op = ConditionOp::Lte; Advance(); }

        if (op.has_value()) {
            auto right = ParseUnary(compiled);
            if (!right.has_value()) return right;

            auto cmp = std::make_unique<ConditionComparison>();
            cmp->left = std::move(*left);
            cmp->op = *op;
            cmp->right = std::move(*right);
            return ConditionNode{std::move(cmp)};
        }

        return left;
    }

    // unary := '!' unary | atom
    Result<ConditionNode> ParseUnary(CompiledCondition& compiled) {
        if (Check(CondTokenType::Not)) {
            Advance();
            auto operand = ParseUnary(compiled);
            if (!operand.has_value()) return operand;

            auto notNode = std::make_unique<ConditionNot>();
            notNode->operand = std::move(*operand);
            return ConditionNode{std::move(notNode)};
        }
        return ParseAtom(compiled);
    }

    // atom := '(' or_expr ')' | provider_call | identifier | literal
    Result<ConditionNode> ParseAtom(CompiledCondition& compiled) {
        // Parenthesized expression
        if (Check(CondTokenType::OpenParen)) {
            Advance();
            auto expr = ParseOrExpr(compiled);
            if (!expr.has_value()) return expr;
            if (!Match(CondTokenType::CloseParen)) {
                return std::unexpected(Error{ErrorCode::ParseError, "expected ')'"});
            }
            return expr;
        }

        // Bool literal
        if (Check(CondTokenType::Bool)) {
            bool val = (m_current.text == "true");
            Advance();
            return ConditionNode{ConditionLiteral{ConditionValue{val}}};
        }

        // Number literal
        if (Check(CondTokenType::Number)) {
            std::string numStr = m_current.text;
            Advance();
            if (numStr.find('.') != std::string::npos) {
                return ConditionNode{ConditionLiteral{ConditionValue{std::stod(numStr)}}};
            }
            return ConditionNode{ConditionLiteral{ConditionValue{static_cast<int64_t>(std::stoll(numStr))}}};
        }

        // String literal
        if (Check(CondTokenType::String)) {
            std::string val = m_current.text;
            Advance();
            return ConditionNode{ConditionLiteral{ConditionValue{std::move(val)}}};
        }

        // Identifier: could be a control ref or provider call
        if (Check(CondTokenType::Identifier)) {
            std::string name = m_current.text;
            Advance();

            // Check for dotted path: provider.key or provider.key("arg")
            if (Check(CondTokenType::Dot)) {
                Advance();
                if (!Check(CondTokenType::Identifier)) {
                    return std::unexpected(Error{ErrorCode::ParseError,
                        "expected identifier after '.'"});
                }

                // Collect full dotted key
                std::string key = m_current.text;
                Advance();

                while (Check(CondTokenType::Dot)) {
                    Advance();
                    if (Check(CondTokenType::Identifier)) {
                        key += "." + m_current.text;
                        Advance();
                    }
                }

                // Check for function call syntax: provider.key("arg")
                std::string argument;
                if (Check(CondTokenType::OpenParen)) {
                    Advance();
                    if (Check(CondTokenType::String)) {
                        argument = m_current.text;
                        Advance();
                    }
                    if (!Match(CondTokenType::CloseParen)) {
                        return std::unexpected(Error{ErrorCode::ParseError,
                            "expected ')' in provider call"});
                    }
                }

                std::string providerKey = key;
                if (!argument.empty()) {
                    providerKey += "(" + argument + ")";
                }
                compiled.referencedProviderKeys.push_back(name + "." + providerKey);

                return ConditionNode{ConditionProviderCall{name, key, argument}};
            }

            // Simple identifier — control reference
            compiled.referencedControlIds.push_back(name);
            return ConditionNode{ConditionControlRef{name}};
        }

        return std::unexpected(Error{ErrorCode::ParseError,
            "unexpected token '" + m_current.text + "' in condition expression"});
    }

    CondLexer m_lexer;
    CondToken m_current;
};

}  // anonymous namespace

// --- MCM2ConditionEngine ---

Result<CompiledCondition> MCM2ConditionEngine::ParseCondition(std::string_view expr) {
    CondExprParser parser(expr);
    return parser.Parse();
}

Result<void> MCM2ConditionEngine::CompileConditions(const MCM2Definition& definition) {
    m_conditions.clear();
    m_dependencyGraph.clear();
    m_cacheValid = false;

    for (const auto& page : definition.pages) {
        for (const auto& section : page.sections) {
            for (const auto& control : section.controls) {
                if (control.conditionExpr.empty()) continue;

                auto parsed = ParseCondition(control.conditionExpr);
                if (!parsed.has_value()) {
                    return std::unexpected(Error{ErrorCode::ParseError,
                        "failed to parse condition for '" + control.id + "': " +
                        parsed.error().message});
                }

                m_conditions[control.id] = std::move(*parsed);
            }
        }
    }

    BuildDependencyGraph();

    if (DetectCycles()) {
        // Mark cyclic conditions
        auto cyclicIds = GetCyclicControls();
        for (const auto& id : cyclicIds) {
            auto it = m_conditions.find(id);
            if (it != m_conditions.end()) {
                it->second.hasCycle = true;
            }
        }
    }

    return {};
}

void MCM2ConditionEngine::RegisterStateProvider(std::shared_ptr<IConditionStateProvider> provider) {
    if (provider) {
        m_providers[provider->ProviderPrefix()] = std::move(provider);
    }
}

void MCM2ConditionEngine::UnregisterStateProvider(std::string_view prefix) {
    m_providers.erase(std::string(prefix));
}

void MCM2ConditionEngine::SetControlValue(std::string_view controlId, const ConditionValue& value) {
    m_controlValues[std::string(controlId)] = value;
    m_cacheValid = false;
}

std::optional<ConditionValue> MCM2ConditionEngine::GetControlValue(std::string_view controlId) const {
    auto it = m_controlValues.find(std::string(controlId));
    if (it != m_controlValues.end()) return it->second;
    return std::nullopt;
}

ControlVisibility MCM2ConditionEngine::EvaluateVisibility(std::string_view controlId) const {
    auto it = m_conditions.find(std::string(controlId));
    if (it == m_conditions.end()) return ControlVisibility::Visible;

    if (it->second.hasCycle) return ControlVisibility::Hidden;

    auto val = Evaluate(it->second.root);
    return ToBool(val) ? ControlVisibility::Visible : ControlVisibility::Hidden;
}

std::vector<std::string> MCM2ConditionEngine::GetAffectedControls(
    std::string_view changedControlId) const {

    std::vector<std::string> affected;
    std::unordered_set<std::string> visited;
    std::vector<std::string> stack;
    stack.emplace_back(changedControlId);

    while (!stack.empty()) {
        auto current = std::move(stack.back());
        stack.pop_back();

        auto depIt = m_dependencyGraph.find(current);
        if (depIt == m_dependencyGraph.end()) continue;

        for (const auto& dependent : depIt->second) {
            if (visited.insert(dependent).second) {
                affected.push_back(dependent);
                stack.push_back(dependent);
            }
        }
    }
    return affected;
}

std::vector<std::string> MCM2ConditionEngine::ReEvaluateAll() const {
    std::vector<std::string> changed;

    for (const auto& [controlId, condition] : m_conditions) {
        auto oldVis = ControlVisibility::Visible;
        auto cacheIt = m_visibilityCache.find(controlId);
        if (cacheIt != m_visibilityCache.end()) oldVis = cacheIt->second;

        auto newVis = EvaluateVisibility(controlId);
        m_visibilityCache[controlId] = newVis;

        if (oldVis != newVis) {
            changed.push_back(controlId);
        }
    }

    m_cacheValid = true;
    return changed;
}

bool MCM2ConditionEngine::HasCondition(std::string_view controlId) const {
    return m_conditions.contains(std::string(controlId));
}

bool MCM2ConditionEngine::HasCycle() const {
    return DetectCycles();
}

std::vector<std::string> MCM2ConditionEngine::GetCyclicControls() const {
    std::vector<std::string> cyclic;

    // DFS with White/Gray/Black coloring
    enum class Color { White, Gray, Black };
    std::unordered_map<std::string, Color> color;

    for (const auto& [id, _] : m_conditions) {
        color[id] = Color::White;
    }

    // Build forward dependency map: controlId -> controls it depends on
    std::unordered_map<std::string, std::vector<std::string>> forwardDeps;
    for (const auto& [id, condition] : m_conditions) {
        for (const auto& ref : condition.referencedControlIds) {
            forwardDeps[id].push_back(ref);
        }
    }

    std::unordered_set<std::string> cyclicSet;

    std::function<bool(const std::string&, std::vector<std::string>&)> dfs =
        [&](const std::string& node, std::vector<std::string>& path) -> bool {
        auto cit = color.find(node);
        if (cit == color.end()) return false; // not a conditioned control

        if (cit->second == Color::Gray) {
            // Found cycle — mark all nodes in the cycle
            for (auto it = path.rbegin(); it != path.rend(); ++it) {
                cyclicSet.insert(*it);
                if (*it == node) break;
            }
            cyclicSet.insert(node);
            return true;
        }

        if (cit->second == Color::Black) return false;

        cit->second = Color::Gray;
        path.push_back(node);

        auto depIt = forwardDeps.find(node);
        if (depIt != forwardDeps.end()) {
            for (const auto& dep : depIt->second) {
                dfs(dep, path);
            }
        }

        path.pop_back();
        cit->second = Color::Black;
        return false;
    };

    for (const auto& [id, _] : m_conditions) {
        if (color[id] == Color::White) {
            std::vector<std::string> path;
            dfs(id, path);
        }
    }

    cyclic.assign(cyclicSet.begin(), cyclicSet.end());
    return cyclic;
}

Result<ConditionValue> MCM2ConditionEngine::EvaluateExpression(std::string_view expr) const {
    auto parsed = ParseCondition(expr);
    if (!parsed.has_value()) return std::unexpected(parsed.error());
    return Evaluate(parsed->root);
}

void MCM2ConditionEngine::BuildDependencyGraph() {
    m_dependencyGraph.clear();

    // Build reverse map: if control B's condition references control A,
    // then A -> B in the dependency graph (changing A affects B)
    for (const auto& [controlId, condition] : m_conditions) {
        for (const auto& refId : condition.referencedControlIds) {
            m_dependencyGraph[refId].insert(controlId);
        }
    }
}

bool MCM2ConditionEngine::DetectCycles() const {
    enum class Color { White, Gray, Black };
    std::unordered_map<std::string, Color> color;

    for (const auto& [id, _] : m_conditions) {
        color[id] = Color::White;
    }

    std::unordered_map<std::string, std::vector<std::string>> forwardDeps;
    for (const auto& [id, condition] : m_conditions) {
        for (const auto& ref : condition.referencedControlIds) {
            forwardDeps[id].push_back(ref);
        }
    }

    std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
        auto cit = color.find(node);
        if (cit == color.end()) return false;
        if (cit->second == Color::Gray) return true;
        if (cit->second == Color::Black) return false;

        cit->second = Color::Gray;

        auto depIt = forwardDeps.find(node);
        if (depIt != forwardDeps.end()) {
            for (const auto& dep : depIt->second) {
                if (dfs(dep)) return true;
            }
        }

        cit->second = Color::Black;
        return false;
    };

    for (const auto& [id, _] : m_conditions) {
        if (color[id] == Color::White) {
            if (dfs(id)) return true;
        }
    }

    return false;
}

ConditionValue MCM2ConditionEngine::Evaluate(const ConditionNode& node) const {
    return std::visit([this](const auto& n) -> ConditionValue {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, ConditionLiteral>) {
            return n.value;
        }
        else if constexpr (std::is_same_v<T, ConditionControlRef>) {
            auto it = m_controlValues.find(n.controlId);
            if (it != m_controlValues.end()) return it->second;
            return ConditionValue{false}; // fail-safe
        }
        else if constexpr (std::is_same_v<T, ConditionProviderCall>) {
            auto pIt = m_providers.find(n.providerPrefix);
            if (pIt == m_providers.end()) return ConditionValue{false};

            std::string resolveKey = n.key;
            if (!n.argument.empty()) {
                resolveKey += "(" + n.argument + ")";
            }

            auto val = pIt->second->Resolve(resolveKey);
            if (val.has_value()) return *val;
            return ConditionValue{false}; // fail-safe
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<ConditionNot>>) {
            auto val = Evaluate(n->operand);
            return ConditionValue{!ToBool(val)};
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<ConditionComparison>>) {
            auto left = Evaluate(n->left);
            auto right = Evaluate(n->right);
            int cmp = CompareValues(left, right);

            switch (n->op) {
                case ConditionOp::Eq:  return ConditionValue{cmp == 0};
                case ConditionOp::Neq: return ConditionValue{cmp != 0};
                case ConditionOp::Gt:  return ConditionValue{cmp > 0};
                case ConditionOp::Lt:  return ConditionValue{cmp < 0};
                case ConditionOp::Gte: return ConditionValue{cmp >= 0};
                case ConditionOp::Lte: return ConditionValue{cmp <= 0};
            }
            return ConditionValue{false};
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<ConditionLogical>>) {
            auto left = Evaluate(n->left);

            if (n->op == LogicalOp::And) {
                if (!ToBool(left)) return ConditionValue{false};
                auto right = Evaluate(n->right);
                return ConditionValue{ToBool(right)};
            } else {
                if (ToBool(left)) return ConditionValue{true};
                auto right = Evaluate(n->right);
                return ConditionValue{ToBool(right)};
            }
        }
        else {
            return ConditionValue{false};
        }
    }, node);
}

bool MCM2ConditionEngine::ToBool(const ConditionValue& val) {
    return std::visit([](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) return v;
        else if constexpr (std::is_same_v<T, int64_t>) return v != 0;
        else if constexpr (std::is_same_v<T, double>) return v != 0.0;
        else if constexpr (std::is_same_v<T, std::string>) return !v.empty();
        else return false;
    }, val);
}

int MCM2ConditionEngine::CompareValues(const ConditionValue& a, const ConditionValue& b) {
    // Convert both to a common type for comparison
    // If both are the same type, compare directly
    // Otherwise try numeric comparison, then string comparison

    return std::visit([&b](const auto& aVal) -> int {
        using A = std::decay_t<decltype(aVal)>;

        return std::visit([&aVal](const auto& bVal) -> int {
            using B = std::decay_t<decltype(bVal)>;

            if constexpr (std::is_same_v<A, B>) {
                // Same type — direct comparison
                if constexpr (std::is_same_v<A, bool>) {
                    return static_cast<int>(aVal) - static_cast<int>(bVal);
                }
                else if constexpr (std::is_same_v<A, int64_t>) {
                    if (aVal < bVal) return -1;
                    if (aVal > bVal) return 1;
                    return 0;
                }
                else if constexpr (std::is_same_v<A, double>) {
                    if (aVal < bVal) return -1;
                    if (aVal > bVal) return 1;
                    return 0;
                }
                else if constexpr (std::is_same_v<A, std::string>) {
                    return aVal.compare(bVal);
                }
                else {
                    return 0;
                }
            }
            // Cross-type numeric comparisons
            else if constexpr ((std::is_same_v<A, int64_t> && std::is_same_v<B, double>) ||
                               (std::is_same_v<A, double> && std::is_same_v<B, int64_t>)) {
                double da = static_cast<double>(aVal);
                double db = static_cast<double>(bVal);
                if (da < db) return -1;
                if (da > db) return 1;
                return 0;
            }
            else if constexpr (std::is_same_v<A, bool> && (std::is_same_v<B, int64_t> || std::is_same_v<B, double>)) {
                double da = aVal ? 1.0 : 0.0;
                double db = static_cast<double>(bVal);
                if (da < db) return -1;
                if (da > db) return 1;
                return 0;
            }
            else if constexpr ((std::is_same_v<A, int64_t> || std::is_same_v<A, double>) && std::is_same_v<B, bool>) {
                double da = static_cast<double>(aVal);
                double db = bVal ? 1.0 : 0.0;
                if (da < db) return -1;
                if (da > db) return 1;
                return 0;
            }
            else {
                // Incompatible types — not equal
                return -1;
            }
        }, b);
    }, a);
}

}  // namespace ohui::mcm
