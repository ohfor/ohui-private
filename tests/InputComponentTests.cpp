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

static const DrawLine* FindDrawLine(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Line) {
            if (count == index) return &std::get<DrawLine>(call.data);
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
// TextInput
// ---------------------------------------------------------------------------

TEST_CASE("TextInput emits background DrawRect + value DrawText", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TextInput {
                value: "hello";
                width: 200;
                height: 30;
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
    auto* bg = FindDrawRect(*result);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->width, Catch::Matchers::WithinAbs(200.0f, 1.0f));

    auto* dt = FindDrawText(*result);
    REQUIRE(dt != nullptr);
    CHECK(dt->text == "hello");
}

TEST_CASE("TextInput with placeholder shows placeholder when empty", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TextInput {
                placeholder: "Search...";
                width: 200;
                height: 30;
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
    CHECK(dt->text == "Search...");
}

TEST_CASE("TextInput placeholder uses text-disabled color", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TextInput {
                placeholder: "Type here";
                width: 200;
                height: 30;
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
    CHECK(dt->color.r == 0x60);
    CHECK(dt->color.g == 0x60);
    CHECK(dt->color.b == 0x60);
}

TEST_CASE("TextInput cursor DrawLine at end of text", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TextInput {
                value: "hi";
                width: 200;
                height: 30;
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
    auto* cursor = FindDrawLine(*result);
    REQUIRE(cursor != nullptr);
    // Cursor is a vertical line (x1 == x2)
    CHECK_THAT(cursor->x1, Catch::Matchers::WithinAbs(cursor->x2, 0.01f));
    CHECK(cursor->y1 < cursor->y2);
}

// ---------------------------------------------------------------------------
// SearchField
// ---------------------------------------------------------------------------

TEST_CASE("SearchField emits search icon + TextInput", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SearchField {
                value: "test";
                width: 200;
                height: 30;
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
    CHECK(CountDrawIcons(*result) >= 1);
    auto* icon = FindDrawIcon(*result);
    REQUIRE(icon != nullptr);
    CHECK(icon->source == "icons/search.png");
}

TEST_CASE("SearchField clear button appears when text present", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SearchField {
                value: "test";
                width: 200;
                height: 30;
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
    CHECK(CountDrawIcons(*result) >= 2);
    auto* clearIcon = FindDrawIcon(*result, 1);
    REQUIRE(clearIcon != nullptr);
    CHECK(clearIcon->source == "icons/clear.png");
}

// ---------------------------------------------------------------------------
// Toggle
// ---------------------------------------------------------------------------

TEST_CASE("Toggle off: switch at left, uses secondary color", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Toggle {
                value: false;
                width: 50;
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
    auto* track = FindDrawRect(*result, 0);
    REQUIRE(track != nullptr);
    CHECK(track->fillColor.r == 0x40);
    CHECK(track->fillColor.g == 0x40);
    CHECK(track->fillColor.b == 0x40);
}

TEST_CASE("Toggle on: switch at right, uses accent color", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Toggle {
                value: true;
                width: 50;
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
    auto* track = FindDrawRect(*result, 0);
    REQUIRE(track != nullptr);
    CHECK(track->fillColor.r == 0x80);
    CHECK(track->fillColor.g == 0x90);
    CHECK(track->fillColor.b == 0xA0);
}

// ---------------------------------------------------------------------------
// Slider
// ---------------------------------------------------------------------------

TEST_CASE("Slider track DrawRect with fill DrawRect at value position", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Slider {
                value: 50;
                min: 0;
                max: 100;
                width: 200;
                height: 30;
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
    CHECK(CountDrawRects(*result) >= 2);
    auto* trackBg = FindDrawRect(*result, 0);
    auto* trackFill = FindDrawRect(*result, 1);
    REQUIRE(trackBg != nullptr);
    REQUIRE(trackFill != nullptr);
    CHECK_THAT(trackBg->width, Catch::Matchers::WithinAbs(200.0f, 1.0f));
    CHECK_THAT(trackFill->width, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

TEST_CASE("Slider thumb DrawRect at correct position for value 50%", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Slider {
                value: 50;
                min: 0;
                max: 100;
                width: 200;
                height: 30;
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
    auto* thumb = FindDrawRect(*result, 2);
    REQUIRE(thumb != nullptr);
    // Thumb center should be near x=100 (50% of 200px track)
    float thumbCenter = thumb->x + thumb->width * 0.5f;
    CHECK_THAT(thumbCenter, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

TEST_CASE("Slider min/max constraints respected", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Slider {
                value: 50;
                min: 0;
                max: 100;
                width: 200;
                height: 30;
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
    auto* trackFill = FindDrawRect(*result, 1);
    REQUIRE(trackFill != nullptr);
    // value=50, min=0, max=100 -> ratio=0.5 -> fill width = 200 * 0.5 = 100
    CHECK_THAT(trackFill->width, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

// ---------------------------------------------------------------------------
// Dropdown
// ---------------------------------------------------------------------------

TEST_CASE("Dropdown closed: shows selected value with chevron icon", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Dropdown {
                selectedValue: "Option A";
                width: 200;
                height: 30;
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
    CHECK(dt->text == "Option A");
    auto* icon = FindDrawIcon(*result);
    REQUIRE(icon != nullptr);
    CHECK(icon->source == "icons/chevron-down.png");
}

TEST_CASE("Dropdown open: shows all options as DrawText list", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Dropdown {
                open: true;
                options: "A,B,C";
                selectedIndex: 0;
                width: 200;
                height: 30;
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
    // 1 selected value text + 3 option texts = 4 DrawTexts minimum
    CHECK(CountDrawTexts(*result) >= 4);
}

TEST_CASE("Dropdown selected option highlighted with accent color", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Dropdown {
                open: true;
                options: "A,B,C";
                selectedIndex: 0;
                width: 200;
                height: 30;
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
    // The selected option text (index 1 = first option after header text) uses accent color
    auto* selectedOpt = FindDrawText(*result, 1);
    REQUIRE(selectedOpt != nullptr);
    CHECK(selectedOpt->color.r == 0x80);
    CHECK(selectedOpt->color.g == 0x90);
    CHECK(selectedOpt->color.b == 0xA0);
}

// ---------------------------------------------------------------------------
// Stepper
// ---------------------------------------------------------------------------

TEST_CASE("Stepper shows value with +/- buttons", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Stepper {
                value: 5;
                min: 0;
                max: 10;
                width: 120;
                height: 30;
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
    CHECK(CountDrawTexts(*result) >= 3);
    auto* minusText = FindDrawText(*result, 0);
    auto* valueText = FindDrawText(*result, 1);
    auto* plusText = FindDrawText(*result, 2);
    REQUIRE(minusText != nullptr);
    REQUIRE(valueText != nullptr);
    REQUIRE(plusText != nullptr);
    CHECK(minusText->text == "-");
    CHECK(valueText->text == "5");
    CHECK(plusText->text == "+");
}

TEST_CASE("Stepper at min: minus button disabled color", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Stepper {
                value: 0;
                min: 0;
                max: 10;
                width: 120;
                height: 30;
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
    auto* minusText = FindDrawText(*result, 0);
    REQUIRE(minusText != nullptr);
    CHECK(minusText->text == "-");
    CHECK(minusText->color.r == 0x60);
    CHECK(minusText->color.g == 0x60);
    CHECK(minusText->color.b == 0x60);
}

TEST_CASE("Stepper at max: plus button disabled color", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Stepper {
                value: 10;
                min: 0;
                max: 10;
                width: 120;
                height: 30;
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
    auto* plusText = FindDrawText(*result, 2);
    REQUIRE(plusText != nullptr);
    CHECK(plusText->text == "+");
    CHECK(plusText->color.r == 0x60);
    CHECK(plusText->color.g == 0x60);
    CHECK(plusText->color.b == 0x60);
}

// ---------------------------------------------------------------------------
// ContextMenu
// ---------------------------------------------------------------------------

TEST_CASE("ContextMenu emits overlay + option list", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ContextMenu {
                items: "Copy,Paste,Delete";
                width: 150;
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
    CHECK(CountDrawRects(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 3);
}

TEST_CASE("ContextMenu option DrawTexts with correct Y positions", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ContextMenu {
                items: "Copy,Paste,Delete";
                width: 150;
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
    REQUIRE(CountDrawTexts(*result) >= 3);
    auto* text0 = FindDrawText(*result, 0);
    auto* text1 = FindDrawText(*result, 1);
    auto* text2 = FindDrawText(*result, 2);
    REQUIRE(text0 != nullptr);
    REQUIRE(text1 != nullptr);
    REQUIRE(text2 != nullptr);
    // Each subsequent item has a larger Y value
    CHECK(text1->y > text0->y);
    CHECK(text2->y > text1->y);
}

TEST_CASE("ContextMenu separator between groups emits DrawLine", "[input-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ContextMenu {
                items: "Copy,---,Delete";
                width: 150;
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
    auto* sep = FindDrawLine(*result);
    REQUIRE(sep != nullptr);
    // Separator is a horizontal line
    CHECK_THAT(sep->y1, Catch::Matchers::WithinAbs(sep->y2, 0.01f));
    CHECK(sep->x2 > sep->x1);
}
