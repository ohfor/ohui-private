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
// StatusBadge
// ---------------------------------------------------------------------------

TEST_CASE("StatusBadge emits pill DrawRect + DrawText", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatusBadge {
                text: "Active";
                width: 80;
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
    CHECK(CountDrawTexts(*result) >= 1);
    auto* pill = FindDrawRect(*result);
    REQUIRE(pill != nullptr);
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Active");
}

TEST_CASE("StatusBadge pill uses radius-pill token (borderRadius ~9999)", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatusBadge {
                text: "OK";
                width: 60;
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
    auto* pill = FindDrawRect(*result);
    REQUIRE(pill != nullptr);
    CHECK_THAT(pill->borderRadius, Catch::Matchers::WithinAbs(9999.0f, 0.01f));
}

TEST_CASE("StatusBadge with icon emits DrawIcon before text", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatusBadge {
                text: "Online";
                icon: "icons/check.png";
                width: 100;
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
    CHECK(CountDrawIcons(*result) >= 1);
    auto* icon = FindDrawIcon(*result);
    REQUIRE(icon != nullptr);
    CHECK(icon->source == "icons/check.png");
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Online");
}

TEST_CASE("StatusBadge color from token", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StatusBadge {
                text: "Error";
                color: #FF1744;
                width: 80;
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
    auto* pill = FindDrawRect(*result);
    REQUIRE(pill != nullptr);
    CHECK(pill->fillColor.r == 0xFF);
    CHECK(pill->fillColor.g == 0x17);
    CHECK(pill->fillColor.b == 0x44);
}

// ---------------------------------------------------------------------------
// IndicatorDot
// ---------------------------------------------------------------------------

TEST_CASE("IndicatorDot emits small circular DrawRect", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            IndicatorDot {
                width: 8;
                height: 8;
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
    auto* dot = FindDrawRect(*result);
    REQUIRE(dot != nullptr);
    CHECK_THAT(dot->width, Catch::Matchers::WithinAbs(8.0f, 0.01f));
    CHECK_THAT(dot->height, Catch::Matchers::WithinAbs(8.0f, 0.01f));
    CHECK_THAT(dot->borderRadius, Catch::Matchers::WithinAbs(4.0f, 0.01f));
}

TEST_CASE("IndicatorDot color property sets fill", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            IndicatorDot {
                color: #00FF00;
                width: 10;
                height: 10;
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
    auto* dot = FindDrawRect(*result);
    REQUIRE(dot != nullptr);
    CHECK(dot->fillColor.r == 0x00);
    CHECK(dot->fillColor.g == 0xFF);
    CHECK(dot->fillColor.b == 0x00);
}

// ---------------------------------------------------------------------------
// AlertBanner
// ---------------------------------------------------------------------------

TEST_CASE("AlertBanner warning severity uses warning color", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlertBanner {
                text: "Warning message";
                severity: warning;
                width: 400;
                height: 40;
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
    CHECK(bg->fillColor.r == 0xFF);
    CHECK(bg->fillColor.g == 0xC1);
}

TEST_CASE("AlertBanner critical severity uses critical color", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlertBanner {
                text: "Critical error";
                severity: critical;
                width: 400;
                height: 40;
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
    CHECK(bg->fillColor.r == 0xFF);
    CHECK(bg->fillColor.g == 0x17);
}

TEST_CASE("AlertBanner with icon emits icon + text", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlertBanner {
                text: "Alert!";
                icon: "icons/warning.png";
                severity: warning;
                width: 400;
                height: 40;
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
    CHECK(icon->source == "icons/warning.png");
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Alert!");
}

TEST_CASE("AlertBanner dismissable emits close button (extra DrawIcon)", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlertBanner {
                text: "Dismissable alert";
                severity: warning;
                dismissable: true;
                width: 400;
                height: 40;
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
    auto* closeIcon = FindDrawIcon(*result);
    REQUIRE(closeIcon != nullptr);
    CHECK(closeIcon->source == "icons/close.png");
}

// ---------------------------------------------------------------------------
// CompletionRing
// ---------------------------------------------------------------------------

TEST_CASE("CompletionRing emits background circle + fill", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CompletionRing {
                value: 50;
                maxValue: 100;
                width: 60;
                height: 60;
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
    auto* bgCircle = FindDrawRect(*result, 0);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(bgCircle != nullptr);
    REQUIRE(fill != nullptr);
    // Background is a circle (borderRadius = size/2)
    CHECK_THAT(bgCircle->borderRadius, Catch::Matchers::WithinAbs(30.0f, 0.01f));
}

TEST_CASE("CompletionRing fill ratio matches value/maxValue", "[indicator-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CompletionRing {
                value: 75;
                maxValue: 100;
                width: 60;
                height: 60;
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
    auto* bgCircle = FindDrawRect(*result, 0);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(bgCircle != nullptr);
    REQUIRE(fill != nullptr);
    // 75/100 = 0.75, size=60 -> fill width = 60 * 0.75 = 45
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(45.0f, 1.0f));
    CHECK_THAT(bgCircle->width, Catch::Matchers::WithinAbs(60.0f, 1.0f));
}
