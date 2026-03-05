#pragma once

#include "ohui/i18n/StringTypes.h"
#include "ohui/core/Result.h"

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::i18n {

class StringTable {
public:
    Result<size_t> LoadDefinitions(std::string_view content);
    Result<size_t> LoadTranslations(Language lang, std::string_view content);

    std::string Resolve(std::string_view key) const;
    std::string Resolve(std::string_view key, std::span<const std::string> params) const;
    std::string ResolvePlural(std::string_view key, int64_t count) const;
    std::string ResolvePlural(std::string_view key, int64_t count,
                              std::span<const std::string> params) const;

    void SetActiveLanguage(Language lang);
    Language GetActiveLanguage() const;

    bool HasKey(std::string_view key) const;
    size_t KeyCount() const;
    std::vector<std::string> GetUntranslatedKeys(Language lang) const;

    static PluralCategory GetPluralCategory(Language lang, int64_t count);

private:
    Language m_activeLanguage{Language::English};
    std::unordered_map<std::string, StringEntry> m_definitions;

    struct TranslationEntry {
        std::string value;
        std::unordered_map<PluralCategory, std::string> pluralForms;
    };
    std::unordered_map<Language, std::unordered_map<std::string, TranslationEntry>> m_translations;

    std::string SubstituteParams(const std::string& text,
                                  std::span<const std::string> params) const;
    const std::string* FindTranslation(std::string_view key) const;
    const std::string* FindPluralTranslation(std::string_view key,
                                              PluralCategory cat) const;
};

}  // namespace ohui::i18n
