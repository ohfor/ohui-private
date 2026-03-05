#include <catch2/catch_test_macros.hpp>

#include "ohui/dsl/USSParser.h"

using namespace ohui::dsl;

TEST_CASE("Empty input produces empty rule set", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("");
    REQUIRE(result.has_value());
    CHECK(result->rules.empty());
    CHECK(result->customProperties.empty());
}

TEST_CASE("Type selector: Button { color: red; }", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { color: red; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);

    auto& rule = result->rules[0];
    REQUIRE(rule.selector.segments.size() == 1);
    REQUIRE(rule.selector.segments[0].size() == 1);
    CHECK(rule.selector.segments[0][0].type == SelectorType::Type);
    CHECK(rule.selector.segments[0][0].value == "Button");
    CHECK(rule.properties.at("color").value == "red");
}

TEST_CASE("Class selector: .highlighted { opacity: 0.8; }", "[uss]") {
    USSParser parser;
    auto result = parser.Parse(".highlighted { opacity: 0.8; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);

    auto& sel = result->rules[0].selector;
    REQUIRE(sel.segments.size() == 1);
    REQUIRE(sel.segments[0].size() == 1);
    CHECK(sel.segments[0][0].type == SelectorType::Class);
    CHECK(sel.segments[0][0].value == "highlighted");
    CHECK(result->rules[0].properties.at("opacity").value == "0.8");
}

TEST_CASE("ID selector: #main-panel { width: 100px; }", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("#main-panel { width: 100px; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);

    auto& sel = result->rules[0].selector;
    REQUIRE(sel.segments.size() == 1);
    REQUIRE(sel.segments[0].size() == 1);
    CHECK(sel.segments[0][0].type == SelectorType::Id);
    CHECK(sel.segments[0][0].value == "main-panel");
    CHECK(result->rules[0].properties.at("width").value == "100px");
}

TEST_CASE("Pseudo-class: Button:hover { color: blue; }", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button:hover { color: blue; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);

    auto& parts = result->rules[0].selector.segments[0];
    REQUIRE(parts.size() == 1);
    CHECK(parts[0].type == SelectorType::Type);
    CHECK(parts[0].value == "Button");
    CHECK(parts[0].pseudoClass == PseudoClass::Hover);
}

TEST_CASE("Compound selector: Button.primary:hover", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button.primary:hover { color: white; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);

    auto& parts = result->rules[0].selector.segments[0];
    // Should have Type(Button) + Class(primary) with :hover on class
    REQUIRE(parts.size() == 2);
    CHECK(parts[0].type == SelectorType::Type);
    CHECK(parts[0].value == "Button");
    CHECK(parts[1].type == SelectorType::Class);
    CHECK(parts[1].value == "primary");
    CHECK(parts[1].pseudoClass == PseudoClass::Hover);
}

TEST_CASE("Descendant combinator: Panel Button { margin: 5px; }", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Panel Button { margin: 5px; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);

    auto& sel = result->rules[0].selector;
    REQUIRE(sel.segments.size() == 2);
    CHECK(sel.segments[0][0].value == "Panel");
    CHECK(sel.segments[1][0].value == "Button");
}

TEST_CASE("Multiple rules preserve source order", "[uss]") {
    USSParser parser;
    auto result = parser.Parse(R"(
        Button { color: red; }
        .cls { opacity: 1.0; }
        #id { width: 50px; }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 3);
    CHECK(result->rules[0].selector.segments[0][0].value == "Button");
    CHECK(result->rules[1].selector.segments[0][0].value == "cls");
    CHECK(result->rules[2].selector.segments[0][0].value == "id");
}

TEST_CASE("Specificity calculation", "[uss]") {
    USSParser parser;

    SECTION("#id = (1,0,0)") {
        auto result = parser.Parse("#foo { width: 10px; }");
        REQUIRE(result.has_value());
        auto spec = result->rules[0].selector.ComputeSpecificity();
        CHECK(spec.ids == 1);
        CHECK(spec.classes == 0);
        CHECK(spec.types == 0);
    }

    SECTION(".class = (0,1,0)") {
        auto result = parser.Parse(".bar { width: 10px; }");
        REQUIRE(result.has_value());
        auto spec = result->rules[0].selector.ComputeSpecificity();
        CHECK(spec.ids == 0);
        CHECK(spec.classes == 1);
        CHECK(spec.types == 0);
    }

    SECTION("Type = (0,0,1)") {
        auto result = parser.Parse("Button { width: 10px; }");
        REQUIRE(result.has_value());
        auto spec = result->rules[0].selector.ComputeSpecificity();
        CHECK(spec.ids == 0);
        CHECK(spec.classes == 0);
        CHECK(spec.types == 1);
    }

    SECTION("Compound: Type.class:pseudo = at least (0,2,1)") {
        auto result = parser.Parse("Button.primary:hover { width: 10px; }");
        REQUIRE(result.has_value());
        auto spec = result->rules[0].selector.ComputeSpecificity();
        CHECK(spec.ids == 0);
        CHECK(spec.classes >= 2);
        CHECK(spec.types == 1);
    }
}

TEST_CASE("Specificity comparison: (1,0,0) > (0,10,0)", "[uss]") {
    Selector::Specificity a{1, 0, 0};
    Selector::Specificity b{0, 10, 0};
    CHECK(a > b);
}

TEST_CASE("Multiple properties in one rule", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { color: red; width: 100px; height: 50px; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);
    CHECK(result->rules[0].properties.size() == 3);
    CHECK(result->rules[0].properties.at("color").value == "red");
    CHECK(result->rules[0].properties.at("width").value == "100px");
    CHECK(result->rules[0].properties.at("height").value == "50px");
}

TEST_CASE("var() reference stored as-is", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { color: var(--primary); }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);
    CHECK(result->rules[0].properties.at("color").value == "var(--primary)");
}

TEST_CASE("var() with fallback preserved", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { color: var(--primary, #ff0000); }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);
    CHECK(result->rules[0].properties.at("color").value == "var(--primary, #ff0000)");
}

TEST_CASE("Custom property declaration goes to customProperties", "[uss]") {
    USSParser parser;
    auto result = parser.Parse(":root { --color-primary: #ff0000; --spacing: 10px; }");
    REQUIRE(result.has_value());
    CHECK(result->customProperties.at("--color-primary") == "#ff0000");
    CHECK(result->customProperties.at("--spacing") == "10px");
}

TEST_CASE("OHUI extension properties accepted", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { -ohui-glow: 1.5; -ohui-z-index: 10; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);
    CHECK(result->rules[0].properties.at("-ohui-glow").value == "1.5");
    CHECK(result->rules[0].properties.at("-ohui-z-index").value == "10");
}

TEST_CASE("Unknown property warned and skipped", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { fake-property: val; color: red; }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);
    CHECK(!result->rules[0].properties.contains("fake-property"));
    CHECK(result->rules[0].properties.contains("color"));
}

TEST_CASE("Block and line comments skipped", "[uss]") {
    USSParser parser;
    auto result = parser.Parse(R"(
        /* block comment */
        Button { color: red; }
        // line comment
        .cls { opacity: 0.5; }
    )");
    REQUIRE(result.has_value());
    CHECK(result->rules.size() == 2);
}

TEST_CASE("Malformed rule skipped, parsing continues", "[uss]") {
    USSParser parser;
    auto result = parser.Parse(R"(
        { color: red; }
        Button { width: 100px; }
    )");
    REQUIRE(result.has_value());
    // The malformed rule (no selector before {) should be skipped
    // Button rule should still parse
    CHECK(result->rules.size() >= 1);
    bool foundButton = false;
    for (auto& r : result->rules) {
        for (auto& seg : r.selector.segments) {
            for (auto& p : seg) {
                if (p.value == "Button") foundButton = true;
            }
        }
    }
    CHECK(foundButton);
}

TEST_CASE("Nested parentheses in values", "[uss]") {
    USSParser parser;
    auto result = parser.Parse("Button { color: rgba(255,0,0,0.5); }");
    REQUIRE(result.has_value());
    REQUIRE(result->rules.size() == 1);
    CHECK(result->rules[0].properties.at("color").value == "rgba(255,0,0,0.5)");
}
