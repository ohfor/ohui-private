#pragma once

#include <string>
#include <unordered_map>

namespace ohui::i18n {

enum class Language {
    English,
    French,
    German,
    Italian,
    Spanish,
    Polish,
    BrazilianPortuguese,
    Russian,
    Japanese,
    SimplifiedChinese,
    TraditionalChinese,
    Korean,
    Czech
};

enum class PluralCategory { Zero, One, Two, Few, Many, Other };

struct StringEntry {
    std::string key;
    std::string defaultValue;
    std::unordered_map<PluralCategory, std::string> pluralForms;
};

}  // namespace ohui::i18n
