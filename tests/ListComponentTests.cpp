#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/dsl/ComponentHandler.h"
#include "ohui/dsl/DSLRuntimeEngine.h"
#include "ohui/dsl/WidgetParser.h"

using namespace ohui;
using namespace ohui::dsl;
using namespace ohui::binding;
using namespace ohui::widget;

static ComponentRegistry MakeRegistry() {
    ComponentRegistry reg;
    reg.RegisterBuiltins();
    return reg;
}

static ParseResult ParseInput(const std::string& input) {
    WidgetParser parser;
    auto result = parser.Parse(input);
    REQUIRE(result.has_value());
    return std::move(*result);
}

static const DrawRect* FindDrawRect(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Rect) {
            if (count == index) return &std::get<DrawRect>(call.data);
            ++count;
        }
    }
    return nullptr;
}

static const DrawText* FindDrawText(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Text) {
            if (count == index) return &std::get<DrawText>(call.data);
            ++count;
        }
    }
    return nullptr;
}

static const DrawIcon* FindDrawIcon(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Icon) {
            if (count == index) return &std::get<DrawIcon>(call.data);
            ++count;
        }
    }
    return nullptr;
}

static size_t CountDrawRects(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Rect) ++count;
    }
    return count;
}

static size_t CountDrawTexts(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Text) ++count;
    }
    return count;
}

static size_t CountDrawIcons(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Icon) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// ListEntry
// ---------------------------------------------------------------------------

TEST_CASE("ListEntry emits icon + primary text + secondary text", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ListEntry {
                icon: "icons/sword.png";
                primaryText: "Iron Sword";
                secondaryText: "One-Handed Weapon";
                width: 400;
                height: 48;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // Should have: 1 icon, at least 2 texts (primary + secondary)
    CHECK(CountDrawIcons(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 2);

    auto* icon = FindDrawIcon(*result);
    REQUIRE(icon != nullptr);
    CHECK(icon->source == "icons/sword.png");

    auto* primary = FindDrawText(*result, 0);
    REQUIRE(primary != nullptr);
    CHECK(primary->text == "Iron Sword");

    auto* secondary = FindDrawText(*result, 1);
    REQUIRE(secondary != nullptr);
    CHECK(secondary->text == "One-Handed Weapon");
}

TEST_CASE("ListEntry without icon omits icon draw call", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ListEntry {
                primaryText: "Iron Sword";
                secondaryText: "One-Handed Weapon";
                width: 400;
                height: 48;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawIcons(*result) == 0);
    CHECK(CountDrawTexts(*result) >= 2);
}

TEST_CASE("ListEntry with right slot child renders at end", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ListEntry {
                primaryText: "Iron Sword";
                width: 400;
                height: 48;
                Label {
                    text: "99";
                    width: 30;
                    height: 20;
                }
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // The child Label should produce its own DrawText
    CHECK(CountDrawTexts(*result) >= 2);
    // Find a text with "99" (the child label)
    bool foundChildLabel = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "99") {
                foundChildLabel = true;
                break;
            }
        }
    }
    CHECK(foundChildLabel);
}

TEST_CASE("ListEntry indicator dots render at right edge", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ListEntry {
                primaryText: "Quest Item";
                indicators: 2;
                width: 400;
                height: 48;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // 2 indicator dots = 2 DrawRects
    CHECK(CountDrawRects(*result) >= 2);

    // The indicator dots should be near the right edge (x > 350 for a 400px width)
    auto* dot0 = FindDrawRect(*result, 0);
    REQUIRE(dot0 != nullptr);
    CHECK(dot0->x > 350.0f);
    CHECK_THAT(dot0->width, Catch::Matchers::WithinAbs(6.0f, 0.01f));
}

// ---------------------------------------------------------------------------
// ListEntryCompact
// ---------------------------------------------------------------------------

TEST_CASE("ListEntryCompact has smaller height than ListEntry", "[list-component]") {
    // Compact entry uses fontSize 12 and y offset 2, vs ListEntry fontSize 14 and y offset 4
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ListEntryCompact {
                primaryText: "Iron Sword";
                width: 400;
                height: 28;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Iron Sword");
    // Compact uses y offset 2.0 (vs 4.0 in ListEntry)
    CHECK_THAT(text->y, Catch::Matchers::WithinAbs(2.0f, 0.01f));
    // Compact uses fontSize 12 (vs 14 in ListEntry)
    CHECK_THAT(text->fontSize, Catch::Matchers::WithinAbs(12.0f, 0.01f));
}

TEST_CASE("ListEntryCompact omits secondary text", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ListEntryCompact {
                primaryText: "Iron Sword";
                width: 400;
                height: 28;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // Only 1 DrawText (primary), no secondary
    CHECK(CountDrawTexts(*result) == 1);
}

// ---------------------------------------------------------------------------
// ScrollList
// ---------------------------------------------------------------------------

TEST_CASE("ScrollList with 3 items all visible: 3 sets of draw calls", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollList {
                itemCount: 3;
                itemHeight: 48;
                scrollOffset: 0;
                viewportHeight: 300;
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // 3 items, each with DrawRect + DrawText = 3 rects + 3 texts
    CHECK(CountDrawRects(*result) == 3);
    CHECK(CountDrawTexts(*result) == 3);
}

TEST_CASE("ScrollList with 10 items viewport shows 4: only 4 emitted", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollList {
                itemCount: 10;
                itemHeight: 48;
                scrollOffset: 0;
                viewportHeight: 192;
                width: 400;
                height: 192;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // viewportHeight 192 / itemHeight 48 = exactly 4 items
    CHECK(CountDrawRects(*result) == 4);
    CHECK(CountDrawTexts(*result) == 4);
}

TEST_CASE("ScrollList scroll offset changes which items are visible", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollList {
                itemCount: 10;
                itemHeight: 48;
                scrollOffset: 96;
                viewportHeight: 192;
                width: 400;
                height: 192;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // scrollOffset 96 / itemHeight 48 = first visible is item 2
    // (96 + 192) / 48 = 6, so items 2..5 visible
    // Verify the first text starts with "Item 2"
    auto* firstText = FindDrawText(*result, 0);
    REQUIRE(firstText != nullptr);
    CHECK(firstText->text == "Item 2");
}

TEST_CASE("ScrollList at bottom shows last items", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollList {
                itemCount: 10;
                itemHeight: 48;
                scrollOffset: 384;
                viewportHeight: 96;
                width: 400;
                height: 96;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // scrollOffset 384 / itemHeight 48 = first visible is item 8
    // (384 + 96) / 48 = 10, so items 8..9 visible
    CHECK(CountDrawRects(*result) == 2);

    auto* firstText = FindDrawText(*result, 0);
    REQUIRE(firstText != nullptr);
    CHECK(firstText->text == "Item 8");

    auto* lastText = FindDrawText(*result, 1);
    REQUIRE(lastText != nullptr);
    CHECK(lastText->text == "Item 9");
}

TEST_CASE("ScrollList empty list emits no item draw calls", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollList {
                itemCount: 0;
                itemHeight: 48;
                scrollOffset: 0;
                viewportHeight: 300;
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    CHECK(result->calls.empty());
}

// ---------------------------------------------------------------------------
// FacetedList
// ---------------------------------------------------------------------------

TEST_CASE("FacetedList renders SearchField + ScrollList", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Sword,Shield,Bow";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // Should have: background rect + search bg rect + search text + item rects + item texts
    CHECK(CountDrawRects(*result) >= 2);
    CHECK(CountDrawTexts(*result) >= 1);

    // The search text should say "Search..." (placeholder) or be present
    auto* searchText = FindDrawText(*result, 0);
    REQUIRE(searchText != nullptr);
    CHECK(searchText->text == "Search...");
}

TEST_CASE("FacetedList with PresetBar renders preset buttons", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Sword,Shield,Bow";
                presets: "All,Weapons,Armor";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());

    // Should have preset button texts
    bool foundPreset = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "All" || dt.text == "Weapons" || dt.text == "Armor") {
                foundPreset = true;
                break;
            }
        }
    }
    CHECK(foundPreset);
}

TEST_CASE("FacetedList active preset filters ScrollList items", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Iron Sword,Steel Shield,Elven Bow";
                activeFilters: "Sword";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());

    // Only "Iron Sword" should pass the "Sword" filter
    bool foundSword = false;
    bool foundShield = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "Iron Sword") foundSword = true;
            if (dt.text == "Steel Shield") foundShield = true;
        }
    }
    CHECK(foundSword);
    CHECK_FALSE(foundShield);
}

TEST_CASE("FacetedList search text filters by content match", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Iron Sword,Steel Shield,Elven Bow";
                searchText: "steel";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());

    // Only "Steel Shield" matches "steel" (case insensitive)
    bool foundShield = false;
    bool foundSword = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "Steel Shield") foundShield = true;
            if (dt.text == "Iron Sword") foundSword = true;
        }
    }
    CHECK(foundShield);
    CHECK_FALSE(foundSword);
}

TEST_CASE("FacetedList ActiveFilterChips shows active facet chips", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Iron Sword,Steel Shield";
                activeFilters: "Iron,Sword";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());

    // Should have chip DrawRects with accent fill color
    bool foundAccentChip = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Rect) {
            auto& dr = std::get<DrawRect>(call.data);
            if (dr.fillColor.r == 0x80 && dr.fillColor.g == 0x90 && dr.fillColor.b == 0xA0) {
                foundAccentChip = true;
                break;
            }
        }
    }
    CHECK(foundAccentChip);

    // Should have chip texts "Iron" and "Sword"
    bool foundIronChip = false;
    bool foundSwordChip = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "Iron") foundIronChip = true;
            if (dt.text == "Sword") foundSwordChip = true;
        }
    }
    CHECK(foundIronChip);
    CHECK(foundSwordChip);
}

TEST_CASE("FacetedList removing chip removes that filter", "[list-component]") {
    // With only "Shield" filter, "Steel Shield" matches but "Iron Sword" does not
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Iron Sword,Steel Shield,Elven Bow";
                activeFilters: "Shield";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());

    // "Steel Shield" should match; "Iron Sword" and "Elven Bow" should not
    bool foundShield = false;
    bool foundSword = false;
    bool foundBow = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "Steel Shield") foundShield = true;
            if (dt.text == "Iron Sword") foundSword = true;
            if (dt.text == "Elven Bow") foundBow = true;
        }
    }
    CHECK(foundShield);
    CHECK_FALSE(foundSword);
    CHECK_FALSE(foundBow);
}

TEST_CASE("FacetedList multiple facets AND-combined", "[list-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            FacetedList {
                items: "Iron Sword,Iron Shield,Steel Sword,Steel Shield";
                activeFilters: "Iron,Shield";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());

    // Only "Iron Shield" matches both "Iron" AND "Shield"
    bool foundIronShield = false;
    bool foundIronSword = false;
    bool foundSteelSword = false;
    bool foundSteelShield = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "Iron Shield") foundIronShield = true;
            if (dt.text == "Iron Sword") foundIronSword = true;
            if (dt.text == "Steel Sword") foundSteelSword = true;
            if (dt.text == "Steel Shield") foundSteelShield = true;
        }
    }
    CHECK(foundIronShield);
    CHECK_FALSE(foundIronSword);
    CHECK_FALSE(foundSteelSword);
    CHECK_FALSE(foundSteelShield);
}
