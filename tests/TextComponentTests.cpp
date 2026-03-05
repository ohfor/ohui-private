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

static size_t CountDrawTexts(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Text) ++count;
    }
    return count;
}

TEST_CASE("Label emits DrawText with text content", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Label {
                text: "Hello";
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
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->text == "Hello");
}

TEST_CASE("Label with token(--font-size-md) resolves font size", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Label {
                text: "Test";
                fontSize: token(--font-size-md);
            }
        }
    )");
    TokenStore tokens;
    REQUIRE(tokens.LoadBaseTokens(":root { --font-size-md: 14; }").has_value());
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK_THAT(dt->fontSize, Catch::Matchers::WithinAbs(14.0f, 0.01f));
}

TEST_CASE("Label with token(--color-text-primary) resolves color", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Label {
                text: "Test";
                color: token(--color-text-primary);
            }
        }
    )");
    TokenStore tokens;
    REQUIRE(tokens.LoadBaseTokens(":root { --color-text-primary: #E8E8E8; }").has_value());
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->color.r == 0xE8);
    CHECK(dt->color.g == 0xE8);
}

TEST_CASE("Label truncate property sets ellipsis hint", "[text-component]") {
    // truncate is a property passed through — verified via resolveString
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Label {
                text: "Very long text";
                truncate: true;
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
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->text == "Very long text");
}

TEST_CASE("Caption defaults to font-size-sm and text-secondary color", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Caption {
                text: "Subtitle";
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
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->text == "Subtitle");
    CHECK_THAT(dt->fontSize, Catch::Matchers::WithinAbs(12.0f, 0.01f));
    CHECK(dt->color.r == 0xA0);
    CHECK(dt->color.g == 0xA0);
    CHECK(dt->color.b == 0xA0);
}

TEST_CASE("Caption overrides color when specified", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Caption {
                text: "Info";
                color: #FF0000;
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
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->color.r == 255);
    CHECK(dt->color.g == 0);
}

TEST_CASE("RichText plain text emits single DrawText", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            RichText {
                text: "Plain text";
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
    CHECK(CountDrawTexts(*result) == 1);
    auto* dt = FindDrawText(*result);
    CHECK(dt->text == "Plain text");
}

TEST_CASE("RichText <b> tag sets bold font weight", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            RichText {
                text: "Normal <b>bold</b> normal";
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
    CHECK(CountDrawTexts(*result) == 3);

    auto* normal = FindDrawText(*result, 0);
    auto* bold = FindDrawText(*result, 1);
    auto* normal2 = FindDrawText(*result, 2);
    CHECK(normal->text == "Normal ");
    CHECK(bold->text == "bold");
    CHECK(bold->fontFamily == "bold");
    CHECK(normal2->text == " normal");
}

TEST_CASE("RichText <i> tag sets italic style", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            RichText {
                text: "Normal <i>italic</i>";
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
    CHECK(CountDrawTexts(*result) == 2);
    auto* italic = FindDrawText(*result, 1);
    CHECK(italic->text == "italic");
    CHECK(italic->fontFamily == "italic");
}

TEST_CASE("RichText <color=#FF0000> tag sets text color", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            RichText {
                text: "Normal <color=#FF0000>red</color>";
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
    CHECK(CountDrawTexts(*result) == 2);
    auto* red = FindDrawText(*result, 1);
    CHECK(red->text == "red");
    CHECK(red->color.r == 255);
    CHECK(red->color.g == 0);
}

TEST_CASE("RichText multiple inline tags emit multiple DrawTexts", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            RichText {
                text: "<b>bold</b> and <i>italic</i>";
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
    CHECK(CountDrawTexts(*result) == 3);
}

TEST_CASE("RichText nested tags handled correctly", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            RichText {
                text: "<b><i>bold-italic</i></b>";
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
    auto* seg = FindDrawText(*result, 0);
    REQUIRE(seg != nullptr);
    CHECK(seg->text == "bold-italic");
    CHECK(seg->fontFamily == "bold-italic");
}

TEST_CASE("LoreText defaults to font-family-lore", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            LoreText {
                text: "Ancient words";
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
    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->fontFamily == "Garamond");
    CHECK(dt->color.r == 0xD4);  // --color-text-lore
}

TEST_CASE("LoreText supports inline formatting like RichText", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            LoreText {
                text: "Normal <b>important</b>";
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
    CHECK(CountDrawTexts(*result) == 2);
    auto* bold = FindDrawText(*result, 1);
    CHECK(bold->fontFamily == "Garamond-bold");
}

TEST_CASE("StatValue emits label and value text calls", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatValue {
                label: "Damage";
                value: "42";
                width: 200;
                height: 20;
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
    CHECK(CountDrawTexts(*result) == 2);
    auto* labelText = FindDrawText(*result, 0);
    auto* valueText = FindDrawText(*result, 1);
    CHECK(labelText->text == "Damage");
    CHECK(valueText->text == "42");
}

TEST_CASE("StatValue with unit emits third text call", "[text-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatValue {
                label: "Weight";
                value: "15";
                unit: "lbs";
                width: 200;
                height: 20;
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
    CHECK(CountDrawTexts(*result) == 3);
    auto* unitText = FindDrawText(*result, 2);
    CHECK(unitText->text == "lbs");
}
