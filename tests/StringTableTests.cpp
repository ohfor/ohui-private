#include <catch2/catch_test_macros.hpp>

#include "ohui/i18n/StringTable.h"

using namespace ohui;
using namespace ohui::i18n;

// --- LoadDefinitions ---

TEST_CASE("LoadDefinitions: single key", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions(R"($GREETING "Hello, world!")");
    REQUIRE(result.has_value());
    CHECK(*result == 1);
    CHECK(table.Resolve("$GREETING") == "Hello, world!");
}

TEST_CASE("LoadDefinitions: multiple keys returns correct count", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions(
        "$HELLO \"Hello\"\n"
        "$GOODBYE \"Goodbye\"\n"
        "$THANKS \"Thank you\"\n"
    );
    REQUIRE(result.has_value());
    CHECK(*result == 3);
}

TEST_CASE("LoadDefinitions: key with positional parameters stored verbatim", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($MSG "You have {0} items in {1}")");
    CHECK(table.Resolve("$MSG") == "You have {0} items in {1}");
}

TEST_CASE("LoadDefinitions: plural form block parsed", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions(
        "$ITEM_COUNT {\n"
        "    one \"{0} item\"\n"
        "    other \"{0} items\"\n"
        "}\n"
    );
    REQUIRE(result.has_value());
    CHECK(*result == 1);
    CHECK(table.HasKey("$ITEM_COUNT"));
}

TEST_CASE("LoadDefinitions: empty input returns 0", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions("");
    REQUIRE(result.has_value());
    CHECK(*result == 0);
}

TEST_CASE("LoadDefinitions: malformed key returns ParseError", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions("$ \"value\"");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::ParseError);
}

TEST_CASE("LoadDefinitions: missing quotes returns ParseError", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions("$KEY value_without_quotes");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::ParseError);
}

// --- LoadTranslations ---

TEST_CASE("LoadTranslations: overwrites default for language", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($HELLO "Hello")");
    (void)table.LoadTranslations(Language::French, R"($HELLO "Bonjour")");

    table.SetActiveLanguage(Language::French);
    CHECK(table.Resolve("$HELLO") == "Bonjour");
}

TEST_CASE("LoadTranslations: partial translation (some keys fall back)", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(
        "$HELLO \"Hello\"\n"
        "$GOODBYE \"Goodbye\"\n"
    );
    (void)table.LoadTranslations(Language::German, R"($HELLO "Hallo")");

    table.SetActiveLanguage(Language::German);
    CHECK(table.Resolve("$HELLO") == "Hallo");
    CHECK(table.Resolve("$GOODBYE") == "Goodbye");  // falls back to default
}

// --- Resolve ---

TEST_CASE("Resolve: simple key returns value", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($TEST "Test value")");
    CHECK(table.Resolve("$TEST") == "Test value");
}

TEST_CASE("Resolve: missing key returns key itself (not empty)", "[i18n]") {
    StringTable table;
    std::string result = table.Resolve("$MISSING_KEY");
    CHECK(result == "$MISSING_KEY");
    CHECK(!result.empty());
}

TEST_CASE("Resolve: with parameters substituted", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($MSG "Hello {0}, welcome to {1}!")");
    std::vector<std::string> params = {"Alice", "Skyrim"};
    CHECK(table.Resolve("$MSG", params) == "Hello Alice, welcome to Skyrim!");
}

TEST_CASE("Resolve: parameter out of range left as-is", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($MSG "Value: {0} and {5}")");
    std::vector<std::string> params = {"first"};
    CHECK(table.Resolve("$MSG", params) == "Value: first and {5}");
}

TEST_CASE("Resolve: active language translation used", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($HELLO "Hello")");
    (void)table.LoadTranslations(Language::Spanish, R"($HELLO "Hola")");

    table.SetActiveLanguage(Language::Spanish);
    CHECK(table.Resolve("$HELLO") == "Hola");
}

TEST_CASE("Resolve: fallback to default when no translation", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($HELLO "Hello")");

    table.SetActiveLanguage(Language::Japanese);
    CHECK(table.Resolve("$HELLO") == "Hello");
}

// --- ResolvePlural ---

TEST_CASE("ResolvePlural: English one/other", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(
        "$ITEMS {\n"
        "    one \"{0} item\"\n"
        "    other \"{0} items\"\n"
        "}\n"
    );

    table.SetActiveLanguage(Language::English);
    CHECK(table.ResolvePlural("$ITEMS", 1) == "1 item");
    CHECK(table.ResolvePlural("$ITEMS", 0) == "0 items");
    CHECK(table.ResolvePlural("$ITEMS", 5) == "5 items");
}

TEST_CASE("ResolvePlural: Russian one/few/many/other (1, 2, 5, 21)", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(
        "$COINS {\n"
        "    one \"{0} moneta\"\n"
        "    few \"{0} monety\"\n"
        "    many \"{0} monet\"\n"
        "    other \"{0} monet\"\n"
        "}\n"
    );

    table.SetActiveLanguage(Language::Russian);
    CHECK(table.ResolvePlural("$COINS", 1) == "1 moneta");
    CHECK(table.ResolvePlural("$COINS", 2) == "2 monety");
    CHECK(table.ResolvePlural("$COINS", 5) == "5 monet");
    CHECK(table.ResolvePlural("$COINS", 21) == "21 moneta");
}

TEST_CASE("ResolvePlural: count substituted as {0}", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(
        "$COUNT {\n"
        "    one \"{0} thing\"\n"
        "    other \"{0} things\"\n"
        "}\n"
    );

    CHECK(table.ResolvePlural("$COUNT", 42) == "42 things");
}

TEST_CASE("ResolvePlural: with additional parameters", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(
        "$LOOT {\n"
        "    one \"{0} item from {1}\"\n"
        "    other \"{0} items from {1}\"\n"
        "}\n"
    );

    std::vector<std::string> params = {"a chest"};
    CHECK(table.ResolvePlural("$LOOT", 3, params) == "3 items from a chest");
    CHECK(table.ResolvePlural("$LOOT", 1, params) == "1 item from a chest");
}

// --- SetActiveLanguage ---

TEST_CASE("SetActiveLanguage changes resolution", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(R"($HELLO "Hello")");
    (void)table.LoadTranslations(Language::French, R"($HELLO "Bonjour")");
    (void)table.LoadTranslations(Language::German, R"($HELLO "Hallo")");

    table.SetActiveLanguage(Language::French);
    CHECK(table.Resolve("$HELLO") == "Bonjour");

    table.SetActiveLanguage(Language::German);
    CHECK(table.Resolve("$HELLO") == "Hallo");

    table.SetActiveLanguage(Language::English);
    CHECK(table.Resolve("$HELLO") == "Hello");
}

// --- GetUntranslatedKeys ---

TEST_CASE("GetUntranslatedKeys returns keys missing for language", "[i18n]") {
    StringTable table;
    (void)table.LoadDefinitions(
        "$A \"a\"\n"
        "$B \"b\"\n"
        "$C \"c\"\n"
    );
    (void)table.LoadTranslations(Language::French, R"($A "aa")");

    auto untranslated = table.GetUntranslatedKeys(Language::French);
    CHECK(untranslated.size() == 2);
    // Should contain $B and $C but not $A
    bool hasB = false, hasC = false, hasA = false;
    for (const auto& k : untranslated) {
        if (k == "$B") hasB = true;
        if (k == "$C") hasC = true;
        if (k == "$A") hasA = true;
    }
    CHECK(hasB);
    CHECK(hasC);
    CHECK(!hasA);
}

// --- HasKey / KeyCount ---

TEST_CASE("HasKey/KeyCount correct", "[i18n]") {
    StringTable table;
    CHECK(table.KeyCount() == 0);
    CHECK(!table.HasKey("$X"));

    (void)table.LoadDefinitions("$X \"x\"\n$Y \"y\"");
    CHECK(table.KeyCount() == 2);
    CHECK(table.HasKey("$X"));
    CHECK(table.HasKey("$Y"));
    CHECK(!table.HasKey("$Z"));
}

// --- GetPluralCategory ---

TEST_CASE("GetPluralCategory: English (1=one, else other)", "[i18n]") {
    CHECK(StringTable::GetPluralCategory(Language::English, 0) == PluralCategory::Other);
    CHECK(StringTable::GetPluralCategory(Language::English, 1) == PluralCategory::One);
    CHECK(StringTable::GetPluralCategory(Language::English, 2) == PluralCategory::Other);
    CHECK(StringTable::GetPluralCategory(Language::English, 100) == PluralCategory::Other);
}

TEST_CASE("GetPluralCategory: Russian (complex rules)", "[i18n]") {
    CHECK(StringTable::GetPluralCategory(Language::Russian, 1) == PluralCategory::One);
    CHECK(StringTable::GetPluralCategory(Language::Russian, 2) == PluralCategory::Few);
    CHECK(StringTable::GetPluralCategory(Language::Russian, 5) == PluralCategory::Many);
    CHECK(StringTable::GetPluralCategory(Language::Russian, 11) == PluralCategory::Many);
    CHECK(StringTable::GetPluralCategory(Language::Russian, 21) == PluralCategory::One);
    CHECK(StringTable::GetPluralCategory(Language::Russian, 22) == PluralCategory::Few);
    CHECK(StringTable::GetPluralCategory(Language::Russian, 25) == PluralCategory::Many);
}

TEST_CASE("GetPluralCategory: Japanese (always other)", "[i18n]") {
    CHECK(StringTable::GetPluralCategory(Language::Japanese, 0) == PluralCategory::Other);
    CHECK(StringTable::GetPluralCategory(Language::Japanese, 1) == PluralCategory::Other);
    CHECK(StringTable::GetPluralCategory(Language::Japanese, 100) == PluralCategory::Other);
}

// --- Comments ---

TEST_CASE("Comments (// and /* */) stripped in definition files", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions(
        "// This is a comment\n"
        "$HELLO \"Hello\" // inline comment\n"
        "/* block\n"
        "   comment */\n"
        "$WORLD \"World\"\n"
    );
    REQUIRE(result.has_value());
    CHECK(*result == 2);
    CHECK(table.Resolve("$HELLO") == "Hello");
    CHECK(table.Resolve("$WORLD") == "World");
}

// --- Escaped quotes ---

TEST_CASE("Escaped quotes in values handled", "[i18n]") {
    StringTable table;
    auto result = table.LoadDefinitions(R"($QUOTE "He said \"hello\" to me")");
    REQUIRE(result.has_value());
    CHECK(table.Resolve("$QUOTE") == R"(He said "hello" to me)");
}
