#include "ohui/dsl/USSParser.h"
#include "ohui/core/Log.h"

#include <unordered_set>

namespace ohui::dsl {

// --- Known property sets ---

static const std::unordered_set<std::string>& GetKnownProperties() {
    static const std::unordered_set<std::string> props{
        // Standard CSS/USS properties
        "color", "background-color", "border-color",
        "border-width", "border-left-width", "border-right-width",
        "border-top-width", "border-bottom-width",
        "border-radius", "border-top-left-radius", "border-top-right-radius",
        "border-bottom-left-radius", "border-bottom-right-radius",
        "opacity", "visibility",
        "font-size", "font-weight", "text-align", "word-wrap",
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "padding", "padding-left", "padding-right", "padding-top", "padding-bottom",
        "margin", "margin-left", "margin-right", "margin-top", "margin-bottom",
        "flex-direction", "justify-content", "align-items", "align-self",
        "align-content", "flex-grow", "flex-shrink", "flex-basis", "flex-wrap",
        "gap", "row-gap", "column-gap",
        "aspect-ratio", "display", "overflow", "position",
        "top", "left", "right", "bottom",
        "transition-property", "transition-duration",
        "transition-timing-function", "transition-delay",
        // OHUI extensions
        "-ohui-glow", "-ohui-edge-fade", "-ohui-z-index",
        "-ohui-sprite-sheet", "-ohui-sprite-columns", "-ohui-sprite-rows",
        "-ohui-sprite-index", "-ohui-radial-angle", "-ohui-radial-radius",
    };
    return props;
}

// --- Parser state ---

class ParserState {
public:
    explicit ParserState(std::string_view input) : m_input(input), m_pos(0) {}

    bool AtEnd() const { return m_pos >= m_input.size(); }
    char Peek() const { return AtEnd() ? '\0' : m_input[m_pos]; }
    char Advance() { return AtEnd() ? '\0' : m_input[m_pos++]; }
    size_t Pos() const { return m_pos; }

    void SkipWhitespace() {
        while (!AtEnd() && (m_input[m_pos] == ' ' || m_input[m_pos] == '\t' ||
                            m_input[m_pos] == '\n' || m_input[m_pos] == '\r')) {
            ++m_pos;
        }
    }

    void SkipWhitespaceAndComments() {
        while (true) {
            SkipWhitespace();
            if (SkipBlockComment()) continue;
            if (SkipLineComment()) continue;
            break;
        }
    }

    bool Match(char c) {
        if (Peek() == c) { ++m_pos; return true; }
        return false;
    }

    // Read an identifier: [a-zA-Z_-][a-zA-Z0-9_-]*
    std::string ReadIdentifier() {
        size_t start = m_pos;
        while (!AtEnd()) {
            char c = m_input[m_pos];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-') {
                ++m_pos;
            } else {
                break;
            }
        }
        return std::string(m_input.substr(start, m_pos - start));
    }

    // Skip to past the next occurrence of char
    void SkipPast(char c) {
        while (!AtEnd() && m_input[m_pos] != c) ++m_pos;
        if (!AtEnd()) ++m_pos;
    }

    // Read a property value, handling nested parens
    std::string ReadPropertyValue() {
        size_t start = m_pos;
        int parenDepth = 0;

        while (!AtEnd()) {
            char c = m_input[m_pos];
            if (c == '(') {
                ++parenDepth;
                ++m_pos;
            } else if (c == ')') {
                if (parenDepth > 0) {
                    --parenDepth;
                    ++m_pos;
                } else {
                    break;
                }
            } else if ((c == ';' || c == '}') && parenDepth == 0) {
                break;
            } else {
                ++m_pos;
            }
        }

        auto val = std::string(m_input.substr(start, m_pos - start));
        // Trim trailing whitespace
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t' ||
                                val.back() == '\n' || val.back() == '\r')) {
            val.pop_back();
        }
        return val;
    }

private:
    bool SkipBlockComment() {
        if (m_pos + 1 < m_input.size() && m_input[m_pos] == '/' && m_input[m_pos + 1] == '*') {
            m_pos += 2;
            while (m_pos + 1 < m_input.size()) {
                if (m_input[m_pos] == '*' && m_input[m_pos + 1] == '/') {
                    m_pos += 2;
                    return true;
                }
                ++m_pos;
            }
            m_pos = m_input.size();
            return true;
        }
        return false;
    }

    bool SkipLineComment() {
        if (m_pos + 1 < m_input.size() && m_input[m_pos] == '/' && m_input[m_pos + 1] == '/') {
            m_pos += 2;
            while (!AtEnd() && m_input[m_pos] != '\n') ++m_pos;
            return true;
        }
        return false;
    }

    std::string_view m_input;
    size_t m_pos;
};

// --- Pseudo-class parsing ---

static PseudoClass ParsePseudoClass(const std::string& name) {
    if (name == "hover") return PseudoClass::Hover;
    if (name == "focus") return PseudoClass::Focus;
    if (name == "active") return PseudoClass::Active;
    if (name == "disabled") return PseudoClass::Disabled;
    return PseudoClass::None;
}

// --- Selector parsing ---

static Selector ParseSelector(ParserState& state) {
    Selector sel;
    std::vector<SelectorPart> currentSegment;

    while (!state.AtEnd() && state.Peek() != '{') {
        char c = state.Peek();

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            // Whitespace = descendant combinator
            state.SkipWhitespaceAndComments();
            if (state.Peek() != '{' && !state.AtEnd() && !currentSegment.empty()) {
                sel.segments.push_back(std::move(currentSegment));
                currentSegment.clear();
            }
            continue;
        }

        if (c == '.') {
            state.Advance();
            SelectorPart part;
            part.type = SelectorType::Class;
            part.value = state.ReadIdentifier();
            // Check for pseudo-class
            if (state.Peek() == ':') {
                state.Advance();
                auto pseudo = state.ReadIdentifier();
                part.pseudoClass = ParsePseudoClass(pseudo);
                if (part.pseudoClass == PseudoClass::None) {
                    // If pseudo parsing failed, it may be standalone pseudo
                }
            }
            currentSegment.push_back(std::move(part));
        } else if (c == '#') {
            state.Advance();
            SelectorPart part;
            part.type = SelectorType::Id;
            part.value = state.ReadIdentifier();
            if (state.Peek() == ':') {
                state.Advance();
                auto pseudo = state.ReadIdentifier();
                part.pseudoClass = ParsePseudoClass(pseudo);
            }
            currentSegment.push_back(std::move(part));
        } else if (c == ':') {
            state.Advance();
            auto pseudo = state.ReadIdentifier();
            SelectorPart part;
            part.type = SelectorType::PseudoClass;
            part.value = pseudo;
            part.pseudoClass = ParsePseudoClass(pseudo);
            currentSegment.push_back(std::move(part));
        } else if (c == '*') {
            state.Advance();
            SelectorPart part;
            part.type = SelectorType::Universal;
            part.value = "*";
            currentSegment.push_back(std::move(part));
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            SelectorPart part;
            part.type = SelectorType::Type;
            part.value = state.ReadIdentifier();
            // Check for pseudo-class on type
            if (state.Peek() == ':') {
                state.Advance();
                auto pseudo = state.ReadIdentifier();
                part.pseudoClass = ParsePseudoClass(pseudo);
            }
            currentSegment.push_back(std::move(part));
        } else {
            // Unknown character in selector, skip
            state.Advance();
        }
    }

    if (!currentSegment.empty()) {
        sel.segments.push_back(std::move(currentSegment));
    }

    return sel;
}

// --- Rule body parsing ---

static void ParseRuleBody(ParserState& state, USSRule& rule, USSRuleSet& ruleSet) {
    const auto& known = GetKnownProperties();

    while (!state.AtEnd() && state.Peek() != '}') {
        state.SkipWhitespaceAndComments();
        if (state.Peek() == '}') break;

        // Read property name
        std::string name = state.ReadIdentifier();
        if (name.empty()) {
            // Try to recover by skipping to next ; or }
            state.SkipPast(';');
            continue;
        }

        // Handle --custom-property (ReadIdentifier already consumed the part after --)
        // Check if what we got starts with "--"
        // Actually, ReadIdentifier reads `-` characters, so --my-var would read as "--my-var"
        bool isCustomProperty = name.size() >= 2 && name[0] == '-' && name[1] == '-';

        state.SkipWhitespaceAndComments();
        if (!state.Match(':')) {
            state.SkipPast(';');
            continue;
        }
        state.SkipWhitespaceAndComments();

        std::string value = state.ReadPropertyValue();
        state.Match(';'); // consume optional semicolon

        if (isCustomProperty) {
            ruleSet.customProperties[name] = value;
        } else if (known.contains(name) || (name.size() > 6 && name.substr(0, 6) == "-ohui-")) {
            USSProperty prop;
            prop.name = name;
            prop.value = value;
            rule.properties[name] = std::move(prop);
        } else {
            ohui::log::debug("USS: unknown property '{}', skipped", name);
        }
    }
}

// --- Main parse ---

Result<USSRuleSet> USSParser::Parse(std::string_view input) const {
    ParserState state(input);
    USSRuleSet ruleSet;

    while (true) {
        state.SkipWhitespaceAndComments();
        if (state.AtEnd()) break;

        // Save position for error recovery
        size_t ruleStart = state.Pos();

        // Parse selector
        Selector selector = ParseSelector(state);

        if (!state.Match('{')) {
            // Malformed rule — skip to next }
            ohui::log::debug("USS: malformed rule at position {}, skipping", ruleStart);
            state.SkipPast('}');
            continue;
        }

        USSRule rule;
        rule.selector = std::move(selector);

        ParseRuleBody(state, rule, ruleSet);

        if (!state.Match('}')) {
            ohui::log::debug("USS: missing closing brace at position {}", state.Pos());
        }

        // Only add rule if it has properties (custom properties go to ruleSet directly)
        if (!rule.properties.empty()) {
            ruleSet.rules.push_back(std::move(rule));
        }
    }

    return ruleSet;
}

}  // namespace ohui::dsl
