#include "ohui/mcm/MCM2DefinitionParser.h"
#include "ohui/core/Log.h"

#include <charconv>
#include <unordered_set>

namespace ohui::mcm {

// --- Token types ---

enum class TokenType {
    Identifier,
    StringLiteral,
    NumberLiteral,
    BoolLiteral,
    OpenBrace,
    CloseBrace,
    OpenBracket,
    CloseBracket,
    Colon,
    Comma,
    EndOfFile,
    Error,
};

struct Token {
    TokenType type;
    std::string text;
    dsl::SourceLocation location;
};

// --- Lexer ---

class Lexer {
public:
    Lexer(std::string_view input, std::string_view filename)
        : m_input(input), m_filename(filename) {}

    Token Next() {
        SkipWhitespaceAndComments();

        if (AtEnd()) return MakeToken(TokenType::EndOfFile, "");

        uint32_t startLine = m_line;
        uint32_t startCol = m_column;
        char c = Peek();

        if (c == '{') { Advance(); return {TokenType::OpenBrace, "{", {std::string(m_filename), startLine, startCol}}; }
        if (c == '}') { Advance(); return {TokenType::CloseBrace, "}", {std::string(m_filename), startLine, startCol}}; }
        if (c == '[') { Advance(); return {TokenType::OpenBracket, "[", {std::string(m_filename), startLine, startCol}}; }
        if (c == ']') { Advance(); return {TokenType::CloseBracket, "]", {std::string(m_filename), startLine, startCol}}; }
        if (c == ':') { Advance(); return {TokenType::Colon, ":", {std::string(m_filename), startLine, startCol}}; }
        if (c == ',') { Advance(); return {TokenType::Comma, ",", {std::string(m_filename), startLine, startCol}}; }
        if (c == '"') return ReadStringLiteral(startLine, startCol);
        // Hex literal: 0x... (must check before general digit path)
        if (c == '0' && m_pos + 1 < m_input.size() &&
            (m_input[m_pos + 1] == 'x' || m_input[m_pos + 1] == 'X')) {
            return ReadHexLiteral(startLine, startCol);
        }
        if (IsDigit(c) || (c == '-' && m_pos + 1 < m_input.size() && IsDigit(m_input[m_pos + 1]))) {
            return ReadNumberLiteral(startLine, startCol);
        }
        if (IsIdentStart(c)) return ReadIdentifierOrKeyword(startLine, startCol);

        Advance();
        return {TokenType::Error, std::string(1, c), {std::string(m_filename), startLine, startCol}};
    }

private:
    bool AtEnd() const { return m_pos >= m_input.size(); }
    char Peek() const { return AtEnd() ? '\0' : m_input[m_pos]; }

    char Advance() {
        if (AtEnd()) return '\0';
        char c = m_input[m_pos++];
        if (c == '\n') { ++m_line; m_column = 1; }
        else { ++m_column; }
        return c;
    }

    void SkipWhitespaceAndComments() {
        while (true) {
            while (!AtEnd() && (Peek() == ' ' || Peek() == '\t' || Peek() == '\n' || Peek() == '\r')) {
                Advance();
            }
            if (m_pos + 1 < m_input.size() && m_input[m_pos] == '/' && m_input[m_pos + 1] == '*') {
                Advance(); Advance();
                while (m_pos + 1 < m_input.size()) {
                    if (m_input[m_pos] == '*' && m_input[m_pos + 1] == '/') {
                        Advance(); Advance();
                        break;
                    }
                    Advance();
                }
                continue;
            }
            if (m_pos + 1 < m_input.size() && m_input[m_pos] == '/' && m_input[m_pos + 1] == '/') {
                Advance(); Advance();
                while (!AtEnd() && Peek() != '\n') Advance();
                continue;
            }
            break;
        }
    }

    Token MakeToken(TokenType type, std::string text) const {
        return {type, std::move(text), {std::string(m_filename), m_line, m_column}};
    }

    static bool IsDigit(char c) { return c >= '0' && c <= '9'; }
    static bool IsHexDigit(char c) {
        return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }
    static bool IsIdentStart(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    static bool IsIdentChar(char c) { return IsIdentStart(c) || IsDigit(c) || c == '.'; }

    Token ReadStringLiteral(uint32_t startLine, uint32_t startCol) {
        Advance(); // skip opening quote
        std::string value;
        while (!AtEnd() && Peek() != '"') {
            if (Peek() == '\\') {
                Advance();
                if (!AtEnd()) {
                    char esc = Advance();
                    switch (esc) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        case '"': value += '"'; break;
                        case '\\': value += '\\'; break;
                        default: value += esc; break;
                    }
                }
            } else {
                value += Advance();
            }
        }
        if (!AtEnd()) Advance(); // skip closing quote
        return {TokenType::StringLiteral, std::move(value), {std::string(m_filename), startLine, startCol}};
    }

    Token ReadNumberLiteral(uint32_t startLine, uint32_t startCol) {
        std::string value;
        if (Peek() == '-') value += Advance();
        while (!AtEnd() && IsDigit(Peek())) value += Advance();
        if (!AtEnd() && Peek() == '.') {
            value += Advance();
            while (!AtEnd() && IsDigit(Peek())) value += Advance();
        }
        return {TokenType::NumberLiteral, std::move(value), {std::string(m_filename), startLine, startCol}};
    }

    Token ReadHexLiteral(uint32_t startLine, uint32_t startCol) {
        std::string value;
        value += Advance(); // '0'
        value += Advance(); // 'x' or 'X'
        while (!AtEnd() && IsHexDigit(Peek())) value += Advance();
        return {TokenType::NumberLiteral, std::move(value), {std::string(m_filename), startLine, startCol}};
    }

    Token ReadIdentifierOrKeyword(uint32_t startLine, uint32_t startCol) {
        std::string value;
        while (!AtEnd() && IsIdentChar(Peek())) value += Advance();

        if (value == "true" || value == "false") {
            return {TokenType::BoolLiteral, std::move(value), {std::string(m_filename), startLine, startCol}};
        }
        return {TokenType::Identifier, std::move(value), {std::string(m_filename), startLine, startCol}};
    }

    std::string_view m_input;
    std::string_view m_filename;
    size_t m_pos{0};
    uint32_t m_line{1};
    uint32_t m_column{1};
};

// --- Recursive descent parser ---

class MCM2ParserImpl {
public:
    MCM2ParserImpl(std::string_view input, std::string_view filename)
        : m_lexer(input, filename), m_filename(filename) {
        Advance();
    }

    MCM2ParseResult ParseFile() {
        MCM2ParseResult result;

        if (m_current.type == TokenType::EndOfFile) {
            AddError(result, ErrorCode::ParseError, "empty input", m_current.location);
            return result;
        }

        ParseMCM(result);
        return result;
    }

private:
    // --- Token management ---

    void Advance() {
        m_current = m_lexer.Next();
    }

    bool Check(TokenType type) const { return m_current.type == type; }

    bool Match(TokenType type) {
        if (Check(type)) { Advance(); return true; }
        return false;
    }

    Token Expect(TokenType type, MCM2ParseResult& result, const std::string& context) {
        if (Check(type)) {
            Token tok = std::move(m_current);
            Advance();
            return tok;
        }
        AddError(result, ErrorCode::ParseError,
                 "expected " + context + ", got '" + m_current.text + "'", m_current.location);
        return m_current;
    }

    bool CheckIdentifier(const std::string& text) const {
        return m_current.type == TokenType::Identifier && m_current.text == text;
    }

    // --- Error handling ---

    void AddError(MCM2ParseResult& result, ErrorCode code, const std::string& msg,
                  const dsl::SourceLocation& loc) {
        result.diagnostics.push_back({code, msg, loc});
        result.hasErrors = true;
    }

    void AddWarning(MCM2ParseResult& result, ErrorCode code, const std::string& msg,
                    const dsl::SourceLocation& loc) {
        result.diagnostics.push_back({code, msg, loc});
    }

    void RecoverToNextBlock() {
        int depth = 0;
        while (m_current.type != TokenType::EndOfFile) {
            if (m_current.type == TokenType::OpenBrace) ++depth;
            else if (m_current.type == TokenType::CloseBrace) {
                if (depth <= 1) { Advance(); return; }
                --depth;
            }
            Advance();
        }
    }

    // --- MCM root ---

    void ParseMCM(MCM2ParseResult& result) {
        result.definition.location = m_current.location;

        if (!CheckIdentifier("mcm")) {
            AddError(result, ErrorCode::ParseError,
                     "expected 'mcm' keyword, got '" + m_current.text + "'", m_current.location);
            return;
        }
        Advance();

        Token idTok = Expect(TokenType::StringLiteral, result, "MCM ID string");
        if (idTok.type != TokenType::StringLiteral) {
            RecoverToNextBlock();
            return;
        }
        result.definition.id = idTok.text;

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '{' after MCM ID", m_current.location);
            RecoverToNextBlock();
            return;
        }

        // Parse top-level properties and pages
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) {
                AddError(result, ErrorCode::ParseError,
                         "unexpected token '" + m_current.text + "' in MCM block", m_current.location);
                Advance();
                continue;
            }

            if (m_current.text == "page") {
                ParsePage(result);
            } else if (m_current.text == "displayName") {
                Advance();
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "display name string");
                if (val.type == TokenType::StringLiteral) {
                    result.definition.displayName = val.text;
                }
            } else if (m_current.text == "version") {
                Advance();
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "version string");
                if (val.type == TokenType::StringLiteral) {
                    result.definition.version = ParseVersionString(val.text);
                }
            } else {
                // Unknown top-level field: skip value
                Advance();
                if (Match(TokenType::Colon)) {
                    SkipValue();
                }
            }
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '}' at end of MCM definition", m_current.location);
        }
    }

    // --- Version ---

    static MCM2Version ParseVersionString(const std::string& str) {
        MCM2Version ver;
        size_t start = 0;
        auto dot1 = str.find('.', start);
        if (dot1 == std::string::npos) {
            ver.major = static_cast<uint16_t>(std::stoi(str));
            return ver;
        }
        ver.major = static_cast<uint16_t>(std::stoi(str.substr(0, dot1)));
        auto dot2 = str.find('.', dot1 + 1);
        if (dot2 == std::string::npos) {
            ver.minor = static_cast<uint16_t>(std::stoi(str.substr(dot1 + 1)));
            return ver;
        }
        ver.minor = static_cast<uint16_t>(std::stoi(str.substr(dot1 + 1, dot2 - dot1 - 1)));
        ver.patch = static_cast<uint16_t>(std::stoi(str.substr(dot2 + 1)));
        return ver;
    }

    // --- Page ---

    void ParsePage(MCM2ParseResult& result) {
        MCM2PageDef page;
        page.location = m_current.location;
        Advance(); // consume "page"

        Token idTok = Expect(TokenType::StringLiteral, result, "page ID string");
        if (idTok.type != TokenType::StringLiteral) {
            RecoverToNextBlock();
            return;
        }
        page.id = idTok.text;

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '{' after page ID", m_current.location);
            RecoverToNextBlock();
            return;
        }

        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) {
                AddError(result, ErrorCode::ParseError,
                         "unexpected token '" + m_current.text + "' in page block", m_current.location);
                Advance();
                continue;
            }

            if (m_current.text == "section") {
                ParseSection(result, page);
            } else if (m_current.text == "displayName") {
                Advance();
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "display name string");
                if (val.type == TokenType::StringLiteral) {
                    page.displayName = val.text;
                }
            } else {
                Advance();
                if (Match(TokenType::Colon)) {
                    SkipValue();
                }
            }
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '}' at end of page", m_current.location);
        }

        result.definition.pages.push_back(std::move(page));
    }

    // --- Section ---

    void ParseSection(MCM2ParseResult& result, MCM2PageDef& page) {
        MCM2SectionDef section;
        section.location = m_current.location;
        Advance(); // consume "section"

        Token idTok = Expect(TokenType::StringLiteral, result, "section ID string");
        if (idTok.type != TokenType::StringLiteral) {
            RecoverToNextBlock();
            return;
        }
        section.id = idTok.text;

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '{' after section ID", m_current.location);
            RecoverToNextBlock();
            return;
        }

        std::unordered_set<std::string> seenControlIds;

        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) {
                AddError(result, ErrorCode::ParseError,
                         "unexpected token '" + m_current.text + "' in section block", m_current.location);
                Advance();
                continue;
            }

            if (m_current.text == "displayName") {
                Advance();
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "display name string");
                if (val.type == TokenType::StringLiteral) {
                    section.displayName = val.text;
                }
            } else if (m_current.text == "collapsed") {
                Advance();
                Expect(TokenType::Colon, result, "':'");
                if (m_current.type == TokenType::BoolLiteral) {
                    section.collapsed = (m_current.text == "true");
                    Advance();
                }
            } else {
                // Try to parse as control type
                auto controlType = TryParseControlType(m_current.text);
                if (controlType.has_value()) {
                    ParseControl(result, section, *controlType, seenControlIds);
                } else {
                    // Unknown field or control type
                    auto loc = m_current.location;
                    auto text = m_current.text;
                    Advance();

                    // Check if next token is a string (could be unknown control type with ID)
                    if (m_current.type == TokenType::StringLiteral) {
                        AddError(result, ErrorCode::ParseError,
                                 "unknown control type '" + text + "'", loc);
                        // Skip the ID string
                        Advance();
                        // If there's a block, skip it
                        if (Check(TokenType::OpenBrace)) {
                            RecoverToNextBlock();
                        }
                    } else if (Match(TokenType::Colon)) {
                        SkipValue();
                    }
                }
            }
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '}' at end of section", m_current.location);
        }

        page.sections.push_back(std::move(section));
    }

    // --- Control type resolution ---

    static std::optional<MCM2ControlType> TryParseControlType(const std::string& name) {
        if (name == "toggle") return MCM2ControlType::Toggle;
        if (name == "slider") return MCM2ControlType::Slider;
        if (name == "dropdown") return MCM2ControlType::Dropdown;
        if (name == "keybind") return MCM2ControlType::KeyBind;
        if (name == "colour") return MCM2ControlType::Colour;
        if (name == "text") return MCM2ControlType::Text;
        if (name == "header") return MCM2ControlType::Header;
        if (name == "empty") return MCM2ControlType::Empty;
        return std::nullopt;
    }

    // --- Control ---

    void ParseControl(MCM2ParseResult& result, MCM2SectionDef& section,
                      MCM2ControlType type, std::unordered_set<std::string>& seenIds) {
        MCM2ControlDef control;
        control.type = type;
        control.location = m_current.location;
        Advance(); // consume control type keyword

        // Control ID (string) — optional for empty
        if (m_current.type == TokenType::StringLiteral) {
            control.id = m_current.text;
            Advance();
        } else if (type != MCM2ControlType::Empty) {
            AddError(result, ErrorCode::ParseError,
                     "expected control ID string", m_current.location);
            // Try to continue — maybe they went straight to '{'
        }

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '{' after control ID", m_current.location);
            RecoverToNextBlock();
            // Still add the control so subsequent parsing continues
            section.controls.push_back(std::move(control));
            return;
        }

        // Parse control properties
        switch (type) {
            case MCM2ControlType::Toggle:   ParseToggleProps(result, control); break;
            case MCM2ControlType::Slider:   ParseSliderProps(result, control); break;
            case MCM2ControlType::Dropdown: ParseDropdownProps(result, control); break;
            case MCM2ControlType::KeyBind:  ParseKeyBindProps(result, control); break;
            case MCM2ControlType::Colour:   ParseColourProps(result, control); break;
            case MCM2ControlType::Text:     ParseTextProps(result, control); break;
            case MCM2ControlType::Header:   ParseHeaderProps(result, control); break;
            case MCM2ControlType::Empty:    ParseEmptyProps(result, control); break;
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '}' at end of control", m_current.location);
        }

        // Check for duplicate IDs
        if (!control.id.empty() && seenIds.contains(control.id)) {
            AddWarning(result, ErrorCode::DuplicateRegistration,
                       "duplicate control ID '" + control.id + "' in section", control.location);
        }
        if (!control.id.empty()) {
            seenIds.insert(control.id);
        }

        section.controls.push_back(std::move(control));
    }

    // --- Common control property parsing ---

    // Returns true if the property was a common field (label, description, condition, onChange)
    bool TryParseCommonProp(MCM2ParseResult& result, MCM2ControlDef& control,
                            const std::string& name) {
        if (name == "label") {
            Expect(TokenType::Colon, result, "':'");
            Token val = Expect(TokenType::StringLiteral, result, "label string");
            if (val.type == TokenType::StringLiteral) control.label = val.text;
            return true;
        }
        if (name == "description") {
            Expect(TokenType::Colon, result, "':'");
            Token val = Expect(TokenType::StringLiteral, result, "description string");
            if (val.type == TokenType::StringLiteral) control.description = val.text;
            return true;
        }
        if (name == "condition") {
            Expect(TokenType::Colon, result, "':'");
            Token val = Expect(TokenType::StringLiteral, result, "condition expression string");
            if (val.type == TokenType::StringLiteral) control.conditionExpr = val.text;
            return true;
        }
        if (name == "onChange") {
            Expect(TokenType::Colon, result, "':'");
            Token val = Expect(TokenType::StringLiteral, result, "onChange callback name");
            if (val.type == TokenType::StringLiteral) control.onChange = val.text;
            return true;
        }
        return false;
    }

    // --- Toggle ---

    void ParseToggleProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2ToggleProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (name == "default") {
                Expect(TokenType::Colon, result, "':'");
                if (m_current.type == TokenType::BoolLiteral) {
                    props.defaultValue = (m_current.text == "true");
                    Advance();
                }
            } else {
                if (Match(TokenType::Colon)) SkipValue();
            }
        }
        control.properties = props;
    }

    // --- Slider ---

    void ParseSliderProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2SliderProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (name == "min") {
                Expect(TokenType::Colon, result, "':'");
                props.minValue = ReadFloat();
            } else if (name == "max") {
                Expect(TokenType::Colon, result, "':'");
                props.maxValue = ReadFloat();
            } else if (name == "step") {
                Expect(TokenType::Colon, result, "':'");
                props.stepSize = ReadFloat();
            } else if (name == "default") {
                Expect(TokenType::Colon, result, "':'");
                props.defaultValue = ReadFloat();
            } else if (name == "format") {
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "format string");
                if (val.type == TokenType::StringLiteral) props.formatString = val.text;
            } else {
                if (Match(TokenType::Colon)) SkipValue();
            }
        }
        control.properties = props;
    }

    // --- Dropdown ---

    void ParseDropdownProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2DropdownProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (name == "options") {
                Expect(TokenType::Colon, result, "':'");
                props.options = ReadStringList(result);
            } else if (name == "default") {
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "default string");
                if (val.type == TokenType::StringLiteral) props.defaultValue = val.text;
            } else {
                if (Match(TokenType::Colon)) SkipValue();
            }
        }
        control.properties = props;
    }

    // --- KeyBind ---

    void ParseKeyBindProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2KeyBindProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (name == "default") {
                Expect(TokenType::Colon, result, "':'");
                props.defaultValue = ReadInt();
            } else {
                if (Match(TokenType::Colon)) SkipValue();
            }
        }
        control.properties = props;
    }

    // --- Colour ---

    void ParseColourProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2ColourProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (name == "default") {
                Expect(TokenType::Colon, result, "':'");
                props.defaultValue = ReadInt();
            } else {
                if (Match(TokenType::Colon)) SkipValue();
            }
        }
        control.properties = props;
    }

    // --- Text ---

    void ParseTextProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2TextProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (name == "default") {
                Expect(TokenType::Colon, result, "':'");
                Token val = Expect(TokenType::StringLiteral, result, "default string");
                if (val.type == TokenType::StringLiteral) props.defaultValue = val.text;
            } else {
                if (Match(TokenType::Colon)) SkipValue();
            }
        }
        control.properties = props;
    }

    // --- Header ---

    void ParseHeaderProps(MCM2ParseResult& result, MCM2ControlDef& control) {
        MCM2HeaderProps props;
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (m_current.type != TokenType::Identifier) { Advance(); continue; }
            std::string name = m_current.text;
            Advance();
            if (TryParseCommonProp(result, control, name)) continue;
            if (Match(TokenType::Colon)) SkipValue();
        }
        control.properties = props;
    }

    // --- Empty ---

    void ParseEmptyProps(MCM2ParseResult& /*result*/, MCM2ControlDef& control) {
        MCM2EmptyProps props;
        // Consume any content inside the block (shouldn't be any, but be resilient)
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            Advance();
        }
        control.properties = props;
    }

    // --- Value readers ---

    float ReadFloat() {
        float val = 0.0f;
        if (m_current.type == TokenType::NumberLiteral) {
            try { val = std::stof(m_current.text); } catch (...) {}
            Advance();
        }
        return val;
    }

    int32_t ReadInt() {
        int32_t val = 0;
        if (m_current.type == TokenType::NumberLiteral) {
            const auto& text = m_current.text;
            if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
                // Hex
                try { val = static_cast<int32_t>(std::stoul(text, nullptr, 16)); } catch (...) {}
            } else {
                try { val = std::stoi(text); } catch (...) {}
            }
            Advance();
        }
        return val;
    }

    std::vector<std::string> ReadStringList(MCM2ParseResult& result) {
        std::vector<std::string> items;
        if (!Match(TokenType::OpenBracket)) {
            AddError(result, ErrorCode::ParseError, "expected '[' for list", m_current.location);
            return items;
        }
        while (!Check(TokenType::CloseBracket) && !Check(TokenType::EndOfFile)) {
            if (m_current.type == TokenType::StringLiteral) {
                items.push_back(m_current.text);
                Advance();
            } else {
                Advance();
            }
            if (!Check(TokenType::CloseBracket)) {
                Match(TokenType::Comma);
            }
        }
        Match(TokenType::CloseBracket);
        return items;
    }

    void SkipValue() {
        // Skip a single value: could be a string, number, bool, list, or block
        if (Check(TokenType::OpenBracket)) {
            int depth = 1;
            Advance();
            while (depth > 0 && !Check(TokenType::EndOfFile)) {
                if (Check(TokenType::OpenBracket)) ++depth;
                else if (Check(TokenType::CloseBracket)) --depth;
                Advance();
            }
        } else if (Check(TokenType::OpenBrace)) {
            int depth = 1;
            Advance();
            while (depth > 0 && !Check(TokenType::EndOfFile)) {
                if (Check(TokenType::OpenBrace)) ++depth;
                else if (Check(TokenType::CloseBrace)) --depth;
                Advance();
            }
        } else {
            Advance(); // skip single-token value
        }
    }

    // --- State ---

    Lexer m_lexer;
    Token m_current;
    std::string_view m_filename;
};

// --- Serializer ---

class MCM2Serializer {
public:
    static std::string Serialize(const MCM2Definition& def) {
        MCM2Serializer s;
        s.WriteMCM(def);
        return s.m_out;
    }

private:
    void WriteMCM(const MCM2Definition& def) {
        Write("mcm \"" + Escape(def.id) + "\" {\n");
        ++m_indent;
        WriteField("displayName", def.displayName);

        std::string verStr = std::to_string(def.version.major) + "." +
                             std::to_string(def.version.minor) + "." +
                             std::to_string(def.version.patch);
        WriteField("version", verStr);

        for (const auto& page : def.pages) {
            Write("\n");
            WritePage(page);
        }

        --m_indent;
        Write("}\n");
    }

    void WritePage(const MCM2PageDef& page) {
        WriteIndent();
        m_out += "page \"" + Escape(page.id) + "\" {\n";
        ++m_indent;
        WriteField("displayName", page.displayName);

        for (const auto& section : page.sections) {
            Write("\n");
            WriteSection(section);
        }

        --m_indent;
        WriteIndent();
        m_out += "}\n";
    }

    void WriteSection(const MCM2SectionDef& section) {
        WriteIndent();
        m_out += "section \"" + Escape(section.id) + "\" {\n";
        ++m_indent;
        WriteField("displayName", section.displayName);
        if (section.collapsed) {
            WriteIndent();
            m_out += "collapsed: true\n";
        }

        for (const auto& control : section.controls) {
            Write("\n");
            WriteControl(control);
        }

        --m_indent;
        WriteIndent();
        m_out += "}\n";
    }

    void WriteControl(const MCM2ControlDef& control) {
        WriteIndent();
        m_out += ControlTypeName(control.type);
        if (!control.id.empty()) {
            m_out += " \"" + Escape(control.id) + "\"";
        }
        m_out += " {\n";
        ++m_indent;

        if (!control.label.empty()) WriteField("label", control.label);
        if (!control.description.empty()) WriteField("description", control.description);

        std::visit([this](const auto& props) { WriteProps(props); }, control.properties);

        if (!control.onChange.empty()) WriteField("onChange", control.onChange);
        if (!control.conditionExpr.empty()) WriteField("condition", control.conditionExpr);

        --m_indent;
        WriteIndent();
        m_out += "}\n";
    }

    void WriteProps(const MCM2ToggleProps& p) {
        WriteIndent();
        m_out += "default: " + std::string(p.defaultValue ? "true" : "false") + "\n";
    }

    void WriteProps(const MCM2SliderProps& p) {
        WriteIndent(); m_out += "min: " + FormatFloat(p.minValue) + "\n";
        WriteIndent(); m_out += "max: " + FormatFloat(p.maxValue) + "\n";
        WriteIndent(); m_out += "step: " + FormatFloat(p.stepSize) + "\n";
        WriteIndent(); m_out += "default: " + FormatFloat(p.defaultValue) + "\n";
        if (!p.formatString.empty()) WriteField("format", p.formatString);
    }

    void WriteProps(const MCM2DropdownProps& p) {
        if (!p.options.empty()) {
            WriteIndent();
            m_out += "options: [";
            for (size_t i = 0; i < p.options.size(); ++i) {
                if (i > 0) m_out += ", ";
                m_out += "\"" + Escape(p.options[i]) + "\"";
            }
            m_out += "]\n";
        }
        if (!p.defaultValue.empty()) WriteField("default", p.defaultValue);
    }

    void WriteProps(const MCM2KeyBindProps& p) {
        WriteIndent();
        m_out += "default: " + std::to_string(p.defaultValue) + "\n";
    }

    void WriteProps(const MCM2ColourProps& p) {
        WriteIndent();
        char buf[16];
        std::snprintf(buf, sizeof(buf), "0x%06X", static_cast<uint32_t>(p.defaultValue));
        m_out += "default: " + std::string(buf) + "\n";
    }

    void WriteProps(const MCM2TextProps& p) {
        if (!p.defaultValue.empty()) WriteField("default", p.defaultValue);
    }

    void WriteProps(const MCM2HeaderProps&) {}
    void WriteProps(const MCM2EmptyProps&) {}

    // --- Helpers ---

    static std::string ControlTypeName(MCM2ControlType type) {
        switch (type) {
            case MCM2ControlType::Toggle: return "toggle";
            case MCM2ControlType::Slider: return "slider";
            case MCM2ControlType::Dropdown: return "dropdown";
            case MCM2ControlType::KeyBind: return "keybind";
            case MCM2ControlType::Colour: return "colour";
            case MCM2ControlType::Text: return "text";
            case MCM2ControlType::Header: return "header";
            case MCM2ControlType::Empty: return "empty";
        }
        return "unknown";
    }

    static std::string Escape(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\t': out += "\\t"; break;
                default: out += c; break;
            }
        }
        return out;
    }

    static std::string FormatFloat(float v) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f", v);
        return buf;
    }

    void WriteField(const std::string& key, const std::string& value) {
        WriteIndent();
        m_out += key + ": \"" + Escape(value) + "\"\n";
    }

    void WriteIndent() {
        for (int i = 0; i < m_indent; ++i) m_out += "    ";
    }

    void Write(const std::string& s) {
        WriteIndent();
        m_out += s;
    }

    std::string m_out;
    int m_indent{0};
};

// --- Public API ---

Result<MCM2ParseResult> MCM2DefinitionParser::Parse(std::string_view input,
                                                     std::string_view filename) const {
    MCM2ParserImpl parser(input, filename);
    return parser.ParseFile();
}

std::string MCM2DefinitionParser::Serialize(const MCM2Definition& def) {
    return MCM2Serializer::Serialize(def);
}

}  // namespace ohui::mcm
