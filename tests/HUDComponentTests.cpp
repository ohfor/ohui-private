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

static size_t CountDrawLines(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Line) ++count;
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
// ResourceBar
// ---------------------------------------------------------------------------

TEST_CASE("ResourceBar emits fill at health ratio", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 75;
                maxValue: 100;
                resourceType: health;
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
    CHECK(CountDrawRects(*result) >= 2);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // 75/100 = 0.75, width=200 -> fill width = 150
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(150.0f, 1.0f));
}

TEST_CASE("ResourceBar health token color for health bar", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 50;
                maxValue: 100;
                resourceType: health;
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
    CHECK(fill->fillColor.r == 0x8B);
}

TEST_CASE("ResourceBar stamina token color for stamina bar", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 50;
                maxValue: 100;
                resourceType: stamina;
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
    CHECK(fill->fillColor.r == 0x2E);
    CHECK(fill->fillColor.g == 0x7D);
}

TEST_CASE("ResourceBar magicka token color for magicka bar", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 50;
                maxValue: 100;
                resourceType: magicka;
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
    CHECK(fill->fillColor.r == 0x15);
}

TEST_CASE("ResourceBar damage flash DrawRect appears after value decrease", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 50;
                maxValue: 100;
                previousValue: 80;
                resourceType: health;
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
    // bg + fill + flash = at least 3 DrawRects
    CHECK(CountDrawRects(*result) >= 3);
}

TEST_CASE("ResourceBar damage flash fades over time", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 50;
                maxValue: 100;
                previousValue: 80;
                deltaTime: 0.3;
                resourceType: health;
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
    CHECK(CountDrawRects(*result) >= 3);
    // Flash rect is the 3rd DrawRect (index 2)
    auto* flash = FindDrawRect(*result, 2);
    REQUIRE(flash != nullptr);
    // opacity = max(0, 1.0 - 0.3 * 2.0) = 0.4
    CHECK_THAT(flash->opacity, Catch::Matchers::WithinAbs(0.4f, 0.05f));
}

TEST_CASE("ResourceBar regen shimmer DrawRect during value increase", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ResourceBar {
                value: 80;
                maxValue: 100;
                previousValue: 50;
                resourceType: health;
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
    // bg + fill + shimmer = at least 3 DrawRects
    CHECK(CountDrawRects(*result) >= 3);
}

// ---------------------------------------------------------------------------
// ShoutMeter
// ---------------------------------------------------------------------------

TEST_CASE("ShoutMeter 3-segment dividers visible", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ShoutMeter {
                value: 50;
                maxValue: 100;
                segments: 3;
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
    // 3 segments -> 2 divider lines
    CHECK(CountDrawLines(*result) >= 2);
}

TEST_CASE("ShoutMeter cooldown fill animates left to right", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ShoutMeter {
                value: 50;
                maxValue: 100;
                segments: 3;
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
    CHECK(CountDrawRects(*result) >= 2);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // 50/100 = 0.5, width=200 -> fill width = 100
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

TEST_CASE("ShoutMeter segments independent: divider positions evenly spaced", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ShoutMeter {
                value: 50;
                maxValue: 100;
                segments: 3;
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
    CHECK(CountDrawLines(*result) >= 2);
    auto* line0 = FindDrawLine(*result, 0);
    auto* line1 = FindDrawLine(*result, 1);
    REQUIRE(line0 != nullptr);
    REQUIRE(line1 != nullptr);
    // 300/3 = 100 -> dividers at x~100 and x~200
    CHECK_THAT(line0->x1, Catch::Matchers::WithinAbs(100.0f, 2.0f));
    CHECK_THAT(line1->x1, Catch::Matchers::WithinAbs(200.0f, 2.0f));
}

// ---------------------------------------------------------------------------
// CompassRose
// ---------------------------------------------------------------------------

TEST_CASE("CompassRose cardinal markers at correct positions for heading=0", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CompassRose {
                heading: 0;
                width: 400;
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
    CHECK(CountDrawTexts(*result) >= 1);
    // "N" should be at center (x~200) since heading=0 and N=0 degrees
    auto* nText = FindDrawText(*result, 0);
    REQUIRE(nText != nullptr);
    CHECK(nText->text == "N");
    CHECK_THAT(nText->x, Catch::Matchers::WithinAbs(200.0f, 2.0f));
}

TEST_CASE("CompassRose markers shift with heading change", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CompassRose {
                heading: 90;
                width: 400;
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
    // "E" at 90 degrees, heading 90 -> offset=0, so "E" should be at center (~200)
    bool foundE = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "E") {
                CHECK_THAT(dt.x, Catch::Matchers::WithinAbs(200.0f, 2.0f));
                foundE = true;
                break;
            }
        }
    }
    CHECK(foundE);
}

TEST_CASE("CompassRose wraps at 360 degrees", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CompassRose {
                heading: 350;
                width: 400;
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
    // "N" at 0 degrees, heading 350 -> offset = 0-350 = -350, normalized to 10
    // 10 < visibleRange (400/4=100), so "N" should be visible
    bool foundN = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "N") {
                foundN = true;
                break;
            }
        }
    }
    CHECK(foundN);
}

TEST_CASE("CompassRose quest marker icon at correct bearing", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CompassRose {
                heading: 0;
                questMarkerBearing: 45;
                width: 400;
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
    CHECK(icon->source == "icons/quest-marker.png");
}

// ---------------------------------------------------------------------------
// DetectionMeter
// ---------------------------------------------------------------------------

TEST_CASE("DetectionMeter Hidden state: disabled color", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            DetectionMeter {
                state: hidden;
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
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->color.r == 0x60);
    CHECK(text->text == "HIDDEN");
}

TEST_CASE("DetectionMeter Caution state: warning color", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            DetectionMeter {
                state: caution;
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
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->color.r == 0xFF);
    CHECK(text->color.g == 0xC1);
    CHECK(text->text == "CAUTION");
}

TEST_CASE("DetectionMeter Detected state: critical color", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            DetectionMeter {
                state: detected;
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
    auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->color.r == 0xFF);
    CHECK(text->color.g == 0x17);
    CHECK(text->text == "DETECTED");
}

// ---------------------------------------------------------------------------
// StealthEye
// ---------------------------------------------------------------------------

TEST_CASE("StealthEye fill proportional to detection level", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StealthEye {
                detectionLevel: 0.5;
                width: 40;
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
    CHECK(CountDrawIcons(*result) >= 1);
    CHECK(CountDrawRects(*result) >= 1);
    auto* fillRect = FindDrawRect(*result, 0);
    REQUIRE(fillRect != nullptr);
    // 0.5 * 40 = 20
    CHECK_THAT(fillRect->width, Catch::Matchers::WithinAbs(20.0f, 1.0f));
}

TEST_CASE("StealthEye fully hidden: zero fill", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StealthEye {
                detectionLevel: 0;
                width: 40;
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
    CHECK(CountDrawIcons(*result) >= 1);
    // No fill DrawRect when detectionLevel == 0
    CHECK(CountDrawRects(*result) == 0);
}

TEST_CASE("StealthEye fully detected: full fill", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            StealthEye {
                detectionLevel: 1.0;
                width: 40;
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
    CHECK(CountDrawIcons(*result) >= 1);
    CHECK(CountDrawRects(*result) >= 1);
    auto* fillRect = FindDrawRect(*result, 0);
    REQUIRE(fillRect != nullptr);
    // 1.0 * 40 = 40
    CHECK_THAT(fillRect->width, Catch::Matchers::WithinAbs(40.0f, 1.0f));
}

// ---------------------------------------------------------------------------
// NotificationToast
// ---------------------------------------------------------------------------

TEST_CASE("NotificationToast emits DrawRect + DrawText with fade opacity", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            NotificationToast {
                text: "Item Added";
                lifetime: 3.0;
                elapsed: 1.0;
                width: 200;
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
    CHECK(CountDrawRects(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 1);
    auto* bg = FindDrawRect(*result, 0);
    auto* text = FindDrawText(*result, 0);
    REQUIRE(bg != nullptr);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Item Added");
    // elapsed=1.0 is past fadeIn (0.3) and before fadeOut start (3.0-0.5=2.5), so opacity ~1.0
    CHECK_THAT(bg->opacity, Catch::Matchers::WithinAbs(1.0f, 0.05f));
    CHECK_THAT(text->opacity, Catch::Matchers::WithinAbs(1.0f, 0.05f));
}

TEST_CASE("NotificationToast after lifetime expires: opacity reaches 0", "[hud-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            NotificationToast {
                text: "Done";
                lifetime: 3.0;
                elapsed: 3.0;
                width: 200;
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
    CHECK(CountDrawRects(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 1);
    auto* bg = FindDrawRect(*result, 0);
    auto* text = FindDrawText(*result, 0);
    REQUIRE(bg != nullptr);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Done");
    // elapsed >= lifetime -> opacity = 0
    CHECK_THAT(bg->opacity, Catch::Matchers::WithinAbs(0.0f, 0.05f));
    CHECK_THAT(text->opacity, Catch::Matchers::WithinAbs(0.0f, 0.05f));
}
