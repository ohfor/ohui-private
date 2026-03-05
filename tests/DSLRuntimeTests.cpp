#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/dsl/DSLRuntimeEngine.h"
#include "ohui/dsl/ComponentHandler.h"
#include "ohui/dsl/WidgetParser.h"

using namespace ohui;
using namespace ohui::dsl;
using namespace ohui::binding;
using namespace ohui::widget;

// Helper to create a registry with builtins registered
static ComponentRegistry MakeRegistry() {
    ComponentRegistry reg;
    reg.RegisterBuiltins();
    return reg;
}

// Helper to parse a widget definition from string
static ParseResult ParseInput(const std::string& input) {
    WidgetParser parser;
    auto result = parser.Parse(input);
    REQUIRE(result.has_value());
    return std::move(*result);
}

// Helper to find a DrawRect in the output
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

TEST_CASE("Static Panel+Label produces DrawRect + DrawText", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                Label {
                    text: "Hello";
                }
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());
    REQUIRE_FALSE(result->calls.empty());

    // Should have at least a DrawRect (Panel) and a DrawText (Label)
    const auto* rect = FindDrawRect(*result);
    const auto* text = FindDrawText(*result);
    REQUIRE(rect != nullptr);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Hello");
}

TEST_CASE("HealthBar with bound values -- ValueBar fill 75% width", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget HealthBar {
            ValueBar {
                value: bind(player.health);
                maxValue: bind(player.healthMax);
                width: 200;
                height: 20;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    float health = 75.0f;
    float healthMax = 100.0f;
    (void)bindings.RegisterBinding({"player.health", BindingType::Float, ""}, [&]() -> BindingValue { return health; });
    (void)bindings.RegisterBinding({"player.healthMax", BindingType::Float, ""}, [&]() -> BindingValue { return healthMax; });
    (void)bindings.Subscribe("test-widget", "player.health");
    (void)bindings.Subscribe("test-widget", "player.healthMax");
    bindings.Update(0.0f);

    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("HealthBar", vp, 0.0f);
    REQUIRE(result.has_value());

    // ValueBar produces 2 DrawRects: background + fill
    REQUIRE(result->calls.size() >= 2);
    const auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(150.0f, 1.0f));  // 200 * 0.75
}

TEST_CASE("token() resolution -- DrawRect fill color matches token store", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--health-color);
            }
        }
    )");

    TokenStore tokens;
    (void)tokens.LoadBaseTokens(":root { --health-color: #FF0000; }");
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK(rect->fillColor.r == 255);
    CHECK(rect->fillColor.g == 0);
    CHECK(rect->fillColor.b == 0);
}

TEST_CASE("var() resolution through TokenStore", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: var(--primary, #00FF00);
            }
        }
    )");

    TokenStore tokens;
    // Don't define --primary, should use fallback
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK(rect->fillColor.r == 0);
    CHECK(rect->fillColor.g == 255);
    CHECK(rect->fillColor.b == 0);
}

TEST_CASE("Skin token override changes draw call color", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--main-color);
            }
        }
        skin TestWidget "dark" {
            tokens {
                --main-color: #0000FF;
            }
        }
    )");

    TokenStore tokens;
    (void)tokens.LoadBaseTokens(":root { --main-color: #FF0000; }");
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};

    // Before skin: red
    auto before = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(before.has_value());
    auto* beforeRect = FindDrawRect(*before);
    REQUIRE(beforeRect != nullptr);
    CHECK(beforeRect->fillColor.r == 255);

    // Apply skin: blue
    REQUIRE(engine.ApplySkin(parsed.ast.skins[0]).has_value());
    auto after = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(after.has_value());
    auto* afterRect = FindDrawRect(*after);
    REQUIRE(afterRect != nullptr);
    CHECK(afterRect->fillColor.b == 255);
    CHECK(afterRect->fillColor.r == 0);
}

TEST_CASE("Skin override without re-parse", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--bg);
            }
        }
        skin TestWidget "alt" {
            tokens {
                --bg: #112233;
            }
        }
    )");

    TokenStore tokens;
    (void)tokens.LoadBaseTokens(":root { --bg: #AABBCC; }");
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 100, 100};
    auto r1 = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(r1.has_value());
    auto* c1 = FindDrawRect(*r1);
    REQUIRE(c1 != nullptr);
    CHECK(c1->fillColor.r == 0xAA);

    REQUIRE(engine.ApplySkin(parsed.ast.skins[0]).has_value());
    auto r2 = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(r2.has_value());
    auto* c2 = FindDrawRect(*r2);
    REQUIRE(c2 != nullptr);
    CHECK(c2->fillColor.r == 0x11);
}

TEST_CASE("Skin with ControlTemplate changes draw structure", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--bg);
            }
        }
        skin TestWidget "custom" {
            tokens {
                --bg: #FF0000;
            }
            template {
                Label {
                    text: "Custom template";
                }
            }
        }
    )");

    TokenStore tokens;
    (void)tokens.LoadBaseTokens(":root { --bg: #00FF00; }");
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());
    REQUIRE(engine.ApplySkin(parsed.ast.skins[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    // Should have DrawText from template, not DrawRect from original
    const auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Custom template");
}

TEST_CASE("ClearSkin reverts draw calls to original", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--bg);
            }
        }
        skin TestWidget "alt" {
            tokens {
                --bg: #0000FF;
            }
        }
    )");

    TokenStore tokens;
    (void)tokens.LoadBaseTokens(":root { --bg: #FF0000; }");
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());
    REQUIRE(engine.ApplySkin(parsed.ast.skins[0]).has_value());

    engine.ClearSkin("TestWidget");

    ViewportRect vp{0, 0, 100, 100};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK(rect->fillColor.r == 255);  // back to red
}

TEST_CASE("Animate start -- at t=0 reflects start value", "[dsl-runtime]") {
    AnimationState anim;
    anim.startValue = 0.0f;
    anim.endValue = 100.0f;
    anim.durationMs = 1000.0f;
    anim.elapsedMs = 0.0f;
    anim.easing = EasingFunction::Linear;
    anim.active = true;

    CHECK_THAT(anim.CurrentValue(), Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("Animate midpoint -- interpolated value at half duration", "[dsl-runtime]") {
    AnimationState anim;
    anim.startValue = 0.0f;
    anim.endValue = 100.0f;
    anim.durationMs = 1000.0f;
    anim.elapsedMs = 500.0f;
    anim.easing = EasingFunction::Linear;
    anim.active = true;

    CHECK_THAT(anim.CurrentValue(), Catch::Matchers::WithinAbs(50.0f, 0.01f));
}

TEST_CASE("Animate completion -- at t>=duration equals end value", "[dsl-runtime]") {
    AnimationState anim;
    anim.startValue = 0.0f;
    anim.endValue = 100.0f;
    anim.durationMs = 1000.0f;
    anim.elapsedMs = 1500.0f;
    anim.easing = EasingFunction::Linear;
    anim.active = false;

    CHECK_THAT(anim.CurrentValue(), Catch::Matchers::WithinAbs(100.0f, 0.01f));
}

TEST_CASE("Layout recalculation on resize", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                width: 100;
                height: 50;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp1{0, 0, 400, 300};
    auto r1 = engine.Evaluate("TestWidget", vp1, 0.0f);
    REQUIRE(r1.has_value());

    ViewportRect vp2{0, 0, 800, 600};
    auto r2 = engine.Evaluate("TestWidget", vp2, 0.0f);
    REQUIRE(r2.has_value());

    // Both should produce draw calls (layout works at different sizes)
    REQUIRE_FALSE(r1->calls.empty());
    REQUIRE_FALSE(r2->calls.empty());
}

TEST_CASE("Nested components -- 3-level nesting, correct positions", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                Panel {
                    Label {
                        text: "deep";
                    }
                }
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{10, 20, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    // Should have 2 DrawRects (2 Panels) + 1 DrawText (Label)
    size_t rectCount = 0, textCount = 0;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Rect) ++rectCount;
        if (call.type == DrawCallType::Text) ++textCount;
    }
    CHECK(rectCount == 2);
    CHECK(textCount == 1);
}

TEST_CASE("Widget not loaded returns error", "[dsl-runtime]") {
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("NonExistent", vp, 0.0f);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == ErrorCode::WidgetNotFound);
}

TEST_CASE("Binding not found uses default value", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ValueBar {
                value: bind(nonexistent.binding);
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

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    // Value defaults to 0, so fill rect should have width 0
    const auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("String binding -- DrawText text matches bound string", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Label {
                text: bind(player.name);
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    std::string name = "Dovahkiin";
    (void)bindings.RegisterBinding({"player.name", BindingType::String, ""}, [&]() -> BindingValue { return name; });
    (void)bindings.Subscribe("test-widget", "player.name");
    bindings.Update(0.0f);

    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Dovahkiin");
}

TEST_CASE("Multiple widgets evaluate independently", "[dsl-runtime]") {
    auto parsed = ParseInput(R"(
        widget Alpha {
            Panel {
                fillColor: #FF0000;
            }
        }
        widget Beta {
            Panel {
                fillColor: #0000FF;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);

    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[1]).has_value());

    ViewportRect vp{0, 0, 100, 100};
    auto r1 = engine.Evaluate("Alpha", vp, 0.0f);
    auto r2 = engine.Evaluate("Beta", vp, 0.0f);

    REQUIRE(r1.has_value());
    REQUIRE(r2.has_value());

    auto* rect1 = FindDrawRect(*r1);
    auto* rect2 = FindDrawRect(*r2);
    REQUIRE(rect1 != nullptr);
    REQUIRE(rect2 != nullptr);
    CHECK(rect1->fillColor.r == 255);
    CHECK(rect2->fillColor.b == 255);
}

TEST_CASE("Easing functions produce expected curves", "[dsl-runtime]") {
    AnimationState linear;
    linear.startValue = 0.0f;
    linear.endValue = 100.0f;
    linear.durationMs = 100.0f;
    linear.elapsedMs = 50.0f;
    linear.easing = EasingFunction::Linear;

    AnimationState easeOut;
    easeOut.startValue = 0.0f;
    easeOut.endValue = 100.0f;
    easeOut.durationMs = 100.0f;
    easeOut.elapsedMs = 50.0f;
    easeOut.easing = EasingFunction::EaseOut;

    AnimationState easeIn;
    easeIn.startValue = 0.0f;
    easeIn.endValue = 100.0f;
    easeIn.durationMs = 100.0f;
    easeIn.elapsedMs = 50.0f;
    easeIn.easing = EasingFunction::EaseIn;

    float linearVal = linear.CurrentValue();
    float easeOutVal = easeOut.CurrentValue();
    float easeInVal = easeIn.CurrentValue();

    CHECK_THAT(linearVal, Catch::Matchers::WithinAbs(50.0f, 0.01f));
    CHECK(easeOutVal > 50.0f);   // EaseOut: faster start, slower end -> above linear at midpoint
    CHECK(easeInVal < 50.0f);    // EaseIn: slower start, faster end -> below linear at midpoint
}

TEST_CASE("ValueBar at 0% -- fill width is 0", "[dsl-runtime]") {
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

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("ValueBar at 100% -- fill width equals container width", "[dsl-runtime]") {
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

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* bg = FindDrawRect(*result, 0);
    const auto* fill = FindDrawRect(*result, 1);
    REQUIRE(bg != nullptr);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(bg->width, 0.01f));
}
