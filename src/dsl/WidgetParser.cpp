#include "ohui/dsl/WidgetParser.h"
#include "ohui/core/Log.h"

#include <unordered_set>

namespace ohui::dsl {

// --- Token types ---

enum class TokenType {
    Keyword_Widget,
    Keyword_Skin,
    Keyword_Property,
    Keyword_Requires,
    Keyword_Animate,
    Keyword_Tokens,
    Keyword_Template,
    Keyword_Bind,
    Keyword_Token,
    Keyword_Var,
    Keyword_Update,
    Identifier,
    StringLiteral,
    NumberLiteral,
    Colon,
    Semicolon,
    OpenBrace,
    CloseBrace,
    OpenParen,
    CloseParen,
    Comma,
    Dot,
    GreaterEqual,
    EndOfFile,
    Error,
};

struct Token {
    TokenType type;
    std::string text;
    SourceLocation location;
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
        if (c == '(') { Advance(); return {TokenType::OpenParen, "(", {std::string(m_filename), startLine, startCol}}; }
        if (c == ')') { Advance(); return {TokenType::CloseParen, ")", {std::string(m_filename), startLine, startCol}}; }
        if (c == ':') { Advance(); return {TokenType::Colon, ":", {std::string(m_filename), startLine, startCol}}; }
        if (c == ';') { Advance(); return {TokenType::Semicolon, ";", {std::string(m_filename), startLine, startCol}}; }
        if (c == ',') { Advance(); return {TokenType::Comma, ",", {std::string(m_filename), startLine, startCol}}; }
        if (c == '.') { Advance(); return {TokenType::Dot, ".", {std::string(m_filename), startLine, startCol}}; }
        if (c == '>' && m_pos + 1 < m_input.size() && m_input[m_pos + 1] == '=') {
            Advance(); Advance();
            return {TokenType::GreaterEqual, ">=", {std::string(m_filename), startLine, startCol}};
        }
        if (c == '"') return ReadStringLiteral(startLine, startCol);
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
    static bool IsIdentStart(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
    }
    static bool IsIdentChar(char c) { return IsIdentStart(c) || IsDigit(c); }

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
        if (!AtEnd()) Advance();
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
        while (!AtEnd() && ((Peek() >= 'a' && Peek() <= 'z') || Peek() == '%')) {
            value += Advance();
        }
        return {TokenType::NumberLiteral, std::move(value), {std::string(m_filename), startLine, startCol}};
    }

    Token ReadIdentifierOrKeyword(uint32_t startLine, uint32_t startCol) {
        std::string value;
        while (!AtEnd() && IsIdentChar(Peek())) value += Advance();
        TokenType type = TokenType::Identifier;
        if (value == "widget") type = TokenType::Keyword_Widget;
        else if (value == "skin") type = TokenType::Keyword_Skin;
        else if (value == "property") type = TokenType::Keyword_Property;
        else if (value == "requires") type = TokenType::Keyword_Requires;
        else if (value == "animate") type = TokenType::Keyword_Animate;
        else if (value == "tokens") type = TokenType::Keyword_Tokens;
        else if (value == "template") type = TokenType::Keyword_Template;
        else if (value == "bind") type = TokenType::Keyword_Bind;
        else if (value == "token") type = TokenType::Keyword_Token;
        else if (value == "var") type = TokenType::Keyword_Var;
        else if (value == "update") type = TokenType::Keyword_Update;
        return {type, std::move(value), {std::string(m_filename), startLine, startCol}};
    }

    std::string_view m_input;
    std::string_view m_filename;
    size_t m_pos{0};
    uint32_t m_line{1};
    uint32_t m_column{1};
};

// --- Recursive descent parser ---

class WidgetParserImpl {
public:
    WidgetParserImpl(std::string_view input, std::string_view filename)
        : m_lexer(input, filename), m_filename(filename) {
        Advance();
    }

    ParseResult ParseFile() {
        ParseResult result;

        while (m_current.type != TokenType::EndOfFile) {
            if (m_current.type == TokenType::Keyword_Widget) {
                ParseWidgetDef(result);
            } else if (m_current.type == TokenType::Keyword_Skin) {
                ParseSkinDef(result);
            } else if (m_current.type == TokenType::Identifier) {
                ParseWidgetInstance(result);
            } else {
                AddError(result, ErrorCode::ParseError,
                         "unexpected token '" + m_current.text + "'", m_current.location);
                RecoverToNextBlock();
            }
        }

        return result;
    }

private:
    // --- Token management ---

    void Advance() {
        if (m_hasPutBack) {
            m_current = std::move(m_putBack);
            m_hasPutBack = false;
        } else {
            m_current = m_lexer.Next();
        }
    }

    void PutBack(Token tok) {
        m_putBack = std::move(m_current);
        m_current = std::move(tok);
        m_hasPutBack = true;
    }

    bool Check(TokenType type) const { return m_current.type == type; }

    bool Match(TokenType type) {
        if (Check(type)) { Advance(); return true; }
        return false;
    }

    Token Expect(TokenType type, ParseResult& result, const std::string& context) {
        if (Check(type)) {
            Token tok = std::move(m_current);
            Advance();
            return tok;
        }
        AddError(result, ErrorCode::ParseError,
                 "expected " + context + ", got '" + m_current.text + "'", m_current.location);
        return m_current;
    }

    // --- Error handling ---

    void AddError(ParseResult& result, ErrorCode code, const std::string& msg,
                  const SourceLocation& loc) {
        result.diagnostics.push_back({code, msg, loc});
        result.hasErrors = true;
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

    void SkipToSemicolon() {
        while (!Check(TokenType::Semicolon) && !Check(TokenType::CloseBrace) &&
               !Check(TokenType::EndOfFile)) {
            Advance();
        }
        Match(TokenType::Semicolon);
    }

    static bool IsKeyword(TokenType t) {
        return t >= TokenType::Keyword_Widget && t <= TokenType::Keyword_Update;
    }

    static bool IsIdentOrKeyword(TokenType t) {
        return t == TokenType::Identifier || IsKeyword(t);
    }

    // --- Widget definition ---

    void ParseWidgetDef(ParseResult& result) {
        SourceLocation loc = m_current.location;
        Advance(); // consume "widget"

        Token nameTok = Expect(TokenType::Identifier, result, "widget name");
        if (nameTok.type != TokenType::Identifier) { RecoverToNextBlock(); return; }

        WidgetDef def;
        def.name = nameTok.text;
        def.location = loc;

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '{' after widget name", m_current.location);
            RecoverToNextBlock();
            return;
        }

        std::unordered_set<std::string> propertyNames;

        if (Check(TokenType::Keyword_Requires)) {
            def.requiresVersion = ParseRequiresClause(result);
        }

        while (Check(TokenType::Keyword_Property)) {
            auto prop = ParsePropertyDecl(result);
            if (!prop.name.empty()) {
                propertyNames.insert(prop.name);
                def.propertyDecls.push_back(std::move(prop));
            }
        }

        if (IsIdentOrKeyword(m_current.type) && !Check(TokenType::Keyword_Widget) &&
            !Check(TokenType::Keyword_Skin)) {
            def.rootComponent = ParseComponent(result, propertyNames);
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '}' at end of widget definition", m_current.location);
        }

        result.ast.widgets.push_back(std::move(def));
    }

    // --- Requires clause ---

    std::optional<VersionConstraint> ParseRequiresClause(ParseResult& result) {
        Advance(); // consume "requires"

        if (!IsIdentOrKeyword(m_current.type) || m_current.text != "ohui") {
            AddError(result, ErrorCode::ParseError, "expected 'ohui' after 'requires'", m_current.location);
            SkipToSemicolon();
            return std::nullopt;
        }
        Advance();

        if (!Match(TokenType::GreaterEqual)) {
            AddError(result, ErrorCode::ParseError, "expected '>=' in requires clause", m_current.location);
            SkipToSemicolon();
            return std::nullopt;
        }

        // The lexer produces "1.3" as a single NumberLiteral (it consumes the dot).
        // Parse major.minor from that single token.
        Token versionTok = Expect(TokenType::NumberLiteral, result, "version number");
        if (versionTok.type != TokenType::NumberLiteral) { SkipToSemicolon(); return std::nullopt; }

        Match(TokenType::Semicolon);

        VersionConstraint vc;
        auto dotPos = versionTok.text.find('.');
        if (dotPos != std::string::npos) {
            vc.major = static_cast<uint16_t>(std::stoi(versionTok.text.substr(0, dotPos)));
            vc.minor = static_cast<uint16_t>(std::stoi(versionTok.text.substr(dotPos + 1)));
        } else {
            vc.major = static_cast<uint16_t>(std::stoi(versionTok.text));
            vc.minor = 0;
        }
        return vc;
    }

    // --- Property declaration ---

    PropertyDecl ParsePropertyDecl(ParseResult& result) {
        SourceLocation loc = m_current.location;
        Advance(); // consume "property"

        PropertyDecl decl;
        decl.location = loc;

        Token nameTok = Expect(TokenType::Identifier, result, "property name");
        if (nameTok.type != TokenType::Identifier) { SkipToSemicolon(); return decl; }
        decl.name = nameTok.text;

        Expect(TokenType::Colon, result, "':'");

        Token typeTok = Expect(TokenType::Identifier, result, "property type");
        if (typeTok.text == "float") decl.type = PropertyType::Float;
        else if (typeTok.text == "int") decl.type = PropertyType::Int;
        else if (typeTok.text == "bool") decl.type = PropertyType::Bool;
        else if (typeTok.text == "string") decl.type = PropertyType::String;
        else if (typeTok.text == "color") decl.type = PropertyType::Color;
        else {
            AddError(result, ErrorCode::TypeError, "unknown property type '" + typeTok.text + "'", typeTok.location);
        }

        Match(TokenType::Semicolon);
        return decl;
    }

    // --- Component (recursive) ---

    std::unique_ptr<ComponentNode> ParseComponent(
        ParseResult& result,
        const std::unordered_set<std::string>& propertyNames) {

        auto node = std::make_unique<ComponentNode>();
        node->location = m_current.location;
        node->typeName = m_current.text;
        Advance(); // consume component type name

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '{' after component type '" + node->typeName + "'", m_current.location);
            return node;
        }

        ParseComponentBody(result, *node, propertyNames);

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '}' at end of component '" + node->typeName + "'", m_current.location);
        }

        return node;
    }

    void ParseComponentBody(ParseResult& result, ComponentNode& node,
                            const std::unordered_set<std::string>& propertyNames) {
        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (Check(TokenType::Keyword_Animate)) {
                node.animations.push_back(ParseAnimateBlock(result));
            } else if (IsIdentOrKeyword(m_current.type)) {
                // Lookahead: after IDENT, is it ':' (property) or '{' (child component)?
                Token saved = std::move(m_current);
                Advance();

                if (Check(TokenType::Colon)) {
                    // Property assignment: name: expr;
                    Advance(); // consume ':'
                    PropertyAssignment pa;
                    pa.name = saved.text;
                    pa.location = saved.location;
                    pa.value = ParseExpr(result, propertyNames);
                    Match(TokenType::Semicolon);
                    node.properties.push_back(std::move(pa));
                } else if (Check(TokenType::OpenBrace)) {
                    // Child component: put the identifier back as current, recurse
                    PutBack(std::move(saved));
                    node.children.push_back(ParseComponent(result, propertyNames));
                } else {
                    AddError(result, ErrorCode::ParseError,
                             "expected ':' or '{' after '" + saved.text + "'", m_current.location);
                    SkipToSemicolon();
                }
            } else {
                AddError(result, ErrorCode::ParseError,
                         "unexpected token '" + m_current.text + "' in component", m_current.location);
                Advance();
            }
        }
    }

    // --- Animate block ---

    AnimateBlock ParseAnimateBlock(ParseResult& result) {
        AnimateBlock anim;
        anim.location = m_current.location;
        Advance(); // consume "animate"

        Token propTok = Expect(TokenType::Identifier, result, "animated property name");
        anim.property = propTok.text;

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '{' after animate property", m_current.location);
            return anim;
        }

        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (!IsIdentOrKeyword(m_current.type)) {
                Advance();
                continue;
            }
            Token name = std::move(m_current);
            Advance();
            if (!Match(TokenType::Colon)) { SkipToSemicolon(); continue; }

            if (name.text == "duration") {
                if (Check(TokenType::NumberLiteral)) {
                    std::string numStr = m_current.text;
                    size_t numEnd = 0;
                    while (numEnd < numStr.size() &&
                           ((numStr[numEnd] >= '0' && numStr[numEnd] <= '9') || numStr[numEnd] == '.')) {
                        ++numEnd;
                    }
                    if (numEnd > 0) anim.durationMs = std::stof(numStr.substr(0, numEnd));
                    Advance();
                }
            } else if (name.text == "easing") {
                if (IsIdentOrKeyword(m_current.type)) {
                    if (m_current.text == "linear") anim.easing = EasingFunction::Linear;
                    else if (m_current.text == "ease-in") anim.easing = EasingFunction::EaseIn;
                    else if (m_current.text == "ease-out") anim.easing = EasingFunction::EaseOut;
                    else if (m_current.text == "ease-in-out") anim.easing = EasingFunction::EaseInOut;
                    Advance();
                }
            } else {
                // Skip unknown property value
                while (!Check(TokenType::Semicolon) && !Check(TokenType::CloseBrace) &&
                       !Check(TokenType::EndOfFile)) {
                    Advance();
                }
            }

            Match(TokenType::Semicolon);
        }
        Match(TokenType::CloseBrace);
        return anim;
    }

    // --- Expression ---

    Expr ParseExpr(ParseResult& result,
                   const std::unordered_set<std::string>& propertyNames) {
        Expr expr;
        expr.location = m_current.location;

        if (Check(TokenType::Keyword_Bind)) {
            Advance();
            Expect(TokenType::OpenParen, result, "'('");
            expr.kind = ExprKind::Bind;
            expr.value = ReadDottedIdentifier();
            Expect(TokenType::CloseParen, result, "')'");
        } else if (Check(TokenType::Keyword_Token)) {
            Advance();
            Expect(TokenType::OpenParen, result, "'('");
            expr.kind = ExprKind::Token;
            expr.value = ReadDottedIdentifier();
            Expect(TokenType::CloseParen, result, "')'");
        } else if (Check(TokenType::Keyword_Var)) {
            Advance();
            Expect(TokenType::OpenParen, result, "'('");
            expr.kind = ExprKind::Var;
            expr.value = ReadDottedIdentifier();
            if (Match(TokenType::Comma)) {
                // Read fallback until ')'
                std::string fb;
                while (!Check(TokenType::CloseParen) && !Check(TokenType::EndOfFile)) {
                    if (!fb.empty() && m_current.type != TokenType::Comma) fb += ' ';
                    fb += m_current.text;
                    Advance();
                }
                // Trim
                size_t start = fb.find_first_not_of(" \t");
                size_t end = fb.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    expr.fallback = fb.substr(start, end - start + 1);
                }
            }
            Expect(TokenType::CloseParen, result, "')'");
        } else if (Check(TokenType::StringLiteral)) {
            expr.kind = ExprKind::Literal;
            expr.value = m_current.text;
            Advance();
        } else if (Check(TokenType::NumberLiteral)) {
            expr.kind = ExprKind::Literal;
            expr.value = m_current.text;
            Advance();
        } else if (IsIdentOrKeyword(m_current.type)) {
            if (propertyNames.contains(m_current.text)) {
                expr.kind = ExprKind::PropertyRef;
            } else {
                expr.kind = ExprKind::Literal;
            }
            expr.value = m_current.text;
            Advance();
        } else {
            AddError(result, ErrorCode::ParseError,
                     "unexpected token '" + m_current.text + "' in expression", m_current.location);
            expr.kind = ExprKind::Literal;
            expr.value = m_current.text;
            Advance();
        }

        return expr;
    }

    std::string ReadDottedIdentifier() {
        std::string result;
        if (IsIdentOrKeyword(m_current.type)) {
            result = m_current.text;
            Advance();
            while (Check(TokenType::Dot)) {
                result += '.';
                Advance();
                if (IsIdentOrKeyword(m_current.type)) {
                    result += m_current.text;
                    Advance();
                }
            }
        }
        return result;
    }

    // --- Skin definition ---

    void ParseSkinDef(ParseResult& result) {
        SourceLocation loc = m_current.location;
        Advance(); // consume "skin"

        Token widgetNameTok = Expect(TokenType::Identifier, result, "widget name for skin");
        if (widgetNameTok.type != TokenType::Identifier) { RecoverToNextBlock(); return; }

        Token skinNameTok = Expect(TokenType::StringLiteral, result, "skin name string");
        if (skinNameTok.type != TokenType::StringLiteral) { RecoverToNextBlock(); return; }

        SkinDef skin;
        skin.widgetName = widgetNameTok.text;
        skin.skinName = skinNameTok.text;
        skin.location = loc;

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '{' after skin name", m_current.location);
            RecoverToNextBlock();
            return;
        }

        if (Check(TokenType::Keyword_Requires)) {
            skin.requiresVersion = ParseRequiresClause(result);
        }

        if (Check(TokenType::Keyword_Tokens)) {
            ParseTokensBlock(result, skin);
        }

        if (Check(TokenType::Keyword_Template)) {
            ParseTemplateBlock(result, skin);
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '}' at end of skin definition", m_current.location);
        }

        result.ast.skins.push_back(std::move(skin));
    }

    void ParseTokensBlock(ParseResult& result, SkinDef& skin) {
        Advance(); // consume "tokens"
        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '{' after 'tokens'", m_current.location);
            return;
        }

        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (!IsIdentOrKeyword(m_current.type)) {
                AddError(result, ErrorCode::ParseError, "expected token name", m_current.location);
                SkipToSemicolon();
                continue;
            }
            std::string name = ReadDottedIdentifier();
            Expect(TokenType::Colon, result, "':'");

            // Consume all tokens until semicolon as the value
            // (handles values like #FF0000 which tokenize as multiple tokens)
            std::string value;
            while (!Check(TokenType::Semicolon) && !Check(TokenType::CloseBrace) &&
                   !Check(TokenType::EndOfFile)) {
                value += m_current.text;
                Advance();
            }
            Match(TokenType::Semicolon);
            skin.tokens.push_back({std::move(name), std::move(value)});
        }
        Match(TokenType::CloseBrace);
    }

    void ParseTemplateBlock(ParseResult& result, SkinDef& skin) {
        Advance(); // consume "template"
        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '{' after 'template'", m_current.location);
            return;
        }

        if (IsIdentOrKeyword(m_current.type)) {
            std::unordered_set<std::string> empty;
            skin.templateRoot = ParseComponent(result, empty);
        }

        if (!Match(TokenType::CloseBrace)) {
            AddError(result, ErrorCode::ParseError, "expected '}' at end of template block", m_current.location);
        }
    }

    // --- Widget instance ---

    void ParseWidgetInstance(ParseResult& result) {
        WidgetInstance instance;
        instance.location = m_current.location;
        instance.widgetTypeName = m_current.text;
        Advance();

        if (!Match(TokenType::OpenBrace)) {
            AddError(result, ErrorCode::ParseError,
                     "expected '{' after widget instance type", m_current.location);
            RecoverToNextBlock();
            return;
        }

        std::unordered_set<std::string> empty;

        while (!Check(TokenType::CloseBrace) && !Check(TokenType::EndOfFile)) {
            if (Check(TokenType::Keyword_Update)) {
                instance.updateDecl = ParseUpdateDecl(result);
            } else if (IsIdentOrKeyword(m_current.type)) {
                Token nameTok = std::move(m_current);
                Advance();
                Expect(TokenType::Colon, result, "':'");

                InstanceProp prop;
                prop.name = nameTok.text;
                prop.location = nameTok.location;
                prop.value = ParseExpr(result, empty);
                Match(TokenType::Semicolon);
                instance.properties.push_back(std::move(prop));
            } else {
                AddError(result, ErrorCode::ParseError,
                         "unexpected token '" + m_current.text + "' in widget instance", m_current.location);
                Advance();
            }
        }
        Match(TokenType::CloseBrace);
        result.ast.instances.push_back(std::move(instance));
    }

    // --- Update declaration ---

    std::optional<UpdateDecl> ParseUpdateDecl(ParseResult& result) {
        Advance(); // consume "update"
        Expect(TokenType::Colon, result, "':'");

        UpdateDecl decl;
        Token modeTok = Expect(TokenType::Identifier, result, "update mode");
        decl.mode = modeTok.text;

        if (Match(TokenType::OpenParen)) {
            while (!Check(TokenType::CloseParen) && !Check(TokenType::EndOfFile)) {
                if (IsIdentOrKeyword(m_current.type)) {
                    decl.watchBindings.push_back(ReadDottedIdentifier());
                }
                if (!Match(TokenType::Comma)) break;
            }
            Expect(TokenType::CloseParen, result, "')'");
        }

        Match(TokenType::Semicolon);
        return decl;
    }

    // --- State ---

    Lexer m_lexer;
    Token m_current;
    Token m_putBack;
    bool m_hasPutBack{false};
    std::string_view m_filename;
};

// --- Public API ---

Result<ParseResult> WidgetParser::Parse(std::string_view input, std::string_view filename) const {
    WidgetParserImpl parser(input, filename);
    return parser.ParseFile();
}

}  // namespace ohui::dsl
