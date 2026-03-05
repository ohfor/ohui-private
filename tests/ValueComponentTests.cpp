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

static size_t CountDrawTexts(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Text) ++count;
    }
    return count;
}

TEST_CASE("StatDelta positive delta uses positive color", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatDelta {
                label: "Damage";
                oldValue: 10;
                newValue: 15;
                width: 300;
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
    // Arrow and new value should be green (positive)
    auto* arrowText = FindDrawText(*result, 2);
    auto* newText = FindDrawText(*result, 3);
    REQUIRE(arrowText != nullptr);
    REQUIRE(newText != nullptr);
    CHECK(arrowText->color.g == 0xAF);  // positive green
    CHECK(newText->color.g == 0xAF);
}

TEST_CASE("StatDelta negative delta uses negative color", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatDelta {
                label: "Damage";
                oldValue: 15;
                newValue: 10;
                width: 300;
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
    auto* arrowText = FindDrawText(*result, 2);
    REQUIRE(arrowText != nullptr);
    CHECK(arrowText->color.r == 0xF4);  // negative red
}

TEST_CASE("StatDelta zero delta uses secondary color", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatDelta {
                label: "Damage";
                oldValue: 10;
                newValue: 10;
                width: 300;
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
    auto* arrowText = FindDrawText(*result, 2);
    REQUIRE(arrowText != nullptr);
    CHECK(arrowText->color.r == 0xA0);  // secondary grey
}

TEST_CASE("StatDelta renders old -> new with arrow", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatDelta {
                oldValue: 10;
                newValue: 20;
                width: 300;
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
    // old + arrow + new = 3 texts (no label)
    CHECK(CountDrawTexts(*result) >= 3);
}

TEST_CASE("DeltaList stacks children vertically (flex column)", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            DeltaList {
                flex-direction: column;
                width: 300;
                height: 100;
                StatDelta {
                    oldValue: 10;
                    newValue: 15;
                    height: 20;
                }
                StatDelta {
                    oldValue: 5;
                    newValue: 3;
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
    // Two StatDeltas, each producing 3 text calls
    CHECK(CountDrawTexts(*result) >= 6);
}

TEST_CASE("DeltaList with 3 StatDeltas produces correct layout", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            DeltaList {
                flex-direction: column;
                width: 300;
                height: 100;
                StatDelta { oldValue: 1; newValue: 2; height: 20; }
                StatDelta { oldValue: 3; newValue: 4; height: 20; }
                StatDelta { oldValue: 5; newValue: 6; height: 20; }
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
    CHECK(CountDrawTexts(*result) >= 9);  // 3 * 3 texts min
}

TEST_CASE("ValueBar at 50% fills half width", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ValueBar {
                value: 50;
                maxValue: 100;
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

TEST_CASE("ValueBar at 0% produces zero-width fill", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ValueBar {
                value: 0;
                maxValue: 100;
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("ValueBar at 100% fills full width", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ValueBar {
                value: 100;
                maxValue: 100;
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(200.0f, 1.0f));
}

TEST_CASE("ValueBar token-based colors resolve correctly", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ValueBar {
                value: 50;
                maxValue: 100;
                fillColor: token(--color-resource-health);
                width: 200;
                height: 20;
            }
        }
    )");
    TokenStore tokens;
    REQUIRE(tokens.LoadBaseTokens(":root { --color-resource-health: #8B0000; }").has_value());
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK(fill->fillColor.r == 0x8B);
}

TEST_CASE("TimerBar at 100% uses default fill color", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TimerBar {
                value: 100;
                maxValue: 100;
                fillColor: #8090A0;
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK(fill->fillColor.r == 0x80);
    CHECK(fill->fillColor.g == 0x90);
}

TEST_CASE("TimerBar below first threshold changes color", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TimerBar {
                value: 40;
                maxValue: 100;
                fillColor: #FFFFFF;
                thresholds: "0.5,0.25";
                thresholdColors: "#FFC107,#FF1744";
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // ratio = 0.4, below 0.5 threshold -> #FFC107
    CHECK(fill->fillColor.r == 0xFF);
    CHECK(fill->fillColor.g == 0xC1);
}

TEST_CASE("TimerBar below second threshold changes color again", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TimerBar {
                value: 20;
                maxValue: 100;
                fillColor: #FFFFFF;
                thresholds: "0.5,0.25";
                thresholdColors: "#FFC107,#FF1744";
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // ratio = 0.2, below both thresholds -> #FF1744
    CHECK(fill->fillColor.r == 0xFF);
    CHECK(fill->fillColor.g == 0x17);
}

TEST_CASE("TimerBar at 0% uses final threshold color", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TimerBar {
                value: 0;
                maxValue: 100;
                fillColor: #FFFFFF;
                thresholds: "0.5,0.25";
                thresholdColors: "#FFC107,#FF1744";
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
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK(fill->fillColor.r == 0xFF);
    CHECK(fill->fillColor.g == 0x17);
}

TEST_CASE("CountBadge renders count text with background", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CountBadge {
                count: 5;
                width: 24;
                height: 24;
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
    auto* bg = FindDrawRect(*result, 0);
    auto* text = FindDrawText(*result, 0);
    REQUIRE(bg != nullptr);
    REQUIRE(text != nullptr);
    CHECK(text->text == "5");
}

TEST_CASE("CountBadge pill shape uses pill radius", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CountBadge {
                count: 3;
                shape: pill;
                width: 40;
                height: 24;
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
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->borderRadius, Catch::Matchers::WithinAbs(9999.0f, 0.01f));
}

TEST_CASE("CountBadge maxDisplay clips count (e.g., 99+)", "[value-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CountBadge {
                count: 150;
                maxDisplay: 99;
                width: 40;
                height: 24;
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
    auto* text = FindDrawText(*result, 0);
    REQUIRE(text != nullptr);
    CHECK(text->text == "99+");
}
