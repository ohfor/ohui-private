#include "ohui/i18n/StringTable.h"
#include "ohui/core/Log.h"

#include <charconv>
#include <sstream>

namespace ohui::i18n {

// --- Comment stripping ---

static std::string StripComments(std::string_view input) {
    std::string result;
    result.reserve(input.size());

    size_t i = 0;
    bool inString = false;

    while (i < input.size()) {
        // Handle escaped characters inside strings
        if (inString && input[i] == '\\' && i + 1 < input.size()) {
            result += input[i];
            result += input[i + 1];
            i += 2;
            continue;
        }

        // Toggle string state on unescaped quotes
        if (input[i] == '"') {
            inString = !inString;
            result += input[i];
            ++i;
            continue;
        }

        // Don't process comments inside strings
        if (inString) {
            result += input[i];
            ++i;
            continue;
        }

        // Line comment
        if (input[i] == '/' && i + 1 < input.size() && input[i + 1] == '/') {
            // Skip to end of line
            while (i < input.size() && input[i] != '\n') ++i;
            continue;
        }

        // Block comment
        if (input[i] == '/' && i + 1 < input.size() && input[i + 1] == '*') {
            i += 2;
            while (i + 1 < input.size() && !(input[i] == '*' && input[i + 1] == '/')) ++i;
            if (i + 1 < input.size()) i += 2;  // skip */
            continue;
        }

        result += input[i];
        ++i;
    }

    return result;
}

// --- Quoted string parsing ---

static bool ParseQuotedString(std::string_view line, size_t& pos, std::string& out) {
    // Skip whitespace
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;

    if (pos >= line.size() || line[pos] != '"') return false;
    ++pos;  // skip opening quote

    out.clear();
    while (pos < line.size()) {
        if (line[pos] == '\\' && pos + 1 < line.size()) {
            char next = line[pos + 1];
            if (next == '"') {
                out += '"';
                pos += 2;
                continue;
            } else if (next == 'n') {
                out += '\n';
                pos += 2;
                continue;
            } else if (next == 't') {
                out += '\t';
                pos += 2;
                continue;
            } else if (next == '\\') {
                out += '\\';
                pos += 2;
                continue;
            }
        }
        if (line[pos] == '"') {
            ++pos;
            return true;
        }
        out += line[pos];
        ++pos;
    }
    return false;  // unterminated string
}

// --- Plural category suffix mapping ---

static const std::pair<std::string_view, PluralCategory> kPluralSuffixes[] = {
    {"_zero", PluralCategory::Zero},
    {"_one", PluralCategory::One},
    {"_two", PluralCategory::Two},
    {"_few", PluralCategory::Few},
    {"_many", PluralCategory::Many},
    {"_other", PluralCategory::Other},
};

static bool TryParsePluralSuffix(std::string_view key, std::string& baseKey, PluralCategory& cat) {
    for (const auto& [suffix, category] : kPluralSuffixes) {
        if (key.size() > suffix.size() && key.ends_with(suffix)) {
            baseKey = std::string(key.substr(0, key.size() - suffix.size()));
            cat = category;
            return true;
        }
    }
    return false;
}

// --- Definition parsing ---

Result<size_t> StringTable::LoadDefinitions(std::string_view content) {
    if (content.empty()) return size_t{0};

    std::string stripped = StripComments(content);
    std::istringstream stream(stripped);
    std::string line;
    size_t count = 0;

    while (std::getline(stream, line)) {
        // Trim leading whitespace
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) ++start;

        if (start >= line.size() || line[start] != '$') continue;

        // Extract key
        size_t keyStart = start + 1;
        size_t keyEnd = keyStart;
        while (keyEnd < line.size() && line[keyEnd] != ' ' && line[keyEnd] != '\t' &&
               line[keyEnd] != '"' && line[keyEnd] != '{') {
            ++keyEnd;
        }

        if (keyEnd == keyStart) {
            return std::unexpected(Error{ErrorCode::ParseError,
                "Empty key name at definition"});
        }

        std::string key = "$" + line.substr(keyStart, keyEnd - keyStart);
        size_t pos = keyEnd;

        // Skip whitespace
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;

        if (pos >= line.size()) {
            return std::unexpected(Error{ErrorCode::ParseError,
                "Missing value for key: " + key});
        }

        if (line[pos] == '"') {
            // Simple value
            std::string value;
            if (!ParseQuotedString(line, pos, value)) {
                return std::unexpected(Error{ErrorCode::ParseError,
                    "Unterminated string for key: " + key});
            }

            auto& entry = m_definitions[key];
            entry.key = key;
            entry.defaultValue = std::move(value);
            ++count;

        } else if (line[pos] == '{') {
            // Plural block
            StringEntry entry;
            entry.key = key;

            std::string blockLine;
            while (std::getline(stream, blockLine)) {
                size_t bStart = 0;
                while (bStart < blockLine.size() &&
                       (blockLine[bStart] == ' ' || blockLine[bStart] == '\t')) ++bStart;

                if (bStart < blockLine.size() && blockLine[bStart] == '}') break;

                if (bStart >= blockLine.size()) continue;

                // Parse: category "value"
                size_t catEnd = bStart;
                while (catEnd < blockLine.size() && blockLine[catEnd] != ' ' &&
                       blockLine[catEnd] != '\t') ++catEnd;

                std::string_view catName(blockLine.data() + bStart, catEnd - bStart);
                PluralCategory cat;

                if (catName == "zero") cat = PluralCategory::Zero;
                else if (catName == "one") cat = PluralCategory::One;
                else if (catName == "two") cat = PluralCategory::Two;
                else if (catName == "few") cat = PluralCategory::Few;
                else if (catName == "many") cat = PluralCategory::Many;
                else if (catName == "other") cat = PluralCategory::Other;
                else continue;  // skip unknown categories

                size_t valPos = catEnd;
                std::string value;
                if (!ParseQuotedString(blockLine, valPos, value)) {
                    return std::unexpected(Error{ErrorCode::ParseError,
                        "Unterminated string in plural block for key: " + key});
                }

                entry.pluralForms[cat] = std::move(value);
            }

            // Use "other" as default value if present
            auto otherIt = entry.pluralForms.find(PluralCategory::Other);
            if (otherIt != entry.pluralForms.end()) {
                entry.defaultValue = otherIt->second;
            }

            if (m_definitions.contains(key)) {
                ohui::log::debug("StringTable: overwriting definition for '{}'", key);
            }
            m_definitions[key] = std::move(entry);
            ++count;

        } else {
            return std::unexpected(Error{ErrorCode::ParseError,
                "Expected '\"' or '{' after key: " + key});
        }
    }

    return count;
}

// --- Translation loading ---

Result<size_t> StringTable::LoadTranslations(Language lang, std::string_view content) {
    if (content.empty()) return size_t{0};

    std::string stripped = StripComments(content);
    std::istringstream stream(stripped);
    std::string line;
    size_t count = 0;

    auto& langMap = m_translations[lang];

    while (std::getline(stream, line)) {
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) ++start;

        if (start >= line.size() || line[start] != '$') continue;

        size_t keyStart = start + 1;
        size_t keyEnd = keyStart;
        while (keyEnd < line.size() && line[keyEnd] != ' ' && line[keyEnd] != '\t' &&
               line[keyEnd] != '"') {
            ++keyEnd;
        }

        if (keyEnd == keyStart) continue;

        std::string fullKey = "$" + line.substr(keyStart, keyEnd - keyStart);
        size_t pos = keyEnd;

        std::string value;
        if (!ParseQuotedString(line, pos, value)) continue;

        // Check if this is a plural suffix translation
        std::string baseKey;
        PluralCategory cat;
        if (TryParsePluralSuffix(fullKey, baseKey, cat)) {
            langMap[baseKey].pluralForms[cat] = std::move(value);
        } else {
            langMap[fullKey].value = std::move(value);
        }
        ++count;
    }

    return count;
}

// --- Resolution ---

std::string StringTable::Resolve(std::string_view key) const {
    // Try active language translation
    const std::string* translation = FindTranslation(key);
    if (translation) return *translation;

    // Fall back to definition default
    auto it = m_definitions.find(std::string(key));
    if (it != m_definitions.end()) return it->second.defaultValue;

    // Unknown key: return key itself
    return std::string(key);
}

std::string StringTable::Resolve(std::string_view key,
                                  std::span<const std::string> params) const {
    std::string text = Resolve(key);
    return SubstituteParams(text, params);
}

std::string StringTable::ResolvePlural(std::string_view key, int64_t count) const {
    std::string countStr = std::to_string(count);
    std::array<std::string, 1> params = {countStr};
    return ResolvePlural(key, count, params);
}

std::string StringTable::ResolvePlural(std::string_view key, int64_t count,
                                        std::span<const std::string> params) const {
    PluralCategory cat = GetPluralCategory(m_activeLanguage, count);

    // Try translated plural form
    const std::string* translated = FindPluralTranslation(key, cat);
    if (translated) {
        // Build combined params: count as {0}, then user params as {1}, {2}, ...
        std::vector<std::string> allParams;
        allParams.push_back(std::to_string(count));
        for (const auto& p : params) allParams.push_back(p);
        return SubstituteParams(*translated, allParams);
    }

    // Try definition plural form
    auto defIt = m_definitions.find(std::string(key));
    if (defIt != m_definitions.end()) {
        const auto& entry = defIt->second;
        auto pluralIt = entry.pluralForms.find(cat);
        if (pluralIt != entry.pluralForms.end()) {
            std::vector<std::string> allParams;
            allParams.push_back(std::to_string(count));
            for (const auto& p : params) allParams.push_back(p);
            return SubstituteParams(pluralIt->second, allParams);
        }

        // Fall back to "other" plural form
        auto otherIt = entry.pluralForms.find(PluralCategory::Other);
        if (otherIt != entry.pluralForms.end()) {
            std::vector<std::string> allParams;
            allParams.push_back(std::to_string(count));
            for (const auto& p : params) allParams.push_back(p);
            return SubstituteParams(otherIt->second, allParams);
        }

        // Fall back to default value
        std::vector<std::string> allParams;
        allParams.push_back(std::to_string(count));
        for (const auto& p : params) allParams.push_back(p);
        return SubstituteParams(entry.defaultValue, allParams);
    }

    // Unknown key
    return std::string(key);
}

void StringTable::SetActiveLanguage(Language lang) {
    m_activeLanguage = lang;
}

Language StringTable::GetActiveLanguage() const {
    return m_activeLanguage;
}

bool StringTable::HasKey(std::string_view key) const {
    return m_definitions.contains(std::string(key));
}

size_t StringTable::KeyCount() const {
    return m_definitions.size();
}

std::vector<std::string> StringTable::GetUntranslatedKeys(Language lang) const {
    std::vector<std::string> result;

    auto langIt = m_translations.find(lang);

    for (const auto& [key, entry] : m_definitions) {
        bool hasTranslation = false;
        if (langIt != m_translations.end()) {
            hasTranslation = langIt->second.contains(key);
        }
        if (!hasTranslation) {
            result.push_back(key);
        }
    }

    return result;
}

// --- Plural rules (CLDR) ---

PluralCategory StringTable::GetPluralCategory(Language lang, int64_t count) {
    // Use absolute value for plural rules
    int64_t n = count < 0 ? -count : count;

    switch (lang) {
        case Language::Japanese:
        case Language::SimplifiedChinese:
        case Language::TraditionalChinese:
        case Language::Korean:
            return PluralCategory::Other;

        case Language::English:
        case Language::German:
        case Language::Italian:
        case Language::Spanish:
        case Language::BrazilianPortuguese:
            return (n == 1) ? PluralCategory::One : PluralCategory::Other;

        case Language::French:
            // CLDR: one when i = 0,1 (i.e. n is 0 or 1)
            return (n == 0 || n == 1) ? PluralCategory::One : PluralCategory::Other;

        case Language::Czech:
            if (n == 1) return PluralCategory::One;
            if (n >= 2 && n <= 4) return PluralCategory::Few;
            return PluralCategory::Other;

        case Language::Polish: {
            int64_t mod10 = n % 10;
            int64_t mod100 = n % 100;
            if (n == 1) return PluralCategory::One;
            if (mod10 >= 2 && mod10 <= 4 && !(mod100 >= 12 && mod100 <= 14))
                return PluralCategory::Few;
            if (mod10 == 0 || (mod10 >= 5 && mod10 <= 9) || (mod100 >= 12 && mod100 <= 14))
                return PluralCategory::Many;
            return PluralCategory::Other;
        }

        case Language::Russian: {
            int64_t mod10 = n % 10;
            int64_t mod100 = n % 100;
            if (mod10 == 1 && mod100 != 11) return PluralCategory::One;
            if (mod10 >= 2 && mod10 <= 4 && !(mod100 >= 12 && mod100 <= 14))
                return PluralCategory::Few;
            if (mod10 == 0 || (mod10 >= 5 && mod10 <= 9) || (mod100 >= 11 && mod100 <= 14))
                return PluralCategory::Many;
            return PluralCategory::Other;
        }
    }

    return PluralCategory::Other;
}

// --- Private helpers ---

std::string StringTable::SubstituteParams(const std::string& text,
                                            std::span<const std::string> params) const {
    if (params.empty()) return text;

    std::string result;
    result.reserve(text.size());

    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '{' && i + 1 < text.size()) {
            size_t start = i + 1;
            size_t end = start;
            while (end < text.size() && text[end] >= '0' && text[end] <= '9') ++end;

            if (end > start && end < text.size() && text[end] == '}') {
                size_t index = 0;
                auto [ptr, ec] = std::from_chars(text.data() + start, text.data() + end, index);
                if (ec == std::errc{} && index < params.size()) {
                    result += params[index];
                    i = end + 1;
                    continue;
                }
            }
        }
        result += text[i];
        ++i;
    }

    return result;
}

const std::string* StringTable::FindTranslation(std::string_view key) const {
    auto langIt = m_translations.find(m_activeLanguage);
    if (langIt == m_translations.end()) return nullptr;

    auto keyIt = langIt->second.find(std::string(key));
    if (keyIt == langIt->second.end()) return nullptr;

    if (keyIt->second.value.empty()) return nullptr;
    return &keyIt->second.value;
}

const std::string* StringTable::FindPluralTranslation(std::string_view key,
                                                        PluralCategory cat) const {
    auto langIt = m_translations.find(m_activeLanguage);
    if (langIt == m_translations.end()) return nullptr;

    auto keyIt = langIt->second.find(std::string(key));
    if (keyIt == langIt->second.end()) return nullptr;

    auto catIt = keyIt->second.pluralForms.find(cat);
    if (catIt == keyIt->second.pluralForms.end()) return nullptr;

    return &catIt->second;
}

}  // namespace ohui::i18n
